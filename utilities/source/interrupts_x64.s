.global DummyInterruptHandler
DummyInterruptHandler:
    iretq

# void LoadTaskRegister(uint16_t selector)
.global LoadTaskRegister
LoadTaskRegister:
    ltr %cx
    ret
