#!/bin/sh

cd `dirname $0`

app_name=demo_soc_dev_wired
app_path=$(pwd)
app=$app_path/$app_name
cd -

cd `dirname $1`
cfg=$(pwd)/`basename $1`
echo $cfg
cd -

mkdir -p $app_path/tuya_db_files

while true
do
    $app $cfg $app_path
    code=$?
    if [ $code -eq 1 ];then
        upgrade_name=upgrade
        cd `dirname $0`
        mv -f $upgrade_name $app_name
        chmod +x $app_name
    elif [ $code -eq 255 ];then
        return
    fi
done

