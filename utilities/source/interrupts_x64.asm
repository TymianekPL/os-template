.CODE

PUSH_FRAME MACRO
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
ENDM

POP_FRAME MACRO
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ENDM

EXTERN KeHandleInterruptFrame:PROC

DEFINE_ISR MACRO name, num, hasError
    name PROC
        IF hasError EQ 0
            push 0
        ENDIF
        push num

        PUSH_FRAME

        mov rcx, rsp
        call KeHandleInterruptFrame
        mov rsp, rax

        POP_FRAME

        add rsp, 16
        iretq
    name ENDP
ENDM
DEFINE_ISR InterruptStub0, 0, 0
DEFINE_ISR InterruptStub1, 1, 0
DEFINE_ISR InterruptStub2, 2, 0
DEFINE_ISR InterruptStub3, 3, 0
DEFINE_ISR InterruptStub4, 4, 0
DEFINE_ISR InterruptStub5, 5, 0
DEFINE_ISR InterruptStub6, 6, 0
DEFINE_ISR InterruptStub7, 7, 0
DEFINE_ISR InterruptStub8, 8, 1
DEFINE_ISR InterruptStub9, 9, 0
DEFINE_ISR InterruptStub10, 10, 1
DEFINE_ISR InterruptStub11, 11, 1
DEFINE_ISR InterruptStub12, 12, 1
DEFINE_ISR InterruptStub13, 13, 1
DEFINE_ISR InterruptStub14, 14, 1
DEFINE_ISR InterruptStub15, 15, 0
DEFINE_ISR InterruptStub16, 16, 0
DEFINE_ISR InterruptStub17, 17, 0
DEFINE_ISR InterruptStub18, 18, 0
DEFINE_ISR InterruptStub19, 19, 0
DEFINE_ISR InterruptStub20, 20, 0
DEFINE_ISR InterruptStub21, 21, 0
DEFINE_ISR InterruptStub22, 22, 0
DEFINE_ISR InterruptStub23, 23, 0
DEFINE_ISR InterruptStub24, 24, 0
DEFINE_ISR InterruptStub25, 25, 0
DEFINE_ISR InterruptStub26, 26, 0
DEFINE_ISR InterruptStub27, 27, 0
DEFINE_ISR InterruptStub28, 28, 0
DEFINE_ISR InterruptStub29, 29, 0
DEFINE_ISR InterruptStub30, 30, 0
DEFINE_ISR InterruptStub31, 31, 0
DEFINE_ISR InterruptStub32, 32, 0
DEFINE_ISR InterruptStub33, 33, 0
DEFINE_ISR InterruptStub34, 34, 0
DEFINE_ISR InterruptStub35, 35, 0
DEFINE_ISR InterruptStub36, 36, 0
DEFINE_ISR InterruptStub37, 37, 0
DEFINE_ISR InterruptStub38, 38, 0
DEFINE_ISR InterruptStub39, 39, 0
DEFINE_ISR InterruptStub40, 40, 0
DEFINE_ISR InterruptStub41, 41, 0
DEFINE_ISR InterruptStub42, 42, 0
DEFINE_ISR InterruptStub43, 43, 0
DEFINE_ISR InterruptStub44, 44, 0
DEFINE_ISR InterruptStub45, 45, 0
DEFINE_ISR InterruptStub46, 46, 0
DEFINE_ISR InterruptStub47, 47, 0
DEFINE_ISR InterruptStub48, 48, 0
DEFINE_ISR InterruptStub49, 49, 0
DEFINE_ISR InterruptStub50, 50, 0
DEFINE_ISR InterruptStub51, 51, 0
DEFINE_ISR InterruptStub52, 52, 0
DEFINE_ISR InterruptStub53, 53, 0
DEFINE_ISR InterruptStub54, 54, 0
DEFINE_ISR InterruptStub55, 55, 0
DEFINE_ISR InterruptStub56, 56, 0
DEFINE_ISR InterruptStub57, 57, 0
DEFINE_ISR InterruptStub58, 58, 0
DEFINE_ISR InterruptStub59, 59, 0
DEFINE_ISR InterruptStub60, 60, 0
DEFINE_ISR InterruptStub61, 61, 0
DEFINE_ISR InterruptStub62, 62, 0
DEFINE_ISR InterruptStub63, 63, 0
DEFINE_ISR InterruptStub64, 64, 0
DEFINE_ISR InterruptStub65, 65, 0
DEFINE_ISR InterruptStub66, 66, 0
DEFINE_ISR InterruptStub67, 67, 0
DEFINE_ISR InterruptStub68, 68, 0
DEFINE_ISR InterruptStub69, 69, 0
DEFINE_ISR InterruptStub70, 70, 0
DEFINE_ISR InterruptStub71, 71, 0
DEFINE_ISR InterruptStub72, 72, 0
DEFINE_ISR InterruptStub73, 73, 0
DEFINE_ISR InterruptStub74, 74, 0
DEFINE_ISR InterruptStub75, 75, 0
DEFINE_ISR InterruptStub76, 76, 0
DEFINE_ISR InterruptStub77, 77, 0
DEFINE_ISR InterruptStub78, 78, 0
DEFINE_ISR InterruptStub79, 79, 0
DEFINE_ISR InterruptStub80, 80, 0
DEFINE_ISR InterruptStub81, 81, 0
DEFINE_ISR InterruptStub82, 82, 0
DEFINE_ISR InterruptStub83, 83, 0
DEFINE_ISR InterruptStub84, 84, 0
DEFINE_ISR InterruptStub85, 85, 0
DEFINE_ISR InterruptStub86, 86, 0
DEFINE_ISR InterruptStub87, 87, 0
DEFINE_ISR InterruptStub88, 88, 0
DEFINE_ISR InterruptStub89, 89, 0
DEFINE_ISR InterruptStub90, 90, 0
DEFINE_ISR InterruptStub91, 91, 0
DEFINE_ISR InterruptStub92, 92, 0
DEFINE_ISR InterruptStub93, 93, 0
DEFINE_ISR InterruptStub94, 94, 0
DEFINE_ISR InterruptStub95, 95, 0
DEFINE_ISR InterruptStub96, 96, 0
DEFINE_ISR InterruptStub97, 97, 0
DEFINE_ISR InterruptStub98, 98, 0
DEFINE_ISR InterruptStub99, 99, 0
DEFINE_ISR InterruptStub100, 100, 0
DEFINE_ISR InterruptStub101, 101, 0
DEFINE_ISR InterruptStub102, 102, 0
DEFINE_ISR InterruptStub103, 103, 0
DEFINE_ISR InterruptStub104, 104, 0
DEFINE_ISR InterruptStub105, 105, 0
DEFINE_ISR InterruptStub106, 106, 0
DEFINE_ISR InterruptStub107, 107, 0
DEFINE_ISR InterruptStub108, 108, 0
DEFINE_ISR InterruptStub109, 109, 0
DEFINE_ISR InterruptStub110, 110, 0
DEFINE_ISR InterruptStub111, 111, 0
DEFINE_ISR InterruptStub112, 112, 0
DEFINE_ISR InterruptStub113, 113, 0
DEFINE_ISR InterruptStub114, 114, 0
DEFINE_ISR InterruptStub115, 115, 0
DEFINE_ISR InterruptStub116, 116, 0
DEFINE_ISR InterruptStub117, 117, 0
DEFINE_ISR InterruptStub118, 118, 0
DEFINE_ISR InterruptStub119, 119, 0
DEFINE_ISR InterruptStub120, 120, 0
DEFINE_ISR InterruptStub121, 121, 0
DEFINE_ISR InterruptStub122, 122, 0
DEFINE_ISR InterruptStub123, 123, 0
DEFINE_ISR InterruptStub124, 124, 0
DEFINE_ISR InterruptStub125, 125, 0
DEFINE_ISR InterruptStub126, 126, 0
DEFINE_ISR InterruptStub127, 127, 0
DEFINE_ISR InterruptStub128, 128, 0
DEFINE_ISR InterruptStub129, 129, 0
DEFINE_ISR InterruptStub130, 130, 0
DEFINE_ISR InterruptStub131, 131, 0
DEFINE_ISR InterruptStub132, 132, 0
DEFINE_ISR InterruptStub133, 133, 0
DEFINE_ISR InterruptStub134, 134, 0
DEFINE_ISR InterruptStub135, 135, 0
DEFINE_ISR InterruptStub136, 136, 0
DEFINE_ISR InterruptStub137, 137, 0
DEFINE_ISR InterruptStub138, 138, 0
DEFINE_ISR InterruptStub139, 139, 0
DEFINE_ISR InterruptStub140, 140, 0
DEFINE_ISR InterruptStub141, 141, 0
DEFINE_ISR InterruptStub142, 142, 0
DEFINE_ISR InterruptStub143, 143, 0
DEFINE_ISR InterruptStub144, 144, 0
DEFINE_ISR InterruptStub145, 145, 0
DEFINE_ISR InterruptStub146, 146, 0
DEFINE_ISR InterruptStub147, 147, 0
DEFINE_ISR InterruptStub148, 148, 0
DEFINE_ISR InterruptStub149, 149, 0
DEFINE_ISR InterruptStub150, 150, 0
DEFINE_ISR InterruptStub151, 151, 0
DEFINE_ISR InterruptStub152, 152, 0
DEFINE_ISR InterruptStub153, 153, 0
DEFINE_ISR InterruptStub154, 154, 0
DEFINE_ISR InterruptStub155, 155, 0
DEFINE_ISR InterruptStub156, 156, 0
DEFINE_ISR InterruptStub157, 157, 0
DEFINE_ISR InterruptStub158, 158, 0
DEFINE_ISR InterruptStub159, 159, 0
DEFINE_ISR InterruptStub160, 160, 0
DEFINE_ISR InterruptStub161, 161, 0
DEFINE_ISR InterruptStub162, 162, 0
DEFINE_ISR InterruptStub163, 163, 0
DEFINE_ISR InterruptStub164, 164, 0
DEFINE_ISR InterruptStub165, 165, 0
DEFINE_ISR InterruptStub166, 166, 0
DEFINE_ISR InterruptStub167, 167, 0
DEFINE_ISR InterruptStub168, 168, 0
DEFINE_ISR InterruptStub169, 169, 0
DEFINE_ISR InterruptStub170, 170, 0
DEFINE_ISR InterruptStub171, 171, 0
DEFINE_ISR InterruptStub172, 172, 0
DEFINE_ISR InterruptStub173, 173, 0
DEFINE_ISR InterruptStub174, 174, 0
DEFINE_ISR InterruptStub175, 175, 0
DEFINE_ISR InterruptStub176, 176, 0
DEFINE_ISR InterruptStub177, 177, 0
DEFINE_ISR InterruptStub178, 178, 0
DEFINE_ISR InterruptStub179, 179, 0
DEFINE_ISR InterruptStub180, 180, 0
DEFINE_ISR InterruptStub181, 181, 0
DEFINE_ISR InterruptStub182, 182, 0
DEFINE_ISR InterruptStub183, 183, 0
DEFINE_ISR InterruptStub184, 184, 0
DEFINE_ISR InterruptStub185, 185, 0
DEFINE_ISR InterruptStub186, 186, 0
DEFINE_ISR InterruptStub187, 187, 0
DEFINE_ISR InterruptStub188, 188, 0
DEFINE_ISR InterruptStub189, 189, 0
DEFINE_ISR InterruptStub190, 190, 0
DEFINE_ISR InterruptStub191, 191, 0
DEFINE_ISR InterruptStub192, 192, 0
DEFINE_ISR InterruptStub193, 193, 0
DEFINE_ISR InterruptStub194, 194, 0
DEFINE_ISR InterruptStub195, 195, 0
DEFINE_ISR InterruptStub196, 196, 0
DEFINE_ISR InterruptStub197, 197, 0
DEFINE_ISR InterruptStub198, 198, 0
DEFINE_ISR InterruptStub199, 199, 0
DEFINE_ISR InterruptStub200, 200, 0
DEFINE_ISR InterruptStub201, 201, 0
DEFINE_ISR InterruptStub202, 202, 0
DEFINE_ISR InterruptStub203, 203, 0
DEFINE_ISR InterruptStub204, 204, 0
DEFINE_ISR InterruptStub205, 205, 0
DEFINE_ISR InterruptStub206, 206, 0
DEFINE_ISR InterruptStub207, 207, 0
DEFINE_ISR InterruptStub208, 208, 0
DEFINE_ISR InterruptStub209, 209, 0
DEFINE_ISR InterruptStub210, 210, 0
DEFINE_ISR InterruptStub211, 211, 0
DEFINE_ISR InterruptStub212, 212, 0
DEFINE_ISR InterruptStub213, 213, 0
DEFINE_ISR InterruptStub214, 214, 0
DEFINE_ISR InterruptStub215, 215, 0
DEFINE_ISR InterruptStub216, 216, 0
DEFINE_ISR InterruptStub217, 217, 0
DEFINE_ISR InterruptStub218, 218, 0
DEFINE_ISR InterruptStub219, 219, 0
DEFINE_ISR InterruptStub220, 220, 0
DEFINE_ISR InterruptStub221, 221, 0
DEFINE_ISR InterruptStub222, 222, 0
DEFINE_ISR InterruptStub223, 223, 0
DEFINE_ISR InterruptStub224, 224, 0
DEFINE_ISR InterruptStub225, 225, 0
DEFINE_ISR InterruptStub226, 226, 0
DEFINE_ISR InterruptStub227, 227, 0
DEFINE_ISR InterruptStub228, 228, 0
DEFINE_ISR InterruptStub229, 229, 0
DEFINE_ISR InterruptStub230, 230, 0
DEFINE_ISR InterruptStub231, 231, 0
DEFINE_ISR InterruptStub232, 232, 0
DEFINE_ISR InterruptStub233, 233, 0
DEFINE_ISR InterruptStub234, 234, 0
DEFINE_ISR InterruptStub235, 235, 0
DEFINE_ISR InterruptStub236, 236, 0
DEFINE_ISR InterruptStub237, 237, 0
DEFINE_ISR InterruptStub238, 238, 0
DEFINE_ISR InterruptStub239, 239, 0
DEFINE_ISR InterruptStub240, 240, 0
DEFINE_ISR InterruptStub241, 241, 0
DEFINE_ISR InterruptStub242, 242, 0
DEFINE_ISR InterruptStub243, 243, 0
DEFINE_ISR InterruptStub244, 244, 0
DEFINE_ISR InterruptStub245, 245, 0
DEFINE_ISR InterruptStub246, 246, 0
DEFINE_ISR InterruptStub247, 247, 0
DEFINE_ISR InterruptStub248, 248, 0
DEFINE_ISR InterruptStub249, 249, 0
DEFINE_ISR InterruptStub250, 250, 0
DEFINE_ISR InterruptStub251, 251, 0
DEFINE_ISR InterruptStub252, 252, 0
DEFINE_ISR InterruptStub253, 253, 0
DEFINE_ISR InterruptStub254, 254, 0
DEFINE_ISR InterruptStub255, 255, 0

