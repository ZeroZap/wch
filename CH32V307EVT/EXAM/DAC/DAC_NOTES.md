# CH32V307 DAC 例程学习笔记

> 基于 `CH32V307EVT/EXAM/DAC/` 目录下 8 个官方例程以及 `SRC/Peripheral/inc/ch32v30x_dac.h`、`SRC/Peripheral/src/ch32v30x_dac.c` 库源码整理。
>
> 适用芯片：**CH32V30x** 系列（CH32V307 内置 2 路 12 位 DAC）。

---

## 1. DAC 硬件概览

| 特性 | 说明 |
|------|------|
| 分辨率 | 12 位 |
| 通道数 | 2 路（`DAC_Channel_1` / `DAC_Channel_2`） |
| 引脚 | PA4 → DAC1_OUT1，PA5 → DAC1_OUT2 |
| 数据对齐 | 8 位右对齐 / 12 位左对齐 / 12 位右对齐 |
| 输出缓冲 | 可选 `OutputBuffer_Enable` / `OutputBuffer_Disable` |
| 触发源 | 软件触发 / 定时器 TRGO (T2/T4/T5/T6/T7/T8) / EXTI_9 事件 |
| 内建波形 | 噪声波 (LFSR) / 三角波 |
| DMA | DAC1 → DMA2_CH3，DAC2 → DMA2_CH4 |
| 关键寄存器 | `CTLR` (控制)、`SWTR` (软件触发)、`R12BDHR1/R12BDHR2` (通道保持寄存器)、`RD12BDHR` (双通道)、`DOR1/DOR2` (输出数据) |

电压计算（参考电压通常为 3.3 V 或 Vref+）：

```
V_OUT = Vref × DOR / 4095
```

12 位右对齐时，`DOR` 范围为 0 ~ 4095。

---

## 2. 例程一览

| 例程目录 | 功能 | 触发方式 | 输出波形 |
|----------|------|----------|----------|
| `DAC_Normal_OUT` | 不触发，直接写寄存器输出 | 无（软件写保持寄存器即生效） | 直流阶梯 |
| `DAC_Timer_Trig` | TIM8_TRGO 触发，每次触发把保持寄存器值搬到 DOR | TIM8 TRGO | 直流阶梯 |
| `DAC_Exit_9_Trig` | EXTI Line9 (PB9) 事件触发 | EXTI9 事件 | 直流阶梯 |
| `DAC_DMA` | TIM8_TRGO + DMA 循环搬运 8 个样本 | TIM8 TRGO → DMA2_CH3 | 任意波形（示例为不规则数据） |
| `DualDAC_SineWave` | 双通道同步输出相同正弦波 | TIM4 TRGO + DMA2_CH3 → 双通道 | 正弦波 |
| `DualDAC_Triangle` | 双通道不同幅度三角波 | TIM4 TRGO | 不同幅度三角波 |
| `DAC_Noise_Generation` | 软件触发 + 内建 LFSR 噪声 | 软件触发 | 伪随机噪声 |
| `DAC_Triangle_Generation` | 软件触发 + 内建三角波 | 软件触发 | 三角波 |

每个例程的固定样板：
- `User/main.c`：应用代码。
- `User/ch32v30x_conf.h`：包含所有外设库头。
- `User/system_ch32v30x.c/h`：系统时钟配置（一般使用默认 8 MHz HSI / 144 MHz 系统时钟）。
- `User/ch32v30x_it.c/h`：默认只提供 NMI / HardFault。

---

## 3. 公共初始化步骤

所有 DAC 例程都遵循以下初始化骨架（除噪声/三角波内建波形模式不需要触发源也必须选一个，否则波形不会刷新）：

