;********************************************************************************
; 文件名: os_cpu.s
; 描述: MH开发板 RTOS 上下文切换核心汇编 (Cortex-M4)
;********************************************************************************

    AREA    |.text|, CODE, READONLY ; AREA: 定义一个段 (这里是代码段)

    PRESERVE8                       ; 栈8字节对齐
    THUMB                           ; THUMB: 使用 Thumb 指令集 (ARM Cortex-M 必须)

    IMPORT  current_tcb             ; IMPORT: 引入外部变量 (相当于C语言的 extern)
    IMPORT  next_tcb
    IMPORT  switch_context_logic

    EXPORT  PendSV_Handler          ; EXPORT: 导出函数供外部调用
    EXPORT  os_start

;========================================================================
; 函数: os_start
; 描述: 启动第一个任务 (从 Main -> Task)
;========================================================================
os_start
    ; [指令释义] CPSID: Change Processor State, Disable Interrupts (关中断)
    CPSID   I                       ; 1. 关中断

    ; --- 获取第一个任务的栈指针 ---
    ; [指令释义] LDR: Load Register (从内存读取数据 -> 寄存器)
    LDR     R0, =current_tcb        ; R0 = &current_tcb
    LDR     R1, [R0]                ; R1 = current_tcb
    LDR     R0, [R1]                ; R0 = current_tcb->stack_ptr

    ; --- 恢复软件保存的寄存器 ---
    ; [指令释义] LDMIA: Load Multiple Increment After (批量读取/出栈)
    LDMIA   R0!, {R4-R11}           ; 弹出 R4-R11，R0指向硬件帧

    ; --- 切换到 PSP ---
    ; [指令释义] MSR: Move to Special Register (通用寄存器 -> 特殊寄存器)
    MSR     PSP, R0                 ; 设置 PSP

    ; [指令释义] MOV: Move (数据赋值/搬运)
    MOV     R1, #2
    MSR     CONTROL, R1             ; 切换到 PSP 模式

    ; [指令释义] ISB: Instruction Synchronization Barrier (指令同步隔离/清洗流水线)
    ISB                             ; 指令同步隔离

    ; --- 手动解包硬件帧 (模拟中断退出) ---
    ; 此时 SP 已指向 PSP，需手动弹出 xPSR, PC, LR, R12, R0-R3
    LDMIA   SP!, {R0-R3}            ; 丢弃 R0-R3
    LDMIA   SP!, {R12}              ; 丢弃 R12
    LDMIA   SP!, {R14}              ; 恢复 LR
    LDMIA   SP!, {R6}               ; 弹出 PC (任务入口) 到 R6
    LDMIA   SP!, {R7}               ; 丢弃 xPSR (用 R7 暂存)

    ; [指令释义] CPSIE: Change Processor State, Enable Interrupts (开中断)
    CPSIE   I                       ; 2. 开中断

    ; [指令释义] BX: Branch and Exchange (跳转到寄存器指定的地址)
    BX      R6                      ; 3. 跳转到任务函数

;========================================================================
; 函数: PendSV_Handler
; 描述: 任务切换 (Task A <-> Task B)
;========================================================================
PendSV_Handler
    CPSID   I                       ; 关中断

    ; --- 保存当前任务 (Context Save) ---
    ; [指令释义] MRS: Move From Special Register (特殊寄存器 -> 通用寄存器)
    MRS     R0, PSP                 ; 获取 PSP

    ; [指令释义] CBZ: Compare and Branch on Zero (如果为0则跳转)
    CBZ     R0, Switch_Point        ; 首次运行保护(可选)

    ; [指令释义] STMDB: Store Multiple Decrement Before (批量存储/压栈)
    STMDB   R0!, {R4-R11}           ; 压入 R4-R11

    LDR     R1, =current_tcb        ; 保存新栈顶到 TCB
    LDR     R2, [R1]

    ; [指令释义] STR: Store Register (寄存器 -> 写入内存)
    STR     R0, [R2]                ; 更新 TCB 中的 stack_ptr

Switch_Point
    ; --- 执行调度算法 (C语言) ---
    ; [指令释义] PUSH: Push registers to stack (压入当前堆栈)
    PUSH    {R14}                   ; 保护 LR

    ; [指令释义] BL: Branch with Link (调用函数，返回地址存入 LR)
    BL      switch_context_logic    ; 调用 C 函数更新 current_tcb

    ; [指令释义] POP: Pop registers from stack (从当前堆栈弹出)
    POP     {R14}

    ; --- 切换 TCB 指针 ---
    LDR     R1, =next_tcb           ; 获取 next_tcb
    LDR     R2, [R1]

    LDR     R3, =current_tcb        ; current_tcb = next_tcb
    STR     R2, [R3]

    ; --- 恢复下一个任务 (Context Restore) ---
    LDR     R0, [R2]                ; 获取新任务栈顶
    LDMIA   R0!, {R4-R11}           ; 弹出 R4-R11
    MSR     PSP, R0                 ; 更新 PSP

    CPSIE   I                       ; 开中断
    BX      R14                     ; 退出中断 (硬件自动恢复剩余寄存器)

    ; [指令释义] ALIGN: 确保下一条指令地址对齐 (编译器指令)
    ALIGN
    END