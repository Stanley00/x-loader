/*
 *  Copyright (C) 2016 Ingenic Semiconductor Co.,Ltd
 *
 *  X1000 series bootloader for u-boot/rtos/linux
 *
 *  Zhang YanMing <yanming.zhang@ingenic.com, jamincheung@126.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <common.h>
#include <string.h>
#include <assert.h>

#ifdef printf
#undef printf
#endif
#include <stdio.h>

#define RTC_FREQ 32768

struct wdt_cfg {
    uint32_t tdr;
    uint32_t tcsr;
};

struct {
        int div;
        uint32_t value;
} wdt_div_table[] = {
        {1   , TCSR_PRESCALE_1   },
        {4   , TCSR_PRESCALE_4   },
        {16  , TCSR_PRESCALE_16  },
        {64  , TCSR_PRESCALE_64  },
        {256 , TCSR_PRESCALE_256 },
        {1024, TCSR_PRESCALE_1024},
};

int main(int argc, char* argv[]) {
    struct wdt_cfg wdt_cfg;

    uint32_t timeout = CONFIG_WDT_TIMEOUT_MS;
    uint32_t timeout_min, timeout_max;
    uint32_t src_freq;
    uint32_t src_clk;
    int wdt_div = -1;
    int i = 0;

    assert(timeout > 0);

#ifdef CONFIG_WDT_CLK_SRC_RTC

#ifdef CONFIG_RTCCLK_SEL
    src_freq = CONFIG_EXTAL_FREQ * 1000 * 1000 / 512;
#else
    src_freq = RTC_FREQ;
#endif
    src_clk = TCSR_RTC_EN;

#else
    src_freq = CONFIG_EXTAL_FREQ * 1000 * 1000;
    src_clk = TCSR_EXT_EN;
#endif

    memset(&wdt_cfg, 0, sizeof(struct wdt_cfg));

    for (i = 0; i < sizeof(wdt_div_table); i++) {
        timeout_min = wdt_div_table[i].div * 0x2 * 1000 / src_freq; //WDT_TDR set should bigger than 0x1
        timeout_max = wdt_div_table[i].div * 0xffff * 1000 / src_freq;
        if (timeout >= timeout_min && timeout <= timeout_max
                && (src_freq * 2 / wdt_div_table[i].div < CONFIG_EXTAL_FREQ * 1000 * 1000)) {
            wdt_div = wdt_div_table[i].value;
            wdt_cfg.tdr = timeout * src_freq / wdt_div_table[i].div / 1000 + 1;
            break;
        }
    }

    assert(wdt_div != -1);

    wdt_cfg.tcsr = src_clk | wdt_div;

    printf("/*\n");
    printf(" * DO NOT MODIFY.\n");
    printf(" *\n");
    printf(" * This file was generated by wdt_params_creator\n");
    printf(" *\n");
    printf(" */\n");
    printf("\n");

    printf("#ifndef WDT_CFG_H\n");
    printf("#define WDT_CFG_H\n");
    printf("\n");
    printf("#define WDT_CFG_TCSR          0x%x\n", wdt_cfg.tcsr);
    printf("#define WDT_CFG_TDR           0x%x\n", wdt_cfg.tdr);
    printf("\n");
    printf("#endif /* WDT_CFG_H */\n");

    return 0;
}