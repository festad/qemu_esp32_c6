/*
 * ESP32-C6 Interrupt Matrix
 * NOTE: This file is just a 1:1 copy of C3's intmatrix, but with the right registers
 *
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */
#pragma once

#include "hw/riscv/riscv_hart.h"
#include "target/riscv/esp_cpu.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "hw/misc/esp32c6_reg.h"


/**
 * Number of inputs in the C6's Interrupt Matrix
 */
#define ESP32C6_INT_MATRIX_INPUTS   77

/**
 * Name of the output lines of the Interrupt Matrix (that shall be connected to the CPU)
 */
#define ESP32C6_INT_MATRIX_OUTPUT_NAME "misc.esp32c6.intmatrix.out_irqs"

/**
 * Number of CPU peripheral interrupts on the C3.
 * This can be considered the output of the interrupt matrix.
 */
#define ESP32C6_CPU_INT_COUNT       28
#define ESP32C6_CPU_INT_MAX         32

#define TYPE_ESP32C6_INTMATRIX "misc.esp32c6.intmatrix"
#define TYPE_ESP32C6_INTMATRIX_PRIO "misc.esp32c6.intmatrix-prio"
#define ESP32C6_INTMATRIX(obj) OBJECT_CHECK(ESP32C6IntMatrixState, (obj), TYPE_ESP32C6_INTMATRIX)

/******************************
 * Interrupt Matrix Registers *
 * base address: 0x60010000   *
 ******************************/
/**
 * Size of the I/O region, in bytes, of the C3 Interrupt Matrix
 */
#define ESP32C6_INTMATRIX_IO_SIZE (0x800)
#define ESP32C6_INTMATRIX_PRIO_IO_SIZE (0xA4)

/**
 * Index of the other interrupt matrix registers
 */
#define ESP32C6_INTMTX_CORE0_INT_STATUS_0_REG (0x0134 / sizeof(uint32_t))
#define ESP32C6_INTMTX_CORE0_INT_STATUS_1_REG (0x0138 / sizeof(uint32_t))
#define ESP32C6_INTMTX_CORE0_INT_STATUS_2_REG (0x013C / sizeof(uint32_t))
#define ESP32C6_INTMTX_CORE0_INTERRUPT_REG_DATE_REG (0x07FC / sizeof(uint32_t))


/***************************************
 * Interrupt Priority Matrix Registers *
 * base address: 0x600c5000            *
 ***************************************/
#define ESP32C6_INTPRI_CORE0_CPU_INT_ENABLE_REG     (0x000 / sizeof(uint32_t)) /* A bitmap that shows which among the 32 CPU interrupts is enabled*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_TYPE_REG       (0x004 / sizeof(uint32_t)) /* A bitmap that shows which type (level/edge) each of the 32 CPU interrupts is*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_EIP_STATUS_REG (0x008 / sizeof(uint32_t)) /* A bitmap read-only that shows which among the 32 CPU interrupts is in pending state*/

#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_00_REG     (0x00c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_01_REG     (0x010 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_02_REG     (0x014 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_03_REG     (0x018 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_04_REG     (0x01c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_05_REG     (0x020 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_06_REG     (0x024 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_07_REG     (0x028 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_08_REG     (0x02c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_09_REG     (0x030 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_10_REG     (0x034 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_11_REG     (0x038 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_12_REG     (0x03c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_13_REG     (0x040 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_14_REG     (0x044 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_15_REG     (0x048 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_16_REG     (0x04c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_17_REG     (0x050 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_18_REG     (0x054 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_19_REG     (0x058 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_20_REG     (0x05c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_21_REG     (0x060 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_22_REG     (0x064 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_23_REG     (0x068 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_24_REG     (0x06c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_25_REG     (0x070 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_26_REG     (0x074 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_27_REG     (0x078 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_28_REG     (0x07c / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_29_REG     (0x080 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_30_REG     (0x084 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRI_31_REG     (0x088 / sizeof(uint32_t)) /* Contains a 4-bit value between 0 and 15 representing the priority of the CPU interrupt*/

