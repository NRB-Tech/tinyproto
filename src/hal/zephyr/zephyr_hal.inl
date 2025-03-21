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
#include "zephyr_hal.h"
#include "tiny_types.h"
void tiny_mutex_create(tiny_mutex_t *mutex)
{
    k_mutex_init(mutex);
}

void tiny_mutex_destroy(tiny_mutex_t *mutex)
{
    /* No destroy function in Zephyr's k_mutex */
    (void)mutex;
}

void tiny_mutex_lock(tiny_mutex_t *mutex)
{
    k_mutex_lock(mutex, K_FOREVER);
}

uint8_t tiny_mutex_try_lock(tiny_mutex_t *mutex)
{
    return k_mutex_lock(mutex, K_NO_WAIT) == 0;
}

void tiny_mutex_unlock(tiny_mutex_t *mutex)
{
    k_mutex_unlock(mutex);
}

void tiny_events_create(tiny_events_t *events)
{
    k_event_init(&events->event);
}

void tiny_events_destroy(tiny_events_t *events)
{
    /* No destroy function in Zephyr's k_event */
    (void)events;
}

uint8_t tiny_events_wait(tiny_events_t *events, uint8_t bits, uint8_t clear, uint32_t timeout)
{
    k_timeout_t timeout_ticks = (timeout == TINY_FLAG_WAIT_FOREVER) ? K_FOREVER : K_MSEC(timeout);
    uint32_t result = k_event_wait(&events->event, bits, clear ? true : false, timeout_ticks);
    return (uint8_t)result;
}

uint8_t tiny_events_check_int(tiny_events_t *event, uint8_t bits, uint8_t clear)
{
    return tiny_events_wait(event, bits, clear, 0);
}

void tiny_events_set(tiny_events_t *events, uint8_t bits)
{
    k_event_post(&events->event, bits);
}

void tiny_events_clear(tiny_events_t *events, uint8_t bits)
{
    k_event_set(&events->event, k_event_wait(&events->event, 0xFF, 0, K_NO_WAIT) & ~bits);
}

void tiny_sleep(uint32_t millis)
{
    k_msleep(millis);
}

void tiny_sleep_us(uint32_t us)
{
    k_usleep(us);
}

uint32_t tiny_millis()
{
    return (uint32_t)(k_uptime_get());
}

uint32_t tiny_micros()
{
    return (uint32_t)(k_uptime_get() * 1000);
} 