/****************************************************************************
 * arch/arm/src/armv8-r/dwt.h
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

#ifndef __ARCH_ARM_SRC_ARMV8_R_DWT_H
#define __ARCH_ARM_SRC_ARMV8_R_DWT_H

#include <arch/armv8-r/cp14.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Data Watchpoint and Trace Register (DWT) Definitions *********************/

typedef unsigned long u32;

/* Watchpoints type */
#define DWT_FUNCTION_WATCHPOINT_RO      0b01   /* read only */
#define DWT_FUNCTION_WATCHPOINT_WO      0b10   /* write only */
#define DWT_FUNCTION_WATCHPOINT_RW      0b11   /* read and write */

/* Byte address select */
#define ARM_BREAKPOINT_BAS_0001         0b0001
#define ARM_BREAKPOINT_BAS_0011         0b0011
#define ARM_BREAKPOINT_BAS_1111         0b1111

/* Privilege Levels */
#define ARM_BREAKPOINT_PRIV             0b01   /* privilege mode */
#define ARM_BREAKPOINT_USER             0b10   /* user mode */

/* DSCR monitor/halting bits. */
#define ARM_DSCR_HDBGEN         (1 << 14)
#define ARM_DSCR_MDBGEN         (1 << 15)

#define CORESIGHT_UNLOCK                0xc5acce55

struct debugpoint_control_register
  {
    unsigned int
    res1            : 8,                /* Reserved, RES1 */
    bt              : 4,                /* Breakpoint Type */
    lbn             : 4,                /* Linked breakpoint number */
    ssc             : 2,                /* Security state control */
    hmc             : 1,                /* Higher mode control */
    res0            : 4,                /* Reserved, RES0 */
    bas             : 4,                /* Byte address select */
    lsc             : 2,                /* Load/store control or reserved */
    pmc             : 2,                /* Privilege mode control */
    e               : 1;                /* Enable breakpoint */
  };

#define GET_DBCR(r)                     r.e | r.pmc << 1 | r.lsc << 3 |         \
  r.bas << 5 | r.hmc << 13 | r.ssc << 14 | r.lbn << 16 | r.bt << 20

#define ARM_BKPT_NUM()    8
#define ARM_RWPT_NUM()    8
#define ARM_DEBUG_MAX     16

#define READ_DEBUGPOINT_REGISTER(r, n)                                          \
  ({                                                                            \
    uint32_t __dbgpt_reg = 0;                                                   \
    switch (n)                                                                  \
      {                                                                         \
        case 0:     __dbgpt_reg = CP14_GET_CONCAT(r, 0); break;                 \
        case 1:     __dbgpt_reg = CP14_GET_CONCAT(r, 1); break;                 \
        case 2:     __dbgpt_reg = CP14_GET_CONCAT(r, 2); break;                 \
        case 3:     __dbgpt_reg = CP14_GET_CONCAT(r, 3); break;                 \
        case 4:     __dbgpt_reg = CP14_GET_CONCAT(r, 4); break;                 \
        case 5:     __dbgpt_reg = CP14_GET_CONCAT(r, 5); break;                 \
        case 6:     __dbgpt_reg = CP14_GET_CONCAT(r, 6); break;                 \
        case 7:     __dbgpt_reg = CP14_GET_CONCAT(r, 7); break;                 \
      }                                                                         \
    __dbgpt_reg;                                                                \
  })
#define WRITE_DEBUGPOINT_REGISTER(r, n, value)                                  \
  ({                                                                            \
    switch (n)                                                                  \
      {                                                                         \
        case 0:     CP14_SET_CONCAT(r, 0, value); break;                        \
        case 1:     CP14_SET_CONCAT(r, 1, value); break;                        \
        case 2:     CP14_SET_CONCAT(r, 2, value); break;                        \
        case 3:     CP14_SET_CONCAT(r, 3, value); break;                        \
        case 4:     CP14_SET_CONCAT(r, 4, value); break;                        \
        case 5:     CP14_SET_CONCAT(r, 5, value); break;                        \
        case 6:     CP14_SET_CONCAT(r, 6, value); break;                        \
        case 7:     CP14_SET_CONCAT(r, 7, value); break;                        \
      }                                                                         \
  })

#define READ_BREAKPOINT_CONTROL_REGISTER(n) READ_DEBUGPOINT_REGISTER(DBGBCR, n)
#define READ_BREAKPOINT_VALUE_REGISTER(n)   READ_DEBUGPOINT_REGISTER(DBGBVR, n)
#define WRITE_BREAKPOINT_CONTROL_REGISTER(n, value)                             \
                                    WRITE_DEBUGPOINT_REGISTER(DBGBCR, n, value)
#define WRITE_BREAKPOINT_VALUE_REGISTER(n, value)                               \
                                    WRITE_DEBUGPOINT_REGISTER(DBGBVR, n, value)

#define READ_WATCHPOINT_CONTROL_REGISTER(n) READ_DEBUGPOINT_REGISTER(DBGWCR, n)
#define READ_WATCHPOINT_VALUE_REGISTER(n)   READ_DEBUGPOINT_REGISTER(DBGWVR, n)
#define WRITE_WATCHPOINT_CONTROL_REGISTER(n, value)                             \
                                    WRITE_DEBUGPOINT_REGISTER(DBGWCR, n, value)
#define WRITE_WATCHPOINT_VALUE_REGISTER(n, value)                               \
                                    WRITE_DEBUGPOINT_REGISTER(DBGWVR, n, value)

#endif