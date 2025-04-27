/****************************************************************************
 * arch/rh850/src/u2ax/chip.h
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

#ifndef __ARCH_RH850_SRC_U2AX_CHIP_H
#define __ARCH_RH850_SRC_U2AX_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#define SWSRESA  (*((unsigned int*)(0xFF98840C)))
#define RESF     (*((unsigned int*)(0xFF988500))) /* Reset factor register */
#define BRSHW_SWRESET_TRIGGERT_MASK  0x00000010   /* Software reset flag (SRES2F0) */

#ifdef CONFIG_ARCH_CHIP_U2A16ICUM
#include "u2a16icum/u2a16icum.h"

void u2a16icum_earlypllinit(void);

static inline unsigned int u2a16icum_getcoreid(void)
{
    unsigned int coreid;
    __asm__ __volatile__(
        "stsr  0, %0, 2\n"
        : "=r"(coreid)
        :: "cc", "memory");
    coreid >>= 17;
    return coreid;
}
#endif

#endif /* __ARCH_RH850_SRC_U2AX_CHIP_H */
