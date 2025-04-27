
    .extern _int_stack_limit
    .extern _rh850_svcall

    .section ".exceptionhandle", .text

.align 4
.globl _rh850_exception_handler
_rh850_exception_handler:
    jr _rh850_exception_handler

.align 4
.globl _rh850_trap0_handler
_rh850_trap0_handler:
    prepare {lp}, 0
    pushsp r6 - r19                     /* Save General Purpose Register (caller save register) */
    pushsp r1
    stsr EIPSW, r6                      /* Save EIPSW */
    stsr EIPC, r7                       /* Save EIPC */
    pushsp r6 - r7
    pushsp r20 - r30                    /* Save General Purpose Register (callee save register) */

    mov sp, r6

    jarl _rh850_svcall, lp

    mov r10, sp

    popsp r20 - r30                     /* Restore General Purpose Register (callee save register) */
    popsp r6 - r7
    ldsr r7, EIPC                       /* Restore EIPC */
    ldsr r6, EIPSW                      /* Restore EIPSW */

    popsp r1                       /* Restore General Purpose Register (caller save register) */
    popsp r6 - r19

    dispose 0, {lp}

    eiret

.align 4
.globl _rh850_isr_handler
_rh850_isr_handler:
    /* Save previous context into stack */
    prepare {lp}, 0
    pushsp r6 - r19                     /* Save General Purpose Register (caller save register) */
    pushsp r1
    stsr EIPSW, r6                      /* Save EIPSW */
    stsr EIPC, r7                       /* Save EIPC */
    pushsp r6 - r7
    pushsp r20 - r30                    /* Save General Purpose Register (callee save register) */

    stsr EIIC, r6

    mov 0x00000FFF,r7

    and r7,r6

    mov sp, r7

    mov _int_stack_limit, sp                   /*change stack to isr stack*/

    jarl _rh850_doirq, lp

    mov r10, sp

    popsp r20 - r30                     /* Restore General Purpose Register (callee save register) */
    popsp r6 - r7
    ldsr r7, EIPC                       /* Restore EIPC */
    ldsr r6, EIPSW                      /* Restore EIPSW */

    popsp r1                            /* Restore General Purpose Register (caller save register) */
    popsp r6 - r19

    dispose 0, {lp}

    eiret