#include "tuya_iot_soc_dev_entry.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_iot_com_api.h"
#include "uni_log.h"

#include "tuya_cloud_base_defs.h"
#include "tuya_iot_base_api.h"
#include "base_event.h"
#include "gw_intf.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "tuya_cli.h"
#include "app_agent.h"

// UserTODO
// SOC固件版本，用于OTA管理，格式必须为"XX.XX.XX"，其中XX必须为数字
#ifndef USER_SW_VER
#define USER_SW_VER         "1.0.0"
#endif

// 涂鸦云上的产品KEY，需登陆tuya.com创建产品分配唯一key
#define PRODUCT_KEY         "U0fxNCEnZptKnQZy"  //DP控制开关
// SD本地配置存储路径，该路径必须对应一个可读写文件系统分区
#define CFG_STORAGE_PATH    "./tuya_db_files/"
// UUID和AUTHKEY用于涂鸦云设备的安全认证，每个设备所用key均为唯一
#define UUID                "f2ef8b136911f4b0"
#define AUTHKEY             "6PqrvTXJh1ye5tF4SABzl1qNmd53slMX"
#define IPADDR              "192.168.31.168"
#define OTA_FILE            "./upgrade.ota"
#define EVENT_RESET         "dev.reset"

ty_devos_virtual_t g_devos_virual = {0};

STATIC VOID __soc_dev_rev_upgrade_info_cb(IN CONST FW_UG_S *fw);// SOC设备升级入口
STATIC VOID __soc_dev_status_changed_cb(IN CONST GW_STATUS_E status);// SOC设备云端状态变更回调
STATIC VOID __soc_dev_dp_query_cb(IN CONST TY_DP_QUERY_S *dp_qry);// SOC设备特定数据查询入口
STATIC VOID __soc_dev_obj_dp_cmd_cb(IN CONST TY_RECV_OBJ_DP_S *dp);// SOC设备格式化指令数据下发入口
STATIC VOID __soc_dev_raw_dp_cmd_cb(IN CONST TY_RECV_RAW_DP_S *dp);// SOC设备透传指令数据下发入口
STATIC VOID __soc_dev_restart_req_cb(GW_RESET_TYPE_E type);// SOC设备进程重启请求入口
STATIC VOID __soc_dev_net_status_cb(IN CONST GW_BASE_NW_STAT_T stat);// SOC外网状态变动回调
STATIC VOID __soc_dev_reset_cb(event_raw_data_t *data);
STATIC int __soc_dev_pre_init();
int app_path_get(char * buf, int size);

CHAR_T *ty_devos_get_ipaddr()
{
    return g_devos_virual.ipaddr;
}
CHAR_T *ty_devos_get_token()
{
    if (strlen(g_devos_virual.token) > 0)
        return g_devos_virual.token;
    
    return NULL;
}

int ty_devos_monitor()
{
    // monitor network status
    GW_BASE_NW_STAT_T nw_status = 0;
    get_base_gw_nw_status(&nw_status);
    switch (nw_status) {
    case GB_STAT_LAN_CONN:
        if (g_devos_virual.status.nw_status == GB_STAT_LAN_UNCONN)
            PR_NOTICE("DEVOS MONITOR: network status LAN connected!");
        if (g_devos_virual.status.nw_status == GB_STAT_CLOUD_CONN)
            PR_NOTICE("DEVOS MONITOR: network status MQTT disconnected!");

        // save the status
        g_devos_virual.status.nw_status = nw_status;
        break;
    case GB_STAT_CLOUD_CONN:
        if (g_devos_virual.status.nw_status == GB_STAT_LAN_UNCONN ||
            g_devos_virual.status.nw_status == GB_STAT_LAN_CONN)
            PR_NOTICE("DEVOS MONITOR: network status MQTT connected!");

        // save the status
        g_devos_virual.status.nw_status = nw_status;
        break;
    case GB_STAT_LAN_UNCONN:
        if (g_devos_virual.status.nw_status == GB_STAT_LAN_CONN)
            PR_NOTICE("DEVOS MONITOR: network status LAN disconnected!");
        if (g_devos_virual.status.nw_status == GB_STAT_CLOUD_CONN)
            PR_NOTICE("DEVOS MONITOR: network status MQTT disconnected!");
        
        // save the status
        g_devos_virual.status.nw_status = nw_status;
        break;
    default:
        PR_NOTICE("DEVOS MONITOR: network status INVALID!");
    }

    // monitor device status
    GW_WORK_STAT_T wsm_stat = get_gw_cntl()->gw_wsm.stat;
    switch (wsm_stat) {
    case UNREGISTERED:
        if (g_devos_virual.status.wm_status != wsm_stat)
            PR_NOTICE("DEVOS MONITOR: device status UNREGISTERED!");
        g_devos_virual.status.wm_status = wsm_stat;
        break;
    case REGISTERED:
        if (g_devos_virual.status.wm_status != wsm_stat)
            PR_NOTICE("DEVOS MONITOR: device status REGISTERED!");
        g_devos_virual.status.wm_status = wsm_stat;
        break;
    case ACTIVATED:
        if (g_devos_virual.status.wm_status != wsm_stat)
            PR_NOTICE("DEVOS MONITOR: device status ACTIVED!");
        g_devos_virual.status.wm_status = wsm_stat;
        break;
    default:
        PR_NOTICE("DEVOS MONITOR: device status INVALID!");
    }
        
    
    return;
}

