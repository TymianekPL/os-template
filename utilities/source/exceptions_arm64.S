KiInterrupt0:
    mov x9, #0
    b CommonExceptionEntry

KiInterrupt1:
    mov x9, #1
    b CommonExceptionEntry

KiInterrupt2:
    mov x9, #2
    b CommonExceptionEntry

KiInterrupt3:
    mov x9, #3
    b CommonExceptionEntry

KiInterrupt4:
    mov x9, #4
    b CommonExceptionEntry

KiInterrupt5:
    mov x9, #5
    b CommonExceptionEntry

KiInterrupt6:
    mov x9, #6
    b CommonExceptionEntry

KiInterrupt7:
    mov x9, #7
    b CommonExceptionEntry

KiInterrupt8:
    mov x9, #8
    b CommonExceptionEntry

KiInterrupt9:
    mov x9, #9
    b CommonExceptionEntry

KiInterrupt10:
    mov x9, #10
    b CommonExceptionEntry

KiInterrupt11:
    mov x9, #11
    b CommonExceptionEntry

KiInterrupt12:
    mov x9, #12
    b CommonExceptionEntry

KiInterrupt13:
    mov x9, #13
    b CommonExceptionEntry

KiInterrupt14:
    mov x9, #14
    b CommonExceptionEntry

KiInterrupt15:
    mov x9, #15
    b CommonExceptionEntry

KiInterrupt16:
    mov x9, #16
    b CommonExceptionEntry

KiInterrupt17:
    mov x9, #17
    b CommonExceptionEntry

KiInterrupt18:
    mov x9, #18
    b CommonExceptionEntry

KiInterrupt19:
    mov x9, #19
    b CommonExceptionEntry

KiInterrupt20:
    mov x9, #20
    b CommonExceptionEntry

KiInterrupt21:
    mov x9, #21
    b CommonExceptionEntry

KiInterrupt22:
    mov x9, #22
    b CommonExceptionEntry

KiInterrupt23:
    mov x9, #23
    b CommonExceptionEntry

KiInterrupt24:
    mov x9, #24
    b CommonExceptionEntry

KiInterrupt25:
    mov x9, #25
    b CommonExceptionEntry

KiInterrupt26:
    mov x9, #26
    b CommonExceptionEntry

KiInterrupt27:
    mov x9, #27
    b CommonExceptionEntry

KiInterrupt28:
    mov x9, #28
    b CommonExceptionEntry

KiInterrupt29:
    mov x9, #29
    b CommonExceptionEntry

KiInterrupt30:
    mov x9, #30
    b CommonExceptionEntry

KiInterrupt31:
    mov x9, #31
    b CommonExceptionEntry

KiInterrupt32:
    mov x9, #32
    b CommonExceptionEntry

KiInterrupt33:
    mov x9, #33
    b CommonExceptionEntry

KiInterrupt34:
    mov x9, #34
    b CommonExceptionEntry

KiInterrupt35:
    mov x9, #35
    b CommonExceptionEntry

KiInterrupt36:
    mov x9, #36
    b CommonExceptionEntry

KiInterrupt37:
    mov x9, #37
    b CommonExceptionEntry

KiInterrupt38:
    mov x9, #38
    b CommonExceptionEntry

KiInterrupt39:
    mov x9, #39
    b CommonExceptionEntry

KiInterrupt40:
    mov x9, #40
    b CommonExceptionEntry

    .global CommonExceptionEntry
CommonExceptionEntry:
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

    stp fp,  lr,  [sp, #29*8]

    str x9, [sp, #31*8]
    str x9, [sp, #32*8]
    str x9, [sp, #33*8]
    str x9, [sp, #34*8]

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

# void SwitchStackAndCall(uintptr_t newStack, void* parameter, void (*function)(void*))
.global SwitchStackAndCall
SwitchStackAndCall:
    and     x0, x0, #0xFFFFFFFFFFFFFFF0
    mov     sp, x0
    mov     x0, x1
    br      x2
.balign 0x800
.global ExceptionVectorTable

ExceptionVectorTable:
    .balign 0x80
    b KiInterrupt0
    .balign 0x80
    b KiInterrupt1
    .balign 0x80
    b KiInterrupt2
    .balign 0x80
    b KiInterrupt3
    .balign 0x80
    b KiInterrupt4
    .balign 0x80
    b KiInterrupt5
    .balign 0x80
    b KiInterrupt6
    .balign 0x80
    b KiInterrupt7
    .balign 0x80
    b KiInterrupt8
    .balign 0x80
    b KiInterrupt9
    .balign 0x80
    b KiInterrupt10
    .balign 0x80
    b KiInterrupt11
    .balign 0x80
    b KiInterrupt12
    .balign 0x80
    b KiInterrupt13
    .balign 0x80
    b KiInterrupt14
    .balign 0x80
    b KiInterrupt15
    .balign 0x80
    b KiInterrupt16
    .balign 0x80
    b KiInterrupt17
    .balign 0x80
    b KiInterrupt18
    .balign 0x80
    b KiInterrupt19
    .balign 0x80
    b KiInterrupt20
    .balign 0x80
    b KiInterrupt21
    .balign 0x80
    b KiInterrupt22
    .balign 0x80
    b KiInterrupt23
    .balign 0x80
    b KiInterrupt24
    .balign 0x80
    b KiInterrupt25
    .balign 0x80
    b KiInterrupt26
    .balign 0x80
    b KiInterrupt27
    .balign 0x80
    b KiInterrupt28
    .balign 0x80
    b KiInterrupt29
    .balign 0x80
    b KiInterrupt30
    .balign 0x80
    b KiInterrupt31
    .balign 0x80
    b KiInterrupt32
    .balign 0x80
    b KiInterrupt33
    .balign 0x80
    b KiInterrupt34
    .balign 0x80
    b KiInterrupt35
    .balign 0x80
    b KiInterrupt36
    .balign 0x80
    b KiInterrupt37
    .balign 0x80
    b KiInterrupt38
    .balign 0x80
    b KiInterrupt39
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
    .balign 0x80
    b KiInterrupt40
