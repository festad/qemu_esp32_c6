/*
 * ESP32-C6 Interrupt Matrix
 *
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/queue.h"
#include "qemu/timer.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/riscv/riscv_hart.h"
#include "hw/riscv/esp32c6_intmatrix.h"
#include "esp_cpu.h"

#define INTMATRIX_DEBUG     1 // TODO disable
#define INTMATRIX_WARNING   1

#define BIT_SET(reg, bit)   ((reg) & BIT(bit))
#define CLEAR_BIT(reg, bit) do { (reg) &= ~BIT(bit); } while(0)
#define SET_BIT(reg, bit)   do { (reg) |= BIT(bit); } while(0)


static void esp32c6_do_int(ESP32C6IntMatrixState *s, int line)
{
    qemu_irq_pulse(s->out_irqs[line]);
}


static int esp32c6_get_output_line_level(ESP32C6IntMatrixState *s, int line)
{
    int level_shared = 0;

    for (int i = 0; level_shared == 0 && i < ESP32C6_INT_MATRIX_INPUTS; i++) {
        const uint_fast8_t mapped = s->irq_map[i];
        if (mapped == line && (s->irq_levels & BIT(i)))
        {
            level_shared |= 1;
        }
    }

    return level_shared;
}

/**
 * Try to clear the given interrupt from the pending bitmap.
 * If the signal is shared with other interrupt sources, make sure all of them are low (0)
 * before clearing the pending IRQ from the bitmap.
 */
static void esp32c6_intmatrix_clear_pending(ESP32C6IntMatrixState *s, int line)
{
    /* Check if another GPIO IRQ is sharing the same output line. They must all be low before
     * clearing the pending bit. This is due to the fact that output lines
     * can be shared on the ESP32-C3 */
    const int output_level = esp32c6_get_output_line_level(s, line);
    if (!output_level) {
        /* The output interrupt line is 0, so we can clear the pending flag */
        CLEAR_BIT(s->irq_pending, line);
    }
}


static inline bool esp32c6_intmatrix_can_trigger(ESP32C6IntMatrixState *s)
{
    return esp_cpu_accept_interrupts(s->cpu);
}


static void esp32c6_intmatrix_irq_handler(void *opaque, int n, int level)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(opaque);

    /* Update the level mirror */
    assert(n <= ESP32C6_INT_MATRIX_INPUTS);

    /* Make sure level is 0 or 1 */
    level = level ? 1 : 0;

    /* Save the former level of the pin */
    const int former_level = BIT_SET(s->irq_levels, n) ? 1 : 0;

    if (level) {
        SET_BIT(s->irq_levels, n);
    } else {
        CLEAR_BIT(s->irq_levels, n);
    }

    const int line = s->irq_map[n];

    /* If the line is not enable, don't do anything special, the level has been recorded already.
     * Don't do anything if the line is at the same level as before */
    if ((s->irq_enabled & BIT(line)) == 0 || former_level == level) {
        return;
    }

    /* If the new level is high, check that the priority is equal or bigger than the threshold.
     * If that's the case, we can execute the interrupt, else, mark it as pending. */
    if (level == 1) {
#if INTMATRIX_DEBUG
        info_report("\x1b[31m[INTMATRIX] IRQ %d priority set to %d, CPU threshold %d \x1b[0m\n",
                    line, s->irq_prio[line], s->irq_thres);
#endif

        if (s->irq_prio[line] >= s->irq_thres && esp32c6_intmatrix_can_trigger(s)) {
            esp32c6_do_int(s, line);
        } else {
            SET_BIT(s->irq_pending, line);
        }
    } else if (BIT_SET(s->irq_pending, line)) {
        esp32c6_intmatrix_clear_pending(s, line);
    }
}


static void esp32c6_intmatrix_irq_prio_changed(ESP32C6IntMatrixState* s, uint32_t line, uint8_t priority)
{
    const bool accept = esp32c6_intmatrix_can_trigger(s);

    if (accept && priority >= s->irq_thres && BIT_SET(s->irq_pending, line)) {
        /* No need to clear the pending bit here. As soon as the interrupt source will be ACK by the
         * software, its level will be updated, as well as its pending state. */
        esp32c6_do_int(s, line);
    }
}


