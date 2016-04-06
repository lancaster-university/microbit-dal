; The MIT License (MIT)

; Copyright (c) 2016 British Broadcasting Corporation.
; This software is provided by Lancaster University by arrangement with the BBC.

; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following conditions:

; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.

; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
; THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.

   AREA asm_func, CODE, READONLY

; Export our context switching subroutine as a C function for use in mbed
    EXPORT swap_context
    EXPORT save_context
    EXPORT save_register_context
    EXPORT restore_register_context

    ALIGN

; R0 Contains a pointer to the TCB of the fibre being scheduled out.
; R1 Contains a pointer to the TCB of the fibre being scheduled in.
; R2 Contains a pointer to the base of the stack of the fibre being scheduled out.
; R3 Contains a pointer to the base of the stack of the fibre being scheduled in.

swap_context

    ; Write our core registers into the TCB
    ; First, store the general registers

    ; Skip this is we're given a NULL parameter for the TCB
    CMP     R0, #0
    BEQ     store_context_complete

    STR     R0, [R0,#0]
    STR     R1, [R0,#4]
    STR     R2, [R0,#8]
    STR     R3, [R0,#12]
    STR     R4, [R0,#16]
    STR     R5, [R0,#20]
    STR     R6, [R0,#24]
    STR     R7, [R0,#28]

    ; Now the high general purpose registers
    MOV     R4, R8
    STR     R4, [R0,#32]
    MOV     R4, R9
    STR     R4, [R0,#36]
    MOV     R4, R10
    STR     R4, [R0,#40]
    MOV     R4, R11
    STR     R4, [R0,#44]
    MOV     R4, R12
    STR     R4, [R0,#48]

    ; Now the Stack and Link Register.
    ; As this context is only intended for use with a fiber scheduler,
    ; we don't need the PC.
    MOV     R6, SP
    STR     R6, [R0,#52]
    MOV     R4, LR
    STR     R4, [R0,#56]

store_context_complete
    ; Finally, Copy the stack. We do this to reduce RAM footprint, as stack is usually very small at the point
    ; of scheduling, but we need a lot of capacity for interrupt handling and other functions.

    ; Skip this is we're given a NULL parameter for the stack.
    CMP     R2, #0
    BEQ     store_stack_complete

    LDR     R4, [R0,#60]         ; Load R4 with the fiber's defined stack_base.
store_stack
    SUBS    R4, #4
    SUBS    R2, #4

    LDR     R5, [R4]
    STR     R5, [R2]

    CMP     R4, R6
    BNE     store_stack

store_stack_complete

    ;
    ; Now page in the new context.
    ; Update all registers except the PC. We can also safely ignore the STATUS register, as we're just a fiber scheduler.
    ;
    LDR     R4, [R1, #56]
    MOV     LR, R4
    LDR     R6, [R1, #52]
    MOV     SP, R6

    ; Copy the stack in.
    ; n.b. we do this after setting the SP to make comparisons easier.

    ; Skip this is we're given a NULL parameter for the stack.
    CMP     R3, #0
    BEQ     restore_stack_complete

    LDR     R4, [R1,#60]         ; Load R4 with the fiber's defined stack_base.

restore_stack
    SUBS    R4, #4
    SUBS    R3, #4

    LDR     R5, [R3]
    STR     R5, [R4]

    CMP     R4, R6
    BNE     restore_stack

restore_stack_complete
    LDR     R4, [R1, #48]
    MOV     R12, R4
    LDR     R4, [R1, #44]
    MOV     R11, R4
    LDR     R4, [R1, #40]
    MOV     R10, R4
    LDR     R4, [R1, #36]
    MOV     R9, R4
    LDR     R4, [R1, #32]
    MOV     R8, R4

    LDR     R7, [R1, #28]
    LDR     R6, [R1, #24]
    LDR     R5, [R1, #20]
    LDR     R4, [R1, #16]
    LDR     R3, [R1, #12]
    LDR     R2, [R1, #8]
    LDR     R0, [R1, #0]
    LDR     R1, [R1, #4]

    ; Return to caller (scheduler).
    BX      LR


; R0 Contains a pointer to the TCB of the fibre to snapshot
; R1 Contains a pointer to the base of the stack of the fibre being snapshotted

save_context

    ; Write our core registers into the TCB
    ; First, store the general registers

    STR     R0, [R0,#0]
    STR     R1, [R0,#4]
    STR     R2, [R0,#8]
    STR     R3, [R0,#12]
    STR     R4, [R0,#16]
    STR     R5, [R0,#20]
    STR     R6, [R0,#24]
    STR     R7, [R0,#28]

    ; Now the high general purpose registers
    MOV     R4, R8
    STR     R4, [R0,#32]
    MOV     R4, R9
    STR     R4, [R0,#36]
    MOV     R4, R10
    STR     R4, [R0,#40]
    MOV     R4, R11
    STR     R4, [R0,#44]
    MOV     R4, R12
    STR     R4, [R0,#48]

    ; Now the Stack and Link Register.
    ; As this context is only intended for use with a fiber scheduler,
    ; we don't need the PC.
    MOV     R6, SP
    STR     R6, [R0,#52]
    MOV     R4, LR
    STR     R4, [R0,#56]

    ; Finally, Copy the stack. We do this to reduce RAM footprint, as stackis usually very small at the point
    ; of sceduling, but we need a lot of capacity for interrupt handling and other functions.

    LDR     R4, [R0,#60]         ; Load R4 with the fiber's defined stack_base.

store_stack1
    SUBS    R4, #4
    SUBS    R1, #4

    LDR     R5, [R4]
    STR     R5, [R1]

    CMP     R4, R6
    BNE     store_stack1

    ; Restore scratch registers.

    LDR     R7, [R0, #28]
    LDR     R6, [R0, #24]
    LDR     R5, [R0, #20]
    LDR     R4, [R0, #16]

    ; Return to caller (scheduler).
    BX      LR


; R0 Contains a pointer to the TCB of the fiber to snapshot
save_register_context

    ; Write our core registers into the TCB
    ; First, store the general registers

    STR     R0, [R0,#0]
    STR     R1, [R0,#4]
    STR     R2, [R0,#8]
    STR     R3, [R0,#12]
    STR     R4, [R0,#16]
    STR     R5, [R0,#20]
    STR     R6, [R0,#24]
    STR     R7, [R0,#28]

    ; Now the high general purpose registers
    MOV     R4, R8
    STR     R4, [R0,#32]
    MOV     R4, R9
    STR     R4, [R0,#36]
    MOV     R4, R10
    STR     R4, [R0,#40]
    MOV     R4, R11
    STR     R4, [R0,#44]
    MOV     R4, R12
    STR     R4, [R0,#48]

    ; Now the Stack Pointer and Link Register.
    ; As this context is only intended for use with a fiber scheduler,
    ; we don't need the PC.
    MOV     R4, SP
    STR     R4, [R0,#52]
    MOV     R4, LR
    STR     R4, [R0,#56]

    ; Restore scratch registers.
    LDR     R4, [R0, #16]

    ; Return to caller (scheduler).
    BX      LR


restore_register_context

    ;
    ; Now page in the new context.
    ; Update all registers except the PC. We can also safely ignore the STATUS register, as we're just a fiber scheduler.
    ;
    LDR     R4, [R0, #56]
    MOV     LR, R4
    LDR     R4, [R0, #52]
    MOV     SP, R4

    ; High registers...
    LDR     R4, [R0, #48]
    MOV     R12, R4
    LDR     R4, [R0, #44]
    MOV     R11, R4
    LDR     R4, [R0, #40]
    MOV     R10, R4
    LDR     R4, [R0, #36]
    MOV     R9, R4
    LDR     R4, [R0, #32]
    MOV     R8, R4

    ; Low registers...
    LDR     R7, [R0, #28]
    LDR     R6, [R0, #24]
    LDR     R5, [R0, #20]
    LDR     R4, [R0, #16]
    LDR     R3, [R0, #12]
    LDR     R2, [R0, #8]
    LDR     R0, [R0, #0]
    LDR     R1, [R0, #4]

    ; Return to caller (normally the scheduler).
    BX      LR

    ALIGN
    END
