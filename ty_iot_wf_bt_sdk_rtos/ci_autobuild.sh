#!/bin/sh

# CI系统传入的参数说明：
#
# $1 - SDK项目名称，如： tuya_iot_wifi_tls
# $2 - SDK产物版本，如： 4.1.3
# $3 - 开发环境名称，如： linux-ubuntu-6.2.0_64Bit
# $4 - SDK产物包路径，如： output/dist/tuya_iot_wifi_tls_linux-ubuntu-6.2.0_64Bit_4.1.3.tar.gz
#
# 例：
# ./ci_autobuild.sh tuya_iot_wifi_tls 4.1.3 linux-ubuntu-6.2.0_64Bit output/dist/tuya_iot_wifi_tls_linux-ubuntu-6.2.0_64Bit_4.1.3.tar.gz


set -e

cd `dirname $0`

# 通过环境变量传递生成的产物包全路径名称
export CI_PACKAGE_FULLNAME="$(pwd)/$4"

./prepare.sh "$1" "$2" "$3"

echo -e "Build SDK Begin"
make clean
make config
make pack
echo -e "Build Finish"

