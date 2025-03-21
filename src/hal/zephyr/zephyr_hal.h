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

#pragma once

#include <zephyr/kernel.h>

typedef struct k_mutex tiny_mutex_t;

#ifndef CONFIG_ENABLE_CHECKSUM
#define CONFIG_ENABLE_CHECKSUM
#endif

#ifndef CONFIG_ENABLE_FCS16
#define CONFIG_ENABLE_FCS16
#endif

#ifndef CONFIG_ENABLE_FCS32
#define CONFIG_ENABLE_FCS32
#endif

#if IS_ENABLED(CONFIG_MULTITHREADING)
#define CONFIG_TINYHAL_THREAD_SUPPORT 1
#else
#define CONFIG_TINYHAL_THREAD_SUPPORT 0
#endif

/**
 * Mutex type used by Tiny Protocol implementation.
 * The type declaration depends on platform.
 */
typedef struct k_mutex tiny_mutex_t;

/**
 * Event groups type used by Tiny Protocol implementation.
 * The type declaration depends on platform.
 */
typedef struct {
    struct k_event event;
} tiny_events_t; 