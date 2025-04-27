/****************************************************************************
 * arch/rh850/src/common/rh850_saveusercontext.c
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
#include <nuttx/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_saveusercontext
 *
 * Description:
 *   Save the current thread context
 *
 ****************************************************************************/
.globl up_saveusercontext
up_saveusercontext:
.globl _up_saveusercontext
_up_saveusercontext:
    st.w r30, 0[r6]
    st.w r29, 4[r6]
    st.w r28, 8[r6]
    st.w r27, 12[r6]
    st.w r26, 16[r6]
    st.w r25, 20[r6]
    st.w r24, 24[r6]
    st.w r23, 28[r6]
    st.w r22, 32[r6]
    st.w r21, 36[r6]
    st.w r20, 40[r6]
    st.w r20, 44[r6]
    st.w lp,  48[r6]
    st.w r1,  52[r6]
    st.w r19, 56[r6]
    st.w r18, 60[r6]
    st.w r17, 64[r6]
    st.w r16, 68[r6]
    st.w r15, 72[r6]
    st.w r14, 76[r6]
    st.w r13, 80[r6]
    st.w r12, 84[r6]
    st.w r11, 88[r6]
    st.w r10, 92[r6]
    st.w r9,  96[r6]
    st.w r8,  100[r6]
    st.w r7,  104[r6]
    st.w r6,  108[r6]
    st.w lp, 112[r6]

    jmp [lp]