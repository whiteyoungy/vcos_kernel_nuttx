/****************************************************************************
 * arch/rh850/src/u2ax/u2a16icum/u2a16icum_ostm.h
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

#ifndef _U2A16ICUM_OSTM_H
#define _U2A16ICUM_OSTM_H

#include "stdint.h"

#define OSTM_FREQUENCY      60000000

#define REG08(reg)                                 (*(volatile uint8_t *)(reg))
#define REG16(reg)                                 (*(volatile uint16_t *)(reg))
#define REG32(reg)                                 (*(volatile uint32_t *)(reg))

#define OSTM0_BASE       0xFFFEE000UL
#define OSTM1_BASE       0xFFFEE040UL

#define OSTM_CMP(n)       REG32(((unsigned char *)(n)) + 0x00UL)
#define OSTM_CNT(n)       REG32(((unsigned char *)(n)) + 0x04UL)
#define OSTM_TE(n)        REG08(((unsigned char *)(n)) + 0x10UL)
#define OSTM_TS(n)        REG08(((unsigned char *)(n)) + 0x14UL)
#define OSTM_TT(n)        REG08(((unsigned char *)(n)) + 0x18UL)
#define OSTM_CTL(n)       REG08(((unsigned char *)(n)) + 0x20UL)
#define OSTM_EMU(n)       REG08(((unsigned char *)(n)) + 0x24UL)

#define OSTM_IE      (1U << 7U)
#define OSTM_MD2     (1U << 2U)
#define OSTM_MD1     (1U << 1U)
#define OSTM_MD0     (1U << 0U)

static inline void u2a16icum_ostm_set_ctl(void * base, uint8_t mode)
{
    REG32(0xfffb2000ul) = 0x01ul;
    OSTM_CTL(base) = mode;
}

static inline void u2a16icum_ostm_set_compare(void * base, uint32_t value)
{
    OSTM_CMP(base) = value;
}

static inline void u2a16icum_ostm_start(void * base)
{
    OSTM_TS(base) = 0x1;
}

static inline void u2a16icum_ostm_stop(void * base)
{
    OSTM_TT(base) = 0x1;
}

static inline uint32_t u2a16icum_ostm_gettime(void * base)
{
    return OSTM_CNT(base);
}

#endif