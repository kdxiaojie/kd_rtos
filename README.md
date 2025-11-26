[新增功能]
1.阻塞式延时 (os_delay) 彻底告别 cpu_delay_ms 忙等待模式。现在任务调用 os_delay 后，会主动把自己从就绪列表 (ReadyList) 移除，并挂入延时列表 (DelayedList)，立即释放 CPU 资源给低优先级任务或空闲任务。
2.任务状态管理 实现了任务在就绪态 (Ready) 与阻塞态 (Blocked) 之间的自动流转。
3.防御性编程接口 在 os_delay 内部增加了调度锁状态检测，禁止在调度器锁定期间 (OSSchedLock) 调用阻塞函数，防止系统逻辑死锁。

[bug]
1.修复链表遍历迭代器失效 (Iterator Invalidation) 问题
[问题描述]： 在 SysTick_Handler 中处理延时任务唤醒时，需要遍历 DelayedList。旧代码在移除当前节点后（特别是当移除的是头节点，或者链表被删空时），原有的 while 循环判断条件（如 node != head）会失效。这导致循环无法正常退出，代码会尝试二次操作已移除的节点，引发链表指针错乱、死循环或 HardFault。
[解决方案]： 重构了链表遍历逻辑。采用“先备份下一个节点”的策略，并在每次执行移除操作后，立即检查链表头是否为空 (head == NULL) 以及是否遍历完一圈。这种防御性逻辑确保了在双向循环链表中动态删除节点的安全性。

---------------------------------------[旧代码]-------------------------------------------
// ... SysTick_Handler 内部 ...
list_node_t *node = DelayedList.head;
list_node_t *next_node;

// 漏洞：只用 node != NULL 作为入口判断，且退出条件单一
while (node != NULL)
{
    next_node = node->next; // 备份下一个

    task_tcb *tcb = (task_tcb *)(node->owner_tcb);
    if (tcb->delay_ticks > 0) tcb->delay_ticks--;

    if (tcb->delay_ticks == 0)
    {
        // 【动作 A】移除节点
        // 如果列表里只有这一个节点，移除后 DelayedList.head 变成 NULL
        list_remove(&DelayedList, node);

        list_insert_end(&ReadyList[tcb->task_priority], node);
        bitmap_set(tcb->task_priority);
    }

    node = next_node;

    // 【漏洞爆发点】退出条件判断
    // 如果链表被删空了，DelayedList.head 是 NULL。
    // 而 node 是刚才备份的任务地址 (比如 0x20001000)。
    // 判断：0x20001000 == NULL ？ -> 假！
    // 结果：循环没有退出，继续跑下一圈！访问野指针/死循环！
    if (node == DelayedList.head) break;
}
---------------------------------------[旧代码]-------------------------------------------


---------------------------------------[解决方法]-------------------------------------------
if (tcb->delay_ticks == 0)
        {
            list_remove(&DelayedList, node);
            list_insert_end(&ReadyList[tcb->task_priority], node);
            bitmap_set(tcb->task_priority);
        }

        // 【修复点 1】关键防御：判断链表是否为空！！！！
        // 如果刚才移除了最后一个任务，head 变成了 NULL。
        // 必须立刻退出，否则后面全是乱跑。
        if (DelayedList.head == NULL) break;

        node = next_node;

        // 【修复点 2】正常退出：跑完一圈了吗？
        // 检查是否回到了（现在的）头节点
        if (node == DelayedList.head) break;
---------------------------------------[解决方法]-------------------------------------------



2.修复调度锁死锁 (Scheduler Lock Deadlock)
[问题描述]： 在按键测试任务中，当系统通过 OSSchedLock() 上锁后（此时禁止任务切换），任务代码错误地调用了 os_delay()。
os_delay 的逻辑是“将任务移出就绪表并请求切换”，而调度锁的逻辑是“拒绝切换”。这导致该任务处于“既不在就绪表（系统认为它睡了），却又实际霸占着 CPU（因为没切走）”的薛定谔状态，最终导致系统卡死在空闲循环或无法响应。
[解决方案]：
（1）修正了按键任务的测试逻辑。在调度器锁定期间，强制使用 cpu_delay_ms (死延时) 进行等待，绝不主动让出 CPU，直到解锁。

void task_key_func(void)
{
    while(1)
    {
        // ============================================================
        // KEY1 (PA0): 加锁测试
        // ============================================================
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
        {
            // !!! 修正点 1：智能消抖 !!!
            // 如果已经锁了，绝对不能调 os_delay，否则回不来！
            if(OSSchedLockNesting == 0)
                os_delay(20);      // 没锁：用阻塞延时，让出CPU
            else
                cpu_delay_ms(20);  // 锁了：只能用死延时霸占CPU


（2）同时修正了 os_delay 的内部逻辑，如果检测到调度锁生效，直接返回，拒绝执行挂起操作。

void os_delay(uint32_t ticks)
{
    if (ticks == 0) return;

    // !!! 核心防御：如果调度器被锁了，禁用 os_delay !!!
    // !否则任务会把自己挂起，但又切不走，导致系统逻辑崩溃
    if (OSSchedLockNesting > 0)
    {
        return;
    }
-----------------------------