//
// Created by HAIRONG ZHU on 25-1-14.
//

#include "debug_console.h"

#include "esp_console.h"
#include "esp_log.h"


struct {
    struct arg_dbl *parm1 = arg_dbln("m", "parm1", "<float>", 0, 1, "设置参数 parm1");
    struct arg_dbl *parm2 = arg_dbln("q", "parm2", "<float>", 0, 1, "设置参数 parm2");
    struct arg_dbl *parm3 = arg_dbln("p", "parm3", "<float>", 0, 1, "设置参数 parm3");
    struct arg_dbl *parm4 = arg_dbln("i", "parm4", "<float>", 0, 1, "设置参数 parm4");
    struct arg_dbl *parm5 = arg_dbln("d", "parm5", "<float>", 0, 1, "设置参数 parm5");
    struct arg_end *end = arg_end(20);
} set_params_args;

float *m_parm_list[5];

DebugConsole::DebugConsole(float parm_list[5]) {
    for (int i = 0; i < 5; i++) {
        m_parm_list[i] = &parm_list[i];
    }

    const esp_console_cmd_t cmd = {
            .command = "set",
            .help = "设置 5 个参数",
            .hint = nullptr,
            .func = &DebugConsole::set_params_cmd,
            .argtable = &set_params_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    // 启动 REPL，这时你就可以通过串口进行交互了
    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "debug_console>"; // 自定义提示符
    repl_config.max_cmdline_length = 256;

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
#error Unsupported console type
#endif

    // 启动 REPL，这时你就可以通过串口进行交互了
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

int DebugConsole::set_params_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &set_params_args);
    if (nerrors != 0) {
        arg_print_errors(stdout, set_params_args.end, "set_parm");
        return 1;
    }
    if (set_params_args.parm1->count > 0) {
        *m_parm_list[0] = (float) set_params_args.parm1->dval[0];
    }
    if (set_params_args.parm2->count > 0) {
        *m_parm_list[1] = (float) set_params_args.parm2->dval[0];
    }
    if (set_params_args.parm3->count > 0) {
        *m_parm_list[2] = (float) set_params_args.parm3->dval[0];
    }
    if (set_params_args.parm4->count > 0) {
        *m_parm_list[3] = (float) set_params_args.parm4->dval[0];
    }
    if (set_params_args.parm5->count > 0) {
        *m_parm_list[4] = (float) set_params_args.parm5->dval[0];
    }
    // 同时打印5个参数
    ESP_LOGI("set_parm", "parm0: %f, parm1: %f, parm2: %f, parm3: %f, parm4: %f",
             *m_parm_list[0], *m_parm_list[1], *m_parm_list[2], *m_parm_list[3], *m_parm_list[4]);
    return 0;
}