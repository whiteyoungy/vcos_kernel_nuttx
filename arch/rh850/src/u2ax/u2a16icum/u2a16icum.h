/****************************************************************************
 * arch/rh850/src/u2ax/u2a16icum/u2a16icum.h
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifndef __ARCH_RH850_SRC_U2AX_U2A16ICUM_H
#define __ARCH_RH850_SRC_U2AX_U2A16ICUM_H

#define HW_INIT_CORE_ID         3
#define HW_G0MEV_BASE           0xFFFEEC00
#define HW_CORE_START_PATTERN   0x0FEE0BEE
#define HW_CORE_SYNC_REG        HW_G0MEV_BASE
#define CPU_CORE_NUM            1

#define RAM_ORIGIN              0xFEDF0000
#define RAM_LIMIT               0xFEDFFBF0

#define FPU_USED                0
#define MPU_USED                0

#define MOSCE        (*((volatile unsigned int *)(0xFF988000))) /* MainOSC enable register */
#define MOSCS        (*((volatile unsigned int *)(0xFF988004))) /* MainOSC status register */
#define CKSC_CPUC    (*((volatile unsigned int *)(0xFF980100))) /* CLK_CPU selector control register */
#define CKSC_CPUS    (*((volatile unsigned int *)(0xFF980108))) /* CLK_CPU selector status register */
#define CLKD_PLLC    (*((volatile unsigned int *)(0xFF980120))) /* CLK_PLLO divider control register */
#define CLKD_PLLS    (*((volatile unsigned int *)(0xFF980128))) /* CLK_PLLO divider status register */
#define CLKKCPROT1   (*((volatile unsigned int *)(0xFF980700))) /* Clock Controller Register Key Code Protection Register 1 */
#define MSRKCPROT    (*((volatile unsigned int *)(0xFF981710))) /* Module Standby Register Key Code Protection Register */
#define MSR_OSTM     (*((volatile unsigned int *)(0xFF981180))) /* Module Standby Register for OSTM */
#define PLLE         (*((volatile unsigned int *)(0xFF980000))) /* PLL enable register */
#define PLLS         (*((volatile unsigned int *)(0xFF980004))) /* PLL status register */

void u2a16icum_cmdreg_init(void);
#endif  /* __ARCH_RH850_SRC_U2AX_U2A16ICUM_H */