# WCH 项目模板笔记

本文从 `Doc/Ref/wch-dev-skill` 中提取新项目设置指导，整理为适用于本仓库未来项目模板、CubeX 生成及示例迁移的规则。

官方 EVT 项目、工具链项目文件、启动代码文件、链接器/分散加载文件、RM、DS 及当前仓库源码仍是最终依据。

## 来源文件

- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/new_project.md`

## 模板策略

以最接近的官方 EVT 示例为起点。除非已掌握准确芯片系列的启动代码文件、链接器/分散加载文件、系统时钟代码、调试输出、中断文件及工具链项目元数据，否则不要从孤立的代码片段生成项目。

规则：

- 按架构、芯片系列、工具链及功能配置选择项目模板。
- 验证前，保留最接近的 EVT 示例中的厂商启动代码、系统、链接器、分散加载、调试及 StdPeriphDriver 文件。
- 将 BLE、USB、Ethernet、RTOS、IAP 及高速接口示例视为功能配置，而不是裸 GPIO 模板上的少量附加内容。
- 生成的模板应保留来源可追溯性，能追溯到准确的 EVT 示例及 `Doc/Ref/wch-dev-skill` 来源指南。

## 系列模板矩阵

| 系列 | 工具链风格 | 基础项目规则 | 必须保留 |
|---|---|---|---|
| CH57x BLE/RISC-V | MounRiver `.project` / `.wvproj` | 链接或复制 `Ld`、`RVMSIS`、`Startup`、`StdPeriphDriver`；尽早选择 BLE 或非 BLE 流程 | BLE 所需的 `config.h`、BLE 所需的 `Main_Circulation()`、CH57x 链接器 RAM 布局。 |
| CH58x/CH59x BLE/RISC-V | MounRiver `.project` / `.wvproj` | 从 `EXAM` 复制最接近的 BLE、USB、UART、LCD 或 GPIO 示例 | 正确的 `CH58x_common.h` 或 `CH59x_common.h`、BLE 堆、链接资源。 |
| CH32V 通用系列 | MounRiver `.project` / `.wvproj` | 复制最接近的 CH32V103/V20x/V307/V407 EVT 示例 | 正确的 `ch32v*_conf.h`、`system_ch32v*.c`、启动代码、`Ld/Link.ld`、调试 UART。 |
| CH32V 低成本系列 / CH32L103 | MounRiver `.project` / `.wvproj` | 复制最接近的 CH32V003/CH32V006/CH32L103 示例 | 较小的 Flash/RAM 链接器值、USART 或 SDI printf 等调试模式、低资源配置。 |
| CH32M030 | MounRiver `.project` / `.wvproj` | 从 `CH32M030EVT` 复制最接近的 RISC-V V3B 示例 | `ch32m030.h`、64 KB Flash/12 KB RAM、RISC-V startup/`Link.ld`；不得套用 CH32F ARM 模板。 |
| CH32X/CH64x USB-PD | MounRiver `.project` / `.wvproj` | 复制最接近的 GPIO、USART、USBPD、USB、PIOC、DMA、PWR 或 IAP 示例 | 正确的 `ch32x035.h`、`ch32x3x5.h`、`ch643.h` 或 `ch641.h`；启动代码及存储容量配置。 |
| CH32H415/416/417 | MounRiver `.project` / `.wvproj` | 从 `CH32H47TEVT` 复制最接近的示例并选择 V3F/V5F 配置 | 共享 `ch32h417.h`/startup、内核专用链接器、IPC/HSEM/双 SysTick；默认及 IAP 偏移按工程选择。 |
| CH569 / CH56x | MounRiver `.project` / `.wvproj` | 复制最接近的 CH569 GPIO/UART/高速接口示例 | `CH56x_common.h`、`Flash_Lib`、启动代码、RAM/DMA 可访问存储区规则。 |
| CH561/CH563 ARM7TDMI | Keil MDK `.uvproj` 及分散加载文件（来源线索） | 当前 SDK 未导入对应 EVT；导入官方 SDK 后再选择模板 | 头文件、`SYSFREQ`、startup、分散加载和库文件名称均为来源未验证。 |
| CH32F ARM Cortex-M | Keil MDK 或 MounRiver | 当前 SDK 未导入 CH32F EVT，导入后再复制准确的 CH32F10x/F20x 示例 | `ch32f10x`/`ch32f20x` 相关内容目前仅为来源未验证，不得借用 CH32M030 工程。 |
| CH5xx 8051 | Keil C51 / SDCC 风格项目 | 复制 EVT C51 项目结构，或创建 `Public`、驱动程序及应用程序组 | `CH5xx.H`、`Debug.C/H`、C51 内存模型，以及预留 ISP 引导加载程序时的代码 ROM 限制。 |

## 通用项目结构

生成的模板应将以下层次分开：

```text
Application code       # User main, application modules, profiles
Board support          # LED, debug UART, buttons, sensor/codec/transceiver pins
Feature profile        # BLE, USB, ETH, RTOS, IAP, CAN, storage, display
Vendor driver source   # StdPeriphDriver, register headers, BLE stack, USB stack
Core/startup/linker    # startup assembly, system clock, vector table, Link.ld/.sct
Toolchain metadata     # .project/.wvproj, .uvproj, include paths, defines
```

规则：

- 不要将电路板引脚和应用程序逻辑放入厂商驱动程序目录。
- 使生成的用户文件保持精简且可替换；避免直接编辑厂商头文件。
- 在模板元数据中保存所有必需的包含路径、链接资源、预处理器定义、链接器脚本及启动代码文件。
- 除非有明确的仓库级迁移计划，否则使用官方示例的目录约定。

## 主循环规则

裸机 StdPeriph 风格模板：

1. 当系列使用 NVIC/PFIC 分组时，配置中断优先级分组。
2. 更新 `SystemCoreClock` 并初始化延时/调试输出。
3. 调试输出可用时，打印时钟和芯片 ID。
4. 配置时钟和引脚后，再初始化 GPIO 及功能外设。
5. 保持主循环显式可见；不要将阻塞式协议循环隐藏在无关的辅助函数之后。

BLE 模板：

- 按官方 BLE 示例使用的顺序初始化时钟、可选 DCDC、调试 UART、BLE 协议栈、HAL、GAP 角色、服务及应用程序状态。
- 调用 `Main_Circulation()`，并将其视为不会返回的函数。
- 在 `config.h` 中设置 `BLE_MEMHEAP_SIZE`、缓冲区长度/数量、休眠策略、TX 功率及连接数限制。

8051 模板：

- 写入受保护寄存器时使用安全模式序列。
- 明确使用 `data` 和 `xdata` 存储区。
- 仅在完成时钟、调试 UART 及外设状态配置后启用全局中断。
- 确保 ROM/代码大小限制不侵入任何预留的 ISP 引导加载程序区域。

## 链接器与存储器规则

链接器/分散加载文件是模板的关键资源。

规则：

- 不要在 Flash/RAM 大小、引导加载程序偏移、ITCM/DTCM、BLE RAM 预留或 IAP 布局不同的芯片之间共用链接器脚本。
- 对于 IAP 应用程序，应从 `Doc/IAP/wch-iap-ota-notes.md` 推导 Flash 起始地址和长度，而不是在各处硬编码 `0x1000`。
- 对于 CH32H417，应保留 V5F/V3F 链接器差异及引导加载程序偏移规则。
- CH32V USB/UART IAP 当前 APP 起点为 `0x5000`；CH32X USB/UART 为 `0x5000`、Host 可为 `0x6000`/`0x8000`；CH32H 的默认、USB/UART IAP 和 Host IAP 也各不相同，模板元数据必须保存准确示例路径和映像角色。
- CH32V407 默认链接为 576 KB Flash/136 KB RAM且保留 RAM 首 1 KB，并有 512 KB/200 KB 选项；不得继续使用 256 KB/1 MB、64 KB RAM 的概括值生成脚本。
- 对于 CH57x/CH58x/CH59x BLE 项目，更改堆大小前应对照 BLE 协议栈及芯片存储器布局验证 RAM 起始地址/长度。
- 对于 CH561/CH563，来源指向 Keil 分散加载及 ARM7 启动规则，但当前 SDK 未导入对应 EVT；不得据此生成未经验证的模板，也不能套用 CH32 RISC-V 链接器。
- 对于 CH5xx，应在项目选项中强制实施 Keil C51 内存模型、代码分组及 ISP 引导加载程序预留。

## 工具链元数据规则

MounRiver/Eclipse 风格项目：

- 保留 `.project`、存在时的 `.cproject`、`.wvproj`、链接资源路径、包含路径、构建定义及链接器脚本选择。
- `Ld`、`RVMSIS`、`Startup` 及 `StdPeriphDriver` 等链接资源必须指向所选系列正确的 EVT `SRC` 树。
- 缺少链接资源通常会导致未定义引用或缺少头文件。

Keil MDK/C51 项目：

- 保留 `.uvproj` 或 `.uvprojx`、目标器件、包含路径、分散加载文件、内存模型及 HEX 输出设置。
- 对于 C51，应明确设置内存模型及代码分组；生成的代码不应在 `data`、`idata`、`xdata` 及代码分组假设之间静默切换。

## 调试输出规则

- 每个模板选择一种调试输出路径：UART、SDI printf、类似半主机的调试方式或无输出。
- 在电路板元数据中保存调试 UART 实例、引脚、波特率及所需 GPIO 模式。
- 不要依赖调试输出来保证生产环境中的时序或低功耗行为。
- 如果调试引脚与 USB-PD、PIOC、KEYSCAN 或 SWD/SWDIO 重映射等功能冲突，应明确标出冲突。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| 从代码片段而非 EVT 项目起步 | 缺少启动代码、链接器、系统或项目元数据 | 首先复制最接近的官方示例。 |
| 芯片头文件或 `*_conf.h` 错误 | 构建使用错误的外设定义却成功，或在链接时失败 | 按准确系列选择头文件及驱动程序前缀。 |
| 缺少链接资源 | 出现未定义引用或缺少包含文件 | 保留 `.project` 链接资源路径。 |
| 在不同存储容量型号间复用链接器脚本 | Flash/RAM 重叠、引导加载程序被覆盖或 BLE 堆损坏 | 根据准确芯片及映像角色推导存储器配置。 |
| 遗漏 BLE `Main_Circulation()` | BLE 协议栈停止处理 | 保留 BLE 主循环模式。 |
| 将 CH32V 模板复制到 CH561/CH563 | 架构和启动模型错误 | 使用 ARM7 寄存器级 Keil 模板。 |
| 将 CH32 模板复制到 CH5xx | 架构、内存模型及中断语法错误 | 使用 Keil C51/8051 特定结构。 |
| 未配置调试 UART 引脚 | 无 `printf` 输出 | 对调试 UART 引脚及时钟建模。 |

## 模板元数据检查清单

未来的 CubeX/模板元数据应明确表示以下字段：

- 目标芯片、封装、架构、系列、准确的 EVT 示例来源及功能配置。
- 工具链类型、项目文件名、链接资源、包含路径、定义、启动代码文件、系统文件、链接器/分散加载文件。
- 存储器布局：Flash/RAM 起始地址和长度、引导加载程序/应用程序偏移、BLE 堆、栈/堆、ITCM/DTCM 或 DMA 可访问区域。
- 调试路径：UART/SDI/无输出、引脚、波特率、时钟及冲突说明。
- 主循环配置：裸机、BLE TMOS、RTOS、IAP 引导加载程序、USB 协议栈、Ethernet 协议栈或功能特定的调度程序。
- 板级钩子：LED、按钮、复位、BOOT 引脚、收发器/编解码器/传感器引脚、振荡器及电源轨。
- 验证来源：准确的 EVT 示例、RM/DS 章节、来源指南及成功的构建命令/工具链版本。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮尚未逐一对照所有 EVT 项目文件、编译器版本、启动汇编、链接器/分散加载文件、存储容量、调试探针或电路板型号进行验证。
- 在对照准确的目标 EVT 示例和工具链完成检查前，应将目录结构、项目文件名、包含路径、链接器值、调试引脚选择及主循环代码片段视为初步信息。
