# CH32V 通用系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH32V103、CH32V20x、CH32V307 和 CH32V407 系列指导，形成面向未来 HAL 元数据和模板工作的仓库专用归一化说明。

官方 EVT 示例、RM、DS、启动代码文件、链接脚本、开发板原理图和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 子系列 | 本仓库中的示例 EVT 根目录 | 头文件模式 | 说明 |
|---|---|---|---|
| CH32V103 | `CH32V103EVT/` | `ch32v10x.h`, `ch32v10x_*` | 通用 StdPeriph 风格 RISC-V 基准；源说明中的功能表应按芯片验证。 |
| CH32V20x | `CH32V20xEVT/` | `ch32v20x.h`, `ch32v20x_*` | V20x/V208 变体视封装增加 BLE/USB/高速功能。 |
| CH32V307 | `CH32V307EVT/` | `ch32v30x.h`, `ch32v30x_*` | 更高性能的系列，带以太网示例和 64 KB RAM 变体。 |
| CH32V407 | `CH32V407EVT/` | `ch32v4x7.h`, `ch32v4x7_*` | 高功能 CH32V 系列，带 USB HS、以太网、SDIO、DVP、FSMC/LTDC 风格源参考资料。 |

## 架构、工具链、启动代码、链接器

- 架构：WCH RISC-V，使用 StdPeriphDriver 风格的外设 API。
- 工具链：带 GCC 风格 `Ld/Link.ld` 的 MounRiver Studio 工程文件（`.project`、`.cproject`、`.wvproj`）。
- 共享工程结构：`User/`、`SRC/Peripheral/inc`、`SRC/Peripheral/src`、`SRC/Startup`、`SRC/RVMSIS` 和 `Ld/Link.ld`。
- 模板起点：复制最接近的 EVT 示例，而不是根据代码片段构建。
- 调试约定：源模板中通常调用 `SystemCoreClockUpdate()`、延时/调试初始化、`USART_Printf_Init(115200)`，并打印芯片 ID。

## 内存和引导布局

内存大小随子系列和封装有很大差异。

| 子系列 | 源说明中的 Flash/RAM 示例 | Flash 擦除说明 |
|---|---|---|
| CH32V103 | 32/64/128 KB Flash、10/20 KB RAM | 源说明列出 1 KB 页。 |
| CH32V20x | 通常为 32/64/128 KB，另有更大的 D8 示例；10/20/64 KB RAM | 源说明列出 4 KB 页。 |
| CH32V307 | 128/256 KB Flash、64 KB RAM | 源说明列出 4 KB 页。 |
| CH32V407 | 当前默认链接为 576 KB Flash、136 KB RAM，RAM 首 1 KB 保留；脚本还列出 512 KB Flash/200 KB RAM 选项 | 源说明列出 4 KB 页。 |

规则：

- 不要在不同内存容量的芯片间复用同一个 `Link.ld`。
- 当前 CH32V103/V20x/V307/V407 USB/UART IAP APP 从 `0x00005000` 开始，即预留 20 KB，而不是 `0x00004000`；Host IAP 仍有其他偏移，例如 CH32V307 为 `0x00006000`，必须按所选工程验证。
- 写入 Flash 前必须先擦除；局部更新需要围绕擦除页执行读取-修改-擦除-写入。
- 栈和堆大小由链接器控制，必须针对 RTOS、USB、ETH、SDIO、图形或大型缓冲区进行调整。

## 外设覆盖重点

源示例索引列出了广泛的覆盖范围，但必须按芯片验证确切支持情况：

- 通用：GPIO、USART、SPI、I2C、ADC、定时器/PWM、DMA、EXTI、Flash、RCC、PWR、看门狗。
- 通信：CAN 必须按子系列的当前驱动和 EXAM 目录验证，不能概括为所有 CH32V 均有；当前 `CH32V103EVT` 未见 CAN 驱动头或 CAN 示例证据。普遍支持 USB FS，CH32V407 风格源支持 USB HS。
- 高速和媒体：有能力的 V208/V307/V407 变体支持以太网，源示例列出时还包括 SDIO、FSMC、DVP、I2S、LTDC/ARGB/I3C/PSRAM。
- 系统和模拟：视子系列而定，包括 RTC、CRC、RNG、DAC、OPA、TouchKey、PMP、FPU。
- RTOS：当前 CH32V103、CH32V20x、CH32V307、CH32V407 EVT 均有 FreeRTOS、RT-Thread、HarmonyOS LiteOS 及 TencentOS 工程；具体目录、startup 和 port 仍须按芯片选择，不能沿用旧来源索引中漏列 CH32V20x 的结论。

## 主题交叉引用

- 通用外设规则：`Doc/HAL/wch-hal-normalization.md`。
- 启动代码/链接器/中断规则：`Doc/Core/wch-core-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。
- 以太网：`Doc/ETH/wch-ethernet-notes.md`。
- USB：`Doc/USB/wch-usb-notes.md`。
- CAN/I2S/DVP：`Doc/IO/wch-io-media-notes.md`。
- SDIO/存储：`Doc/Storage/wch-storage-notes.md`。
- IAP：`Doc/IAP/wch-iap-ota-notes.md`。
- RTOS：`Doc/RTOS/wch-rtos-notes.md`。

## 已知系列陷阱

- 访问寄存器前，必须在正确的总线上启用外设时钟。
- 启用外设信号前，必须选择复用功能 GPIO 模式。
- I2C 示例要求在每个总线阶段检查事件；跳过检查会导致挂起或传输无效数据。
- 对确有 CAN 驱动和示例的目标，CAN 接收需要配置滤波器；不要将此规则反推为 CH32V103 已支持 CAN。
- 准确转换前需要校准 ADC。
- `printf()` 需要通过 `_write()` 或等效调试辅助程序重定向。
- 以太网、USB、SDIO、DVP 和大型 DMA 缓冲区不应分配在小栈上。
- 不要假设 CH32V103/V20x/V307/V407 具有相同的高速外设集合。

## 验证清单

- 验证每个子系列的 EVT 根目录，以及 GPIO、USART、ADC、定时器、Flash、DMA、USB、CAN、ETH、SDIO、IAP 和 RTOS 的确切示例路径。
- 按子系列验证头文件前缀、启动代码文件、系统文件、链接脚本、`*_conf.h` 和中断处理程序名称。
- 根据 DS/RM 验证 Flash/RAM 大小和封装专用外设的可用性。
- 根据所选 IAP EVT 示例验证 IAP 链接器偏移和向量重定位函数。
- 根据当前头文件和示例验证高速外设缓冲区放置位置和 DMA 要求。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对所有 CH32V103/V20x/V307/V407 EVT 树、RM、DS、工程文件、启动代码文件、链接脚本、封装或开发板原理图进行完整验证。
- 在根据确切目标芯片和官方 EVT 资料完成核查前，应将功能可用性和内存值视为线索。
