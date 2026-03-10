
    AREA |.text|, CODE, READONLY, ALIGN=11

    IMPORT KeHandleInterruptFrame

KiInterrupt0 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x0, #0
    b CommonExceptionEntry
KiInterrupt0 endp

KiInterrupt1 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x0, #1
    b CommonExceptionEntry
KiInterrupt1 endp

KiInterrupt2 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x0, #2
    b CommonExceptionEntry
KiInterrupt2 endp

KiInterrupt3 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x0, #3
    b CommonExceptionEntry
KiInterrupt3 endp

KiInterrupt4 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #4
    b CommonExceptionEntry
KiInterrupt4 endp

KiInterrupt5 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x0, #5
    b CommonExceptionEntry
KiInterrupt5 endp

KiInterrupt6 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #6
    b CommonExceptionEntry
KiInterrupt6 endp

KiInterrupt7 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #7
    b CommonExceptionEntry
KiInterrupt7 endp

KiInterrupt8 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #8
    b CommonExceptionEntry
KiInterrupt8 endp

KiInterrupt9 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #9
    b CommonExceptionEntry
KiInterrupt9 endp

KiInterrupt10 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #10
    b CommonExceptionEntry
KiInterrupt10 endp

KiInterrupt11 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #11
    b CommonExceptionEntry
KiInterrupt11 endp

KiInterrupt12 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #12
    b CommonExceptionEntry
KiInterrupt12 endp

KiInterrupt13 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #13
    b CommonExceptionEntry
KiInterrupt13 endp

KiInterrupt14 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #14
    b CommonExceptionEntry
KiInterrupt14 endp

KiInterrupt15 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #15
    b CommonExceptionEntry
KiInterrupt15 endp

KiInterrupt16 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #16
    b CommonExceptionEntry
KiInterrupt16 endp

KiInterrupt17 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #17
    b CommonExceptionEntry
KiInterrupt17 endp