#define ESP32C6_INTPRI_CORE0_CPU_INT_THRESH_REG     (0x08C / sizeof(uint32_t)) /* A minimum value such that all CPU interrupts with priority below this value are ignored*/
#define ESP32C6_INTPRI_CORE0_CPU_INT_CLEAR_REG      (0x0A8 / sizeof(uint32_t)) /* A bitmap where each of the 32 interrupts can be cleared */
#define ESP32C6_INTPRI_CPU_INTR_FROM_CPU_1_REG      (0x090 / sizeof(uint32_t)) /* An undocumented register with only one available bit (a reset bit) */
#define ESP32C6_INTPRI_CPU_INTR_FROM_CPU_2_REG      (0x094 / sizeof(uint32_t)) /* An undocumented register with only one available bit (a reset bit) */
#define ESP32C6_INTPRI_CPU_INTR_FROM_CPU_3_REG      (0x098 / sizeof(uint32_t)) /* An undocumented register with only one available bit (a reset bit) */
#define ESP32C6_INTPRI_CPU_INTR_FROM_CPU_4_REG      (0x09C / sizeof(uint32_t)) /* An undocumented register with only one available bit (a reset bit) */
#define ESP32C6_INTPRI_DATE_REG                     (0x0A0 / sizeof(uint32_t)) /* An undocumented register, most likely used for version controlling */

/**
 * Index where priority registers start
 */
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_START (0x00c / sizeof(uint32_t))
#define ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_END   (0x088 / sizeof(uint32_t))


/* Bit value for the type of interrupt trigger  */
#define ESP322C6_INTMATRIX_TRIG_LEVEL   0
#define ESP322C6_INTMATRIX_TRIG_EDGE    1

typedef struct ESP32C6IntMatrixState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    MemoryRegion iomem_prio;
    uint8_t irq_map[ESP32C6_INT_MATRIX_INPUTS];
    
    /* In the following fields, "interrupts" refer to the CPU lines (31)
     * and not the peripheral source. */

    /* ESP32-C6 CPU has 32 CPU interrupts numbered from 0 to 31, of which 
      - 3 are local CLINT interrupts (0, 3, 4, 7)
      - 28 are  external interrupts (1-2, 5-6, 8-31)
    */

    /* A priority from 0-15 is assigned to each of the 28 non-static interrupts,
       since these 28 interrupts are not contiguous we still allocate space for
       all the 32 interrupt just for simplicity of indexing.
       Notice the missing +1 that makes the difference with the ESP32C3, here
       interrupts are counted starting from 0, not from 1.
    */
    uint8_t irq_prio[ESP32C6_CPU_INT_MAX];

    /* Current priority threshold of the CPU interrupts */
    uint8_t irq_thres;

    /* Keep a bitmap of the pending interrupts */

    /* This bitmap refers to the CPU interrupts (32), so
       the use of an uint32_t type should be sufficient, but
       for now we keep the original type.
    */
    uint64_t irq_pending;

    /* Bitmap that records the enabled/disabled interrupts */

    /* Again, 32 bits should be sufficient because the enabled interrupts
       we are refering to here are the 32 CPU interrupts, not all the 
       77 peripheral source interrupts.
    */
    uint64_t irq_enabled;

    /* The "mode" property, not mentioned in the esp32c3*/
    uint64_t irq_mode;

    /* Fast mirror to access IRQ levels */
    uint64_t irq_levels;
    EspRISCVCPU *cpu;

    /* Output IRQ used to notify the CPU, indexed from 0 to 31.
       In the ESP32C6 the available the non-core local input (CLINT) are
       1,2,5,6,8~31
       The CLINT interrupts are 
       0,3,4,7 
    */
    qemu_irq out_irqs[ESP32C6_CPU_INT_MAX];
} ESP32C6IntMatrixState;

