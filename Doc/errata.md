## 公共

### RTOS 移植

#### 不能用硬件中断

不能采取

```c
void ISR_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
```

而是采取

```c
void NMI_Handler(void) __attribute__((interrupt()));
```





## CH32H417



