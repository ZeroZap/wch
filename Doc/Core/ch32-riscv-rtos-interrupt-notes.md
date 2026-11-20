# CH32 RISC-V RTOS 中断注意事项

本文记录 CH32/WCH RISC-V 系列在移植或使用 RTOS 时与中断相关的注意事项，尤其是 WCH 快速中断属性的使用边界。

参考资料：

- `Doc/Core/RISC-V 平台移植 RTOS.pdf`
- `Doc/Core/wch-core-notes.md`
- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/`

## 结论

在 CH32 RISC-V 上运行 FreeRTOS、RT-Thread、LiteOS、uCOS 等 RTOS 时，不应随意使用 WCH 快速中断属性：

```c
__attribute__((interrupt("WCH-Interrupt-fast")))
```

尤其是以下中断处理函数应避免使用快速中断：

- RTOS tick 中断。
- 软件中断、PendSV 类或任何用于任务切换的中断。
- 会调用 RTOS `FromISR` API 的外设中断。
- 会触发 `portYIELD_FROM_ISR`、任务通知、信号量、队列或事件组调度的中断。
- 任何依赖 RTOS 上下文保存/恢复模型的中断。

默认建议使用 RTOS port 或 EVT 示例中的普通中断写法，例如：

```c
void XXX_IRQHandler(void) __attribute__((interrupt()));
```

## 原因

RTOS 的 RISC-V 移植依赖固定的上下文保存和恢复模型，包括：

- 任务栈布局。
- `mepc`、`mstatus` 等 CSR 的保存与恢复。
- `mret` 返回路径。
- 软件中断触发上下文切换。
- 中断栈或 `mscratch` 的使用。
- `ra`、`sp`、通用寄存器等任务上下文。

WCH 快速中断通常涉及 WCH 特定的快速入口、硬件压栈或 PFIC 行为。如果 RTOS port 没有明确适配该机制，快速中断可能绕过或改变 RTOS 期望的上下文保存/恢复流程，导致：

- 任务切换后寄存器被破坏。
- `mepc` 或 `mstatus` 恢复异常。
- 中断返回路径不符合 RTOS 预期。
- `FromISR` API 触发调度后异常。
- 偶发 HardFault、死机或任务栈损坏。

因此，快速中断不应作为 RTOS 工程中的默认 ISR 声明方式。

## 仓库示例依据

CH32H417 FreeRTOS 示例中，SysTick 中断使用普通 interrupt 属性，而不是 WCH 快速中断属性。

V3F 示例：

```c
void SysTick0_Handler(void) __attribute__((interrupt()));
```

路径：

- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/V3F/FreeRTOS/portable/GCC/RISC-V/port.c`

V5F 示例：

```c
void SysTick1_Handler(void) __attribute__((interrupt()));
```

路径：

- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/V5F/FreeRTOS/portable/GCC/RISC-V/port.c`

FreeRTOS port 还使用 `mscratch` 切换中断栈：

```c
#define GET_INT_SP()   __asm volatile("csrrw sp,mscratch,sp")
#define FREE_INT_SP()  __asm volatile("csrrw sp,mscratch,sp")
```

路径：

- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/V3F/FreeRTOS/portable/GCC/RISC-V/portmacro.h`
- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/V5F/FreeRTOS/portable/GCC/RISC-V/portmacro.h`

上下文切换逻辑位于：

- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/V3F/FreeRTOS/portable/GCC/RISC-V/portASM.S`
- `CH32H417TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/V5F/FreeRTOS/portable/GCC/RISC-V/portASM.S`

这些实现说明 RTOS port 对中断入口、栈和上下文保存有明确假设。

## 何时可以评估快速中断

只有在满足以下条件时，才可以评估在 RTOS 工程中使用 `WCH-Interrupt-fast`：

1. 对应芯片、启动代码、链接脚本和编译器均支持该快速中断属性。
2. RTOS port 明确说明支持 WCH 快速中断，或已针对快速中断完成适配。
3. 该中断不调用任何 RTOS API。
4. 该中断不触发任务切换。
5. 该中断不依赖 RTOS 管理的任务栈上下文。
6. 已验证中断嵌套、`mstatus`、`mepc`、`mscratch`、中断栈和返回路径。
7. 已进行长时间压力测试，确认无任务栈损坏、寄存器破坏和偶发异常。

若无法确认以上条件，应使用普通中断属性。

## 实践建议

RTOS 工程中推荐按以下规则处理 ISR：

| 场景 | 建议 |
|---|---|
| RTOS tick 中断 | 使用 RTOS port 原始写法 |
| 软件中断/任务切换中断 | 使用 RTOS port 原始写法 |
| 调用 `FromISR` API 的外设中断 | 使用普通 `interrupt()` |
| 需要唤醒任务的中断 | 使用普通 `interrupt()` |
| 裸机、极短、不可抢占的高实时性中断 | 可按芯片手册和 EVT 示例评估快速中断 |
| BLE/TMOS 等官方协议栈要求 highcode 的中断 | 按对应协议栈 EVT/文档验证后使用 |

通用模板：

```c
void USARTx_IRQHandler(void) __attribute__((interrupt()));

void USARTx_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* 清中断标志，读取/写入外设数据 */

    /* 如需通知任务，使用 FromISR API */
    /* vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken); */

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

注意：具体 API 名称和 `portYIELD_FROM_ISR` 写法应以所使用 RTOS 及其 CH32 port 为准。

## 与裸机示例的区别

`Doc/Core/wch-core-notes.md` 中记录了 WCH RISC-V 裸机或 EVT 风格示例常见的快速中断声明：

```c
__attribute__((interrupt("WCH-Interrupt-fast")))
```

这说明该属性在部分 WCH RISC-V 裸机场景中可用，但不代表 RTOS 工程可以无条件使用。RTOS 工程应优先服从 RTOS port 的中断入口和上下文切换约定。
