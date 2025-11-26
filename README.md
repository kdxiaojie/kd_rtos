1.将任务切换的pendsv中断交给systick_handler，实现自动切换，引入时间片轮转
2.引入调度锁机制，允许在不关中断的情况下禁止任务切换

修复启动时序崩溃 (HardFault)：
[问题]：使用cpu_tick_init()对SysTick 初始化后立即触发中断并尝试触发 PendSV，此时 os_start 尚未执行，任务未初始化，导致非法内存访问。
[解决]：在 main 函数入口调用 __disable_irq() 全局关中断，直到 os_start 配置好 PSP 并执行 CPSIE I 后才允许中断响应。