/****************************************************************************
 * include/nuttx/timers/watchdog.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __INCLUDE_NUTTX_TIMERS_WATCHDOG_H
#define __INCLUDE_NUTTX_TIMERS_WATCHDOG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <nuttx/irq.h>
#include <nuttx/fs/ioctl.h>

#ifdef CONFIG_WATCHDOG

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IOCTL Commands ***********************************************************/

/* The watchdog driver uses a standard character driver framework.  However,
 * since the watchdog driver is a device control interface and not a data
 * transfer interface, the majority of the functionality is implemented in
 * driver ioctl calls.  The watchdog ioctl commands are lised below:
 *
 * These are detected and handled by the "upper half" watchdog timer driver.
 *
 * WDIOC_START      - Start the watchdog timer
 *                    Argument: Ignored
 * WDIOC_STOP       - Stop the watchdog timer
 *                    Argument: Ignored
 * WDIOC_GETSTATUS  - Get the status of the watchdog timer.
 *                    Argument:  A writeable pointer to struct
 *                               watchdog_status_s.
 * WDIOC_SETTIMEOUT - Reset the watchdog timeout to this value
 *                    Argument: A 32-bit timeout value in milliseconds.
 * WDIOC_CAPTURE    - Do not reset.  Instead, called this handler.
 *                    Argument: A pointer to struct watchdog_capture_s.
 * WDIOC_KEEPALIVE  - Reset the watchdog timer ("ping", "pet the dog");
 *                    Argument: Ignored
 *
 * These may be supported by certain "lower half" drivers
 *
 * WDIOC_MINTIME    - Set the minimum ping time.  If two keepalive ioctls
 *                    are received within this time, a reset event will
 *                    be generated.  This feature should assume to be
 *                    disabled after WDIOC_SETTIMEOUT.
 *                    Argument: A 32-bit time value in milliseconds.
 */

#define WDIOC_START      _WDIOC(0x001)
#define WDIOC_STOP       _WDIOC(0x002)
#define WDIOC_GETSTATUS  _WDIOC(0x003)
#define WDIOC_SETTIMEOUT _WDIOC(0x004)
#define WDIOC_CAPTURE    _WDIOC(0x005)
#define WDIOC_KEEPALIVE  _WDIOC(0x006)

#define WDIOC_MINTIME    _WDIOC(0x080)

/* Bit Settings *************************************************************/

/* Bit settings for the struct watchdog_status_s flags field */

#define WDFLAGS_ACTIVE   (1 << 0) /* 1=The watchdog timer is running */
#define WDFLAGS_RESET    (1 << 1) /* 1=Reset when the watchdog timer expires */
#define WDFLAGS_CAPTURE  (1 << 2) /* 1=Call the user function when the
                                   *   watchdog timer expires */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This is the type of the argument passed to the WDIOC_CAPTURE ioctl */

struct watchdog_capture_s
{
  CODE xcpt_t newhandler;   /* The new watchdog capture handler */
  CODE xcpt_t oldhandler;   /* The previous watchdog capture handler (if any) */
};

/* This is the type of the argument passed to the WDIOC_GETSTATUS ioctl and
 * and returned by the "lower half" getstatus() method.
 */

struct watchdog_status_s
{
  uint32_t  flags;          /* See WDFLAGS_* definitions above */
  uint32_t  timeout;        /* The current timeout setting (in milliseconds) */
  uint32_t  timeleft;       /* Time left until the watchdog expiration
                             * (in milliseconds) */
};

/* This structure provides the "lower-half" driver operations available to
 * the "upper-half" driver.
 */

struct watchdog_lowerhalf_s;
struct watchdog_ops_s
{
  /* Required methods *******************************************************/

  /* Start the watchdog timer, resetting the time to the current timeout */

  CODE int (*start)(FAR struct watchdog_lowerhalf_s *lower);

  /* Stop the watchdog timer */

  CODE int (*stop)(FAR struct watchdog_lowerhalf_s *lower);

  /* Optional methods *******************************************************/

  /* Reset the watchdog timer to the current timeout value, prevent any
   * imminent watchdog timeouts.  This is sometimes referred as "pinging" the
   * watchdog timer or "petting the dog".
   */

  CODE int (*keepalive)(FAR struct watchdog_lowerhalf_s *lower);

