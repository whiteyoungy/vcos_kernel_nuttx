/****************************************************************************
 * arch/arm/src/armv8-r/arm_vectortab.S
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

	.file	"arm_vectortab.S"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Symbols
 ****************************************************************************/

#ifdef CONFIG_ARM_TOOLCHAIN_ARMCLANG
	.eabi_attribute Tag_ABI_align_preserved, 1
#endif

	.syntax	unified
	.arm

	.globl		_sys_vector_start
	.globl		_sys_vector_end

/****************************************************************************
 * Assembly Macros
 ****************************************************************************/

/****************************************************************************
 * Name: _vector_start
 *
 * Description:
 *   Vector initialization block
 ****************************************************************************/

	.section	.sys_vectors, "ax"

_sys_vector_start:
	b		__start		/* 0x00: Reset */
	b		arm_vectorundefinsn		/* 0x04: Undefined instruction */
	b		arm_vectorsvc		/* 0x08: Software interrupt */
	b		arm_vectorprefetch	/* 0x0c: Prefetch abort */
	b		arm_vectordata		/* 0x10: Data abort */
	b		arm_vectoraddrexcptn		/* 0x14: Address exception (reserved) */
	b		arm_vectorirq		/* 0x18: IRQ */
	b		arm_vectorfiq		/* 0x1c: FIQ */

	.globl	__start
	.globl	arm_vectorundefinsn
	.globl	arm_vectorsvc
	.globl	arm_vectorprefetch
	.globl	arm_vectordata
	.globl	arm_vectoraddrexcptn
	.globl	arm_vectorirq
	.globl	arm_vectorfiq

.Lresethandler:
	.long	__start
.Lundefinedhandler:
	.long	arm_vectorundefinsn
.Lsvchandler:
	.long	arm_vectorsvc
.Lprefetchaborthandler:
	.long	arm_vectorprefetch
.Ldataaborthandler:
	.long	arm_vectordata
.Laddrexcptnhandler:
	.long	arm_vectoraddrexcptn
.Lirqhandler:
	.long	arm_vectorirq
.Lfiqhandler:
	.long	arm_vectorfiq

_sys_vector_end:
	.size	_sys_vector_start, . - _sys_vector_start

/****************************************************************************
 * Name: _hyp_vector_start
 *
 * Description:
 *   Vector initialization block
 ****************************************************************************/

	.section	.hyp_vectors, "ax"
	.globl		_hyp_vector_start

_hyp_vector_start:
	b		__start					/* 0x00: Reset */
	b		$+0x0					/* 0x04: Undefined instruction */
	b		$+0x0					/* 0x08: hvc handler */
	b		$+0x0					/* 0x0c: Prefetch abort */
	b		$+0x0					/* 0x10: Data abort */
#if defined(CONFIG_ARCH_CHIP_SCHU_ISP_STL)
	b		el2_hyptrap_handler			/* 0x14: hyptrap handler */
#else
	b		$+0x0					/* 0x14: hyptrap handler */
#endif
	b		$+0x0					/* 0x18: IRQ */
	b		$+0x0					/* 0x1c: FIQ */

	.globl	__start

.Lhypresethandler:
	.long	__start

_hyp_vector_end:
	.size	_hyp_vector_start, . - _hyp_vector_start
	.end