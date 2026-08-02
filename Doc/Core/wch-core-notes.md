# WCH 内核、存储器、启动代码与工具链笔记

本文从 `Doc/Ref/wch-dev-skill` 中提取内核架构、存储器布局、启动代码/链接器、中断及 Flash 规则，整理为适用于本仓库的笔记。

范围：

- RISC-V WCH 系列：CH57x、CH58x/CH59x、CH32V、CH32X、CH569、CH32H。
- ARM 系列：来源尚未由当前 SDK 验证的 CH32F Cortex-M3，以及 CH561/CH563 ARM7TDMI。
- 8051 系列：CH543 至 CH559。
- 重点：头文件/工具链、链接器布局、存储区域、段放置、中断属性、Flash 擦除/写入规则及 IAP 偏移。

官方 EVT 示例、启动代码文件、链接器脚本、RM 及 DS 仍是最终依据。

## 来源文件

- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`

## 架构与工具链矩阵

| 架构 | 系列 | 典型头文件 | 构建工具 | 链接器或存储器描述 |
|---|---|---|---|---|
| RISC-V BLE | CH57x、CH58x/CH59x | `CH57x_common.h`、`CH58x_common.h`、`CH59x_common.h` | MounRiver | `Ld/Link.ld` |
| RISC-V StdPeriph | CH32V103、CH32V20x、CH32V307、CH32V407 | `ch32v10x.h`、`ch32v20x.h`、`ch32v30x.h`、`ch32v4x7.h` | MounRiver | `Ld/Link.ld` |
| RISC-V 低资源 StdPeriph | CH32V003、CH32V006、CH32L103、CH32M030 | `ch32v00x.h`、`ch32l103.h`、`ch32m030.h` | MounRiver | `Ld/Link.ld` |
| RISC-V USB-PD | CH32X035、CH32X315、CH641、CH643 | `ch32x035.h`、`ch32x3x5.h`、`ch641.h`、`ch643.h` | MounRiver | `Ld/Link.ld` |
| RISC-V 高性能系列 | CH32H417 | `ch32h417.h` | MounRiver | `Link_v5f.ld`、内核特定的链接器文件 |
| RISC-V Ethernet | CH569 | `CH56x_common.h` | MounRiver | `Ld/Link.ld` |
| ARM Cortex-M3 | CH32F103、CH32F20x（当前 SDK 未导入，来源未验证） | `ch32f10x.h`、`ch32f20x.h` | Keil MDK 或 MounRiver | `.sct` 或 `Ld/Link.ld` |
| ARM7TDMI | CH561、CH563（当前 SDK 未导入，来源未验证） | `CH561SFR.H`、`CH563SFR.H`、`SYSFREQ.H` | Keil MDK | 分散加载文件 `.sct` |
| 8051 | CH543 至 CH559 | `CH5xx.H` 或芯片特定的 `CH554.H` 风格头文件 | Keil C51 或 SDCC | 8051 存储空间，无标准 GCC 链接器脚本 |

## 包含文件与启动规则

- 不要混用不同系列的头文件。除非 EVT 证明文件共享，否则 CH32V10x 驱动程序不应包含 CH32V30x 或 CH32X 头文件。
- BLE 项目应先包含 BLE 协议栈头文件和 HAL 头文件，再包含应用程序角色头文件。
- CH32V/CH32F/CH32X/CH32H StdPeriph 项目通常包含芯片根头文件及外设头文件，或系列的汇总头文件 `*_conf.h`。
- CH561/CH563 代码为寄存器级代码，使用 `R8_`、`R16_`、`R32_` 等 SFR 宏。
- 8051 代码依赖存储器限定符及编译器特定的中断声明。

## 标准存储器模型

多数 WCH RISC-V 和 ARM Cortex-M 风格器件采用以下概念布局：

| 区域 | 典型起始地址 | 用途 |
|---|---|---|
| Flash | `0x00000000` | 向量表、代码、常量、已初始化数据的加载映像 |
| SRAM | `0x20000000` 或系列特定的 RAM 基地址 | `.data`、`.bss`、堆、栈 |
| 外设寄存器 | 许多 CH32 风格器件位于 `0x40000000` 范围 | APB/AHB 外设 |

例外：

- CH572 为 240 KB Flash，RAM 位于 `0x20000000`、长度 12 KB；CH573 为 448 KB Flash，RAM 位于 `0x20003800`、长度 18 KB。
- CH32M030 为 QingKe RISC-V V3B，当前默认链接脚本为 64 KB Flash、12 KB RAM。
- CH32H417 将 ITCM 与 DTCM 分开：ITCM 位于 `0x200A0000`，DTCM 位于 `0x200C0000`。
- CH561/CH563 的 SFR 位于 `0x00400000`，SRAM 位于 `0x00808000`。
- 8051 具有独立的 code、data、idata、xdata、bit 及 SFR 空间。

## 段放置

常见 GCC 链接器段：

| 段 | 位置 | 含义 |
|---|---|---|
| `.init` | Flash | `main` 之前的启动及初始化代码 |
| `.text` | Flash | 应用程序代码及只读数据 |
| `.data` | RAM，从 Flash 加载 | 启动时复制的已初始化全局/静态变量 |
| `.bss` | RAM | 启动时清零的零初始化全局/静态变量 |
| `.stack` | RAM 顶部或显式区域 | 调用栈及中断上下文 |
| `.highcode` | 系列特定的 RAM 或高速存储器 | 对延迟敏感的代码及中断处理程序 |

规则：

- 验证当前链接器/启动代码组合是否确实将 `.highcode` 复制到 RAM 或从 RAM 执行。
- CH57x 来源笔记显示 `.highcode` 从 Flash 加载并在 RAM 中执行，用于快速中断路径。
- CH58x/CH59x 来源笔记将 `.highcode` 列为中断处理程序的专用链接器段。
- CH32H417 使用 ITCM（`RAM_CODE`）放置 `.highcode` 及性能关键例程。
- CH32V003 来源笔记指出 `.highcode` 可由 RAM 承载，而某些相关低成本型号可能有所不同。

## 中断属性

| 系列或架构 | 典型中断声明 | 说明 |
|---|---|---|
| RISC-V WCH | `__attribute__((interrupt("WCH-Interrupt-fast")))` | CH32V/CH32X/CH57x/CH58x 风格示例使用。 |
| RISC-V BLE 快速处理程序 | 快速中断加 `__attribute__((section(".highcode")))` | 仅在验证链接器/启动代码后使用。 |
| ARM Cortex-M3 CH32F | 来源笔记中的示例使用 WCH 快速属性 | 对照实际编译器及 EVT 启动代码验证。 |
| ARM7TDMI CH561/CH563 | `__irq` 风格处理程序 | 寄存器级代码，Keil MDK 分散加载/启动代码。 |
| 8051 | `__interrupt(n)` 或编译器等效语法 | 向量号很重要；语法因编译器而异。 |

## Flash 擦除与写入规则

不能将 Flash 视为可按字节重写的 RAM。写入前务必验证擦除粒度。

| 系列 | 来源资料中的擦除/写入说明 |
|---|---|
| CH57x | 256 字节扇区；局部更新需要读取-修改-擦除-写入。 |
| CH58x/CH59x | 256 字节擦除扇区；写入前必须擦除；字对齐很重要。 |
| CH32V103 | 1KB 页擦除。 |
| CH32V20x/CH32V307/CH32V407 | 来源笔记中为 4KB 页擦除。 |
| CH32V003 | 来源笔记中擦除单元为 64 字节。 |
| CH32V006/CH32L103 | 来源笔记中擦除单元为 1KB。 |
| CH32H417 | 4KB 页擦除，4 字节编程单元。 |
| CH561/CH563 | 4KB 擦除块，最小写入单位为 4 字节；Data-Flash 独立。 |
| CH5xx 8051 | 访问 DataFlash 需要安全模式和中断屏蔽；代码 Flash 和 DataFlash 规则因芯片而异。 |

页内局部数据的读取-修改-写入规则：

1. 将整个页或扇区复制到 RAM。
2. 在 RAM 中修改目标字节。
3. 擦除整个页或扇区。
4. 写回整个页或所需的对齐字。

## IAP 与启动偏移

IAP 布局因系列而异。不要盲目复用偏移。

| 系列 | 来源笔记中的模式 |
|---|---|
| CH57x CH572 | 引导加载程序位于 `0x00000000`，应用程序通常位于 `0x00001000`；OTA 双映像布局划分映像 A/B 及 DataFlash 标志。 |
| CH579 | 来源笔记中引导加载程序位于 Flash 顶部；应用程序从 `0x00000000` 开始。 |
| CH58x/CH59x | IAP 应用程序通常偏移至 `0x00001000`；Flash/RAM 大小因芯片而异。 |
| CH32V 通用系列 | 当前 USB/UART IAP 的 APP 从 `0x00005000` 开始，即预留 20 KB；Host IAP 可使用其他偏移（例如 CH32V307 为 `0x00006000`），必须以所选示例链接脚本为准并重定位向量表。 |
| CH32V 低成本系列 | 来源笔记中通常有 4KB 启动区域；IAP 应用程序从 `0x00001000` 开始。 |
| CH32H417 | 默认 V3F 从 `0x00000000`、默认 V5F 从 `0x00010000` 开始；USB/UART IAP APP 的 V3F 为 `0x00006000`，Host IAP APP 的 V3F/V5F 分别为 `0x0000C000`/`0x0001C000`。 |
| CH561/CH563 | Flash 起始处通常有 4KB IAP 区域；需验证分散加载文件。 |
| 8051 | 引导加载程序位置取决于芯片，例如来源笔记中的 CH552 和 CH559 不同。 |

所有 IAP 设计的验证检查清单：

- 从 EVT 链接器或分散加载文件确认引导加载程序大小及起始地址。
- 确认应用程序起始地址及向量表重定位行为。
- 确认 Flash 页大小及擦除/写入 API。
- 确认 NV/DataFlash/SNV/OTA 标志区域不与应用程序代码或备份映像重叠。
- 确认栈指针有效性检查使用正确的 RAM 范围。

## CH32H417 内核特定笔记

CH32H417 需要特殊处理，因为来源笔记描述了多个内核和 RAM 区域。

要点：

- 当前 `CH32H47TEVT` 默认链接脚本中，V3F Flash 从 `0x00000000` 开始，V5F 从 `0x00010000` 开始；不能据此禁止普通 V3F 应用从零启动。
- USB/UART IAP 的 V3F APP 从 `0x00006000` 开始；USB Host IAP 的 V3F/V5F APP 分别从 `0x0000C000`/`0x0001C000` 开始。
- 位于 `0x200A0000` 的 ITCM RAM 用于快速执行代码。
- 位于 `0x200C0000` 的 DTCM RAM 用于数据、bss、堆及栈。
- V3F 使用另一套 RAM 模型：`RAM_CODE` 位于 `0x20100000`、长度 64 KB，数据 RAM 位于 `0x20110000` 区域；默认脚本保留起始 256 字节作为 `RAM_LOAD`，其余 448 KB 区域用于 `.data`、`.bss`、堆和栈。
- `ch32h417.h` 和 `startup_ch32h417_v3f.S`/`startup_ch32h417_v5f.S` 在当前 EVT 中共享覆盖 CH32H415/416/417；双核协作还需建模 IPC、HSEM 以及 `SysTick0`/`SysTick1`，不能按单核 CH32V 模板处理。
- 链接器文件特定于内核；未检查当前内核前，不要应用通用 CH32V 链接器脚本。

## ARM7TDMI CH561/CH563 笔记

来源笔记中的 CH561/CH563 并非 StdPeriphDriver 器件。当前 SDK 未导入对应 EVT，以下仅记录来源线索，不能视为仓库已验证实现。

规则：

- 使用寄存器级 SFR 宏，而非 CH32V/CH32F StdPeriph API。
- 依赖时钟或 RAM 布局前，配置 `FREQ_SYS` 及存储器相关选项。
- SRAM 大小可能取决于 `SYSFREQ.H` 中的 `MEM_DATA`。
- Flash 擦除单元为 4KB。
- 中断处理程序使用 Keil/ARM7 约定，而非 GCC RISC-V 属性。
- 来源笔记中 CH563 具有 CH561 所没有的 USB 和 GPIO 端口 D 功能。

## 8051 笔记

8051 存储器并非平坦的 ARM/RISC-V 地址空间。

存储空间：

| 限定符 | 空间 | 用途 |
|---|---|---|
| `data` | 直接寻址内部 RAM | 小型、快速变量。 |
| `idata` | 间接寻址内部 RAM | 更多内部 RAM，用于类似栈的用途。 |
| `xdata` | 通过 MOVX 访问的外部数据 RAM | 大型缓冲区、USB 缓冲区。 |
| `code` | 代码存储器 | 常量表及只读数据。 |
| `bit` | 位寻址区域 | 布尔标志。 |

规则：

- 写入受保护寄存器前使用安全模式：`SAFE_MOD = 0x55; SAFE_MOD = 0xAA;`，随后写入，再执行 `SAFE_MOD = 0x00;`。
- 按要求在 Flash/DataFlash 写入期间禁用中断（`EA = 0`，随后恢复）。
- 按 EVT 及端点 DMA 寄存器的要求保持 USB 缓冲区地址对齐。
- 大型缓冲区应使用 `xdata`，避免耗尽 `data`。

## 跨系列内核检查清单

添加新的 HAL 系列适配器或模板前，验证：

- 准确的芯片型号及封装。
- 架构及编译器/工具链。
- 正确的根头文件及外设头文件命名。
- 启动代码文件及向量表实现。
- 链接器/分散加载文件中的 Flash 和 RAM 大小。
- 栈和堆大小。
- `.highcode` 或等效快速代码的放置。
- 中断声明语法。
- Flash 擦除单元、写入单元、解锁序列及保护规则。
- 需要引导加载程序支持时的 IAP 偏移及向量重定位。
- 该系列使用 StdPeriphDriver 还是寄存器级 SFR 访问。

## 验证状态

- 仅提取自 `wch-dev-skill` Markdown。
- 生成代码前，必须对照 `Doc/DS`、`Doc/RM` 及仓库中的 EVT 链接器/启动代码文件检查准确地址和大小。
- 下一轮验证应将上述笔记与各已导入 EVT 树中的 `Ld/Link.ld`、`.sct`、启动代码文件及 Flash 驱动程序实现进行比较。