void device_local_reset(int argc, char *argv[])
{
    int result;


    if (argc > 1 && (0 == strcmp("factory", argv[1])))  {
        PR_DEBUG("cmd is reset factory, ungister");
        result = tuya_iot_gw_reset();
    } else {
        PR_DEBUG("unactive");
        result = tuya_iot_gw_unactive();
    }

    if (OPRT_OK != result) {
        PR_ERR("gw reset error:%d",result);
    }
}

static const cli_cmd_t s_cli_cmd[] = {
    {
        .name   = "reset",
        .help   = "usage: reset or reset factory",
        .func   = device_local_reset,
    }
};

int main(int argc, char *argv[])
{
    int rt = OPRT_OK;

    // 配置文件路径可以手动输入，如果没输入，则使用当前路径
    char *cfg_path = NULL;
    if (argc > 1) {
        cfg_path = argv[1];
    }
    
    // 虚拟设备初始化之前，需要从配置文件里load 相关的配置，如果失败，则退出
    TUYA_CALL_ERR_RETURN(__soc_dev_pre_init(argv[0], cfg_path));
    
    printf("tuya Device OS demo, Version %s \r\n", USER_SW_VER);
    printf("Device OS Version: %s \r\n", tuya_iot_get_sdk_info());
    printf("Product Key: %s \r\n", g_devos_virual.product_key);
    printf("DB Storage Path: %s \r\n", g_devos_virual.storage_path);
    printf("Uuid: %s \r\n", g_devos_virual.uuid);
    printf("Auth Key: %s \r\n", g_devos_virual.auth_key);

    GW_PROD_INFO_S prod_info = {g_devos_virual.uuid, g_devos_virual.auth_key};
    TY_IOT_CBS_S iot_cbs = {__soc_dev_status_changed_cb,__soc_dev_rev_upgrade_info_cb,__soc_dev_restart_req_cb,
                            __soc_dev_obj_dp_cmd_cb,__soc_dev_raw_dp_cmd_cb,__soc_dev_dp_query_cb,NULL,};

    // 初始化 iot sdk
    TUYA_CALL_ERR_RETURN(tuya_os_adapt_reg_wired_intf());
    TUYA_CALL_ERR_RETURN(tuya_iot_init(g_devos_virual.storage_path));
    TUYA_CALL_ERR_RETURN(tuya_iot_set_gw_prod_info(&prod_info));
    TUYA_CALL_ERR_RETURN(tuya_iot_soc_init(&iot_cbs, g_devos_virual.product_key, USER_SW_VER));
    TUYA_CALL_ERR_RETURN(tuya_iot_reg_get_nw_stat_cb_params(__soc_dev_net_status_cb, 1));
 
    //! 初始化 tuya cli
    extern int platform_uart_init(void);
    platform_uart_init();
    tuya_cli_init();
    tuya_cli_cmd_register(&s_cli_cmd, sizeof(s_cli_cmd)/sizeof(s_cli_cmd[0]));

    //! 启动RPC
    //extern void rpc_server_init(void);
    //rpc_server_init();

    // 主循环
    while (1)
    {
        sleep(10);        
        // 监控设备状态，打印状态提示
        ty_devos_monitor();
    }

    return 0;
}

