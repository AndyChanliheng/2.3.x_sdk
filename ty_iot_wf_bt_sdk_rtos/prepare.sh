#!/bin/sh

# ==============================================================================
# 下载依赖组件和工具链，并生成编译配置文件
# ------------------------------------------------------------------------------
# 入参说明：
#
# $1 - SDK项目名称，如： tuya_iot_wifi_tls
# $2 - SDK产物版本，如： 4.1.3
# $3 - 开发环境名称，如： linux-ubuntu-6.2.0_64Bit
#
# ------------------------------------------------------------------------------
# 使用示例：
#
# ./prepare.sh tuya_iot_wired_tls 4.1.3 linux-ubuntu-6.2.0_64Bit
#
# ------------------------------------------------------------------------------


print_not_null()
{
    # $1 为空，返回错误
    if [ x"$1" = x"" ]; then
        return 1
    fi

    echo "$1"
}


set -e
cd `dirname $0`


PROJECT_NAME=`print_not_null $1 || basename $PWD`
PROJECT_VERSION=`print_not_null $2 || bash ./scripts/get_ver_tag.sh`
TARGET_PLATFORM=`print_not_null $3 || bash ./scripts/get_platform.sh make.yaml`
echo PROJECT_NAME=$PROJECT_NAME
echo PROJECT_VERSION=$PROJECT_VERSION
echo TARGET_PLATFORM=$TARGET_PLATFORM


[ -z $PROJECT_NAME ] && echo "error: no project name!" && exit 99
[ -z $PROJECT_VERSION ] && echo "error: no version!" && exit 99
[ -z $TARGET_PLATFORM ] && echo "error: no platform!" && exit 99


embed update --platforms=$TARGET_PLATFORM --sdkversion=$PROJECT_VERSION

CI_DOWNLOAD_DIR=./tmp/${PROJECT_NAME}_${TARGET_PLATFORM}_${PROJECT_VERSION}
if [ -d $CI_DOWNLOAD_DIR/sdk/include ]; then 
    cp -a $CI_DOWNLOAD_DIR/sdk/include ./sdk/
fi

if [ -f platforms/$TARGET_PLATFORM/prepare.sh ]; then
    bash platforms/$TARGET_PLATFORM/prepare.sh
fi

bash ./scripts/prepare_param.sh $PROJECT_NAME $PROJECT_VERSION $TARGET_PLATFORM