ReloadCS SEGMENT
ReloadCSCode PROC
    push 08h
    lea rax, NextInstr
    push rax
    db 48h, 0CBh
ReloadCSCode ENDP

NextInstr:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    ret
ReloadCS ENDS

.data
PUBLIC KiInterruptVectorTable
ALIGN 8
KiInterruptVectorTable:
    dq OFFSET InterruptStub0
    dq OFFSET InterruptStub1
    dq OFFSET InterruptStub2
    dq OFFSET InterruptStub3
    dq OFFSET InterruptStub4
    dq OFFSET InterruptStub5
    dq OFFSET InterruptStub6
    dq OFFSET InterruptStub7
    dq OFFSET InterruptStub8
    dq OFFSET InterruptStub9
    dq OFFSET InterruptStub10
    dq OFFSET InterruptStub11
    dq OFFSET InterruptStub12
    dq OFFSET InterruptStub13
    dq OFFSET InterruptStub14
    dq OFFSET InterruptStub15
    dq OFFSET InterruptStub16
    dq OFFSET InterruptStub17
    dq OFFSET InterruptStub18
    dq OFFSET InterruptStub19
    dq OFFSET InterruptStub20
    dq OFFSET InterruptStub21
    dq OFFSET InterruptStub22
    dq OFFSET InterruptStub23
    dq OFFSET InterruptStub24
    dq OFFSET InterruptStub25
    dq OFFSET InterruptStub26
    dq OFFSET InterruptStub27
    dq OFFSET InterruptStub28
    dq OFFSET InterruptStub29
    dq OFFSET InterruptStub30
    dq OFFSET InterruptStub31
    dq OFFSET InterruptStub32
    dq OFFSET InterruptStub33
    dq OFFSET InterruptStub34
    dq OFFSET InterruptStub35
    dq OFFSET InterruptStub36
    dq OFFSET InterruptStub37
    dq OFFSET InterruptStub38
    dq OFFSET InterruptStub39
    dq OFFSET InterruptStub40
    dq OFFSET InterruptStub41
    dq OFFSET InterruptStub42
    dq OFFSET InterruptStub43
    dq OFFSET InterruptStub44
    dq OFFSET InterruptStub45
    dq OFFSET InterruptStub46
    dq OFFSET InterruptStub47
    dq OFFSET InterruptStub48
    dq OFFSET InterruptStub49
    dq OFFSET InterruptStub50
    dq OFFSET InterruptStub51
    dq OFFSET InterruptStub52
    dq OFFSET InterruptStub53
    dq OFFSET InterruptStub54
    dq OFFSET InterruptStub55
    dq OFFSET InterruptStub56
    dq OFFSET InterruptStub57
    dq OFFSET InterruptStub58
    dq OFFSET InterruptStub59
    dq OFFSET InterruptStub60
    dq OFFSET InterruptStub61
    dq OFFSET InterruptStub62
    dq OFFSET InterruptStub63
    dq OFFSET InterruptStub64
    dq OFFSET InterruptStub65
    dq OFFSET InterruptStub66
    dq OFFSET InterruptStub67
    dq OFFSET InterruptStub68
    dq OFFSET InterruptStub69
    dq OFFSET InterruptStub70
    dq OFFSET InterruptStub71
    dq OFFSET InterruptStub72
    dq OFFSET InterruptStub73
    dq OFFSET InterruptStub74
    dq OFFSET InterruptStub75
    dq OFFSET InterruptStub76
    dq OFFSET InterruptStub77
    dq OFFSET InterruptStub78
    dq OFFSET InterruptStub79
    dq OFFSET InterruptStub80
    dq OFFSET InterruptStub81
    dq OFFSET InterruptStub82
    dq OFFSET InterruptStub83
    dq OFFSET InterruptStub84
    dq OFFSET InterruptStub85
    dq OFFSET InterruptStub86
    dq OFFSET InterruptStub87
    dq OFFSET InterruptStub88
    dq OFFSET InterruptStub89
    dq OFFSET InterruptStub90
    dq OFFSET InterruptStub91
    dq OFFSET InterruptStub92
    dq OFFSET InterruptStub93
    dq OFFSET InterruptStub94
    dq OFFSET InterruptStub95
    dq OFFSET InterruptStub96
    dq OFFSET InterruptStub97
    dq OFFSET InterruptStub98
    dq OFFSET InterruptStub99
    dq OFFSET InterruptStub100
    dq OFFSET InterruptStub101
    dq OFFSET InterruptStub102
    dq OFFSET InterruptStub103
    dq OFFSET InterruptStub104
    dq OFFSET InterruptStub105
    dq OFFSET InterruptStub106
    dq OFFSET InterruptStub107
    dq OFFSET InterruptStub108
    dq OFFSET InterruptStub109
    dq OFFSET InterruptStub110
    dq OFFSET InterruptStub111
    dq OFFSET InterruptStub112
    dq OFFSET InterruptStub113
    dq OFFSET InterruptStub114
    dq OFFSET InterruptStub115
    dq OFFSET InterruptStub116
    dq OFFSET InterruptStub117
    dq OFFSET InterruptStub118
    dq OFFSET InterruptStub119
    dq OFFSET InterruptStub120
    dq OFFSET InterruptStub121
    dq OFFSET InterruptStub122
    dq OFFSET InterruptStub123
    dq OFFSET InterruptStub124
    dq OFFSET InterruptStub125
    dq OFFSET InterruptStub126
    dq OFFSET InterruptStub127
    dq OFFSET InterruptStub128
    dq OFFSET InterruptStub129
    dq OFFSET InterruptStub130
    dq OFFSET InterruptStub131
    dq OFFSET InterruptStub132
    dq OFFSET InterruptStub133
    dq OFFSET InterruptStub134
    dq OFFSET InterruptStub135
    dq OFFSET InterruptStub136
    dq OFFSET InterruptStub137
    dq OFFSET InterruptStub138
    dq OFFSET InterruptStub139
    dq OFFSET InterruptStub140
    dq OFFSET InterruptStub141
    dq OFFSET InterruptStub142
    dq OFFSET InterruptStub143
    dq OFFSET InterruptStub144
    dq OFFSET InterruptStub145
    dq OFFSET InterruptStub146
    dq OFFSET InterruptStub147
    dq OFFSET InterruptStub148
    dq OFFSET InterruptStub149
    dq OFFSET InterruptStub150
    dq OFFSET InterruptStub151
    dq OFFSET InterruptStub152
    dq OFFSET InterruptStub153
    dq OFFSET InterruptStub154
    dq OFFSET InterruptStub155
    dq OFFSET InterruptStub156
    dq OFFSET InterruptStub157
    dq OFFSET InterruptStub158
    dq OFFSET InterruptStub159
    dq OFFSET InterruptStub160
    dq OFFSET InterruptStub161
    dq OFFSET InterruptStub162
    dq OFFSET InterruptStub163
    dq OFFSET InterruptStub164
    dq OFFSET InterruptStub165
    dq OFFSET InterruptStub166
    dq OFFSET InterruptStub167
    dq OFFSET InterruptStub168
    dq OFFSET InterruptStub169
    dq OFFSET InterruptStub170
    dq OFFSET InterruptStub171
    dq OFFSET InterruptStub172
    dq OFFSET InterruptStub173
    dq OFFSET InterruptStub174
    dq OFFSET InterruptStub175
    dq OFFSET InterruptStub176
    dq OFFSET InterruptStub177
    dq OFFSET InterruptStub178
    dq OFFSET InterruptStub179
    dq OFFSET InterruptStub180
    dq OFFSET InterruptStub181
    dq OFFSET InterruptStub182
    dq OFFSET InterruptStub183
    dq OFFSET InterruptStub184
    dq OFFSET InterruptStub185
    dq OFFSET InterruptStub186
    dq OFFSET InterruptStub187
    dq OFFSET InterruptStub188
    dq OFFSET InterruptStub189
    dq OFFSET InterruptStub190
    dq OFFSET InterruptStub191
    dq OFFSET InterruptStub192
    dq OFFSET InterruptStub193
    dq OFFSET InterruptStub194
    dq OFFSET InterruptStub195
    dq OFFSET InterruptStub196
    dq OFFSET InterruptStub197
    dq OFFSET InterruptStub198
    dq OFFSET InterruptStub199
    dq OFFSET InterruptStub200
    dq OFFSET InterruptStub201
    dq OFFSET InterruptStub202
    dq OFFSET InterruptStub203
    dq OFFSET InterruptStub204
    dq OFFSET InterruptStub205
    dq OFFSET InterruptStub206
    dq OFFSET InterruptStub207
    dq OFFSET InterruptStub208
    dq OFFSET InterruptStub209
    dq OFFSET InterruptStub210
    dq OFFSET InterruptStub211
    dq OFFSET InterruptStub212
    dq OFFSET InterruptStub213
    dq OFFSET InterruptStub214
    dq OFFSET InterruptStub215
    dq OFFSET InterruptStub216
    dq OFFSET InterruptStub217
    dq OFFSET InterruptStub218
    dq OFFSET InterruptStub219
    dq OFFSET InterruptStub220
    dq OFFSET InterruptStub221
    dq OFFSET InterruptStub222
    dq OFFSET InterruptStub223
    dq OFFSET InterruptStub224
    dq OFFSET InterruptStub225
    dq OFFSET InterruptStub226
    dq OFFSET InterruptStub227
    dq OFFSET InterruptStub228
    dq OFFSET InterruptStub229
    dq OFFSET InterruptStub230
    dq OFFSET InterruptStub231
    dq OFFSET InterruptStub232
    dq OFFSET InterruptStub233
    dq OFFSET InterruptStub234
    dq OFFSET InterruptStub235
    dq OFFSET InterruptStub236
    dq OFFSET InterruptStub237
    dq OFFSET InterruptStub238
    dq OFFSET InterruptStub239
    dq OFFSET InterruptStub240
    dq OFFSET InterruptStub241
    dq OFFSET InterruptStub242
    dq OFFSET InterruptStub243
    dq OFFSET InterruptStub244
    dq OFFSET InterruptStub245
    dq OFFSET InterruptStub246
    dq OFFSET InterruptStub247
    dq OFFSET InterruptStub248
    dq OFFSET InterruptStub249
    dq OFFSET InterruptStub250
    dq OFFSET InterruptStub251
    dq OFFSET InterruptStub252
    dq OFFSET InterruptStub253
    dq OFFSET InterruptStub254
    dq OFFSET InterruptStub255

; void LoadTaskRegister(uint16_t selector)
PUBLIC LoadTaskRegister
LoadTaskRegister PROC
    ltr cx
    ret
LoadTaskRegister ENDP

; void SwitchStackAndCall(uintptr_t newStack, void* parameter, void (*function)(void*))
PUBLIC SwitchStackAndCall
SwitchStackAndCall PROC
    and rcx, -16
    mov rsp, rcx
    sub rsp, 32
    mov rcx, rdx
    call r8
    int 3
SwitchStackAndCall ENDP

END
