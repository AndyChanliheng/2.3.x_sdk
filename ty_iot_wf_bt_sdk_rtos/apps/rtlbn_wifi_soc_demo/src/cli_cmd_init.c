#include "tuya_cloud_com_defs.h"


extern void mqtt_app_ext_proto_test(void);
extern void lan_pro_cntl_test(void);


void tuya_test_cli_cmd_init(void)
{
    mqtt_app_ext_proto_test();
    lan_pro_cntl_test();
}