#include "uni_log.h"
#include "tuya_cli.h"

void lan_pro_cntl_disable_cmd(int argc, char *argv[])
{
    lan_pro_cntl_disable();
}

void lan_pro_cntl_enable_cmd(int argc, char *argv[])
{
    lan_pro_cntl_enable();
}

static const cli_cmd_t s_lan_cntl_test_cmd[] = {
    {
        .name   = "lan_disable",
        .help   = "lan_pro_cntl_disable test",
        .func   = lan_pro_cntl_disable_cmd,
    },
    {
        .name   = "lan_enable",
        .help   = "lan_pro_cntl_enable test",
        .func   = lan_pro_cntl_enable_cmd,
    }
};


void lan_pro_cntl_test(void)
{
    tuya_cli_cmd_register(s_lan_cntl_test_cmd, sizeof(s_lan_cntl_test_cmd)/sizeof(s_lan_cntl_test_cmd[0]));
}