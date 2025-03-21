/*
    Copyright 2024 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "zephyr_serial.h"
#include "tiny_types.h"
struct uart_rx_context {
    const struct device *dev;
    bool is_initialized;
    bool is_open;
    uint8_t rx_buf[256];
    size_t head;
    size_t tail;
    struct k_sem rx_sem;
    struct k_mutex rx_mutex;
};

// --- Compatible UART detection ---

#define UART_COMPAT_LIST(_,node_id)                        \
    _(node_id, st_stm32_usart)                              \
    _(node_id, st_stm32_uart)                               \
    _(node_id, st_stm32_lpuart)                             \
    _(node_id, nordic_nrf_uart)                             \
    _(node_id, nordic_nrf_uarte)                            \
    _(node_id, ns16550)                                     \
    _(node_id, nxp_kinetis_uart)                            \
    _(node_id, nxp_kinetis_lpuart)                          \
    _(node_id, nxp_lpc_uart)                                \
    _(node_id, nxp_lpc11u6x_uart)                           \
    _(node_id, nxp_imx_uart)                                \
    _(node_id, nxp_imx_iuart)                               \
    _(node_id, silabs_gecko_uart)                           \
    _(node_id, silabs_gecko_leuart)                         \
    _(node_id, atmel_sam_uart)                              \
    _(node_id, atmel_sam0_uart)                             \
    _(node_id, atmel_at91_usart)                            \
    _(node_id, ti_stellaris_uart)                           \
    _(node_id, ti_cc13xx_cc26xx_uart)                       \
    _(node_id, ti_cc32xx_uart)                              \
    _(node_id, ti_msp432p4xx_uart)                          \
    _(node_id, renesas_ra_uart_sci)                         \
    _(node_id, renesas_ra_sci_uart)                         \
    _(node_id, renesas_ra8_uart_sci_b)                      \
    _(node_id, renesas_rzt2m_uart)                          \
    _(node_id, renesas_smartbond_uart)                      \
    _(node_id, sifive_uart0)                                \
    _(node_id, gaisler_apbuart)                             \
    _(node_id, litex_uart)                                  \
    _(node_id, ambiq_uart)                                  \
    _(node_id, efinix_sapphire_uart0)                       \
    _(node_id, openisa_rv32m1_lpuart)                       \
    _(node_id, snps_hostlink_uart)                          \
    _(node_id, snps_nsim_uart)                              \
    _(node_id, lowrisc_opentitan_uart)                      \
    _(node_id, cdns_uart)                                   \
    _(node_id, neorv32_uart)                                \
    _(node_id, zephyr_native_posix_uart)                    \
    _(node_id, zephyr_native_tty_uart)                      \
    _(node_id, zephyr_cdc_acm_uart)                         \
    _(node_id, zephyr_uart_emul)                            \
    _(node_id, zephyr_nus_uart)                             \
    _(node_id, raspberrypi_pico_uart)                       \
    _(node_id, raspberrypi_pico_uart_pio)                   \
    _(node_id, microchip_xec_uart)                          \
    _(node_id, microchip_coreuart)                          \
    _(node_id, altr_jtag_uart)                              \
    _(node_id, altr_uart)                                   \
    _(node_id, arm_cmsdk_uart)                              \
    _(node_id, arm_sbsa_uart)                               \
    _(node_id, intel_sedi_uart)                             \
    _(node_id, intel_lw_uart)                               \
    _(node_id, ite_it8xxx2_uart)                            \
    _(node_id, nuvoton_npcx_uart)                           \
    _(node_id, nuvoton_numaker_uart)                        \
    _(node_id, nuvoton_numicro_uart)                        \
    _(node_id, xlnx_xps_uartlite_1.00.a)                    \
    _(node_id, xlnx_xuartps)                                \
    _(node_id, brcm_bcm2711_aux_uart)                       \
    _(node_id, infineon_cat1_uart)                          \
    _(node_id, infineon_xmc4xxx_uart)                       \
    _(node_id, telink_b91_uart)                             \
    _(node_id, cypress_psoc6_uart)                          \
    _(node_id, sensry_sy1xx_uart)                           \
    _(node_id, adi_max32_uart)

#define UART_CONTEXT_INIT_IF_COMPAT(node_id, compat) \
    IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, compat), \
        ({ .dev = DEVICE_DT_GET(node_id), .is_initialized = false },))

#define UART_CONTEXT_INITIALIZER(node_id) \
    UART_COMPAT_LIST(UART_CONTEXT_INIT_IF_COMPAT, node_id)

static struct uart_rx_context uart_contexts[] = {
    DT_FOREACH_STATUS_OKAY_NODE(UART_CONTEXT_INITIALIZER)
};

BUILD_ASSERT(ARRAY_SIZE(uart_contexts) > 0, "No UART nodes with \"okay\" status found");

static void uart_rx_callback(const struct device *dev, void *user_data)
{
    struct uart_rx_context *ctx = (struct uart_rx_context *)user_data;
    uint8_t byte;

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        while (uart_fifo_read(dev, &byte, 1) > 0) {
            size_t next_head = (ctx->head + 1) % sizeof(ctx->rx_buf);
            if (next_head != ctx->tail) {
                ctx->rx_buf[ctx->head] = byte;
                ctx->head = next_head;
                k_sem_give(&ctx->rx_sem);
            }
        }
    }
}

tiny_serial_handle_t tiny_serial_open(const char *name, uint32_t baud)
{
    for (size_t i = 0; i < ARRAY_SIZE(uart_contexts); i++) {
        if (uart_contexts[i].dev && strcmp(uart_contexts[i].dev->name, name) == 0) {
            if (uart_contexts[i].is_open) {
                return (tiny_serial_handle_t)&uart_contexts[i];
            }
            if (!uart_contexts[i].is_initialized) {
                k_sem_init(&uart_contexts[i].rx_sem, 0, 1);
                k_mutex_init(&uart_contexts[i].rx_mutex);
                uart_contexts[i].is_initialized = true;
            }
            int result = 0;
            if (baud) {
                struct uart_config config = {
                    .baudrate = baud,
                    .parity = UART_CFG_PARITY_NONE,
                    .stop_bits = UART_CFG_STOP_BITS_1,
                    .data_bits = UART_CFG_DATA_BITS_8,
                    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
                };
                result = uart_configure(uart_contexts[i].dev, &config);
            }
            if (result != 0) {
                return TINY_SERIAL_INVALID;
            }
            result = uart_irq_callback_user_data_set(uart_contexts[i].dev, uart_rx_callback, &uart_contexts[i]);
            if (result != 0) {
                return TINY_SERIAL_INVALID;
            }
            uart_irq_rx_enable(uart_contexts[i].dev);
            uart_contexts[i].is_open = true;
            return (tiny_serial_handle_t)&uart_contexts[i];
        }
    }
    return TINY_SERIAL_INVALID;
}

void tiny_serial_close(tiny_serial_handle_t handle)
{
    struct uart_rx_context *ctx = (struct uart_rx_context *)handle;
    if (ctx && ctx->dev) {
        uart_irq_rx_disable(ctx->dev);
        ctx->is_open = false;
    }
}

int tiny_serial_read(tiny_serial_handle_t handle, void *buf, int size)
{
    return tiny_serial_read_timeout(handle, buf, size, 100);
}

int tiny_serial_send(tiny_serial_handle_t handle, const void *buf, int size)
{
    struct uart_rx_context *ctx = (struct uart_rx_context *)handle;
    const uint8_t *buffer = (const uint8_t *)buf;
    
    if (!ctx || !ctx->dev)
    {
        return TINY_ERR_FAILED;
    }
    
    for (int i = 0; i < size; i++)
    {
        uart_poll_out(ctx->dev, buffer[i]);
    }
    
    return size;
}

/* Alias of tiny_serial_send with timeout parameter (ignored) */
int tiny_serial_send_timeout(tiny_serial_handle_t handle, const void *buf, int size, uint32_t timeout_ms)
{
    /* Zephyr UART poll_out is blocking, so we just call the regular function */
    (void)timeout_ms; /* Unused parameter */
    return tiny_serial_send(handle, buf, size);
}

int tiny_serial_read_timeout(tiny_serial_handle_t handle, void *buf, int size, uint32_t timeout_ms)
{
    size_t received = 0;
    struct uart_rx_context *ctx = (struct uart_rx_context *)handle;
    k_mutex_lock(&ctx->rx_mutex, K_FOREVER);

    while (received < size) {
        if (ctx->head == ctx->tail) {
            if (k_sem_take(&ctx->rx_sem, K_MSEC(timeout_ms)) != 0) break;
        }

        while (ctx->tail != ctx->head && received < size) {
            ((uint8_t *)buf)[received++] = ctx->rx_buf[ctx->tail];
            ctx->tail = (ctx->tail + 1) % sizeof(ctx->rx_buf);
        }
    }

    k_mutex_unlock(&ctx->rx_mutex);
    return received;
} 