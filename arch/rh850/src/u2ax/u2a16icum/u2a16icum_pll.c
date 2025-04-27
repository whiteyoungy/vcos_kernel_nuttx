/****************************************************************************
 * arch/rh850/src/u2ax/u2a16icum/u2a16icum_pll.c
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

#include <rh850_internal.h>
#include <nuttx/init.h>
#include <nuttx/board.h>
#include "chip.h"

#define BRSHWNOP10() do {asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");} while(0)
static void u2a16icum_hw_time_100nop(void)
{
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
}

void u2a16icum_earlypllinit(void)
{
  CLKKCPROT1 = 0xa5a5a501;    /* enable writing to protected registers */

  MOSCE = 0x01;
  while (MOSCS != 0x03);      /* Enabled by default, wait for Main OSC to stabilize */
  u2a16icum_hw_time_100nop();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();

  PLLE = 0x01;
  while (PLLS != 0x03);       /* Enabled by default if Option Byte STARTUPPLL is set, wait for PLL to stabilize */
  u2a16icum_hw_time_100nop();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();
  BRSHWNOP10();

  CLKD_PLLC = 0x02;           /* Change PLL division ratio to 1/2 */
  while (CLKD_PLLS != 0x03);  /* Verify divider synchronization */
  CKSC_CPUC = 0x00;           /* Change system clock source to CLK_PLLO */
  while (CKSC_CPUS != 0x00);  /* Verify change */
  CLKD_PLLC = 0x01;           /* Change PLL division ratio to 1/1 */
  while (CLKD_PLLS != 0x03);  /* Verify divider synchronization */

  /* Clock selection config */

  CLKKCPROT1 = 0xa5a5a500;    /* disable writing to protected registers */
  MSRKCPROT = 0xa5a5a501;     /* Enable writing to protected registers of standby controller */
  MSR_OSTM     = 0x0;         /* Enable clocks for OSTMn */
  MSRKCPROT = 0xa5a5a500;     /* Disable writing to protected registers of standby controller */
}
