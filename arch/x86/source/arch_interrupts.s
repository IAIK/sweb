

; ok, this is our main interrupt handling stuff
BITS 32

%define KERNEL_DS 0x10

%macro pushAll 0
        pushad
        push ds
        push es
%endmacro

%macro popAll 0
        pop es
        pop ds
        popad
%endmacro

%macro changeData 0
        push dword KERNEL_DS
        pop  es
        push dword KERNEL_DS
        pop  ds
%endmacro


extern arch_saveThreadRegisters

; ; ; 
; macros for irq and int handlers
; ; ;
;;; IRQ handlers
%macro irqhandler 1
global arch_irqHandler_%1
extern irqHandler_%1
arch_irqHandler_%1:
        pushAll
        changeData
        call arch_saveThreadRegisters
        call irqHandler_%1
        popAll
        iretd
%endmacro

%macro dummyhandler  1
global arch_dummyHandler_%1
extern dummyHandler_%1
arch_dummyHandler_%1:
        pushAll
        changeData
        call dummyHandler_%1
        popAll
        iretd
%endmacro

section .text

extern pageFaultHandler
global arch_pageFaultHandler
arch_pageFaultHandler:
        pushAll
        changeData
        call arch_saveThreadRegisters
        push ebp
        mov  ebp, esp
        sub  esp, 8
        mov  eax, dword[esp + 52] ; error cd
        mov  dword[esp + 4], eax
        mov  eax, cr2             ; page fault address
        mov  dword[esp + 0], eax
        call pageFaultHandler
        leave
        popAll
        add esp, 0x04             ; remove error_cd
        iretd


  irqhandler 0
  irqhandler 1
  irqhandler 2
  irqhandler 3
  irqhandler 4
  irqhandler 5
  irqhandler 6
  irqhandler 7
  irqhandler 8
  irqhandler 9
  irqhandler 10
  irqhandler 11
  irqhandler 12
  irqhandler 13
  irqhandler 14
  irqhandler 15
  
  irqhandler 65
  
dummyhandler 0
dummyhandler 1
dummyhandler 2
dummyhandler 3
dummyhandler 4
dummyhandler 5
dummyhandler 6
dummyhandler 7
dummyhandler 8
dummyhandler 9
dummyhandler 10
dummyhandler 11
dummyhandler 12
dummyhandler 13
dummyhandler 14
dummyhandler 15
dummyhandler 16
dummyhandler 17
dummyhandler 18
dummyhandler 19
dummyhandler 20
dummyhandler 21
dummyhandler 22
dummyhandler 23
dummyhandler 24
dummyhandler 25
dummyhandler 26
dummyhandler 27
dummyhandler 28
dummyhandler 29
dummyhandler 30
dummyhandler 31
dummyhandler 32
dummyhandler 33
dummyhandler 34
dummyhandler 35
dummyhandler 36
dummyhandler 37
dummyhandler 38
dummyhandler 39
dummyhandler 40
dummyhandler 41
dummyhandler 42
dummyhandler 43
dummyhandler 44
dummyhandler 45
dummyhandler 46
dummyhandler 47
dummyhandler 48
dummyhandler 49
dummyhandler 50
dummyhandler 51
dummyhandler 52
dummyhandler 53
dummyhandler 54
dummyhandler 55
dummyhandler 56
dummyhandler 57
dummyhandler 58
dummyhandler 59
dummyhandler 60
dummyhandler 61
dummyhandler 62
dummyhandler 63
dummyhandler 64
dummyhandler 65
dummyhandler 66
dummyhandler 67
dummyhandler 68
dummyhandler 69
dummyhandler 70
dummyhandler 71
dummyhandler 72
dummyhandler 73
dummyhandler 74
dummyhandler 75
dummyhandler 76
dummyhandler 77
dummyhandler 78
dummyhandler 79
dummyhandler 80
dummyhandler 81
dummyhandler 82
dummyhandler 83
dummyhandler 84
dummyhandler 85
dummyhandler 86
dummyhandler 87
dummyhandler 88
dummyhandler 89
dummyhandler 90
dummyhandler 91
dummyhandler 92
dummyhandler 93
dummyhandler 94
dummyhandler 95
dummyhandler 96
dummyhandler 97
dummyhandler 98
dummyhandler 99
dummyhandler 100
dummyhandler 101
dummyhandler 102
dummyhandler 103
dummyhandler 104
dummyhandler 105
dummyhandler 106
dummyhandler 107
dummyhandler 108
dummyhandler 109
dummyhandler 110
dummyhandler 111
dummyhandler 112
dummyhandler 113
dummyhandler 114
dummyhandler 115
dummyhandler 116
dummyhandler 117
dummyhandler 118
dummyhandler 119
dummyhandler 120
dummyhandler 121
dummyhandler 122
dummyhandler 123
dummyhandler 124
dummyhandler 125
dummyhandler 126
dummyhandler 127


