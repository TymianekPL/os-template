.686P
.MODEL FLAT

.CODE

PUBLIC DummyInterruptHandler
DummyInterruptHandler PROC
    iret
DummyInterruptHandler ENDP

; void LoadTaskRegister(uint16_t selector)
PUBLIC _LoadTaskRegister
_LoadTaskRegister PROC
    mov ax, [esp+4]
    ltr ax
    ret
_LoadTaskRegister ENDP

END
