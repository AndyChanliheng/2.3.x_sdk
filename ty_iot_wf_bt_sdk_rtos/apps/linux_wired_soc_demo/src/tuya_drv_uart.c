 /*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 * @author  :   Linch                                                         *
 * @date    :   2019-08-28                                                    *
 * @brief   :                                                                 *
 *                                                                            *
 =============================================================================*/

/*============================ INCLUDES ======================================*/
#include "tuya_uart.h"
#include "stdio.h"
#include <termios.h>
#include "mem_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

/*============================ MACROS ========================================*/
#define UART_DEV_NUM            2

#define UART_DEV_CHECK(__UART, __PORT)                                      \
    __UART = uart_dev_instance_get(__PORT);                                 \
    if (NULL == __UART) {                                                   \
        return OPRT_INVALID_PARM;                                           \
    }

/*============================ TYPES =========================================*/
typedef struct {
    tuya_uart_t         dev;
    pthread_t           id;
    int                 fd;
    tuya_uart_isr_cb    isr_cb;
} uart_dev_t;

/*============================ PROTOTYPES ====================================*/
static int uart_dev_init        (tuya_uart_t *uart, tuya_uart_cfg_t *cfg);
static int uart_dev_write_byte  (tuya_uart_t *uart, uint8_t byte);
static int uart_dev_read_byte   (tuya_uart_t *uart, uint8_t *byte);
static int uart_dev_control     (tuya_uart_t *uart, uint8_t cmd, void *arg);
static int uart_dev_deinit      (tuya_uart_t *uart);

/*============================ LOCAL VARIABLES ===============================*/
static uart_dev_t s_uart_dev[UART_DEV_NUM];

static const tuya_uart_ops_t  uart_dev_ops = {
    .init       = uart_dev_init,
    .read_byte  = uart_dev_read_byte,
    .write_byte = uart_dev_write_byte,
    .control    = uart_dev_control,
    .deinit     = uart_dev_deinit,
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/
int platform_uart_init(void)
{
    s_uart_dev[TUYA_UART0].dev.ops = (tuya_uart_ops_t *)&uart_dev_ops;
    TUYA_UART_8N1_CFG(&s_uart_dev[TUYA_UART0].dev, TUYA_UART_BAUDRATE_115200, 256, 0);
    tuya_driver_register(&s_uart_dev[TUYA_UART0].dev.node, TUYA_DRV_UART, TUYA_UART0);
}


static void uart_dev_irq_handler(void *parameter)
{
    uart_dev_t *uart_dev = (uart_dev_t *)parameter;

    for (;;) {
        fd_set readfd;

        FD_ZERO(&readfd);
        FD_SET(uart_dev->fd, &readfd);
        select(uart_dev->fd + 1, &readfd, NULL, NULL, NULL);
        if (FD_ISSET(uart_dev->fd, &readfd)){  		
            uart_dev->isr_cb(&uart_dev->dev, TUYA_UART_INT_RX_EVENT);
        }
    }
}

static int uart_dev_init(tuya_uart_t *uart, tuya_uart_cfg_t *cfg)
{
    uart_dev_t *uart_dev = (uart_dev_t *)uart;

    struct termios term_orig;
    struct termios term_vi;

    if (TUYA_UART0 == uart->node.port) {
        struct termios term_orig;
        struct termios term_vi;

        uart_dev->fd = open("/dev/stdin", O_RDWR);
        if (0 > uart_dev->fd) {
            return OPRT_COM_ERROR;
        } 
        tcgetattr(uart_dev->fd, &term_orig);
        term_vi = term_orig;
        term_vi.c_lflag &= (~ICANON & ~ECHO);   // leave ISIG ON- allow intr's
        term_vi.c_iflag &= (~IXON & ~ICRNL);
        tcsetattr(uart_dev->fd, TCSANOW, &term_vi);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&uart_dev->id ,&attr, uart_dev_irq_handler, uart);
    pthread_attr_destroy(&attr);

    return OPRT_OK;
}

static int uart_dev_write_byte(tuya_uart_t *uart, uint8_t byte)
{
    uart_dev_t *uart_dev = (uart_dev_t *)uart;

    write(uart_dev->fd, &byte, 1);

    return OPRT_OK;
}

static int uart_dev_control(tuya_uart_t *uart, uint8_t cmd, void *arg)
{
    int result = OPRT_OK;

    uart_dev_t *uart_dev = (uart_dev_t *)uart;

    switch (cmd) {
    case TUYA_DRV_SET_INT_CMD:
        break;

    case TUYA_DRV_CLR_INT_CMD:
        break;

    case TUYA_DRV_SET_ISR_CMD:
        uart_dev->isr_cb = (tuya_uart_isr_cb)arg;
        break;
    }

    return result;
}

static int uart_dev_read_byte(tuya_uart_t *uart, uint8_t *byte)
{
    uart_dev_t *uart_dev = (uart_dev_t *)uart;

    read(uart_dev->fd, byte, 1);

    return OPRT_OK;
}

static int uart_dev_deinit(tuya_uart_t *uart)
{
    uart_dev_t *uart_dev = (uart_dev_t *)uart;

    return OPRT_OK;
}
