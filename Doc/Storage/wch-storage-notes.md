# WCH 存储接口笔记

本文从 `Doc/Ref/wch-dev-skill` 提取可移动及高速存储相关指导，整理为适用于本仓库的规则，供后续 HAL 元数据、开发板模板及示例索引使用。

最终应以官方 EVT 示例、RM、DS、开发板原理图、存储卡/器件数据手册及当前仓库源码为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/sd_card.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/emmc_storage.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/hspi_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/emmc_storage.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/hspi_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/sdio_sdcard.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/sdio_sdcard.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/sdio_sdcard.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/qspi_flash.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`
- `CH32V307EVT/EXAM/SDIO/SDIO_eMMC/`
- `CH32V407EVT/EXAM/SDIO/SDIO_eMMC/`
- `CH32H47TEVT/EXAM/SDIO/SDIO_eMMC/`
- `CH32H47TEVT/EXAM/SDMMC/SDMMC_eMMC/`
- `CH32H47TEVT/EXAM/SDMMC/SDMMC_eMMC_DDR/`

## 接口覆盖范围

| 接口 | 来源笔记涉及的系列 | 主要约束 |
|---|---|---|
| 通过 SDIO/SDMMC 连接的 SD 卡 | CH32V307/CH32V407、CH32F 风格来源、CH32H417 | 卡检测/写保护取决于开发板；识别阶段必须以低速时钟启动，并在存储卡协商后切换。 |
| CH56x 存储示例中的 SD 卡 | CH569 / CH56x | 必须依据 CH56x 内存规则检查缓冲区位置及 DMA 可访问性。 |
| eMMC | CH56x、CH561/CH563、CH32V307、CH32V407、CH32H417 | 当前 EVT 包含 V307/V407 的 SDIO eMMC，以及 H417 的 SDIO eMMC、SDMMC eMMC 和 SDMMC eMMC DDR 路径；固定焊接式存储的总线宽度、分区、块长度、时序及初始化流程与可移动 SD 卡策略不同。 |
| HSPI | CH56x 和 CH561/CH563 来源笔记 | 具有 DMA/FIFO/时序要求的高速主从式接口；不得将其建模为普通低速 SPI。 |
| QSPI 外部闪存 | CH32H417 来源笔记 | 闪存命令集、空操作周期、地址宽度、四线使能及内存映射模式均取决于器件。 |

## 分类规则

- 将 SD 卡、eMMC、HSPI、QSPI、普通 SPI 闪存及 MCU 内部闪存划分为独立的元数据类别。
- SDIO/SDMMC 存储 API 需要包含存储卡状态、总线宽度、块大小、时钟、DMA 及文件系统集成字段。
- eMMC 模板不应假设存在卡检测或可移动介质行为。
- HSPI 应保留其专用 FIFO、DMA、时钟及角色配置，而不是并入通用 SPI 抽象。
- QSPI 内存映射模式属于外部存储器/总线设计决策；除非目标明确支持原地执行及相应缓存规则，否则不能替代内部程序闪存。

## SDIO 与 SD 卡规则

通用 SD 初始化顺序：

1. 按需使能 GPIO、SDIO/SDMMC、DMA 及卡检测时钟。
2. 将命令、时钟及数据引脚配置为正确的复用功能模式。
3. 高速传输前先使用安全的识别时钟启动。
4. 复位并识别存储卡，读取 CID/CSD，并确定容量/版本。
5. 仅在确认存储卡及开发板布线支持后选择总线宽度。
6. 仅在初始化完成后提高传输时钟。
7. 按 DMA 要求对齐数据缓冲区，并确保其在整个传输期间有效。
8. 处理存储卡移除、写保护、超时、CRC 及传输完成状态。

规则：

- 不得假设支持 4 位模式；许多开发板仅连接了 1 位模式所需线路。
- 使用 DMA 时，不得将扇区缓冲区放在生命周期较短的栈上。
- 将块设备访问与 FATFS 等文件系统集成分离。
- 将卡检测和写保护引脚视为开发板元数据，而非 MCU 功能。
- 确认目标系列是否要求 DMA 缓冲区位于特定 RAM 区域。

## eMMC 规则

eMMC 是焊接式托管 NAND，需要采用不同于可移动 SD 卡的策略：

- 显式建模总线宽度、电压、时钟、分区选择、启动分区用途及擦除组大小。
- 保留命令/响应超时处理；eMMC 忙状态持续时间可能长于简单寄存器轮询的预期。
- 块大小及寻址模式应与器件和初始化结果关联。
- 避免采用仅适用于可移动 SD 卡槽的断电重启假设。
- 若使用 eMMC 存储固件镜像，在定义启动/更新布局前应与 `Doc/IAP/wch-iap-ota-notes.md` 交叉核对。

当前 EVT 覆盖摘要：

| 系列 | 接口与模式 | 准确示例路径 | 已核对边界 |
|---|---|---|---|
| CH32V307 | SDIO eMMC | `CH32V307EVT/EXAM/SDIO/SDIO_eMMC/` | 示例注释和 `SD_EnableWideBusOperation(2)` 表明采用 8 位数据总线。 |
| CH32V407 | SDIO eMMC | `CH32V407EVT/EXAM/SDIO/SDIO_eMMC/` | 示例支持 `wmode=2` 的 8 位数据总线。 |
| CH32H417 | SDIO eMMC | `CH32H47TEVT/EXAM/SDIO/SDIO_eMMC/` | 示例连接 D0-D7，采用 8 位数据总线；这不是 SDMMC STR/strobe 路径。 |
| CH32H417 | SDMMC eMMC | `CH32H47TEVT/EXAM/SDMMC/SDMMC_eMMC/` | 示例为 8 位数据总线，并列出独立 `STR--PD3` 引脚；这是普通 SDMMC 示例，不等同于 DDR 示例。 |
| CH32H417 | SDMMC eMMC DDR | `CH32H47TEVT/EXAM/SDMMC/SDMMC_eMMC_DDR/` | 示例为 8 位 DDR，列出 `STR--PD3`，并要求按板卡调整命令、数据输入/输出调谐和时钟分频参数。 |

必须把“8 位数据总线”和“STR/strobe 信号”建模为不同字段：8 位表示 D0-D7 总线宽度，STR/strobe 是 SDMMC 的独立采样/时序信号。不得因示例同时出现二者而写成单一的“8bit+STR”能力，也不得把普通 SDMMC eMMC 与 SDMMC eMMC DDR 合并为同一种传输模式。

## HSPI 规则

HSPI 示例属于高速数据传输范畴，而非通用 SPI 代码片段。

规则：

- 显式描述主从角色、数据宽度、FIFO 阈值、DMA 通道及中断所有权。
- 根据所选时钟树及外部器件要求推导总线时序。
- 使用满足对齐要求的静态或长生命周期 DMA 缓冲区。
- 在对端器件有要求时使用流控制、就绪标志或数据包成帧机制。
- 未经 EVT 验证，不得将低速 SPI 阻塞传输辅助函数复用于 HSPI 高吞吐路径。

## QSPI 规则

QSPI 外部闪存支持取决于具体器件。

规则：

- 在开发板/器件元数据中存储闪存操作码表、地址宽度、空操作周期、线模式、页大小、擦除大小及状态寄存器布局。
- 严格按照闪存数据手册要求执行写使能、擦除、编程及等待忙状态序列。
- 仅在设置并验证闪存厂商定义的四线使能位后进入四线模式。
- 仅在验证命令时序、空操作周期、缓存行为及地址范围后启用内存映射模式。
- 将 QSPI 擦除/编程操作与内部闪存 HAL API 分离。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| 识别阶段过早提高 SD 时钟 | 存储卡无法初始化 | 以低速启动，仅在选卡后提高时钟。 |
| 假设采用 4 位 SD 总线布线 | 无法传输数据或出现 CRC 错误 | 记录开发板总线宽度及引脚映射。 |
| DMA 缓冲区位于栈或错误的 RAM 中 | 传输失败或数据损坏 | 在 DMA 可访问的内存中使用对齐的静态/全局缓冲区。 |
| 将 eMMC 当作可移动 SD 卡 | 检测、电源或分区处理错误 | 对固定介质行为及 eMMC 分区建模。 |
| 将 HSPI 并入通用 SPI | 缺失 FIFO/DMA/时序行为 | 将 HSPI 保留为专用高速接口。 |
| 在不同闪存芯片间照搬 QSPI 空操作周期 | 读取结果无效 | 以外部闪存数据手册为准。 |

## 元数据检查清单

后续存储模板应显式表示以下字段：

- 接口类别：SDIO/SDMMC SD 卡、eMMC、HSPI、QSPI 闪存或普通 SPI 闪存。
- 目标芯片、封装、时钟源、总线时钟、引脚、复用功能映射、DMA 通道、中断及内存区域。
- 开发板字段：卡检测、写保护、电源开关、电压、总线宽度、外部闪存/eMMC 型号及布线限制。
- 传输字段：块大小、扇区大小、擦除组大小、FIFO 阈值、对齐、超时及 CRC/错误策略。
- 集成字段：FATFS/块设备适配器、固件更新存储角色、启动分区用途及缓存/一致性约束。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮已抽查当前仓库 V307/V407/H417 上述五组 eMMC EVT 示例，确认接口类别、8 位数据线以及 H417 SDMMC 的 STR/strobe 布线，并区分普通与 DDR 示例。
- 尚未根据芯片实物、RM、DS、开发板原理图、具体 eMMC/QSPI 器件数据手册、文件系统移植或时序测量验证；API 细节、DMA 要求、命令序列、电气条件、速度等级及调谐参数仍须按目标器件核对。