```c
// 1. 开时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // DAC 引脚在 APB2
RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC,  ENABLE);  // DAC 外设在 APB1

// 2. 配置 GPIO 为模拟输入（AIN），防止数字输入缓冲影响模拟信号
GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;            // 或 | GPIO_Pin_5
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
GPIO_Init(GPIOA, &GPIO_InitStructure);

// 3. 配置 DAC 参数
DAC_InitType.DAC_Trigger            = ...;            // 见下表
DAC_InitType.DAC_WaveGeneration     = DAC_WaveGeneration_None;
DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
DAC_InitType.DAC_OutputBuffer       = DAC_OutputBuffer_Disable;
DAC_Init(DAC_Channel_1, &DAC_InitType);

// 4. 使能通道
DAC_Cmd(DAC_Channel_1, ENABLE);

// 5.（可选）使能 DMA 请求
DAC_DMACmd(DAC_Channel_1, ENABLE);
```

⚠️ **注意 `DAC_OutputBuffer`**：本族例程几乎都选 `Disable`，让 DAC 直接驱动引脚；启用输出缓冲后驱动能力增强但最低输出电压不能到 0、最高不能到 Vref。调试时按需切换。

---

## 4. 触发源选择

`DAC_Trigger` 可选值（来自 `ch32v30x_dac.h`）：

| 宏 | 含义 |
|----|------|
| `DAC_Trigger_None` | 不需触发，写保持寄存器即更新 DOR |
| `DAC_Trigger_T6_TRGO` | TIM6 TRGO |
| `DAC_Trigger_T8_TRGO` | TIM8 TRGO（仅 High-density，本族即此） |
| `DAC_Trigger_T7_TRGO` | TIM7 TRGO |
| `DAC_Trigger_T5_TRGO` | TIM5 TRGO |
| `DAC_Trigger_T2_TRGO` | TIM2 TRGO |
| `DAC_Trigger_T4_TRGO` | TIM4 TRGO |
| `DAC_Trigger_Ext_IT9` | EXTI Line9 事件（PB9 上升沿） |
| `DAC_Trigger_Software` | 软件触发，需要写 `DAC->SWTR` 对应位，由硬件自动清零 |

---

## 5. 数据对齐

```c
DAC_SetChannel1Data(DAC_Align_12b_R, value);   // 0..4095，最常用
DAC_SetChannel1Data(DAC_Align_12b_L, value);   // 左对齐，写 16 位 value 时取高 12 位
DAC_SetChannel1Data(DAC_Align_8b_R,  value);   // 8 位右对齐，等效放大 16 倍写入
```

`Dual` 版本：

```c
DAC_SetDualChannelData(DAC_Align_12b_R, data2, data1);
// 库函数原型：注意参数顺序 Data2 在前、Data1 在后
// 内部写入：data = (Data2 << 16) | Data1  → 高 16 位是 ch2，低 16 位是 ch1
```

直接寄存器操作（更高效、见 `DAC_Timer_Trig` 例程）：

```c
DAC->R12BDHR1 = value;  // 通道 1 的 12 位右对齐保持寄存器
DAC->R12BDHR2 = value;  // 通道 2
DAC->RD12BDHR = (ch2 << 16) | ch1;  // 双通道同时写入
```

---

## 6. 各例程精读

### 6.1 `DAC_Normal_OUT`（无触发的直流输出）

- 触发选 `DAC_Trigger_None`。
- 主循环 `DAC_SetChannel1Data(DAC_Align_12b_R, DAC_Value[i])`，间隔 1 s 切换电压。
- 适合作为最简参考：写完即生效，不依赖任何定时器或中断。

### 6.2 `DAC_Timer_Trig`（定时器触发）

- `DAC_Trigger = DAC_Trigger_T8_TRGO`。
- `TIM8_Init(0x7, 48000-1)`：自动重装载 7，分频 48000（计数向下），`TIM_SelectOutputTrigger(TIM8, TIM_TRGOSource_Update)` 把更新事件作为 TRGO。
- 主循环写 `DAC->R12BDHR1 = DAC_Value[i]`：值先进入保持寄存器，**每次 TRGO 上升沿**才被搬运到 `DOR1`，从而 PA4 同步更新。
- 主循环打印 `R12BDHR1` / `DOR1` / `TIM8->CNT` 可观察到：DOR 仅在 TRGO 时变化，而 CNT 在 0~7 之间递减。

