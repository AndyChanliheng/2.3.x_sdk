#include "mqc_app.h"
#include "uni_log.h"

static char route_passwd[64 + 1] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

OPERATE_RET mqtt_route_passwd_reply(char *passwd, int netid)
{
    char data[256];

    //! make data
    snprintf(data, sizeof(data) - 1, 
            "{\"reqType\":\"routerPwd\",\"routerNetId\":%d,\"routerPwd\":\"%s\"}",
            netid, passwd);

    int oprt = mqc_app_ext_proto_data_rept(data, 1);

    if (OPRT_OK != oprt) {
        PR_ERR("mqc app ext proto pub failed");
    }

    return oprt;
}

OPERATE_RET mqtt_route_passwd_modify(IN ty_cJSON *root_json)
{
    cJSON *passwd;
    cJSON *netid;

    passwd = cJSON_GetObjectItem(root_json, "routerPwd");
    strncpy(route_passwd, passwd->valuestring, sizeof(route_passwd) - 1);

    netid = cJSON_GetObjectItem(root_json, "routerNetId");

    return mqtt_route_passwd_reply(route_passwd, netid->valueint);
}

OPERATE_RET mqtt_route_passwd_query(IN ty_cJSON *root_json)
{
    cJSON *netid;

    netid = cJSON_GetObjectItem(root_json, "routerNetId");

    return mqtt_route_passwd_reply(route_passwd, netid->valueint);
}


#include "tuya_cli.h"


void mqc_app_ext_unreg_test(int argc, char *argv[])
{
    int result;
    mqc_app_unreg_ext_proto("routerPwdQry");
}


extern VOID mqc_app_disconnect(VOID);
extern VOID mqc_restart(VOID);


void mqc_app_disconnect_cmd(int argc, char *argv[])
{
    mqc_app_disconnect();
}

void mqc_restart_cmd(int argc, char *argv[])
{
    mqc_restart();
}

void sum(int argc, char *argv[])
{
    int sum;
 
    sum = atoi(argv[1]) + atoi(argv[2]);
 
    PR_DEBUG("sum = %d", sum);
}

static const cli_cmd_t s_mqc_test_cmd[] = {
    {
        .name   = "mqc_disconenct",
        .help   = "mqtt disconenct test",
        .func   = mqc_app_disconnect_cmd,
    },
    {
        .name   = "mqc_restart",
        .help   = "mqtt restart test",
        .func   = mqc_restart_cmd,
    },
    {
        .name   = "mqc_unreg",
        .help   = "mqtt app ext unreg test",
        .func   = mqc_app_ext_unreg_test,
    },
    {
        .name   = "sum",
        .help   = "sum test",
        .func   = sum,
    },
};



VOID mqtt_app_ext_proto_test(VOID)
{
    mqc_app_reg_ext_proto("routerPwdQry", mqtt_route_passwd_query);
    mqc_app_reg_ext_proto("routerPwd",    mqtt_route_passwd_modify);

    tuya_cli_cmd_register(s_mqc_test_cmd, sizeof(s_mqc_test_cmd)/sizeof(s_mqc_test_cmd[0]));
}   