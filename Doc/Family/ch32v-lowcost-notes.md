# CH32V 低成本和 CH32L103 系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH32V003、CH32V006/007/M007 和 CH32L103 指导，形成仓库专用归一化说明。

官方 EVT 示例、RM、DS、启动代码文件、链接脚本、开发板原理图和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 源组 | 仓库 EVT 根目录 | 源说明中的内存 | 关键约束 |
|---|---|---|---|
| CH32V003 | 验证 `CH32V003EVT/` 可用性 | 16 KB Flash、2 KB RAM | RAM 极小、64 字节 Flash 页，SDI printf 通常很有用。 |
| CH32V006/007/M007 | 验证 `CH32V006EVT/` 可用性 | 最高 62 KB Flash、8 KB RAM | 比 V003 有更多模拟/触摸/电机示例。 |
| CH32L103 | `CH32L103EVT/` | 64 KB Flash、20 KB RAM | 源说明中的 USBFS、USB-PD、CAN、RTOS、低功耗和 RTC 示例。 |

## 架构、工具链、启动代码、链接器

- 架构：低成本 WCH RISC-V StdPeriph 风格系列。
- 工具链：MounRiver `.project` / `.wvproj`，带 `SRC/Core`、`SRC/Debug`、`SRC/Ld`、`SRC/Peripheral` 和 `SRC/Startup` 链接资源。
- 头文件名不同：CH32V003 使用 `ch32v00x.h`，CH32V006 源说明提及 `ch32v00X.h`，CH32L103 使用 `ch32l103.h`。
- 小型芯片需要明确规划栈/堆，并谨慎使用局部缓冲区。

## 内存和引导布局

- 源说明列出 CH32V003 有 16 KB Flash / 2 KB RAM，链接器示例中的默认栈为 256 字节。
- CH32V006/007/M007 源说明列出最高 62 KB Flash / 8 KB RAM。
- CH32L103 源说明列出 64 KB Flash / 20 KB RAM。
- CH32V003/006 的旧来源说明包含保留 4 KB 引导区、APP 从 `0x1000` 开始的布局，但不能应用到 CH32L103。当前 CH32L103 EVT 的 USB/UART IAP APP 从 `0x5000` 开始，USB Host IAP APP 从 `0x8000` 开始。
- Flash 擦除大小不同：CH32V003 使用 64 字节页；源说明中 CH32V006/L103 使用 1024 字节页。

## 外设和示例覆盖范围

CH32V003 示例涵盖 ADC、DMA、EXTI、Flash、GPIO、I2C、IAP、中断、看门狗、OPA、PWR、RCC、SDI printf、SPI、SysTick、定时器/PWM、USART、USART IAP 和 WWDG。源说明中 CH32V006 增加了更多 OPA/CMP、触摸、电机、空闲接收和 SLI/SLTIM 风格示例。CH32L103 增加了 CAN、CRC、RTOS、USB、USB-PD、RTC、LPTIM、PMP、更丰富的 OPA/CMP/PGA 和低功耗示例。

## 主题交叉引用

- 通用外设：`Doc/HAL/wch-hal-normalization.md`。
- USB-PD：`Doc/USBPD/wch-usbpd-notes.md`。
- USB：`Doc/USB/wch-usb-notes.md`。
- 系统/模拟/电源：`Doc/System/wch-system-analog-power-notes.md`。
- 模板：`Doc/Templates/wch-project-template-notes.md`。
- RTOS：`Doc/RTOS/wch-rtos-notes.md`。

## 已知系列陷阱

- 初始化前启用外设和 GPIO 时钟。
- 使用正确的低成本系列头文件；不要包含 CH57x 或无关 CH32 头文件。
- Flash 页大小因芯片而异；不要假设 CH32V003 使用 1 KB 页。
- GPIO 复用功能、模拟输入和 I2C 开漏模式必须与外设匹配。
- EVT 示例使用 WCH 快速中断属性时，RISC-V 中断处理程序也需要该属性。
- 在波特率或时序相关代码前调用 `SystemCoreClockUpdate()` 和 `Delay_Init()`。
- TIM1 PWM 需要启用主输出。
- EXTI 线需要显式进行 GPIO 到 EXTI 的映射。
- CH32V003 RAM 太小，不适合大型局部缓冲区、RTOS 或庞大的生成栈。

## 验证清单

- 验证本仓库中存在的确切 EVT 根目录和芯片覆盖范围。
- 按芯片验证头文件名、启动代码文件、链接器 RAM/Flash 大小、栈大小和调试方法。
- 验证 Flash 页大小、IAP 布局、SDI printf 可用性、中断属性和 RAM 中向量示例。
- 验证 CH32L103 专有功能，例如 USB、USB-PD、CAN、RTOS、RTC、LPTIM 和更丰富的模拟模块。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对所有低成本 CH32V/CH32L EVT 树、RM、DS、工程文件、启动代码文件、链接脚本、封装或开发板原理图进行完整验证。
- 在根据确切官方资料完成核查前，应将芯片支持、内存大小、页大小和功能可用性视为初步信息。
