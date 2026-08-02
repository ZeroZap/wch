# wch-dev-skill Markdown 消化计划

本文档指导将 `Doc/Ref/wch-dev-skill` 中的 Markdown 知识持续提取到 `Doc/` 下适当的文档区域。

`Doc/Ref/wch-dev-skill` 是源知识库。此阶段不要将其删除。目标是逐步消化其中的 `.md` 文件，将其转化为仓库专用说明、索引和 HAL 设计参考，同时以官方 RM / DS / EVT 源文件作为最终事实依据。

## 目标

- 保留 `Doc/Ref/wch-dev-skill`，作为原始参考输入。
- 将有用的 `.md` 内容提取到 `Doc/` 下主题明确的文档中。
- 将通用 AI 技能指南转化为仓库专用的 WCH HAL 说明。
- 如果简洁、可追溯的摘要更合适，则避免逐字复制大型文档。
- 保留指回 `Doc/Ref/wch-dev-skill/...` 的源文件链接，以便追溯。
- 建立支持 HAL 统一和未来生成 CubeX 元数据的长期文档结构。

## 源资料

- `Doc/Ref/wch-dev-skill/SKILL.md`：芯片系列路由、全局 WCH 规则、BLE 规则、工作流和陷阱。
- `Doc/Ref/wch-dev-skill/AGENTS.md`：编码约定、头文件包含模式、主循环模板和检查清单。
- `Doc/Ref/wch-dev-skill/chips/*/recipes/*.md`：按芯片系列和外设划分的场景指南。
- `Doc/Ref/wch-dev-skill/chips/*/resources/*.md`：API 参考、陷阱、内存布局和示例索引。

## 目标文档区域

优先使用现有目录。仅当主题不适合任何现有区域时才创建新目录。

| 源主题 | 目标区域 | 说明 |
|---|---|---|
| BLE 初始化、GAP/GATT、配置、BLE 示例 | `Doc/BLE/` | 将 CH57x 和 CH58x/CH59x 的 BLE 规则提取为主题明确的 BLE 说明。 |
| RISC-V 内核、中断属性、启动、链接器、内存布局 | `Doc/Core/` | 与内核/工具链参考资料合并，并明确保留架构差异。 |
| 以太网示例和陷阱 | `Doc/ETH/` | 提取 CH569、CH32V307、CH32F20x、CH561/CH563、CH32H417 以太网说明。 |
| USB 设备/主机/USB3 说明 | `Doc/USB/` | 开始提取 USB 时创建。 |
| USB-PD、Type-C、PIOC | `Doc/USBPD/` | 开始提取 USB-PD 时创建。 |
| 通用 GPIO/RCC/UART/SPI/I2C/ADC/定时器/闪存/DMA 模式 | `Doc/HAL/` | 创建为 HAL 规范化说明，而不是原始芯片示例。 |
| 芯片系列路由、内存映射、示例、陷阱 | `Doc/Family/` | 主动规范化某个芯片系列时，创建该系列的说明。 |
| 新项目模板、工具链元数据、最小主循环 | `Doc/Templates/` | 开始提取项目模板时创建。 |
| IAP/OTA/引导加载程序/链接器偏移 | `Doc/IAP/` | 开始提取 IAP 时创建。 |
| FreeRTOS/RT-Thread/HarmonyOS/UCOS 参考资料 | `Doc/RTOS/` | 开始提取 RTOS 时创建。 |
| 显示、HMI 和专用外设 | `Doc/HMI/` | 开始提取显示/HMI 时创建。 |
| CAN、I2S、SAI、DVP 摄像头流接口 | `Doc/IO/` | 开始提取 IO/媒体时创建。 |
| SD/SDIO/eMMC/HSPI/QSPI 存储接口 | `Doc/Storage/` | 开始提取存储接口时创建。 |
| 低功耗、RTC、看门狗、模拟、CRC/RNG 实用功能 | `Doc/System/` | 开始提取系统实用功能时创建。 |
| ECDC、CRC、RNG、安全边界 | `Doc/Security/` | 开始提取安全和密码学内容时创建。 |