### 6.3 `DAC_Exit_9_Trig`（外部中断事件触发）

- `DAC_Trigger = DAC_Trigger_Ext_IT9`，PB9 上拉输入，下降沿产生 EXTI 事件（不是中断）。
- 关键配置：
  ```c
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;  // 事件，不进 NVIC
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  ```
- 主循环 1 s 改写一次保持寄存器值，每次 PB9 上升沿到来时，DAC 触发把新值搬到 PA4。可以在 PB9 上接按键或外部脉冲源验证。

### 6.4 `DAC_DMA`（定时器 + DMA）

- 触发链：`TIM8 更新事件` → `TRGO` → `DAC DMA 请求` → `DMA2_CH3` 把 `dacbuff16bit[]` 循环搬运到 `DAC->R12BDHR1`。
- DMA 配置关键点：
  ```c
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(DAC->R12BDHR1);
  DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize         = 8;
  DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
  ```
- 想输出任意波形：把数组换成 sin 表（半字）、调整 `DMA_BufferSize`，并按需调整 TIM8 周期控制采样率。

### 6.5 `DualDAC_SineWave`（双通道同相正弦波）

- 两通道共用 `DAC_Trigger_T4_TRGO`，使能 `DAC_DMACmd(ch1)` 与 `DAC_DMACmd(ch2)`。
- DMA 目标地址改为双通道保持寄存器：
  ```c
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(DAC->RD12BDHR);
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  ```
- 每个 32 位样本布局：`high16 = DAC2 | low16 = DAC1`。所以：
  ```c
  Dual_DAC_Value[i] = (DAC_Value[i] << 16) | DAC_Value[i];
  ```
- 64 点 sin 表，TIM4 周期 3600、`SystemCoreClock=144 MHz`，触发频率 `144e6/3600 = 40 kHz`，正弦频率 `40e3/64 ≈ 625 Hz`。

### 6.6 `DualDAC_Triangle`（双通道不同幅度三角波）

- 走**硬件内置三角波**路径：`DAC_WaveGeneration_Triangle`。
- 通过 `DAC_TriangleAmplitude_4095`（ch1）和 `DAC_TriangleAmplitude_2047`（ch2）得到不同幅度。
- TIM4 周期 36，触发频率 `144e6/36 = 4 MHz`，两个通道分别输出 4 MHz/4095 斜率的三角波。
- 注意：**内置三角/噪声波必须开启 TEN/WAVE 位**。本例程中 `DAC_Triangle_Generation` 用 `DAC->CTLR |= 0x04;` 强制置位 TEN；`DualDAC_Triangle` 借助 `DAC_WaveGeneration_Triangle` 在 `DAC_Init` 里已经置上。

### 6.7 `DAC_Noise_Generation`（伪随机噪声）

- `DAC_WaveGeneration_Noise` + `DAC_LFSRUnmask_Bits11_0`：LFSR 全部 12 位参与，反馈伪随机序列。
- 触发软件写 `DAC->SWTR |= 0x01;` 后硬件产生一个新随机值。可以读到 DOR1 在 `0x0AAA / 0x0D55 / 0x0EAA …` 等值之间跳变。
- 想更平滑的伪随机：选择 `DAC_LFSRUnmask_Bit0`（仅 LSB 注入），输出几乎是 ±1 LSB 的抖动，可叠加在直流上做小幅扰动。

### 6.8 `DAC_Triangle_Generation`（软件触发的三角波）