static void esp32c6_intmatrix_core_prio_changed(ESP32C6IntMatrixState* s, uint64_t new_cpu_priority)
{
    uint64_t pending = s->irq_pending;
    const bool accept = esp32c6_intmatrix_can_trigger(s);

    if (pending && accept) {
        int64_t priority = -1;
        uint_fast32_t line = 0;

        /* Clear all the interrupts that have a lower priority than the new CPU threshold */
        // TODO: Don't use a for loop, the non CLINT interrupts are not contiguous
        for (uint_fast32_t i = 1; i <= ESP32C6_CPU_INT_MAX; i++) {

            const uint64_t line_prio = s->irq_prio[i];
            if (line_prio < new_cpu_priority) {
                CLEAR_BIT(pending, i);
            }
        }

        /* No high level interrupt pending? */
        if (pending == 0) {
            return;
        }

        /* Look for the highest priority pending interrupt */
        // TODO: Don't use a for loop, the non CLINT interrupts are not contiguous
        for (uint_fast32_t i = 1; i <= ESP32C6_CPU_INT_MAX; i++) {
            const int64_t line_prio = (int64_t) s->irq_prio[i];
            if (BIT_SET(pending, i) && line_prio > priority) {
                priority = line_prio;
                line = i;
            }
        }

        /* Make sure a line was selected with its new priority */
        assert(line != 0);
        assert(priority >= new_cpu_priority);
        /* No need to clear the pending bit here. As soon as the interrupt source will be ACK by the
         * software, its level will be update, as well as its pending state. */
        esp32c6_do_int(s, line);
    }
}


/**
 * This function is called when the status (enabled/disabled) of a line has just been changed.
 * It will update the pending IRQ map.
 */
static void esp32c6_intmatrix_irq_status_changed(ESP32C6IntMatrixState* s, uint32_t line, int enabled)
{
    const bool accept = esp32c6_intmatrix_can_trigger(s);

    if (!enabled) {

        /* IRQ has just been disabled, if any interrupt is pending, clear it */
        CLEAR_BIT(s->irq_pending, line);

    } else if (esp32c6_get_output_line_level(s, line)) {

        /* IRQ has just been re-enabled, we have to check if any interrupt source is mapped to it, and
         * if that's the case, check if their level is high, as we would need to potentially trigger an
         * interrupt. */
        SET_BIT(s->irq_pending, line);

        if (accept) {
            /* If the CPU can accept interrupt, trigger an interrupt now */
            esp32c6_do_int(s, line);
        }
    }
}


/**
 * Callback invoked by the CPU as soon as interrupts are re-enabled
 */
static bool esp32c6_intmatrix_mie_enabled(void* opaque)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(opaque);
    /* We need to check if any interrupt is pending and trigger it. We have such function already, triggered when
     * the core priority changes, let's reuse this function by giving the same core priority */
    esp32c6_intmatrix_core_prio_changed(s, s->irq_thres);
    return s->irq_pending != 0;
}


static uint64_t esp32c6_intmatrix_read(void* opaque, hwaddr addr, unsigned int size)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(opaque);
    const uint32_t index = addr / sizeof(uint32_t);
    uint32_t r = 0;

    if (index < ESP32C6_INT_MATRIX_INPUTS) {
        r = s->irq_map[index];
    } else if (index == ESP32C6_INTMTX_CORE0_INT_STATUS_0_REG) {
        warn_report("[INTMATRIX] Unsupported read to ESP32C6_INTMTX_CORE0_INT_STATUS_0_REG\n");
        r = 0;
    } else if (index == ESP32C6_INTMTX_CORE0_INT_STATUS_1_REG) {
        warn_report("[INTMATRIX] Unsupported read to ESP32C6_INTMTX_CORE0_INT_STATUS_1_REG\n");
        r = 0;
    } else if (index == ESP32C6_INTMTX_CORE0_INT_STATUS_2_REG) {
        warn_report("[INTMATRIX] Unsupported read to ESP32C6_INTMTX_CORE0_INT_STATUS_2_REG\n");
        r = 0;
    } else if (index == ESP32C6_INTMTX_CORE0_INTERRUPT_REG_DATE_REG) {
        r = 0x22031100;
    } else {
#if INTMATRIX_WARNING
        /* Other registers are not supported yet */
        warn_report("[INTMATRIX] Unsupported read to %08lx\n", addr);
#endif
    }

    return r;
}