global arch_syscallHandler
extern syscallHandler
arch_syscallHandler:
        pushAll
        changeData
		call arch_saveThreadRegisters
        call syscallHandler
        popAll
        iretd


dummyhandler 129
dummyhandler 130
dummyhandler 131
dummyhandler 132
dummyhandler 133
dummyhandler 134
dummyhandler 135
dummyhandler 136
dummyhandler 137
dummyhandler 138
dummyhandler 139
dummyhandler 140
dummyhandler 141
dummyhandler 142
dummyhandler 143
dummyhandler 144
dummyhandler 145
dummyhandler 146
dummyhandler 147
dummyhandler 148
dummyhandler 149
dummyhandler 150
dummyhandler 151
dummyhandler 152
dummyhandler 153
dummyhandler 154
dummyhandler 155
dummyhandler 156
dummyhandler 157
dummyhandler 158
dummyhandler 159
dummyhandler 160
dummyhandler 161
dummyhandler 162
dummyhandler 163
dummyhandler 164
dummyhandler 165
dummyhandler 166
dummyhandler 167
dummyhandler 168
dummyhandler 169
dummyhandler 170
dummyhandler 171
dummyhandler 172
dummyhandler 173
dummyhandler 174
dummyhandler 175
dummyhandler 176
dummyhandler 177
dummyhandler 178
dummyhandler 179
dummyhandler 180
dummyhandler 181
dummyhandler 182
dummyhandler 183
dummyhandler 184
dummyhandler 185
dummyhandler 186
dummyhandler 187
dummyhandler 188
dummyhandler 189
dummyhandler 190
dummyhandler 191
dummyhandler 192
dummyhandler 193
dummyhandler 194
dummyhandler 195
dummyhandler 196
dummyhandler 197
dummyhandler 198
dummyhandler 199
dummyhandler 200
dummyhandler 201
dummyhandler 202
dummyhandler 203
dummyhandler 204
dummyhandler 205
dummyhandler 206
dummyhandler 207
dummyhandler 208
dummyhandler 209
dummyhandler 210
dummyhandler 211
dummyhandler 212
dummyhandler 213
dummyhandler 214
dummyhandler 215
dummyhandler 216
dummyhandler 217
dummyhandler 218
dummyhandler 219
dummyhandler 220
dummyhandler 221
dummyhandler 222
dummyhandler 223
dummyhandler 224
dummyhandler 225
dummyhandler 226
dummyhandler 227
dummyhandler 228
dummyhandler 229
dummyhandler 230
dummyhandler 231
dummyhandler 232
dummyhandler 233
dummyhandler 234
dummyhandler 235
dummyhandler 236
dummyhandler 237
dummyhandler 238
dummyhandler 239
dummyhandler 240
dummyhandler 241
dummyhandler 242
dummyhandler 243
dummyhandler 244
dummyhandler 245
dummyhandler 246
dummyhandler 247
dummyhandler 248
dummyhandler 249
dummyhandler 250
dummyhandler 251
dummyhandler 252
dummyhandler 253
dummyhandler 254
dummyhandler 255