// SOC设备升级相关代码开始
STATIC VOID __upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fclose(p_upgrade_fd);

    if(download_result == 0) {
        PR_DEBUG("SOC Upgrade File Download Success");
        // UserTODO
        exit(1);
    }else {
        PR_ERR("SOC Upgrade File Download Fail.ret = %d", download_result);
    }
}

STATIC OPERATE_RET __get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len, IN CONST UINT_T offset,
                                      IN CONST BYTE_T *data, IN CONST UINT_T len, OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
//    PR_DEBUG("Rev File Data");
//    PR_DEBUG("Total_len:%u", total_len);
//    PR_DEBUG("Offset:%u Len:%u", offset, len);
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fwrite(data, 1, len, p_upgrade_fd);
    *remain_len = 0;

    return OPRT_OK;
}

// SOC设备升级入口
VOID __soc_dev_rev_upgrade_info_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("SOC Rev Upgrade Info");
    PR_DEBUG("fw->tp:%d", fw->tp);
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->fw_hmac:%s", fw->fw_hmac);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);

    FILE *p_upgrade_fd = fopen(g_devos_virual.ota_path, "w+b");
    if(NULL == p_upgrade_fd){
        PR_ERR("open upgrade file fail. upgrade fail %s", g_devos_virual.ota_path);
        return;
    }
    OPERATE_RET op_ret = tuya_iot_upgrade_gw(fw, __get_file_data_cb, __upgrade_notify_cb, p_upgrade_fd);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_upgrade_gw err:%d",op_ret);
    }
}

// 升级相关代码结束

// SOC设备云端状态变更回调
VOID __soc_dev_status_changed_cb(IN CONST GW_STATUS_E status)
{
    PR_DEBUG("SOC TUYA-Cloud Status:%d", status);
}

// SOC设备特定数据查询入口
VOID __soc_dev_dp_query_cb(IN CONST TY_DP_QUERY_S *dp_qry)
{
    PR_DEBUG("SOC Rev DP Query Cmd");
    if(dp_qry->cid != NULL) PR_ERR("soc not have cid.%s", dp_qry->cid);

    if(dp_qry->cnt == 0) {
        PR_DEBUG("soc rev all dp query");
        // UserTODO
    }else {
        PR_DEBUG("soc rev dp query cnt:%d", dp_qry->cnt);
        UINT_T index = 0;
        for(index = 0; index < dp_qry->cnt; index++) {
            PR_DEBUG("rev dp query:%d", dp_qry->dpid[index]);
            // UserTODO
        }
    }
}

// SOC设备格式化指令数据下发入口
VOID __soc_dev_obj_dp_cmd_cb(IN CONST TY_RECV_OBJ_DP_S *dp)
{
    PR_DEBUG("SOC Rev DP Obj Cmd t1:%d t2:%d CNT:%u", dp->cmd_tp, dp->dtt_tp, dp->dps_cnt);
    if(dp->cid != NULL) PR_ERR("soc not have cid.%s", dp->cid);

    UINT_T index = 0;
    for(index = 0; index < dp->dps_cnt; index++)
    {
        CONST TY_OBJ_DP_S *p_dp_obj = dp->dps + index;
        PR_DEBUG("idx:%d dpid:%d type:%d ts:%u", index, p_dp_obj->dpid, p_dp_obj->type, p_dp_obj->time_stamp);
        switch (p_dp_obj->type) {
        case PROP_BOOL:     { PR_DEBUG("bool value:%d", p_dp_obj->value.dp_bool); break;}
        case PROP_VALUE:    { PR_DEBUG("INT value:%d", p_dp_obj->value.dp_value); break;}
        case PROP_STR:      { PR_DEBUG("str value:%s", p_dp_obj->value.dp_str); break;}
        case PROP_ENUM:     { PR_DEBUG("enum value:%u", p_dp_obj->value.dp_enum); break;}
        case PROP_BITMAP:   { PR_DEBUG("bits value:0x%X", p_dp_obj->value.dp_bitmap); break;}
        default:            { PR_ERR("idx:%d dpid:%d type:%d ts:%u is invalid", index, p_dp_obj->dpid, p_dp_obj->type, p_dp_obj->time_stamp); break;}
        }// end of switch

        if (p_dp_obj->dpid == 1) {
            if (p_dp_obj->value.dp_bool)
                lan_pro_cntl_enable();
            else
                lan_pro_cntl_disable();
        }
    }
    // UserTODO

    // 用户处理完成之后需要主动上报最新状态，这里简单起见，直接返回收到的数据，认为处理全部成功。
    OPERATE_RET op_ret = dev_report_dp_json_async(dp->cid,dp->dps,dp->dps_cnt);
    if(OPRT_OK != op_ret) {
        PR_ERR("dev_report_dp_json_async op_ret:%d",op_ret);
    }
}

