/***********************************************************
*  File: tuya_device.c
*  Author: lql
*  Date: 20171128
***********************************************************/
#define _TUYA_DEVICE_GLOBAL
#include "tuya_device.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_led.h"
#include "tuya_uart.h"
#include "tuya_gpio.h"
#include "tuya_key.h"
#include "hw_table.h"
#include "uni_time_queue.h"
#include "gw_intf.h"
#include "uni_log.h"
#include "uni_thread.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#ifndef	PVOID
typedef void * PVOID;
#endif

#define LED_CHANNEL_NUM      2     //LED通道个数
#define KEY_CHANNEL_NUM      2     //KEY通道个数
#define KEY_TIMER_MS      	 20    //key timer inteval

#define WF_RST_KEY   TY_GPIOA_0      //重置建
#define KEY_RST_TIME       3000            //按键重置时间 ms

/***********************************************************
*************************variable define********************
***********************************************************/
BOOL_T is_count_down = TRUE; // 倒计时开关
BOOL_T count_downing = FALSE; // 倒计时开关

TIMER_ID cd_timer;// 倒计时定时器
int cd_upload_period = 30;// 倒计时状态上报周期 单位:秒

OPERATE_RET dp_upload_proc(CHAR_T *jstr);
VOID Start_boot_up(VOID);

// 倒计时回调
STATIC VOID cd_timer_cb(UINT_T timerID,PVOID_T pTimerArg);

typedef struct
{
    INT_T   ionum;
    TY_GPIO_PORT_E iopin[8];
}CTRL_GROUP;

typedef struct
{
    INT_T group_num;
    CTRL_GROUP *group;
}GPIO_TEST_TABLE;
#if 1
STATIC CTRL_GROUP groups[] = 
{
    // group 0
    {
        //
        .ionum  = 3,
        //  TY_GPIOA_19,
        .iopin = {TY_GPIOA_15, TY_GPIOA_19, TY_GPIOA_22}
    },
    #if 1
     // group 1
    {
        // 继电器
        .ionum  = 2,
        // 按钮
        .iopin = {TY_GPIOA_14, TY_GPIOA_29}
    },
    #endif
     // group 2
    {
        // 继电器
        .ionum  = 2,
        // 按钮
        .iopin = {TY_GPIOA_0, TY_GPIOA_5}
    },
    #if 1
     // group 3
    {
        // 继电器
        .ionum  = 2,
        // 按钮
        .iopin = {TY_GPIOA_12, TY_GPIOA_30}
    },
    #endif
};
GPIO_TEST_TABLE gpio_test_table = {
    .group_num = 4,
    .group = groups
};
#endif

/***********************************************************
*************************function define********************
***********************************************************/

/***********************************************************
*  Function: app_init
*  Input: none
*  Output: none
*  Return: none
*  Note: called by user_main
***********************************************************/
VOID app_init(VOID) {
    return ;
}

VOID mf_user_callback(VOID)
{

}
VOID status_changed_cb(IN CONST GW_STATUS_E status)
{
    PR_DEBUG("gw status changed to %d", status);

    if(GW_NORMAL == status) {
        Start_boot_up();
    }
}

OPERATE_RET get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len, IN CONST UINT_T offset,
                                     IN CONST BYTE_T *data, IN CONST UINT_T len, OUT UINT_T *remain_len, IN PVOID pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("Total_len:%d ", total_len);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    return OPRT_OK;
}

VOID upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID pri_data)
{
    PR_DEBUG("download  Finish");
    PR_DEBUG("download_result:%d", download_result);
}

VOID gw_ug_inform_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev GW Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    //PR_DEBUG("fw->fw_md5:%s", fw->fw_md5);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%d", fw->file_size);

    tuya_iot_upgrade_gw(fw, get_file_data_cb, upgrade_notify_cb, NULL);
}

VOID dev_dp_query_cb(IN CONST TY_DP_QUERY_S *dp_qry)
{
    PR_DEBUG("Recv DP Query Cmd");
}

VOID dev_obj_dp_cb(IN CONST TY_RECV_OBJ_DP_S *dp)
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
    }
    // UserTODO

    // 用户处理完成之后需要主动上报最新状态，这里简单起见，直接返回收到的数据，认为处理全部成功。
    OPERATE_RET op_ret = dev_report_dp_json_async(dp->cid,dp->dps,dp->dps_cnt);
    if(OPRT_OK != op_ret) {
        PR_ERR("dev_report_dp_json_async op_ret:%d",op_ret);
    }

#if 0
    OPERATE_RET op_ret = OPRT_OK;
    op_ret = dev_report_dp_json_async(dp->cid,dp->dps,dp->dps_cnt);
    if(OPRT_OK != op_ret) {
        PR_ERR("dev_report_dp_json_async error:%d",op_ret);
    }
    //dev_report_dp_json_async(IN CONST CHAR *dev_id,IN CONST TY_OBJ_DP_S *dp_data,IN CONST UINT cnt)
    //dev_report_dp_stat_sync(IN CONST CHAR *dev_id,IN CONST TY_OBJ_DP_S *dp_data,IN CONST UINT cnt,IN CONST UINT timeout)
