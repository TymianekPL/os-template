.CODE

PUBLIC DummyInterruptHandler
DummyInterruptHandler PROC
    iretq
DummyInterruptHandler ENDP

; void LoadTaskRegister(uint16_t selector)
PUBLIC LoadTaskRegister
LoadTaskRegister PROC
    ltr cx
    ret
LoadTaskRegister ENDP

END
