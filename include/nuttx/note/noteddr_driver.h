/****************************************************************************
 * include/nuttx/note/noteddr_driver.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_NOTE_NOTEDDR_DRIVER_H
#define __INCLUDE_NUTTX_NOTE_NOTEDDR_DRIVER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/fs/ioctl.h>

#include <stdbool.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IOCTL Commands ***********************************************************/

/* NOTEDDR_CLEAR
 *              - Clear all contents of the circular buffer
 *                Argument: Ignored
 * NOTEDDR_GETMODE
 *              - Get overwrite mode
 *                Argument: A writable pointer to unsigned int
 * NOTEDDR_SETMODE
 *              - Set overwrite mode
 *                Argument: A read-only pointer to unsigned int
 */

#ifdef CONFIG_DRIVERS_NOTEDDR
#define NOTEDDR_CLEAR           _NOTEDDRIOC(0x01)
#define NOTEDDR_GETMODE         _NOTEDDRIOC(0x02)
#define NOTEDDR_SETMODE         _NOTEDDRIOC(0x03)
#endif

/* Overwrite mode definitions */

#ifdef CONFIG_DRIVERS_NOTEDDR
#define NOTEDDR_MODE_OVERWRITE_DISABLE      0
#define NOTEDDR_MODE_OVERWRITE_ENABLE       1
#define NOTEDDR_MODE_OVERWRITE_OVERFLOW     2
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct noteddr_driver_s;

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct noteddr_driver_s g_noteddr_driver;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if defined(__KERNEL__) || defined(CONFIG_BUILD_FLAT)

/****************************************************************************
 * Name: noteddr_register
 *
 * Description:
 *   Register RAM note driver at /dev/note/ram that can be used by an
 *   application to read note data from the circular note buffer.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Zero on succress. A negated errno value is returned on a failure.
 *
 ****************************************************************************/

#ifdef CONFIG_DRIVERS_NOTEDDR
int noteddr_register(void);

FAR struct note_driver_s *
noteddr_initialize(FAR const char *devpath, size_t bufsize, bool overwrite);
#endif

#endif /* defined(__KERNEL__) || defined(CONFIG_BUILD_FLAT) */

#endif /* __INCLUDE_NUTTX_NOTE_NOTEDDR_DRIVER_H */