## 提取规则

- 提取过程中不要删除 `Doc/Ref/wch-dev-skill`。
- 不要将 `wch-dev-skill` 视为最终权威依据。应与 `Doc/DS`、`Doc/RM` 和 `*EVT` 源文件交叉核对。
- 优先采用摘要、表格和 HAL 相关结论，而不是整篇复制操作指南。
- 每份提取后的说明都应包含“源文件”章节，并列出原始 `Doc/Ref/wch-dev-skill/...` 路径。
- 将芯片特定行为与跨芯片系列的 HAL 设计规则分开。
- 如果操作指南描述了可运行代码，应先将其映射到最接近的 EVT 示例路径，再转化为仓库指南。
- 生成的目标文档应足够精简，以便按芯片系列或主题维护。

## 保留的开发规则

这些规则应在相关时纳入目标文档。

### 芯片系列路由

| 芯片或系列 | 源系列 |
|---|---|
| CH57x, CH572, CH573, CH579 | `ch57x` |
| CH58x, CH59x, CH583, CH585, CH592, CH595 | `ch58x-ch59x` |
| CH32V103, CH32V20x, CH32V307, CH32V407 | `ch32v-general` |
| CH32V003, CH32V006, CH32L103 | `ch32v-lowcost` |
| CH32F103, CH32F20x | `ch32f-arm`，当前仓库未导入对应 EVT |
| CH32M030 | 上游曾错误归入 `ch32f-arm`；当前 SDK 已验证为 QingKe RISC-V V3B，应独立路由 |
| CH32X035, CH32X315, CH641, CH643 | `ch32x-usbpd` |
| CH569 | `ch56x-ethernet` |
| CH32H417 | `ch32h-highperf` |
| CH561, CH563 | `ch561-ch563` |
| CH543 至 CH559 | `ch5xx-8051` |

### 通用 WCH 规则

- 在 HAL 设计中使用 WCH API 之前，应对照 EVT 源文件、头文件、示例、RM 或 DS 进行验证。
- 访问外设前启用外设时钟。
- 初始化外设前配置 GPIO 模式和复用功能。
- 写入闪存前，按照正确的页或扇区大小执行擦除。
- 不要混用不同芯片系列的头文件、启动文件、链接器脚本或中断属性。
- 对于可运行示例，从最接近的 EVT 示例开始进行适配。
- 对于现有代码，进行最小且正确的原地修改。

### BLE 规则

- 保持严格的初始化顺序：系统时钟、BLE 协议栈初始化、HAL 初始化、GAP 角色初始化、应用初始化，最后进入事件循环。
- `Main_Circulation()` 是 BLE 事件循环，不得返回。
- 按以下顺序注册 GATT 服务：GAP、GATT、设备信息、自定义服务。
- 芯片系列有要求时，BLE 中断处理函数应采用快速中断形式，并置于 RAM/highcode 段中。
- 检查 `config.h` 中的堆大小、缓冲区大小、MTU、睡眠策略、TX 功率和连接数限制。

### 架构特定规则

- RISC-V WCH 中断处理函数通常使用 `__attribute__((interrupt("WCH-Interrupt-fast")))`。
- CH57x/CH58x/CH59x BLE 中断处理函数可能还需要放置在 `.highcode` 中。
- CH32V/CH32F/CH32X/CH32H StdPeriphDriver 代码应在设置中断前配置 NVIC 优先级分组。
- ARM7TDMI CH561/CH563 代码采用寄存器级操作，使用 `R8_`、`R16_` 和 `R32_` 形式的访问，而不是 StdPeriphDriver。
- 8051 代码必须使用受保护寄存器的安全模式序列，以及正确的 `data` 或 `xdata` 限定符。

## 提取工作流

对每个主题或芯片系列使用以下工作流。