static uint64_t esp32c6_intmatrix_prio_read(void* opaque, hwaddr addr, unsigned int size)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(opaque);
    const uint32_t index = addr / sizeof(uint32_t);
    uint32_t r = 0;

    if (index == ESP32C6_INTPRI_CORE0_CPU_INT_ENABLE_REG) {
        r = s->irq_enabled;
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_TYPE_REG) {
        r = 0; /* by default assume the interrupt to be of type "level" and not "edge" */
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_EIP_STATUS_REG) {
        warn_report("[INTMATRIX] Unsupported read to ESP32C6_INTPRI_CORE0_CPU_INT_EIP_STATUS_REG\n");
        r = 0;
    }
    else if (index >= ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_START && index <= ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_END) {
        /* Here interrupts don't start at 1 but at 0, and interrupts 0,3,4,7 are actually CLINT so they have
           static priority
        */
       /* From ...PRIO_START exclude the 0th, 3rd, 4th and 7th register (CLINT interrupts)*/
       if (index != ESP32C6_INTPRI_CORE0_CPU_INT_PRI_00_REG &&
           index != ESP32C6_INTPRI_CORE0_CPU_INT_PRI_03_REG &&
           index != ESP32C6_INTPRI_CORE0_CPU_INT_PRI_04_REG &&
           index != ESP32C6_INTPRI_CORE0_CPU_INT_PRI_07_REG) {
            const uint32_t line = index - ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_START;
            r = s->irq_prio[line];
        }
        else {
            warn_report("[INTMATRIX] Unsupported read from a CLINT interrupt register\n");
            r = 0;
        }
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_THRESH_REG) {
        r = s->irq_thres;
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_CLEAR_REG) {
        warn_report("[INTMATRIX] Unsupported read to ESP32C6_INTPRI_CORE0_CPU_INT_CLEAR_REG\n");
        r = 0;
    } else if (index >= ESP32C6_INTPRI_CPU_INTR_FROM_CPU_1_REG && 
               index <= ESP32C6_INTPRI_CPU_INTR_FROM_CPU_4_REG) {
        warn_report("[INTMATRIX] Unsupported read to ESP32C6_INTPRI_CPU_INTR_FROM_CPU_X_REG\n");
        r = 0;
    } else if (index == ESP32C6_INTPRI_DATE_REG) {
        r = 0x22031100;
    }
    else {
#if INTMATRIX_WARNING
        /* Other registers are not supported yet */
        warn_report("[INTMATRIX] Unsupported read to %08lx\n", addr);
#endif
    }
    return r;
}

static void esp32c6_intmatrix_write(void* opaque, hwaddr addr, uint64_t value, unsigned int size)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(opaque);
    const uint32_t index = addr / sizeof(uint32_t);

    if (index < ESP32C6_INT_MATRIX_INPUTS) {
        /* There are 5 least significant bits to set the CPU interrupt id*/
        s->irq_map[index] = (value & 0x1f);
#if INTMATRIX_DEBUG
        info_report("\x1b[31m[INTMATRIX] Mapping interrupt %d to CPU line %d\x1b[0m\n", index, s->irq_map[index]);
#endif
    } else if (index == ESP32C6_INTMTX_CORE0_INT_STATUS_0_REG) {
        warn_report("[INTMATRIX] Unsupported write to ESP32C6_INTMTX_CORE0_INT_STATUS_0_REG\n");
    } else if (index == ESP32C6_INTMTX_CORE0_INT_STATUS_1_REG) {
        warn_report("[INTMATRIX] Unsupported write to ESP32C6_INTMTX_CORE0_INT_STATUS_1_REG\n");
    } else if (index == ESP32C6_INTMTX_CORE0_INT_STATUS_2_REG) {
        warn_report("[INTMATRIX] Unsupported write to ESP32C6_INTMTX_CORE0_INT_STATUS_2_REG\n");
    } else if (index == ESP32C6_INTMTX_CORE0_INTERRUPT_REG_DATE_REG) {
        warn_report("[INTMATRIX] Unsupported write to ESP32C6_INTMTX_CORE0_INTERRUPT_REG_DATE_REG\n");
    } else {
#if INTMATRIX_WARNING
        /* Other registers are not supported yet */
        warn_report("[INTMATRIX] Unsupported write to %08lx (%08lx)\n", addr, value);
#endif
    }
}