#endif
}

VOID dev_raw_dp_cb(IN CONST TY_RECV_RAW_DP_S *dp)
{
    PR_DEBUG("raw data dpid:%d",dp->dpid);

    PR_DEBUG("recv len:%d",dp->len);
    #if 0
    INT_T i = 0;
    
    for(i = 0;i < dp->len;i++) {
        PR_DEBUG_RAW("%02X ",dp->data[i]);
    }
    #endif
    PR_DEBUG_RAW("\n");
    PR_DEBUG("end");
}

STATIC VOID __get_wf_status(IN CONST GW_WIFI_NW_STAT_E stat)
{
    hw_set_wifi_led_stat(&g_hw_table, stat);

    return;
}

/***********************************************************
*  Function: gpio_test
*  Input: none
*  Output: none
*  Return: none
*  Note: For production testing
***********************************************************/
BOOL_T gpio_test(VOID)
{
#if 1
    // sys_log_uart_off();
    INT_T idx,i,j;
    for(idx = 0; idx < gpio_test_table.group_num; idx++) {
        for(i = 0; i < gpio_test_table.group[idx].ionum; i++) {
            //set io direction
            for(j = 0; j < gpio_test_table.group[idx].ionum; j++) {
                if(i== j) {
                    tuya_gpio_inout_set(gpio_test_table.group[idx].iopin[j],FALSE);
                }else {
                    tuya_gpio_inout_set(gpio_test_table.group[idx].iopin[j],TRUE);
                }
            }
            PR_NOTICE("2222");
            // write 1
            tuya_gpio_write(gpio_test_table.group[idx].iopin[i],TRUE);
            for(j = 0; j < gpio_test_table.group[idx].ionum; j++) {
                if(i!= j) {
                    if(tuya_gpio_read(gpio_test_table.group[idx].iopin[j]) != 1) {
                        PR_ERR("gpio test err");
                        return FALSE;
                    }
                }
            }

            // write 0
            tuya_gpio_write(gpio_test_table.group[idx].iopin[i],FALSE);
            for(j = 0; j < gpio_test_table.group[idx].ionum; j++) {
                if(i!= j) {
                    if(tuya_gpio_read(gpio_test_table.group[idx].iopin[j]) != 0) {
                        PR_ERR("gpio test err");
                        return FALSE;
                    }
                }
            }
            
        }
    }
#endif
    // sys_log_uart_on();
    PR_NOTICE("gpio test succ");
    return TRUE;
}



/***********************************************************
*  Function: pre_device_init
*  Input: none
*  Output: none
*  Return: none
*  Note: to initialize device before device_init
***********************************************************/
VOID pre_device_init(VOID)
{
    PR_DEBUG("%s",tuya_iot_get_sdk_info());
    PR_DEBUG("%s:%s",APP_BIN_NAME,DEV_SW_VERSION);
    SetLogManageAttr(TY_LOG_LEVEL_DEBUG);
}

#include "tuya_cli.h"
#include "mf_test.h"


extern int mf_test_control(uint8_t cmd, void *args);

void facpin_test(int argc, char *argv[])
{   
    CHAR_T facpin[20 + 1];

    int result = mf_test_facpin_get(facpin);

    if (OPRT_OK != result) {
        PR_ERR("facpin get failed %d", result);
        return;
    }
    PR_NOTICE("facpin -> %s", facpin);

    CHAR_T facpin2[20 + 1];

    mf_test_control(2, facpin2);
    if (OPRT_OK != result) {
        PR_ERR("facpin2 get failed %d", result);
        return;
    }
    PR_NOTICE("facpin2 -> %s", facpin);
}

void device_local_reset(int argc, char *argv[])
{
    int result;

    if (argc > 1 && (0 == strcmp("factory", argv[1])))  {
        PR_DEBUG("cmd is reset factory, ungister");
        result = tuya_iot_wf_gw_reset();
    } else {
        PR_DEBUG("unactive");
        result = tuya_iot_wf_gw_unactive();
    }

    if (OPRT_OK != result) {
        PR_ERR("gw reset error:%d",result);
    }
}

static const cli_cmd_t s_test_cmd[] = {
    {
        .name   = "facpin",
        .help   = "facpin read test",
        .func   = facpin_test,
    },
    {
        .name   = "reset",
        .help   = "local reset [factory]",
        .func = device_local_reset,
    }
};


