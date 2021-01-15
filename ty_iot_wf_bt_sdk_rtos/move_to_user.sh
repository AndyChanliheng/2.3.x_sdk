#########################################################################
# File Name: move_to_user.sh
# Author: zll
# mail: zhnlion@126.com
# Created Time: Thu 17 Dec 2020 03:29:46 PM CST
#########################################################################
#!/bin/bash
make clean
make config
make sdk
cp output/bl608_ty_iot_wf_bt_sdk_rtos/lib/libtuya_iot.a ../../2.3.x_app/ty_iot_wf_au_bl608_app/sdk/lib/libtuya_iot.a
cp output/bl608_ty_iot_wf_bt_sdk_rtos/lib/libtuya_iot.a.stripped ../../2.3.x_app/ty_iot_wf_au_bl608_app/sdk/lib/libtuya_iot.a.stripped