1. 选择一个主题，例如 BLE 外设、GPIO、闪存、以太网、USB-PD 或内存布局。
2. 找出所有相关的 `Doc/Ref/wch-dev-skill/**/*.md` 文件。
3. 阅读技能文档中引用的相应 EVT 示例路径。
4. 对照仓库头文件和官方文档，交叉核对关键 API 名称和行为。
5. 在 `Doc/<topic>/` 下创建或更新目标说明。
6. 将源文件路径和验证说明添加到目标说明中。
7. 更新本文档中的 TODO 清单。
8. 保留 `Doc/Ref/wch-dev-skill`，供后续提取使用。

## 迁移 TODO 清单

- [x] 恢复 `Doc/Ref/wch-dev-skill`，将其作为原始源文件参考。
- [x] 使用以提取为重点的计划替换原先以删除为重点的计划。
- [x] 清点所有顶层技能文档：`SKILL.md`、`AGENTS.md`、`README.md`、`README_EN.md`、`CHANGELOG.md`。
- [x] 生成按芯片系列和主题分组的完整 `Doc/Ref/wch-dev-skill/**/*.md` 源文件索引。
- [x] 将全局芯片系列路由提取到 `Doc/Family/` 下的仓库专用芯片系列索引中。
- [x] 创建芯片系列规范化入口说明，用于未来各芯片系列的 HAL 说明和 EVT 验证。
- [x] 为 CH32V 通用系列、CH58x/CH59x 系列和 CH56x 以太网系列创建首批各芯片系列说明。
- [x] 为 CH57x、CH32X/CH6xx USB-PD 和 CH32H 高性能系列创建第二批各芯片系列说明。
- [x] 为 CH32F ARM、CH32M030 RISC-V、CH32V 低成本/CH32L103、CH561/CH563 和 CH5xx 8051 系列创建其余各芯片系列说明。
- [x] 创建 `Doc/Templates/`，并提取新建项目、工具链、链接器、启动和最小主循环说明。
- [x] 将 `ch57x` 和 `ch58x-ch59x` 中的 BLE 说明提取到 `Doc/BLE/`。
- [x] 将内存布局、启动、链接器、中断和架构陷阱提取到 `Doc/Core/`。
- [x] 将以太网说明提取到 `Doc/ETH/`。
- [x] 创建 `Doc/HAL/`，并提取 GPIO、RCC、UART、SPI、I2C、ADC、定时器、闪存和 DMA 的通用外设规范化说明。
- [x] 创建 `Doc/USB/`，并提取 USB 设备、主机、USB HS 和 USB3 说明。
- [x] 创建 `Doc/USBPD/`，并提取 USB-PD、Type-C 和 PIOC 说明。
- [x] 创建 `Doc/IAP/`，并提取 IAP、OTA、引导加载程序和链接器偏移说明。
- [x] 创建 `Doc/RTOS/`，并在适用时提取 RTOS 相关参考资料。
- [x] 每份提取后的目标文档均包含源文件路径和验证状态。
- [x] 使 `README.md` 与当前提取状态保持一致。
- [x] 创建 `Doc/HMI/`，并提取显示、HMI 和专用外设说明。
- [x] 创建 `Doc/IO/`，并提取 CAN、I2S、SAI 和 DVP 摄像头接口说明。
- [x] 创建 `Doc/Storage/`，并提取 SD/SDIO/eMMC/HSPI/QSPI 存储接口说明。
- [x] 创建 `Doc/System/`，并提取低功耗、RTC、看门狗、模拟、CRC/RNG 和实用外设说明。
- [x] 创建 `Doc/Security/`，并提取 ECDC、CRC、RNG 和安全边界说明。

## 验收标准

- 提取期间保留 `Doc/Ref/wch-dev-skill`。
- 提取后的说明位于主题适合的 `Doc/` 目录中。
- 每份提取后的说明均可追溯到源 `.md` 文件，并在可能的情况下追溯到官方 EVT/RM/DS 资料。
- README 将 `Doc/Ref/wch-dev-skill` 描述为源文件参考，而不是计划立即删除的目录。
- 在所有有用的 Markdown 内容均已迁移或有意跳过之前，不考虑删除 `Doc/Ref/wch-dev-skill`。
