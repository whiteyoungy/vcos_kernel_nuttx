/****************************************************************************
 * arch/rh850/src/common/rh850_stub.c
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

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/serial/serial.h>

#include <arch/board/board.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* If we are not using the serial driver for the console, then we still must
 * provide some minimal implementation of up_putc.
 */

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug  writes
 *
 ****************************************************************************/

void up_putc(int ch)
{
  return ch;
}

extern int main(void);

extern uint32_t _data_start[];
extern uint32_t _data_limit[];
extern uint32_t _data_rom_start[];
extern uint32_t _data_rom_limit[];

extern uint32_t _sdata_start[];
extern uint32_t _sdata_limit[];
extern uint32_t _sdata_rom_start[];
extern uint32_t _sdata_rom_limit[];

extern uint32_t _zdata_start[];
extern uint32_t _zdata_limit[];
extern uint32_t _zdata_rom_start[];
extern uint32_t _zdata_rom_limit[];

void ghs_c_init(void)
{
  memcpy(_data_start, _data_rom_start,
          ((uint8_t *)_data_rom_limit - (uint8_t *)_data_rom_start));
  memcpy(_sdata_start, _sdata_rom_start,
          ((uint8_t *)_sdata_rom_limit - (uint8_t *)_sdata_rom_start));
  memcpy(_zdata_start, _zdata_rom_start,
          ((uint8_t *)_zdata_rom_limit - (uint8_t *)_zdata_rom_start));
  main();
}
void __attribute__((weak)) __gh_fputs_stdout(void)
{
}

void __attribute__((weak)) __gh_strout(void)
{
}

void __attribute__((weak))  __gh_outchars(void)
{
}

void __attribute__((weak)) __fmul(void)
{
}

