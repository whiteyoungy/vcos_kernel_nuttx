    .extern _startup_entry
    .extern _rh850_trap0_handler
    .extern _rh850_isr_handler
    .extern _rh850_exception_handler

    .section ".intvect", .text

    .global __reset
    .align	512
__reset:    /* 0x0000 */
    jr _startup_entry

    .align 16
__syserr:   /* 0x0010 */
    jr _rh850_exception_handler

    .align 16
__hvtrap:   /* 0x0020 */
    jr _rh850_exception_handler

    .align 16
__fetrap:   /* 0x0030 */
    jr _rh850_exception_handler

    .align 16
__trap0:    /* 0x0040 */
    jr _rh850_trap0_handler

    .align 16
__trap1:    /* 0x0050 */
    jr _rh850_exception_handler

    .align 16
__rie:      /* 0x0060 */
    jr _rh850_exception_handler

    .align 16
__fpe:      /* 0x0070 */
    jr _rh850_exception_handler

    .align 16
__ucpop:    /* 0x0080 */
    jr _rh850_exception_handler

    .align 16
__mip:      /* 0x0090 */
    jr _rh850_exception_handler

    .align 16
__pie:      /* 0x00a0 */
    jr _rh850_exception_handler

    .align 16
__debug:    /* 0x00b0 */
    jr __debug

    .align 16
__mae:      /* 0x00c0 */
    jr _rh850_exception_handler

    .align 16
__rfu:      /* 0x00d0 */
    jr __rfu

    .align 16
__fenmi:    /* 0x00e0 */
    jr __fenmi

    .align 16
__feint:    /* 0x00f0 */
    jr __feint

    .align 16
__eiint0:   /* 0x0100 */
    jr _rh850_isr_handler