// SOC设备透传指令数据下发入口
VOID __soc_dev_raw_dp_cmd_cb(IN CONST TY_RECV_RAW_DP_S *dp)
{
    PR_DEBUG("SOC Rev DP Raw Cmd t1:%d t2:%d dpid:%d len:%u", dp->cmd_tp, dp->dtt_tp, dp->dpid, dp->len);
    if(dp->cid != NULL) PR_ERR("soc not have cid.%s", dp->cid);

    // UserTODO

    // 用户处理完成之后需要主动上报最新状态，这里简单起见，直接返回收到的数据，认为处理全部成功。
    OPERATE_RET op_ret = dev_report_dp_raw_sync(dp->cid,dp->dpid,dp->data,dp->len,0);
    if(OPRT_OK != op_ret) {
        PR_ERR("dev_report_dp_json_async op_ret:%d",op_ret);
    }
}

// SOC设备进程重启请求入口
VOID __soc_dev_restart_req_cb(GW_RESET_TYPE_E type)
{
    // 直接创建默认本地db路径
    CHAR_T sys_cmd[128];
    PR_DEBUG("SOC Rev Restart Req %d", type);
    if (GW_RESET_DATA_FACTORY != type) {
        // UserTODO 设备进程重启
        exit(0);
    } else {
        // 删除配置文件
        memset(sys_cmd, 0, 128);
        sprintf(sys_cmd, "rm -rf %s", g_devos_virual.storage_path);
        system(sys_cmd);
    }
}

// SOC外网状态变动回调
STATIC VOID __soc_dev_net_status_cb(IN CONST GW_BASE_NW_STAT_T stat)
{
    PR_DEBUG("network status:%d", stat);
}

STATIC VOID __soc_dev_reset_cb(event_raw_data_t *data)
{
    PR_NOTICE("DEVOS MONITOR: device UNACTIVED!");
}

STATIC bool_t __soc_dev_pre_init_check_at_env()
{
    // AT环境如下，需要校验环境是否正常，
    // 如果不正常，则启动，并在当前环境下读取配置
    // 如果正常则在Config环境下读取配置
    // -设备运行环境
    //      -| System
    //      -| OTA1
    //      -| OTA2
    //      -| Config
    //      -| DB


    return false;
}

STATIC bool_t __soc_dev_pre_init_check_def_env()
{
    // 默认环境，检查是否存在配置文件，
    if(access("./tuya_device_cfg.json", F_OK) == 0)
        return true;

    return false;
}