static void esp32c6_intmatrix_prio_write(void* opaque, hwaddr addr, uint64_t value, unsigned int size) {
    // TODO double check this, probably some other registers were moved to this table
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(opaque);
    const uint32_t index = addr / sizeof(uint32_t);

    if (index == ESP32C6_INTPRI_CORE0_CPU_INT_ENABLE_REG) {
        /* Check if any bit has changed status */
        uint64_t prev = s->irq_enabled;
        s->irq_enabled = value;
        /* Check which interrupt/bit changed
         * Interrupts starts at 1, so we need to count up to ESP32C3_CPU_INT_COUNT */
        for (int i = 0; i <= ESP32C6_CPU_INT_MAX; i++) {
            /* Exclude the CLINT interrupts, for which we don't choose
               the "enabled" property. See 1.6.2 of the technical reference */
            if (i != 0 && i != 3 && i != 4 && i != 7) {
                const int new_st = value & BIT(i);
                const int old_st = prev  & BIT(i);
                if (new_st != old_st) {
                    esp32c6_intmatrix_irq_status_changed(s, i, new_st ? 1 : 0);
                }
            } else {
                /* WARNING*/
                /* The CLINT interrupts are always enabled, so we don't need to
                   check their status */
                /* This forced set to 1 could be a BIG MISTAKE*/
                s->irq_enabled |= BIT(i);
            }
        }
        /*
           - An M mode interrupt (external or local) further needs to 
              be unmasked at core level by setting the
              corresponding bit in mie CSR.
           - A U mode interrupt (external or local) further needs to 
              be unmasked at core level by setting the
              corresponding bits in uie CSR.
        */
       /* TODO: Support for M and U mode
       */
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_TYPE_REG) {
        if (value != 0) {
            warn_report("[INTMATRIX] Edge-triggered interrupts not supported\n");
        } else {
            warn_report("[INTMATRIX] Unsupported write to %08lx (%08lx)\n", addr, value);
        }
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_EIP_STATUS_REG) {
        warn_report("[INTMATRIX] Unsupported write to READ ONLY ESP32C6_INTPRI_CORE0_CPU_INT_EIP_STATUS_REG\n");
    } else if (index >= ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_START && index <= ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_END) {
        /* There are 4 least significant bits reserved to set the priority value*/
        const uint8_t priority = value & 0xf;
        /* Interrupts start from 0 here*/
        const uint32_t line = (index - ESP32C6_INTPRI_CORE0_CPU_INT_PRIO_START);
        /* Exclude CLINT interrupts*/
        if (line != 0 && line != 3 && line != 4 && line != 7) {
            s->irq_prio[line] = priority;
            esp32c6_intmatrix_irq_prio_changed(s, line, priority);
        } else {
            warn_report("[INTMATRIX] Unsupported write to CLINT interrupt register\n");
        }
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_THRESH_REG) {
        const uint8_t priority = value & 0xf;
        /**
         * If the new priority is the same as the former one, nothing must be done.
         * Else, this could result in an infinite loop. Let's say we have an interrupt source
         * that is mapped to a CPU line, its threshold is 2, the CPU threshold is 3.
         * When the interrupt source is asserted, no interrupt is triggered, because the line's
         * priority is lower than the threshold but the its pending bit is set .
         * As soon as the threshold is lowered to 2 or 1, the interrupt will be triggered because
         * its pending bit is set.
         * NOTE THAT THE PENDING BIT IS STILL SET BECAUSE THE SOURCE IS STILL ASSERTED!
         * As such, if the CPU sets the threshold to the same value, the function
         * `esp32c3_intmatrix_core_prio_changed` called below would re-schedule the same interrupt.
         */
        if (priority != s->irq_thres) {
            s->irq_thres = priority;
            esp32c6_intmatrix_core_prio_changed(s, priority);
            info_report("\x1b[31m[INTMATRIX] Setting CPU IRQ threshold to %d\x1b[0m", priority);
        }
    } else if (index == ESP32C6_INTPRI_CORE0_CPU_INT_CLEAR_REG) {
        warn_report("[INTMATRIX] Unsupported write to ESP32C6_INTPRI_CORE0_CPU_INT_CLEAR_REG\n");
    } else if (index >= ESP32C6_INTPRI_CPU_INTR_FROM_CPU_1_REG && 
               index <= ESP32C6_INTPRI_CPU_INTR_FROM_CPU_4_REG) {
        warn_report("[INTMATRIX] Unsupported write to ESP32C6_INTPRI_CPU_INTR_FROM_CPU_X_REG\n");
    } else if (index == ESP32C6_INTPRI_DATE_REG) {
        warn_report("[INTMATRIX] Unsupported write to READ ONLY ESP32C6_INTPRI_DATE_REG\n");
    } else {
#if INTMATRIX_WARNING
        /* Other registers are not supported yet */
        warn_report("[INTMATRIX] Unsupported write to %08lx (%08lx)\n", addr, value);
#endif 
    }
}

