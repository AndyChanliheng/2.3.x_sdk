#include "tuya_iot_soc_dev_entry.h"
#include "tuya_hal_wired.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tuya_cloud_types.h"

/***********************************************************
*************************micro define***********************
***********************************************************/

/***********************************************************
*************************variable define********************
***********************************************************/
OPERATE_RET tuya_adapter_wired_get_nw_stat(GW_BASE_NW_STAT_T *stat)
{
	return OPRT_OK;
}

// 获取有线网卡的ip地址
OPERATE_RET tuya_adapter_wired_get_ip(OUT NW_IP_S *ip)
{
    // UserTODO
    // 获取系统的一个UP的，并且不是loopback的网络接口，近在linux环境上验证过
#if 0
    struct if_nameindex *name_list;
    CHAR_T *name = NULL;
    int i = 0;

    name_list = if_nameindex();
    if (NULL == name)
        return OPRT_NOT_FOUND;

    while (name_list[i].if_index != 0) {
        if (strcmp(name_list[i].if_name, "lo") != 0) {
            name = name_list[i].if_name;
            break;
        }
    }
    
    int sock;
    char ipaddr[50];

    struct sockaddr_in *sin;
    struct ifreq ifr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
         printf("socket create failse...GetLocalIp!\n");
         return OPRT_COM_ERROR;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "enp0s3", sizeof(ifr.ifr_name) - 1);
    if( ioctl(sock, SIOCGIFADDR, &ifr) < 0 ) {
        printf("ioctl error\n");
        close(sock);
        return OPRT_COM_ERROR;
    }

    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    strcpy(ip->ip,inet_ntoa(sin->sin_addr));
    close(sock);
#else
    //strncpy(ip->ip, ty_devos_get_ipaddr(), SIZEOF(ip->ip));
    strncpy(ip->ip, "192.168.31.168", SIZEOF(ip->ip));
#endif
    return OPRT_OK;
}

// 硬件是否连接外网
BOOL_T tuya_adapter_wired_station_conn(VOID)
{
    // UserTODO
    return TRUE;
}

// 若硬件形态为wifi+有线模式，而且用户需要连接外部WIFI，那么需要实现连接WIFI回调函数，sdk内部会自动调用
OPERATE_RET tuya_adapter_wired_wifi_set_station_connect(IN CONST CHAR_T *ssid,IN CONST CHAR_T *passwd)
{
    // UserTODO
    return OPRT_COM_ERROR;
}

// 若硬件形态为wifi+有线模式，而且用户需要连接外部WIFI，则返回TRUE，否者返回FALSE
BOOL_T tuya_adapter_wired_wifi_need_cfg(VOID)
{
    // UserTODO
    return FALSE;
}

// 若硬件形态为wifi+有线模式，而且用户需要连接外部WIFI，则返回WIFI实际信号强度，否者返回99
OPERATE_RET tuya_adapter_wired_wifi_station_get_conn_ap_rssi(OUT SCHAR_T *rssi)
{
    // UserTODO
    *rssi = 99;
    return OPRT_OK;
}

// 获取有线网卡的MAC地址
OPERATE_RET tuya_adapter_wired_get_mac(OUT NW_MAC_S *mac)
{
    // UserTODO

    mac->mac[0] = 0xc8;
    mac->mac[1] = 0x5b;
    mac->mac[2] = 0x76;
    mac->mac[3] = 0x4d;
    mac->mac[4] = 0x75;
    mac->mac[5] = 0xcd;

    return OPRT_OK;
}

// 当前无需实现
OPERATE_RET tuya_adapter_wired_set_mac(IN CONST NW_MAC_S *mac)
{
    return OPRT_OK;
}

OPERATE_RET tuya_adapter_wired_if_connect_internet(bool *status)
{
    return OPRT_OK;
}

/* add end */