- `DAC_Trigger_Software` + `DAC_WaveGeneration_Triangle` + `DAC_TriangleAmplitude_4095`。
- `DAC->CTLR |= 0x04;` 显式打开 TEN 位。
- 主循环不停 `DAC->SWTR |= 0x01;` 软件触发，PA4 上看到三角波。
- 若要可调频率，把软件触发替换成定时器 TRGO 即可。

---

## 7. 关键 API 速查

```c
void   DAC_DeInit(void);
void   DAC_Init(uint32_t DAC_Channel, DAC_InitTypeDef* DAC_InitStruct);
void   DAC_StructInit(DAC_InitTypeDef* DAC_InitStruct);
void   DAC_Cmd(uint32_t DAC_Channel, FunctionalState NewState);
void   DAC_DMACmd(uint32_t DAC_Channel, FunctionalState NewState);
void   DAC_SoftwareTriggerCmd(uint32_t DAC_Channel, FunctionalState NewState);
void   DAC_DualSoftwareTriggerCmd(FunctionalState NewState);
void   DAC_WaveGenerationCmd(uint32_t DAC_Channel, uint32_t DAC_Wave, FunctionalState NewState);
void   DAC_SetChannel1Data(uint32_t DAC_Align, uint16_t Data);
void   DAC_SetChannel2Data(uint32_t DAC_Align, uint16_t Data);
void   DAC_SetDualChannelData(uint32_t DAC_Align, uint16_t Data2, uint16_t Data1);
uint16_t DAC_GetDataOutputValue(uint32_t DAC_Channel);
```

---

## 8. 常用坑 / 注意点

1. **GPIO 必须 `GPIO_Mode_AIN`**：否则数字输入缓冲对微弱模拟信号造成串扰。
2. **触发源与波形生成需匹配**：触发源决定何时把保持寄存器的值搬到 DOR；选 `None` 时是软件写完后立即搬运。**内置噪声/三角波即便不接外部触发也需要 TEN/WAVE 位置 1**，否则 DOR 不刷新。
3. **`DAC_Trigger_Software` 仍需要写 `DAC->SWTR`**：`DAC_SoftwareTriggerCmd` 是辅助函数，写完后硬件自动清零对应位。
4. **DMA 通道**：DAC1 → DMA2_CH3，DAC2 → DMA2_CH4，DMA 方向 `PeripheralDST`（内存 → 外设）。双通道同时搬运时必须使用 **Word** 宽度，地址指向 `DAC->RD12BDHR`。
5. **`DAC_SetDualChannelData` 参数顺序**是 `(Align, Data2, Data1)`，第二个参数是 DAC2，写代码时容易看反。
6. **定时器选 TRGO 触发**：要 `TIM_SelectOutputTrigger(TIMx, TIM_TRGOSource_Update)` 才会把更新事件映射为 TRGO，否则定时器照常计数但不会触发 DAC。
7. **EXTI_9 触发**用 `EXTI_Mode_Event` 而非 `EXTI_Mode_Interrupt`，因为 DAC 不需要 NVIC。
8. **调试时观察** `DAC->DORx` 才反映真正输出电压；`R12BDHRx` 只是保持寄存器。
9. **输出缓冲**：默认库 `DAC_StructInit` 给的是 `Enable`，但 8 个例程全部显式覆盖为 `Disable`，调试时按手册说明权衡：Disable 输出阻抗较高、Enable 驱动能力更强但不能到轨。

---

## 9. 自学路线建议

1. 先跑 `DAC_Normal_OUT` → 验证硬件和示波器/万用表读数。
2. 切换到 `DAC_Timer_Trig` → 学会 TRGO 链。
3. `DAC_DMA` → 学会用 DMA 输出任意波形（sin、任意）。
4. `DAC_Triangle_Generation` / `DAC_Noise_Generation` → 学会硬件内置波形。
5. `DualDAC_SineWave` / `DualDAC_Triangle` → 双通道 + RD12BDHR。

到此即可掌握 CH32V307 DAC 所有典型应用模式。