/***********************************************************
*  Function: device_init 
*  Input: none
*  Output: none
*  Return: none
***********************************************************/
OPERATE_RET device_init(VOID)
{
    OPERATE_RET op_ret = OPRT_OK;
    // tuya_iot_wf_nw_cfg_ap_pri_set(TRUE);
	
    TY_IOT_CBS_S wf_cbs = {
        status_changed_cb,\
        gw_ug_inform_cb,\
        NULL,
        dev_obj_dp_cb,\
        dev_raw_dp_cb,\
        dev_dp_query_cb,\
        NULL,
    };
    op_ret = tuya_iot_wf_soc_dev_init(GWCM_OLD, WF_START_SMART_AP_CONCURRENT,&wf_cbs,PRODUCT_KEY,DEV_SW_VERSION);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_wf_soc_dev_init err:%d",op_ret);
        return -1;
    }

    op_ret = tuya_iot_reg_get_wf_nw_stat_cb(__get_wf_status);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_reg_get_wf_nw_stat_cb err:%d",op_ret);
        return op_ret;
    }
 
    init_hw(&g_hw_table);

    tuya_cli_init();
    tuya_cli_cmd_register(s_test_cmd, sizeof(s_test_cmd)/sizeof(s_test_cmd[0]));

    extern void tuya_test_cli_cmd_init(void);
    tuya_test_cli_cmd_init();

    return OPRT_OK;
}



STATIC VOID cd_timer_cb(UINT_T timerID,PVOID_T pTimerArg)
{
    int i;// 通道号
    char buff[20];
    // 遍历通道
    for(i=0; i<g_hw_table.channel_num; ++i)
    {
        if(g_hw_table.channels[i].cd_sec < 0)
        {
            continue;// 通道计时关闭
        }
        else
        {
            // 通道计时中
            --g_hw_table.channels[i].cd_sec;

            if(g_hw_table.channels[i].cd_sec <= 0)// 计时到达
            {
                // 置反通道状态
                hw_trig_channel(&g_hw_table, i);
                // 上报通道状态
                if(g_hw_table.channels[i].stat)
                {
                    sprintf(buff, "{\"%d\":true}", g_hw_table.channels[i].dpid);
                }
                else
                {
                    sprintf(buff, "{\"%d\":false}", g_hw_table.channels[i].dpid);
                }
                PR_DEBUG("report countdown timer44");
                dp_upload_proc(buff);
                
                // 上报通道倒计时状态
                sprintf(buff, "{\"%d\":0}",  g_hw_table.channels[i].cd_dpid);
                dp_upload_proc(buff);
                g_hw_table.channels[i].cd_sec = -1; // 关闭通道定时
            }else {
                // 计时未到达
                // 每30s的整数倍上报一次
                if(g_hw_table.channels[i].cd_sec % 30 == 0)
                {
                    sprintf(buff, "{\"%d\":%d}", g_hw_table.channels[i].cd_dpid, g_hw_table.channels[i].cd_sec);
                    PR_DEBUG("report countdown timer55");
                    dp_upload_proc(buff);
                }
            }
        }

    }
}



OPERATE_RET dp_upload_proc(CHAR_T *jstr)
{
    OPERATE_RET op_ret;
    op_ret = sf_obj_dp_report_async(get_gw_cntl()->gw_if.id,jstr,false);
    if(OPRT_OK != op_ret) {
        PR_ERR("sf_obj_dp_report op_ret:%d",op_ret);
        return op_ret;
    }else {
        return OPRT_OK;
    }
}

VOID Start_boot_up(VOID)
{
    UCHAR_T i=0;
    char buff[30];

    for(i = 0; i < g_hw_table.channel_num; i++)
    {
        if(g_hw_table.channels[i].stat){
            sprintf(buff, "{\"%d\":true}", g_hw_table.channels[i].dpid);
        }
        else{
            sprintf(buff, "{\"%d\":false}", g_hw_table.channels[i].dpid);
        }
        dp_upload_proc(buff);

        if(is_count_down){
            g_hw_table.channels[i].cd_sec = -1;
            sprintf(buff, "{\"%d\":0}", g_hw_table.channels[i].cd_dpid);// 回复倒计时0
            
            // 推送倒计时数据
            PR_DEBUG("report countdown timer00");
            dp_upload_proc(buff);
        }
   }

}


/***********************************************************
*  Function:pre_app_init
*  Input: none
*  Output: none
*  Return: none
*  Note: 应用初始化预处理
***********************************************************/
VOID pre_app_init(VOID_T)
{


}


/***********************************************************
*  Function:user_net_changed_cb
*  Input: none
*  Output: none
*  Return: none
*  Note: 进入产测回调
***********************************************************/
VOID mf_user_enter_callback(VOID_T)
{


}


/***********************************************************
*  Function:mf_user_product_test_cb
*  Input: none
*  Output: none
*  Return: none
*  Note: 成品产测
***********************************************************/
OPERATE_RET mf_user_product_test_cb(USHORT_T cmd,UCHAR_T *data, UINT_T len, OUT UCHAR_T **ret_data,OUT USHORT_T *ret_len)
{
    return OPRT_OK;
}

/***********************************************************
*  Function:mf_user_pre_gpio_test_cb
*  Input: none
*  Output: none
*  Return: none
*  Note: 成品产测
***********************************************************/
VOID mf_user_pre_gpio_test_cb(VOID_T)
{
    return OPRT_OK;
}
