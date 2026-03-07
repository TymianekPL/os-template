    AREA |.text|, CODE, READONLY, ALIGN=11

    EXPORT ExceptionVectorTable

ExceptionVectorTable
    ALIGN 128
Vector0
    B Vector0
    ALIGN 128
Vector1
    B Vector1
    ALIGN 128
Vector2
    B Vector2
    ALIGN 128
Vector3
    B Vector3

    ALIGN 128
Vector4
    B Vector4
    ALIGN 128
Vector5
    B Vector5
    ALIGN 128
Vector6
    B Vector6
    ALIGN 128
Vector7
    B Vector7

    ALIGN 128
Vector8
    B Vector8
    ALIGN 128
Vector9
    B Vector9
    ALIGN 128
Vector10
    B Vector10
    ALIGN 128
Vector11
    B Vector11

    ALIGN 128
Vector12
    B Vector12
    ALIGN 128
Vector13
    B Vector13
    ALIGN 128
Vector14
    B Vector14
    ALIGN 128
Vector15
    B Vector15


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
    END