typedef enum {
    ETS_RESERVED0_INTR_SOURCE = 0,
    ETS_RESERVED1_INTR_SOURCE,
    ETS_RESERVED2_INTR_SOURCE,
    ETS_RESERVED3_INTR_SOURCE,
    ETS_RESERVED4_INTR_SOURCE,
    ETS_RESERVED5_INTR_SOURCE,
    ETS_RESERVED6_INTR_SOURCE,
    ETS_RESERVED7_INTR_SOURCE,
    ETS_RESERVED8_INTR_SOURCE,
    ETS_RESERVED9_INTR_SOURCE,
    ETS_RESERVED10_INTR_SOURCE,
    ETS_RESERVED11_INTR_SOURCE,
    ETS_RESERVED12_INTR_SOURCE,
    ETS_PMU_INTR_SOURCE,
    ETS_EFUSE_INTR_SOURCE,
    ETS_LP_RTC_TIMER_INTR_SOURCE,
    ETS_LP_UART_INTR_SOURCE,
    ETS_LP_I2C_INTR_SOURCE,
    ETS_LP_WDT_INTR_SOURCE,
    ETS_LP_PERI_TIMEOUT_INTR_SOURCE,
    ETS_LP_APM_M0_INTR_SOURCE,
    ETS_LP_APM_M1_INTR_SOURCE,
    ETS_CPU_INTR_FROM_CPU_0_INTR_SOURCE,
    ETS_CPU_INTR_FROM_CPU_1_INTR_SOURCE,
    ETS_CPU_INTR_FROM_CPU_2_INTR_SOURCE,
    ETS_CPU_INTR_FROM_CPU_3_INTR_SOURCE,
    ETS_ASSIST_DEBUG_INTR_SOURCE,
    ETS_TRACE_INTR_SOURCE,
    ETS_CACHE_INTR_SOURCE,
    ETS_CPU_PERI_TIMEOUT_INTR_SOURCE,
    ETS_GPIO_INTERRUPT_PRO_INTR_SOURCE,
    ETS_RESERVED31_INTR_SOURCE,
    ETS_RESERVED32_INTR_SOURCE,
    ETS_HP_PERI_TIMEOUT_INTR_SOURCE,
    ETS_RESERVED34_INTR_SOURCE,
    ETS_HP_APM_M0_INTR_SOURCE,
    ETS_HP_APM_M1_INTR_SOURCE,
    ETS_HP_APM_M2_INTR_SOURCE,
    ETS_HP_APM_M3_INTR_SOURCE,
    ETS_LP_APM0_INTR_SOURCE,
    ETS_MSPI_INTR_SOURCE,
    ETS_I2S_INTR_SOURCE,
    ETS_UHCI0_INTR_SOURCE,
    ETS_UART0_INTR_SOURCE,
    ETS_UART1_INTR_SOURCE,
    ETS_LEDC_INTR_SOURCE,
    ETS_TWAI0_INTR_SOURCE,
    ETS_TWAI1_INTR_SOURCE,
    ETS_USB_SERIAL_JTAG_INTR_SOURCE,
    ETS_RMT_INTR_SOURCE,
    ETS_I2C_EXT0_INTR_SOURCE,
    ETS_TG0_T0_INTR_SOURCE,
    ETS_RESERVED52_INTR_SOURCE,
    ETS_TG0_WDT_INTR_SOURCE,
    ETS_TG1_T0_INTR_SOURCE,
    ETS_RESERVED55_INTR_SOURCE,
    ETS_TG1_WDT_INTR_SOURCE,
    ETS_SYSTIMER_TARGET0_INTR_SOURCE,
    ETS_SYSTIMER_TARGET1_INTR_SOURCE,
    ETS_SYSTIMER_TARGET2_INTR_SOURCE,
    ETS_APB_ADC_INTR_SOURCE,
    ETS_PWM_INTR_SOURCE,
    ETS_PCNT_INTR_SOURCE,
    ETS_PARL_IO_INTR_SOURCE,
    ETS_SLC0_INTR_SOURCE,
    ETS_SLC1_INTR_SOURCE,
    ETS_GDMA_IN_CH0_INTR_SOURCE,
    ETS_GDMA_IN_CH1_INTR_SOURCE,
    ETS_GDMA_IN_CH2_INTR_SOURCE,
    ETS_GDMA_OUT_CH0_INTR_SOURCE,
    ETS_GDMA_OUT_CH1_INTR_SOURCE,
    ETS_GDMA_OUT_CH2_INTR_SOURCE,
    ETS_GPSPI2_INTR_SOURCE,
    ETS_AES_INTR_SOURCE,
    ETS_SHA_INTR_SOURCE,
    ETS_RSA_INTR_SOURCE,
    ETS_ECC_INTR_SOURCE,
    ETS_MAX_INTR_SOURCE,                        /**< number of interrupt sources */
} periph_interrupt_t;

_Static_assert(ESP32C6_INT_MATRIX_INPUTS == ETS_MAX_INTR_SOURCE,
               "ESP32C6_INT_MATRIX_INPUTS macroETS_MAX_INTR_SOURCE doesn't match the number of interrupt sources");

/**
 * @brief Since wifi is not supported on ESP32-C6 target emulation, reuse the first interrupt source for ethernet
 */
#define ETS_ETH_MAC_INTR_SOURCE     ETS_RESERVED0_INTR_SOURCE