// SOC初始化加载操作
STATIC int __soc_dev_pre_init(char *app_path, char *cfg_path)
{
    char buffer[4096];
    int read_len = sizeof(buffer);
    int fd = -1;

    // 如果提供了配置文件路径，从该路径读取配置文件启动
    if (NULL != cfg_path) {
        // 从配置路径里读取相关的配置文件，整体的配置环境需要守护程序建立
        fd = open(cfg_path, O_RDONLY);
        printf("detect auto test enviroment, load config from %s\n", cfg_path);
        // 检查是打开文件成功。
        if (fd < 0) {
            printf("open path %s failed\n", cfg_path);
            return -1;
        }
    } else if (__soc_dev_pre_init_check_def_env()){
        // 没有提供配置文件路径，优先从当前路径读取，一般是本地调试
        fd = open("./tuya_device_cfg.json", O_RDONLY);
        printf("detect default enviroment, load config from ./tuya_device_cfg.json\n");
        // 检查是打开文件成功。
        if (fd < 0) {
            printf("open path ./tuya_device_cfg.json failed\n");
            return -1;
        }
    } else {
        // 没提供配置文件，本地也没有配置文件，使用默认配置
        strcpy(g_devos_virual.product_key,  PRODUCT_KEY);
        strcpy(g_devos_virual.uuid, UUID);
        strcpy(g_devos_virual.auth_key, AUTHKEY);
        strcpy(g_devos_virual.ipaddr, IPADDR);
        //! 默认跟随可执行程序一起存放
        app_path_get(g_devos_virual.storage_path, sizeof(g_devos_virual.storage_path) - 1);
        strcat(g_devos_virual.storage_path, "tuya_db_files/");
        //! 默认升级文件跟随可执行程序目录
        app_path_get(g_devos_virual.ota_path, sizeof(g_devos_virual.ota_path) - 1);
        strcat(g_devos_virual.storage_path, "upgrade");
        // 直接创建默认本地db路径
        CHAR_T sys_cmd[128];
        sprintf(sys_cmd, "mkdir -p %s", g_devos_virual.storage_path);
        system(sys_cmd);
        printf("not found any enviroment, use default value in software\n");
        return OPRT_OK;
    }

    // 读取文件内容
    memset(buffer, 0, sizeof(buffer));
    while (read_len == read(fd, buffer, sizeof(buffer)));
    close(fd);

    // 解析JSON
    ty_cJSON *root      = ty_cJSON_Parse(buffer);
    if (NULL == root) {
        printf("load configure failed\n");
        return OPRT_CJSON_PARSE_ERR;
    }

    // 解析JSON内容
    ty_cJSON *pid       = ty_cJSON_GetObjectItem(root, "pid");
    ty_cJSON *uuid      = ty_cJSON_GetObjectItem(root, "uuid");
    ty_cJSON *authkey   = ty_cJSON_GetObjectItem(root, "authkey");
    ty_cJSON *fwkey     = ty_cJSON_GetObjectItem(root, "fwkey");
    ty_cJSON *storage   = ty_cJSON_GetObjectItem(root, "storage");
    ty_cJSON *ipaddr    = ty_cJSON_GetObjectItem(root, "ipaddr");
    ty_cJSON *token     = ty_cJSON_GetObjectItem(root, "token");

    // uuid&authkey&pid
    if (uuid && authkey && pid) {
        strcpy(g_devos_virual.product_key,     pid->valuestring);
        strcpy(g_devos_virual.uuid,    uuid->valuestring);
        strcpy(g_devos_virual.auth_key, authkey->valuestring);
    } else {
        ty_cJSON_Delete(root);
        return OPRT_INVALID_PARM;
    }

    // db config path
    if (storage && strlen(storage->valuestring) && strlen(storage->valuestring) < 128) {
        strcpy(g_devos_virual.storage_path, storage->valuestring);
    } else {
        //! 默认跟随可执行程序一起存放
        app_path_get(g_devos_virual.storage_path, sizeof(g_devos_virual.storage_path) - 1);
        strcat(g_devos_virual.storage_path, "tuya_db_files/");
    }
    printf("storage path %s\n", g_devos_virual.storage_path);

    // 直接创建默认本地db路径
    CHAR_T sys_cmd[128];
    sprintf(sys_cmd, "mkdir -p %s", g_devos_virual.storage_path);
    system(sys_cmd);

    //! ota
    app_path_get(g_devos_virual.ota_path, sizeof(g_devos_virual.ota_path) - 1);
    strcat(g_devos_virual.ota_path, "upgrade");

    // ip
    if (ipaddr && 0 != strlen(ipaddr->valuestring) && strlen(ipaddr->valuestring) < 128) {
        strcpy(g_devos_virual.ipaddr, ipaddr->valuestring);
        printf("found user config ipaddr, bind ipaddr:%s\n", g_devos_virual.ipaddr);
    } else {
        strcpy(g_devos_virual.ipaddr, IPADDR);
        printf("not found any ipaddr, use default value %s in software\n", g_devos_virual.ipaddr);
    }

    // token
    if (token && 0 != strlen(token->valuestring) && strlen(token->valuestring) < 128) {
        strcpy(g_devos_virual.token, token->valuestring);
        printf("found user config token, bind token:%s\n", g_devos_virual.token);
    }

    ty_cJSON_Delete(root);
    
    return 0;
}

int app_path_get(char * buf, int size)
{
    int i;
    int result = readlink("/proc/self/exe", buf, size);
    if (result < 0 || (result >= size)) {
        return -1;
    }
    buf[result] = '\0';
    for (i = result; i >= 0; i--) {
        if (buf[i] == '/') {
            buf[i + 1] = '\0';
            break;
        }
    }

    return i;
}
