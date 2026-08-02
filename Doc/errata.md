## 公共

### RTOS 移植

#### 不得统一替换中断属性

RTOS 中断入口必须沿用目标芯片对应 RTOS port 和 startup 的实现，不能把所有处理程序统一改成 fast 或普通中断属性。

- CH595 FreeRTOS 的 `SysTick_Handler` 使用 `interrupt("WCH-Interrupt-fast")`。
- CH32V20x FreeRTOS 使用普通 `interrupt()`，并配合 port 的 ISR 栈和上下文保存。
- CH32H417 FreeRTOS 使用普通 `interrupt()`，V3F/V5F 还分别使用 `SysTick0`/`SysTick1`。

因此下面两种声明都可能正确，取决于准确的 RTOS port：

```c
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) __attribute__((interrupt()));
```

禁止脱离对应 `portable/`、startup 和向量入口单独替换属性，否则可能破坏硬件压栈、独立 ISR 栈或上下文切换。





## CH32H417