KiInterrupt18 proc
    sub sp, sp, #(36*8)
    stp x0,  x1,  [sp, #0*8]
    stp x2,  x3,  [sp, #2*8]
    stp x4,  x5,  [sp, #4*8]
    stp x6,  x7,  [sp, #6*8]
    stp x8,  x9,  [sp, #8*8]
    stp x10, x11, [sp, #10*8]
    stp x12, x13, [sp, #12*8]
    stp x14, x15, [sp, #14*8]
    stp x16, x17, [sp, #16*8]
    str x18,      [sp, #18*8]

    stp x19, x20, [sp, #19*8]
    stp x21, x22, [sp, #21*8]
    stp x23, x24, [sp, #23*8]
    stp x25, x26, [sp, #25*8]
    stp x27, x28, [sp, #27*8]
    stp fp, lr, [sp, #29*8]
    mov x9, #18
    b CommonExceptionEntry
KiInterrupt18 endp

CommonExceptionEntry proc
    str x0, [sp, #31*8]
    str x0, [sp, #32*8]
    str x0, [sp, #33*8]
    str x0, [sp, #34*8]

    mov x9, sp
    add x9, x9, #(36*8)
    str x9, [sp, #35*8]

    mov x0, sp
    bl KeHandleInterruptFrame
    mov sp, x0

    ldp x0,  x1,  [sp, #0*8]
    ldp x2,  x3,  [sp, #2*8]
    ldp x4,  x5,  [sp, #4*8]
    ldp x6,  x7,  [sp, #6*8]
    ldp x8,  x9,  [sp, #8*8]
    ldp x10, x11, [sp, #10*8]
    ldp x12, x13, [sp, #12*8]
    ldp x14, x15, [sp, #14*8]
    ldp x16, x17, [sp, #16*8]
    ldr x18,      [sp, #18*8]

    ldp x19, x20, [sp, #19*8]
    ldp x21, x22, [sp, #21*8]
    ldp x23, x24, [sp, #23*8]
    ldp x25, x26, [sp, #25*8]
    ldp x27, x28, [sp, #27*8]

    ldp fp,  lr,  [sp, #29*8]

    add sp, sp, #(36*8)

    eret
CommonExceptionEntry endp


    AREA |.text|, CODE, READONLY

    EXPORT _InterlockedExchangeAdd64_nf
_InterlockedExchangeAdd64_nf PROC
add_nf_loop
    ldxr    x2, [x0]
    add     x3, x2, x1
    stxr    w4, x3, [x0]
    cbnz    w4, add_nf_loop
    mov     x0, x2
    ret
_InterlockedExchangeAdd64_nf ENDP

    EXPORT _InterlockedExchangeAdd64_rel
_InterlockedExchangeAdd64_rel PROC
add_rel_loop
    ldxr    x2, [x0]
    add     x3, x2, x1
    stlxr   w4, x3, [x0]
    cbnz    w4, add_rel_loop
    mov     x0, x2
    ret
_InterlockedExchangeAdd64_rel ENDP
    EXPORT _InterlockedExchangeAdd64_acq
_InterlockedExchangeAdd64_acq PROC
add_acq_loop
    ldaxr   x2, [x0]
    add     x3, x2, x1
    stxr    w4, x3, [x0]
    cbnz    w4, add_acq_loop
    mov     x0, x2
    ret
_InterlockedExchangeAdd64_acq ENDP
    EXPORT _InterlockedExchangeAdd64
_InterlockedExchangeAdd64 PROC
add_full_loop
    ldaxr   x2, [x0]
    add     x3, x2, x1
    stlxr   w4, x3, [x0]
    cbnz    w4, add_full_loop
    dmb     ish
    mov     x0, x2
    ret
_InterlockedExchangeAdd64 ENDP
    EXPORT _InterlockedCompareExchange64_nf
_InterlockedCompareExchange64_nf PROC
nf_loop
    ldaxr   x3, [x0]
    cmp     x3, x2
    b.ne    nf_done
    stxr    w4, x1, [x0]
    cbnz    w4, nf_loop
nf_done
    mov     x0, x3
    ret
_InterlockedCompareExchange64_nf ENDP
    EXPORT _InterlockedCompareExchange64_acq
_InterlockedCompareExchange64_acq PROC
acq_loop
    ldaxr   x3, [x0]
    cmp     x3, x2
    b.ne    acq_done
    stxr    w4, x1, [x0]
    cbnz    w4, acq_loop
acq_done
    mov     x0, x3
    ret
_InterlockedCompareExchange64_acq ENDP
    EXPORT _InterlockedCompareExchange64_rel
_InterlockedCompareExchange64_rel PROC
rel_loop
    ldaxr   x3, [x0]
    cmp     x3, x2
    b.ne    rel_done
    stlxr   w4, x1, [x0]
    cbnz    w4, rel_loop
rel_done
    mov     x0, x3
    ret
_InterlockedCompareExchange64_rel ENDP
    EXPORT _InterlockedCompareExchange64
_InterlockedCompareExchange64 PROC
full_loop
    ldaxr   x3, [x0]
    cmp     x3, x2
    b.ne    full_done
    stlxr   w4, x1, [x0]
    cbnz    w4, full_loop
full_done
    dmb     ish
    mov     x0, x3
    ret
_InterlockedCompareExchange64 ENDP

    EXPORT _InterlockedExchange64_nf
_InterlockedExchange64_nf PROC
nf_loop_InterlockedExchange64_nf
    ldxr    x2, [x0]
    stxr    w4, x1, [x0]
    cbnz    w4, nf_loop_InterlockedExchange64_nf
    mov     x0, x2
    ret
_InterlockedExchange64_nf ENDP

    EXPORT _InterlockedExchange64_acq
_InterlockedExchange64_acq PROC
acq_loop_InterlockedExchange64_nf
    ldaxr   x2, [x0]
    stxr    w4, x1, [x0]
    cbnz    w4, acq_loop_InterlockedExchange64_nf
    mov     x0, x2
    ret
_InterlockedExchange64_acq ENDP

    EXPORT _InterlockedExchange64_rel
_InterlockedExchange64_rel PROC
rel_loop_InterlockedExchange64_nf
    ldxr    x2, [x0]
    stlxr   w4, x1, [x0]
    cbnz    w4, rel_loop_InterlockedExchange64_nf
    mov     x0, x2
    ret
_InterlockedExchange64_rel ENDP

    EXPORT _InterlockedExchange64
_InterlockedExchange64 PROC
full_loop_InterlockedExchange64_nf
    ldaxr   x2, [x0]
    stlxr   w4, x1, [x0]
    cbnz    w4, full_loop_InterlockedExchange64_nf
    dmb     ish
    mov     x0, x2
    ret
_InterlockedExchange64 ENDP

    EXPORT SwitchStackAndCall
SwitchStackAndCall PROC
    and     x0, x0, #0xFFFFFFFFFFFFFFF0
    mov     sp, x0
    mov     x0, x1
    br      x2
SwitchStackAndCall ENDP

    EXPORT ExceptionVectorTable
    ALIGN 0x800
ExceptionVectorTable PROC
    ALIGN 0x80
    b KiInterrupt0

    ALIGN 0x80
    b KiInterrupt1

    ALIGN 0x80
    b KiInterrupt2

    ALIGN 0x80
    b KiInterrupt3

    ALIGN 0x80
    b KiInterrupt4

    ALIGN 0x80
    b KiInterrupt5

    ALIGN 0x80
    b KiInterrupt6

    ALIGN 0x80
    b KiInterrupt7

    ALIGN 0x80
    b KiInterrupt8

    ALIGN 0x80
    b KiInterrupt9

    ALIGN 0x80
    b KiInterrupt10

    ALIGN 0x80
    b KiInterrupt11

    ALIGN 0x80
    b KiInterrupt12

    ALIGN 0x80
    b KiInterrupt13

    ALIGN 0x80
    b KiInterrupt14

    ALIGN 0x80
    b KiInterrupt15

    ALIGN 0x80
    b KiInterrupt16

    ALIGN 0x80
    b KiInterrupt17

    ALIGN 0x80
    b KiInterrupt18
ExceptionVectorTable ENDP

    END