static const MemoryRegionOps esp_intmatrix_ops = {
    .read =  esp32c6_intmatrix_read,
    .write = esp32c6_intmatrix_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps esp_intmatrix_prio_ops = {
    .read =  esp32c6_intmatrix_prio_read,
    .write = esp32c6_intmatrix_prio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};


static void esp32c6_intmatrix_reset_hold(Object *obj, ResetType type)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(obj);
    RISCVCPU *cpu = &s->cpu->parent_obj;

    memset(s->irq_map, 0, sizeof(s->irq_map));
    memset(s->irq_prio, 0, sizeof(s->irq_prio));
    s->irq_thres = 0;
    s->irq_pending = 0;
    s->irq_levels = 0;
    s->irq_enabled = 0;
    // TODO: Don't use a for loop, the non CLINT interrupts are not contiguous
    for (int i = 0; i <= ESP32C6_CPU_INT_MAX; i++) {
        qemu_irq_lower(s->out_irqs[i]);
    }

    /* Force the CPU to allow all interrupts */
    riscv_csr_write(&cpu->env, CSR_MIE, BIT(12) - 1);
}


static void esp32c6_intmatrix_realize(DeviceState *dev, Error **errp)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(dev);
    EspRISCVCPU *cpu = s->cpu;
    EspRISCVCPUClass *cpu_klass = ESP_CPU_GET_CLASS(cpu);

    esp32c6_intmatrix_reset_hold(OBJECT(dev), RESET_TYPE_COLD);

    /* Register MIE callback */
    assert(cpu);
    cpu_klass->esp_cpu_register_mie_callback(cpu, esp32c6_intmatrix_mie_enabled, s);
}


static void esp32c6_intmatrix_init(Object *obj)
{
    ESP32C6IntMatrixState *s = ESP32C6_INTMATRIX(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &esp_intmatrix_ops, s,
                          TYPE_ESP32C6_INTMATRIX, ESP32C6_INTMATRIX_IO_SIZE);
    memory_region_init_io(&s->iomem_prio, obj, &esp_intmatrix_prio_ops, s,
                          TYPE_ESP32C6_INTMATRIX_PRIO, ESP32C6_INTMATRIX_PRIO_IO_SIZE);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_mmio(sbd, &s->iomem_prio);

    qdev_init_gpio_in(DEVICE(s), esp32c6_intmatrix_irq_handler, ESP32C6_INT_MATRIX_INPUTS);
    qdev_init_gpio_out_named(DEVICE(s), s->out_irqs, ESP32C6_INT_MATRIX_OUTPUT_NAME, ESP32C6_CPU_INT_MAX + 1);
}


static Property esp32c6_intmatrix_properties[] = {
    DEFINE_PROP_LINK("cpu", ESP32C6IntMatrixState, cpu, TYPE_ESP_RISCV_CPU, EspRISCVCPU*),
    DEFINE_PROP_END_OF_LIST(),
};


static void esp32c6_intmatrix_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    rc->phases.hold = esp32c6_intmatrix_reset_hold;
    dc->realize = esp32c6_intmatrix_realize;
    device_class_set_props(dc, esp32c6_intmatrix_properties);
}


static const TypeInfo esp32c6_intmatrix_info = {
    .name = TYPE_ESP32C6_INTMATRIX,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ESP32C6IntMatrixState),
    .instance_init = esp32c6_intmatrix_init,
    .class_init = esp32c6_intmatrix_class_init
};


static void esp32c6_intmatrix_register_types(void)
{
    type_register_static(&esp32c6_intmatrix_info);
}


type_init(esp32c6_intmatrix_register_types)
