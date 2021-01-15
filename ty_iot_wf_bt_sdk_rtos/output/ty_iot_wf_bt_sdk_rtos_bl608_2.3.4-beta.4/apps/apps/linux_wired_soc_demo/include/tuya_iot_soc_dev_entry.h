#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"
#include "tuya_cloud_com_defs.h"
#include <stdio.h>
#include <unistd.h>

#define MAX_NAME_LEN        32  // 支持最长32字节
#define MAX_DESC_LEN        32  // 支持最长32字节
#define MAX_UUID_LEN        32  // 支持最长32字节
#define MAX_DB_PATH_LEN     128 // 支持最长128字节
#define MAX_OTA_PATH_LEN    128 // 支持ipv6
#define MAX_IP_ADDR_LEN     128 // 支持ipv6

typedef enum {
    OTA1 = 1,
    OTA2 = 2,
}ty_devos_ota_area_t;

typedef struct {
    unsigned int nw_status;
    unsigned int wm_status;
}ty_devos_status_t;

typedef struct {
    CHAR_T uuid[MAX_UUID_LEN+1];                    // 产品 uuid
    CHAR_T auth_key[AUTH_KEY_LEN+1];                // 产品 uuid
    CHAR_T product_key[PRODUCT_KEY_LEN+1];          // 产品 product id
    CHAR_T firmware_key[PRODUCT_KEY_LEN+1];         // 固件 firmware-key
    CHAR_T storage_path[MAX_DB_PATH_LEN+1];         // 固件 firmware-key
    CHAR_T ipaddr[MAX_IP_ADDR_LEN];                 // 设备对外连接的ip address
    CHAR_T token[TOKEN_LEN];                        // 设备激活使用的token

    ty_devos_ota_area_t cur_area;                   // 当前启动area
    CHAR_T ota_path[MAX_OTA_PATH_LEN];              // 设备激活使用的token

    // status
    ty_devos_status_t status;
}ty_devos_virtual_t;

CHAR_T *ty_devos_get_ipaddr();
CHAR_T *ty_devos_get_token();


