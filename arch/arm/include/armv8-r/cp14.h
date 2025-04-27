/****************************************************************************
 * arch/arm/include/armv8-r/cp14.h
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

/* References:
 *  "ARM Architecture Reference Manual, ARMv8-R edition",
 *   Copyright 1996-1998, 2000, 2004-2012 ARM.
 * All rights reserved. ARM DDI 0406C.c (ID051414)
 */

#ifndef __ARCH_ARM_SRC_ARMV8_R_CP14_H
#define __ARCH_ARM_SRC_ARMV8_R_CP14_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* System control register descriptions.
 *
 * CP15 registers are accessed with MRC and MCR instructions as follows:
 *
 *  MRC p14, <Op1>, <Rd>, <CRn>, <CRm>, <Op2> ; Read CP15 Register
 *  MCR p14, <Op1>, <Rd>, <CRn>, <CRm>, <Op2> ; Write CP15 Register
 *
 * Where
 *
 *   <Op1> is the Opcode_1 value for the register
 *   <Rd>  is a general purpose register
 *   <CRn> is the register number within CP15
 *   <CRm> is the operational register
 *   <Op2> is the Opcode_2 value for the register.
 *
 */

#define _CP14(op1,rd,crn,crm,op2)       "p14, " #op1 ", %0, " #crn ", " #crm ", " #op2

#define CP14_DBGOSLAR(r)                _CP14(0, r, c1, c0, 4)  /* Debug OS Lock Access Register */
#define CP14_DBGDSCRext(r)              _CP14(0, r, c0, c2, 2)  /* Debug Status and Control Register, External View */
#define CP14_DBGDSCRint(r)              _CP14(0, r, c0, c1, 0)  /* Debug Status and Control Register, Internal View */

#define CP14_DBGBVR0(r)                _CP14(0, r, c0, c0, 4)  /* Debug Breakpoint Value Registers 0 */
#define CP14_DBGBVR1(r)                _CP14(0, r, c0, c1, 4)  /* Debug Breakpoint Value Registers 1 */
#define CP14_DBGBVR2(r)                _CP14(0, r, c0, c2, 4)  /* Debug Breakpoint Value Registers 2 */
#define CP14_DBGBVR3(r)                _CP14(0, r, c0, c3, 4)  /* Debug Breakpoint Value Registers 3 */
#define CP14_DBGBVR4(r)                _CP14(0, r, c0, c4, 4)  /* Debug Breakpoint Value Registers 4 */
#define CP14_DBGBVR5(r)                _CP14(0, r, c0, c5, 4)  /* Debug Breakpoint Value Registers 5 */
#define CP14_DBGBVR6(r)                _CP14(0, r, c0, c6, 4)  /* Debug Breakpoint Value Registers 6 */
#define CP14_DBGBVR7(r)                _CP14(0, r, c0, c7, 4)  /* Debug Breakpoint Value Registers 7 */

#define CP14_DBGBCR0(r)                _CP14(0, r, c0, c0, 5)  /* Debug Breakpoint Control Registers 0 */
#define CP14_DBGBCR1(r)                _CP14(0, r, c0, c1, 5)  /* Debug Breakpoint Control Registers 1 */
#define CP14_DBGBCR2(r)                _CP14(0, r, c0, c2, 5)  /* Debug Breakpoint Control Registers 2 */
#define CP14_DBGBCR3(r)                _CP14(0, r, c0, c3, 5)  /* Debug Breakpoint Control Registers 3 */
#define CP14_DBGBCR4(r)                _CP14(0, r, c0, c4, 5)  /* Debug Breakpoint Control Registers 4 */
#define CP14_DBGBCR5(r)                _CP14(0, r, c0, c5, 5)  /* Debug Breakpoint Control Registers 5 */
#define CP14_DBGBCR6(r)                _CP14(0, r, c0, c6, 5)  /* Debug Breakpoint Control Registers 6 */
#define CP14_DBGBCR7(r)                _CP14(0, r, c0, c7, 5)  /* Debug Breakpoint Control Registers 7 */

#define CP14_DBGWVR0(r)                _CP14(0, r, c0, c0, 6)  /* Debug Watchpoint Value Registers 0 */
#define CP14_DBGWVR1(r)                _CP14(0, r, c0, c1, 6)  /* Debug Watchpoint Value Registers 1 */
#define CP14_DBGWVR2(r)                _CP14(0, r, c0, c2, 6)  /* Debug Watchpoint Value Registers 2 */
#define CP14_DBGWVR3(r)                _CP14(0, r, c0, c3, 6)  /* Debug Watchpoint Value Registers 3 */
#define CP14_DBGWVR4(r)                _CP14(0, r, c0, c4, 6)  /* Debug Watchpoint Value Registers 4 */
#define CP14_DBGWVR5(r)                _CP14(0, r, c0, c5, 6)  /* Debug Watchpoint Value Registers 5 */
#define CP14_DBGWVR6(r)                _CP14(0, r, c0, c6, 6)  /* Debug Watchpoint Value Registers 6 */
#define CP14_DBGWVR7(r)                _CP14(0, r, c0, c7, 6)  /* Debug Watchpoint Value Registers 7 */

#define CP14_DBGWCR0(r)                _CP14(0, r, c0, c0, 7)  /* Debug Watchpoint Control Registers 0 */
#define CP14_DBGWCR1(r)                _CP14(0, r, c0, c1, 7)  /* Debug Watchpoint Control Registers 1 */
#define CP14_DBGWCR2(r)                _CP14(0, r, c0, c2, 7)  /* Debug Watchpoint Control Registers 2 */
#define CP14_DBGWCR3(r)                _CP14(0, r, c0, c3, 7)  /* Debug Watchpoint Control Registers 3 */
#define CP14_DBGWCR4(r)                _CP14(0, r, c0, c4, 7)  /* Debug Watchpoint Control Registers 4 */
#define CP14_DBGWCR5(r)                _CP14(0, r, c0, c5, 7)  /* Debug Watchpoint Control Registers 5 */
#define CP14_DBGWCR6(r)                _CP14(0, r, c0, c6, 7)  /* Debug Watchpoint Control Registers 6 */
#define CP14_DBGWCR7(r)                _CP14(0, r, c0, c7, 7)  /* Debug Watchpoint Control Registers 7 */

#define CP14_SET(reg, value)            \
  do                                    \
    {                                   \
      __asm__ __volatile__              \
      (                                 \
        "mcr " CP14_ ## reg(0) "\n"     \
        :: "r"(value): "memory"         \
      );                                \
    }                                   \
  while(0)                              \

#define CP14_GET(reg)                   \
  ({                                    \
     uint32_t _value;                   \
     __asm__ __volatile__               \
     (                                  \
       "mrc " CP14_ ## reg(0) "\n"      \
       : "=r"(_value) :: "memory"       \
     );                                 \
     _value;                            \
  })                                    \

#define __CONCAT(r, n) CP14_ ## r ## n
#define CONCAT(r, n) __CONCAT(r, n)

#define CP14_SET_CONCAT(reg, n, value)  \
  do                                    \
    {                                   \
      __asm__ __volatile__              \
      (                                 \
        "mcr " CONCAT(reg, n)(0) "\n"   \
        :: "r"(value): "memory"         \
      );                                \
    }                                   \
  while(0)                              \

#define CP14_GET_CONCAT(reg, n)         \
  ({                                    \
     uint32_t _value;                   \
     __asm__ __volatile__               \
     (                                  \
       "mrc " CONCAT(reg, n)(0) "\n"    \
       : "=r"(_value) :: "memory"       \
     );                                 \
     _value;                            \
  })                                    \

#endif