  /* Get the current watchdog timer status */

  CODE int (*getstatus)(FAR struct watchdog_lowerhalf_s *lower,
                        FAR struct watchdog_status_s *status);

  /* Set a new timeout value (and reset the watchdog timer) */

  CODE int (*settimeout)(FAR struct watchdog_lowerhalf_s *lower,
                         uint32_t timeout);

  /* Don't reset on watchdog timer timeout; instead, call this user provider
   * timeout handler.  NOTE:  Providing handler==NULL will restore the reset
   * behavior.
   */

  CODE xcpt_t (*capture)(FAR struct watchdog_lowerhalf_s *lower,
                         CODE xcpt_t handler);

  /* Any ioctl commands that are not recognized by the "upper-half" driver
   * are forwarded to the lower half driver through this method.
   */

  CODE int (*ioctl)(FAR struct watchdog_lowerhalf_s *lower, int cmd,
                    unsigned long arg);
};

/* This structure provides the publicly visible representation of the
 * "lower-half" driver state structure.  "lower half" drivers will have an
 * internal structure definition that will be cast-compatible with this
 * structure definitions.
 */

struct watchdog_lowerhalf_s
{
  /* Publicly visible portion of the "lower-half" driver state structure. */

  FAR const struct watchdog_ops_s  *ops;  /* Lower half operations */

  /* The remainder of the structure is used by the "lower-half" driver
   * for whatever state storage that it may need.
   */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: watchdog_automonitor_timeout
 *
 * Description:
 *   This function can be called in the watchdog timeout interrupt handler.
 *   If so, callbacks on the watchdog timer notify chain are called when the
 *   watchdog timer times out.
 *
 ****************************************************************************/
#ifdef CONFIG_WATCHDOG_TIMEOUT_NOTIFIER
void watchdog_automonitor_timeout(void);
#endif /* CONFIG_WATCHDOG_TIMEOUT_NOTIFIER */

/****************************************************************************
 * Name: watchdog_register
 *
 * Description:
 *   This function binds an instance of a "lower half" watchdog driver with
 *   the "upper half" watchdog device and registers that device so that can
 *   be used by application code.
 *
 *   When this function is called, the "lower half" driver should be in the
 *   disabled state (as if the stop() method had already been called).
 *
 *   NOTE:  This function would not be called by application code.  Rather it
 *   is called indirectly through the architecture-specific interfaces.
 *
 * Input Parameters:
 *   dev path - The full path to the driver to be registers in the NuttX
 *     pseudo-filesystem.  The recommended convention is to name all watchdog
 *     drivers as "/dev/watchdog0", "/dev/watchdog1", etc.  where the driver
 *     path differs only in the "minor" number at the end of the device name.
 *   lower - A pointer to an instance of lower half watchdog driver.  This
 *     instance is bound to the watchdog driver and must persists as long as
 *     the driver persists.
 *
 * Returned Value:
 *   On success, a non-NULL handle is returned to the caller.  In the event
 *   of any failure, a NULL value is returned.
 *
 ****************************************************************************/

#if defined(CONFIG_WATCHDOG_AUTOMONITOR_BY_ONESHOT)
struct oneshot_lowerhalf_s;
FAR void *watchdog_register(FAR const char *path,
                            FAR struct watchdog_lowerhalf_s *lower,
                            FAR struct oneshot_lowerhalf_s *oneshot);
#elif defined(CONFIG_WATCHDOG_AUTOMONITOR_BY_TIMER)
struct timer_lowerhalf_s;
FAR void *watchdog_register(FAR const char *path,
                            FAR struct watchdog_lowerhalf_s *lower,
                            FAR struct timer_lowerhalf_s *timer);
#else
FAR void *watchdog_register(FAR const char *path,
                            FAR struct watchdog_lowerhalf_s *lower);
#endif

/****************************************************************************
 * Name: watchdog_unregister
 *
 * Description:
 *   This function can be called to disable and unregister the watchdog
 *   device driver.
 *
 * Input Parameters:
 *   handle - This is the handle that was returned by watchdog_register()
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void watchdog_unregister(FAR void *handle);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_WATCHDOG */
#endif /* __INCLUDE_NUTTX_TIMERS_WATCHDOG_H */
