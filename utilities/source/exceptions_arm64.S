.balign 0x800
.global ExceptionVectorTable

ExceptionVectorTable:
    .balign 0x80
1:  b 1b
    .balign 0x80
2:  b 2b
    .balign 0x80
3:  b 3b
    .balign 0x80
4:  b 4b

    .balign 0x80
5:  b 5b
    .balign 0x80
6:  b 6b
    .balign 0x80
7:  b 7b
    .balign 0x80
8:  b 8b

    .balign 0x80
9:  b 9b
    .balign 0x80
10: b 10b
    .balign 0x80
11: b 11b
    .balign 0x80
12: b 12b

    .balign 0x80
13: b 13b
    .balign 0x80
14: b 14b
    .balign 0x80
15: b 15b
    .balign 0x80
16: b 16b
