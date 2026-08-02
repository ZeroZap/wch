# WCH RTOS 集成笔记

本文按当前仓库实际 EVT 源码重新核对 RTOS 工程，整理为适用于未来项目模板、HAL 集成及示例索引的规则。`Doc/Ref/wch-dev-skill` 只作为补充索引，不再作为支持矩阵的主要依据。

范围：

- 当前 EVT 中实际存在的 FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny 及 uC/OS-II 工程。
- 重点核对 V20x、V407、X315、H417 和 CH583/585/592/595 的工程目录、port、tick 与中断入口。
- 围绕启动、tick、中断优先级、栈、堆、链接器布局及 BLE/TMOS 边界的通用集成约束。
- 本轮不定义新的 RTOS 抽象 API，因为来源笔记主要索引示例，而非记录调度程序 API。

官方 EVT 示例、RTOS 上游移植、启动代码文件、链接器脚本、RM、DS 及当前仓库源码仍是最终依据。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/example_list.md`
- `Doc/BLE/wch-ble-notes.md`
- `Doc/Core/wch-core-notes.md`

## 当前 EVT 的 RTOS 支持矩阵

| 目标 | 当前 EVT 中实际存在的 RTOS 工程 | 示例根目录或命名 | 说明 |
|---|---|---|---|
| CH32V103 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_M、TencentOS Tiny | `CH32V103EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS,TencentOS}/` | 四类均有实际工程，具体目录后缀与新系列略有不同。 |
| CH32V20x | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny | `CH32V20xEVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS,TencentOS}/` | 四类均有实际工程，旧索引中“未列出 V20x”不能作为不支持依据。 |
| CH32V307 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny | `CH32V307EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS,TencentOS}/` | 四类均有实际工程。 |
| CH32V407 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny | `CH32V407EVT/EXAM/CPU/OS/` | 四类均有实际工程。 |
| CH32X035 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny | `CH32X035EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS,TencentOS}/` | 四类均有实际工程。 |
| CH32X315 | FreeRTOS、RT-Thread、LiteOS_m、TencentOS Tiny | `CH32X315EVT/EXAM/CPU/OS/` | 四类均有实际工程；目录名为 `LiteOS_m`，不是 `HarmonyOS/LiteOS_m`。 |
| CH32L103 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny | `CH32L103EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS,TencentOS}/` | 四类均有实际工程。 |
| CH32M030 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m、TencentOS Tiny | `CH32M030EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS,TencentOS}/` | 四类均为 QingKe RISC-V V3B 工程；不得与当前未导入的 CH32F ARM 移植合并。 |
| CH32H417 | FreeRTOS | `CH32H47TEVT/EXAM/CPU/OS/FreeRTOS/FreeRTOS_Core/{V3F,V5F}/` | V3F、V5F 各有独立工程和 port；不能再标记为“来源未列出、未经 EVT 验证”。 |
| CH583 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m | `CH583EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS}/` | 三类均有工程。 |
| CH585 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m | `CH585EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS}/` | 三类均有工程。 |
| CH592 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m | `CH592EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS}/` | 三类均有工程。 |
| CH595 | FreeRTOS、RT-Thread、HarmonyOS LiteOS_m | `CH595EVT/EXAM/{FreeRTOS,RT-Thread,HarmonyOS}/` | 三类均有工程；FreeRTOS SysTick 使用 fast interrupt port。 |
| CH32F103 / CH32F20x | 来源笔记列出 FreeRTOS、RT-Thread、LiteOS_M 或 HarmonyOS、TencentOS Tiny | 当前仓库未导入对应 EVT | 仅保留来源索引，导入实际 SDK 后再验证 ARM 工具链、startup 和 port。 |
| CH561/CH563 ARM7TDMI | 仅 CH563 有 uC/OS-II | `UCOS/` | CH561 表中无 RTOS；CH563 表中有 uC/OS-II。 |
| CH5xx 8051 | 已索引的来源笔记中未列出 RTOS 示例 | 不适用 | 除非特定芯片的 EVT 另有证明，否则按裸机处理。 |

## 概念边界

应将以下调度模型分开。

| 概念 | 用途 | 边界规则 |
|---|---|---|
| 裸机超级循环 | 带中断及轮询的直接主循环 | 默认不要向简单外设示例添加 RTOS 依赖。 |
| BLE TMOS | CH57x/CH58x/CH59x BLE 来源中的 BLE 协议栈事件循环、定时器及消息分发 | `Main_Circulation()` 和 `TMOS_SystemProcess()` 是 BLE 协议栈调度要求，并非通用 RTOS 移植。 |
| FreeRTOS / RT-Thread / LiteOS_m / TencentOS Tiny | 具有任务、tick、同步及堆/栈设置的通用 RTOS 内核 | 使用准确芯片的厂商 EVT 移植及启动代码/链接器文件。 |
| uC/OS-II | ARM7TDMI 来源索引中的 CH563 RTOS 移植 | 不要将 CH32 RISC-V FreeRTOS 假设应用于 CH563。 |

规则：

- HAL 模板应将调度程序类型公开为元数据：裸机、BLE TMOS、FreeRTOS、RT-Thread、LiteOS_m、TencentOS Tiny 或 uC/OS-II。
- BLE TMOS 事件处理程序必须返回未处理的事件位；处理一个事件后返回零会丢弃其他待处理事件。
- BLE 项目必须调用 `Main_Circulation()` 或等效协议栈循环；仅使用 `__WFI()` 休眠不会处理 BLE 事件。
- 通用 RTOS 移植需要自己的 tick、上下文切换、中断优先级、堆及任务栈配置。不要从 TMOS 代码片段推断这些细节。

## 项目选择规则

以最接近的官方示例为起点。

规则：

- 尽可能选择同一芯片系列及同一工具链的 RTOS 示例。
- 当启动、向量、Flash 或 RAM 布局不同时，应优先使用准确芯片的 EVT 项目路径，而非同族系列示例。
- 验证前，保留 EVT 示例中的 RTOS 移植文件、启动代码文件、链接器脚本及中断处理程序。
- 将 RTOS 内核配置文件置于模板控制之下，不要隐藏在生成的应用程序代码中。
- 如果来源示例列表未列出某芯片的 RTOS，除非 RM/EVT 确认其不存在，否则应将支持情况标记为未知，而非不支持。

各系列的目录命名并不统一：

- `RT-Thread`、`RT_Thread` 及 `rt-thread` 均出现在来源路径或表格中。
- `HarmonyOS`、`HarmonyOS/LiteOS_m` 及 `LiteOS_M` 出现在来源路径或表格中。
- `TencentOS`、`TencentOS/TencentOS` 及 `TencentOS_Tiny` 出现在来源路径或表格中。
- `FreeRTOS` 可能作为类别或嵌套示例目录出现。

## 启动、Tick 与中断规则

RTOS 启动必须与目标启动代码及中断模型保持一致。

规则：

- 保留 `Doc/Core/wch-core-notes.md` 中架构特定的中断属性，例如系列使用的 WCH 快速中断属性。
- 严格按照 EVT 移植的预期配置 RTOS tick 源。未检查 RTOS 移植前，不要复用裸机 SysTick 延时设置。
- 除非 EVT 示例这样做，否则避免将同一个定时器同时用于 RTOS tick 和应用程序时基。
- 在 StdPeriph 风格项目中启用 RTOS 管理的中断前，配置中断优先级分组。
- 除非 RTOS 移植记录了 ISR 安全的变体，否则不要从 ISR 调用阻塞式 RTOS API。
- 保留移植中的上下文切换及向量表代码。仅在检查准确内核及编译器 ABI 后手动编辑。

### 当前 port 的关键差异

| 目标/代表工程 | SysTick 中断属性与入口 | ISR 栈/核差异 | Tick 与优先级证据 |
|---|---|---|---|
| CH32V20x FreeRTOS | `SysTick_Handler` 使用普通 `__attribute__((interrupt()))`，不是 fast interrupt | `port.c` 定义 `xISRStackTop`，`portASM.S` 在 ISR 调用前切换到独立 ISR 栈 | `FreeRTOSConfig.h` 为 500 Hz；`User/main.c` 配置 `NVIC_PriorityGroup_1` |
| CH32V407 FreeRTOS | 普通 RISC-V interrupt port | 使用该系列自己的 V4 port，不能替换成 CH595 fast port | `FreeRTOSConfig.h` 为 500 Hz；HarmonyOS `User/target_config.h` 为 1000 Hz |
| CH32H417 V3F/V5F FreeRTOS | 两核均使用普通 `__attribute__((interrupt()))` | V3F tick 入口为 `SysTick0_Handler`，V5F 为 `SysTick1_Handler`，各自有独立工程 | 两核 `FreeRTOSConfig.h` 均为 500 Hz |
| CH595 FreeRTOS | `SysTick_Handler` 使用 `__attribute__((interrupt("WCH-Interrupt-fast")))` | QingKe V3C fast interrupt port；不能照搬 V20x/H417 的普通 interrupt 声明 | `FreeRTOSConfig.h` 为 500 Hz；RT-Thread `RT_TICK_PER_SECOND` 和 HarmonyOS target config 为 1000 Hz |

500 Hz 是当前多个 FreeRTOS 示例的代表性配置，不是所有 RTOS、所有芯片的统一要求。中断优先级分组也应以工程为准：例如 V20x FreeRTOS 和 V20x HarmonyOS 的 `main.c` 明确调用 `NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1)`；不能仅因其他工程未在同一位置调用就补入未经 port 要求确认的配置。

与低功耗的交互：

- 裸机示例通常直接使用 `__WFI()` 或休眠模式；RTOS 项目则需要空闲钩子或无 tick 空闲策略。
- 将 BLE TMOS 休眠及 BLE 协议栈时序与通用 RTOS 调度程序结合前，必须进行验证。
- 喂狗应放在职责明确的任务、空闲钩子或系统监控器中，而不是分散在无关任务中。

## 栈、堆与链接器规则

来源笔记中最突出的 RTOS 陷阱是栈压力。

规则：

- 不要将大型缓冲区放在任务栈上。适当时使用静态/全局缓冲区或显式 RTOS 堆分配。
- 根据实际调用深度、库使用情况、ISR 嵌套及协议栈要求确定每个任务栈的大小。
- 将 RTOS 堆、C 堆、BLE 堆、USB 缓冲区、Ethernet 缓冲区、DMA 缓冲区及应用程序栈作为独立的预算项。
- 增加 RTOS 堆或任务数量前，验证链接器 RAM 长度及段放置。
- RTOS 移植支持时启用栈溢出检查。
- 可用时在调试构建中添加高水位线或运行时栈诊断。

跨文档存储器约束：

- BLE 笔记定义 BLE 堆及 TMOS 事件循环要求。
- USB 笔记定义端点缓冲区及控制器可访问存储器要求。
- Ethernet 笔记定义 DMA 描述符及数据包缓冲区。
- IAP 笔记定义引导加载程序/应用程序偏移，这些偏移可能改变 RTOS 映像的可用 Flash。

## 同步与驱动程序规则

RTOS 集成会改变驱动程序归属及阻塞行为。

规则：

- 使用互斥量或单一归属任务保护共享外设。
- 使用队列、消息缓冲区或事件标志将数据从 ISR 传递给任务。
- 尽量减少 ISR 工作：清除标志、移动少量数据、唤醒任务并返回。
- 仅在验证超时及 ISR 唤醒行为后，才用 RTOS 等待替换忙等待轮询。
- 在整个传输期间保持 DMA 缓冲区对齐且有效；不要在生命周期短暂的任务栈上分配 DMA 缓冲区。
- 对于 UART/USB/Ethernet 桥接，使用环形缓冲区及背压，而非在回调中阻塞。

## BLE TMOS 交叉参考

BLE 来源笔记包含 TMOS 事件语义，这些语义与 BLE 和其他调度程序的混合使用有关。

规则：

- `Main_Circulation()` 必须维持 BLE 协议栈事件处理。
- 在基于 TMOS 的示例中，每个 BLE 角色或多连接项目可能需要不同的任务 ID。
- TMOS `ProcessEvent` 处理程序应通过返回 `events ^ HANDLED_EVT` 或等效逻辑，仅清除已处理的位。
- GATT/GAP 操作通常接收 TMOS 任务 ID，以便协议栈消息返回正确的处理程序。
- 如果在 BLE 协议栈示例外围添加通用 RTOS，应定义哪个上下文负责 BLE 协议栈调用，以及如何为 TMOS 处理提供服务。

有关 BLE 特定的角色、堆及事件循环细节，请参见 `Doc/BLE/wch-ble-notes.md`。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| 将 TMOS 视为通用 RTOS | BLE 项目中的调度程序假设错误 | 将 TMOS 与 RTOS 元数据分开。 |
| BLE 示例中遗漏 `Main_Circulation()` | BLE 事件、定时器及消息停止处理 | 除非使用经过验证的集成层进行移植，否则保留 BLE 协议栈事件循环。 |
| TMOS 处理程序处理一个事件后返回零 | 其他待处理事件丢失 | 仅返回未处理的事件位。 |
| 复用不同芯片的 RTOS 示例 | 启动代码、向量、tick、RAM 或链接器布局错误 | 从准确芯片的 EVT 示例起步。 |
| 任务内存在大型局部缓冲区 | 栈溢出、硬故障、变量损坏 | 使用静态/全局缓冲区，或增加经过测量的任务栈大小。 |
| 共享外设但未明确归属 | 竞争条件及传输损坏 | 使用单一归属任务或 RTOS 同步原语。 |
| 在 ISR 或回调中阻塞 | 错过截止时间或死锁 | 使用 ISR 安全的唤醒原语将工作推迟到任务中。 |
| 忽略 IAP/应用程序偏移 | RTOS 映像与引导加载程序或标志页重叠 | 对 RTOS 应用程序链接器文件使用 `Doc/IAP/` 布局规则。 |

## 模板元数据检查清单

未来的 RTOS 项目模板应明确表示以下字段：

- 目标芯片、内核、工具链、启动代码文件、链接器/分散加载文件及向量表机制。
- 调度程序类型：裸机、BLE TMOS、FreeRTOS、RT-Thread、LiteOS_m、TencentOS Tiny 或 uC/OS-II。
- RTOS 示例根目录及准确的 EVT 来源路径。
- Tick 源、tick 频率、中断优先级规则及上下文切换中断/向量要求。
- RTOS 堆大小、各任务栈大小、C 堆/栈、BLE 堆、USB/Ethernet/DMA 缓冲区预留。
- ISR 安全的 API 规则及任务唤醒机制。
- 低功耗集成模型：空闲钩子、无 tick 空闲、BLE 休眠或裸机休眠。
- 调试诊断：栈溢出钩子、malloc 失败钩子、运行时统计、断言、UART 日志。
- 跨功能约束：BLE TMOS 循环、IAP 偏移、USB 端点存储器、Ethernet DMA 及喂狗。

## 验证状态

- 已按当前 EVT 目录和工程文件核验上述支持矩阵，不再只依赖 `Doc/Ref/wch-dev-skill` 的二手索引。
- 已核对代表性 FreeRTOS/RT-Thread/HarmonyOS/TencentOS 工程、FreeRTOS port、SysTick handler、ISR 栈、tick 配置及优先级分组调用。
- 已确认 CH595 使用 fast interrupt，V20x 使用普通 interrupt 并切换独立 ISR 栈，H417 使用普通 interrupt 且 V3F/V5F 分别使用 SysTick0/SysTick1。
- 本轮是静态源码与工程结构核验，未逐工程编译、烧录或进行硬件运行验证；具体产品目标仍应基于准确 EVT 工程完成编译和板级验证。
