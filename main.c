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

#ifdef CONFIG_BOOT_MMC
uint32_t cpu_freq = CONFIG_CPU_SEL_PLL == APLL ? CONFIG_APLL_FREQ : CONFIG_MPLL_FREQ;

#elif (defined CONFIG_BOOT_USB)
/*
 * Bootrom set apll freq for usb boot
 */
#if (CONFIG_EXTAL_FREQ == 24)
uint32_t cpu_freq = 576;
#elif (CONFIG_EXTAL_FREQ == 26)
uint32_t cpu_freq = 624;
#endif

#else
uint32_t cpu_freq = CONFIG_EXTAL_FREQ;
#endif

__attribute__((weak, alias("board_init"))) void board_init(void) {}
__attribute__((weak, alias("board_early_init"))) void board_early_init(void) {}

void x_loader_main(void) {

#ifdef CONFIG_SOFT_BURN
    jump_to_usbboot();
#endif

    /*
     * Open CPU AHB0 APB RTC TCU EFUSE NEMC clock gate
     */
    cpm_outl(0x87fbfffc, CPM_CLKGR);

    /*
     * Don't ask, it's magic...
     */
    cpm_outl(0, CPM_PSWC0ST);
    cpm_outl(16, CPM_PSWC1ST);
    cpm_outl(24, CPM_PSWC2ST);
    cpm_outl(8, CPM_PSWC3ST);

    /*
     * Init board early
     */
    board_early_init();

    /*
     * Init uart
     */
    uart_init();

    /*
     * check SOC id
     */
#ifdef CONFIG_CHECK_SOC_ID
    check_socid();
#endif

    uart_puts("\n\nX-Loader Build: " X_LOADER_DATE " - " X_LOADER_TIME);

    /*
     * Print error pc register
     */
    uint32_t errorpc;
    __asm__ __volatile__ (
        "mfc0 %0, $30, 0\n\t"
        "nop \n\t"
        :"=r"(errorpc)
        :);
    printf("\nreset errorpc: 0x%x\n", errorpc);

    /*
     * Init clock
     */
    clk_init();

#ifdef CONFIG_WDT
    /*
     * Init wdt
     */
    wdt_init();
#endif

    /*
     * Init lpddr
     */
    lpddr_init();

#ifdef CONFIG_DDR_ACCESS_TEST
    /*
     * DDR R/W test
     */
    ddr_access_test();

    hang_reason("Memory test done.\n");
#endif

#ifdef CONFIG_RTCCLK_SRC_EXT
    /*
     * RTC clock source change to external main crystal
     */
    rtc_clk_src_to_ext();
#endif

    /*
     * Init board
     */
    board_init();

#ifndef CONFIG_BOOT_USB
    uart_puts("Going to boot next stage.\n");

    /*
     * Boot next stage(kernel/u-boot/rtos)
     */
    boot_next_stage();

#else /* CONFIG_BOOT_USB */

#ifdef CONFIG_PM_SUSPEND
    suspend_enter(CONFIG_PM_SUSPEND_STATE);
#endif

    pass_params_to_uboot();

    uart_puts("Going to start burner.\n");

    /*
     * Return to rom
     */
    return;
#endif
}
