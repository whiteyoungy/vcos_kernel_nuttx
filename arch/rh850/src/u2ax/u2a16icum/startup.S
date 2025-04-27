#include "u2a16icum.h"
    .extern __reset
    .extern _startup_stack_limit

    .section ".startup", "ax"
.globl startup_entry
startup_entry:
.globl _startup_entry
_startup_entry:

    /* read coreid to r11*/
    stsr  0, r11, 2
    shr   17, r11
PCUWaitLabel:
    cmp   0x3, r11
    bne   PCUWaitLabel


    jr32 pre_asmstarup_hook


.globl pre_asmstarup_hook
pre_asmstarup_hook:
    jr32 coreRegisterInit


/* Initialize core ID independent core registers */
.globl coreRegisterInit
coreRegisterInit:
/* General Purpose Registers */
    mov r0, r1
    mov r0, r2
    mov r0, r3
    mov r0, r4
    mov r0, r5
    mov r0, r6
    mov r0, r7
    mov r0, r8
    mov r0, r9
    mov r0, r10
    mov r0, r11
    mov r0, r12
    mov r0, r13
    mov r0, r14
    mov r0, r15
    mov r0, r16
    mov r0, r17
    mov r0, r18
    mov r0, r19
    mov r0, r20
    mov r0, r21
    mov r0, r22
    mov r0, r23
    mov r0, r24
    mov r0, r25
    mov r0, r26
    mov r0, r27
    mov r0, r28
    mov r0, r29
    mov r0, r30
    mov r0, r31

/* Basic System Registers */
    ldsr r0, 0, 0  /* EIPC */
    ldsr r0, 2, 0  /* FEPC */
    ldsr r0, 16, 0 /* CTPC */
    ldsr r0, 20, 0 /* CTBP */
    ldsr r0, 28, 0 /* EIWR */
    ldsr r0, 29, 0 /* FEWR */
    ldsr r0, 3, 1  /* EBASE */
    ldsr r0, 4, 1  /* INTBP */
    ldsr r0, 11, 1 /* SCCFG */
    ldsr r0, 12, 1 /* SCBP */
    ldsr r0, 6, 2  /* MEA */
    ldsr r0, 7, 2  /* ASID */
    ldsr r0, 8, 2  /* MEI */


/* MPU Function registers */
    ldsr r0, 8, 5   /* MCA */
    ldsr r0, 9, 5   /* MCS */
    ldsr r0, 11, 5  /* MCR */
    ldsr r0, 0, 6   /* MPLA0 */
    ldsr r0, 1, 6   /* MPUA0 */
    ldsr r0, 2, 6   /* MPAT0 */
    ldsr r0, 4, 6   /* MPLA1 */
    ldsr r0, 5, 6   /* MPUA1 */
    ldsr r0, 6, 6   /* MPAT1 */
    ldsr r0, 8, 6   /* MPLA2 */
    ldsr r0, 9, 6   /* MPUA2 */
    ldsr r0, 10, 6  /* MPAT2 */
    ldsr r0, 12, 6  /* MPLA3 */
    ldsr r0, 13, 6  /* MPUA3 */
    ldsr r0, 14, 6  /* MPAT3 */
    ldsr r0, 16, 6  /* MPLA4 */
    ldsr r0, 17, 6  /* MPUA4 */
    ldsr r0, 18, 6  /* MPAT4 */
    ldsr r0, 20, 6  /* MPLA5 */
    ldsr r0, 21, 6  /* MPUA5 */
    ldsr r0, 22, 6  /* MPAT5 */
    ldsr r0, 24, 6  /* MPLA6 */
    ldsr r0, 25, 6  /* MPUA6 */
    ldsr r0, 26, 6  /* MPAT6 */
    ldsr r0, 28, 6  /* MPLA7 */
    ldsr r0, 29, 6  /* MPUA7 */
    ldsr r0, 30, 6  /* MPAT7 */
    ldsr r0, 0, 7   /* MPLA8 */
    ldsr r0, 1, 7   /* MPUA8 */
    ldsr r0, 2, 7   /* MPAT8 */
    ldsr r0, 4, 7   /* MPLA9 */
    ldsr r0, 5, 7   /* MPUA9 */
    ldsr r0, 6, 7   /* MPAT9 */
    ldsr r0, 8, 7   /* MPLA10 */
    ldsr r0, 9, 7   /* MPUA10 */
    ldsr r0, 10, 7  /* MPAT10 */
    ldsr r0, 12, 7  /* MPLA11 */
    ldsr r0, 13, 7  /* MPUA11 */
    ldsr r0, 14, 7  /* MPAT11 */

/* Initialization of the global pointer */
    mov   __gp, gp
/* Initialization of the text pointer */
    mov   __tp, tp

    jr32 coreRegisterInit2

/* Initialize all core ID dependent core registers */
.globl coreRegisterInit2
coreRegisterInit2:
/* Program the brs default vector table base addresses to handle exceptions during startup */
    /* Exception handler vector address */
    mov __reset, r6                  /* set EBASE to __reset*/
    ori   0x01, r6, r6               /* set EBASE.RINT bit to 1*/
    ldsr r6, 3, 1

    di                              /* Disable interrupts */
    stsr  PSW, r6
    ori   0x8000, r6, r6            /* Set PSW.EBV bit to 1. */
    ldsr r6, PSW

    jr32 StartupZeroInitLoop

.globl StartupZeroInitLoop
StartupZeroInitLoop:

    /* set LRAM start and end address */
    mov 0xFEDF0000, ep
    mov 0xFEDFFBF0, r13

    addi -32, r13, r14

initram_loop:
    sst.w r0, 0x00[ep]
    sst.w r0, 0x04[ep]
    sst.w r0, 0x08[ep]
    sst.w r0, 0x0c[ep]
    sst.w r0, 0x10[ep]
    sst.w r0, 0x14[ep]
    sst.w r0, 0x18[ep]
    sst.w r0, 0x1c[ep]
    addi 0x20, ep, ep

    cmp r14, ep
    ble initram_loop


/* Initialize all core registers of actual running core with specific init values */
.globl stackPointerInit
stackPointerInit:
    mov _startup_stack_limit, sp

    jr32 coreRegisterInit3

/* Initialize additional core registers  */
.globl coreRegisterInit3
coreRegisterInit3:

    /* jump to main function */
    jarl _ghs_c_init, lp

_exit:
    br  _exit

/* Minimal reset vector at entry address (e. g. 0x0)   */
.section ".bootcode", "ax"
.align 16
jr32 startup_entry
    nop
    nop
    nop
.align 16
loop:
    jr32 loop
    nop
    nop
    nop