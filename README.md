# wch-hal

`wch-hal` 是一个面向 WCH / 南京沁恒 MCU 的 HAL 整理与统一化仓库。

当前仓库保留了多代 EVT / SDK 原始工程、芯片手册、参考资料和 AI 解读资料，目标不是简单镜像官方包，而是从不同芯片族中抽取可复用的 HAL 能力，逐步形成面向后续 CubeX 代码生成的统一外设模型、初始化流程和工程模板。

## 仓库目标

- 汇总 WCH 多系列 MCU 的 EVT、StdPeriphDriver、启动文件、链接脚本、示例工程和工具资料。
- 对 `CH5xx`、`CH32V00x`、`CH32V1`、`CH32V2`、`CH32V3`、`CH32H` 等系列进行 HAL 接口、外设能力和工程结构对齐。
- 保留官方 RM / DS / EVT 资料作为事实来源，避免后续统一 HAL 时脱离芯片手册。
- 使用 `Doc/Ref/wch-dev-skill` 中的 Markdown 资料作为待消化来源，并按 `Doc/wch-dev-skill-digestion-plan.md` 逐步提取到 `Doc` 的专题目录中；具体事实仍以官方 RM / DS / EVT 源码为准。
- 为后续 CubeX 输出准备芯片族元数据、外设差异表、初始化模板和示例迁移路径。

## 当前覆盖

| 方向 | 当前仓库内容 | 统一 HAL 关注点 |
|------|--------------|-----------------|
| CH5xx / CH57x / CH58x / CH59x | `CH572EVT`、`CH573EVT`、`CH583EVT`、`CH585EVT`、`CH592EVT`、`CH595EVT` | BLE、USB、I2C、SPI、UART、Timer、PWM、ADC、Flash、低功耗、8051/青稞差异 |
| CH32V00x / 低成本 RISC-V | `CH32X035EVT`、`Doc/DS`、`Doc/RM` 中的 CH32V00x / CH32X035 资料 | 小封装资源约束、GPIO、ADC、Timer、OPA/CMP、USB-PD / PIOC 差异 |
| CH32V1 | `CH32V103EVT`、`CH32xV10xRM.PDF` | CH32V10x StdPeriphDriver、USB FS、通用外设初始化模板 |
| CH32V2 | `CH32V20xEVT`、`CH32FV2x_V3xRM.PDF` | BLE / USB / ETH / CAN 等增强外设，V2/V3 共用 RM 下的差异拆分 |
| CH32V3 | `CH32V307EVT`、`CH32FV2x_V3xRM.PDF` | FPU、高速互联、ETH、USB HS、FSMC、CAN、DMA、复杂时钟树 |
| CH32V4 / CH32H 高性能方向 | `CH32V407EVT`、CH32V407 RM/DS、`Doc/Ref/wch-dev-skill/chips/ch32h-highperf` 待提取资料 | LTDC、ARGB、USB HS/SS、ETH、高性能中断和多核/高性能芯片抽象 |
| CH32X | `CH32X035EVT`、`CH32X315EVT` | USB-PD、PIOC、USB、专用外设与通用 CH32 HAL 的边界 |
| CH32M / ARM 参考 | `CH32M030EVT` | 与 CH32F/STM32 风格 StdPeriphDriver 的兼容性参考 |
| CH569 | `CH569EVT` | USB3、ETH、eMMC 等高速接口作为高性能外设参考 |

## 目录结构

```text
wch-hal/
  README.md                 # 仓库定位与整理路线
  CH32*EVT/                 # CH32 系列官方 EVT / SDK 工程
  CH5*EVT/                  # CH57x / CH58x / CH59x / CH569 等 EVT 工程
  Doc/
    DS/                     # 芯片数据手册 Datasheet
    RM/                     # 参考手册 Reference Manual
    Family/                 # 芯片族路由和来源映射
    BLE/                    # BLE 协议与应用资料
    Core/                   # 内核、架构、工具链相关资料
    HAL/                    # 通用外设归一化资料
    ETH/                    # 以太网相关资料
    USB/                    # USB FS/HS/SS 相关资料
    USBPD/                  # USB-PD、Type-C、PIOC 相关资料
    HMI/                    # LCD、TouchKey、KEYSCAN 和专用接口资料
    IAP/                    # IAP、OTA、bootloader、链接偏移资料
    RTOS/                   # RTOS 示例覆盖和移植约束资料
    IO/                     # CAN、I2S/SAI、DVP camera 等流式接口资料
    Storage/                # SD/SDIO/eMMC/HSPI/QSPI 存储接口资料
    System/                 # 低功耗、RTC、watchdog、模拟和工具外设资料
    Templates/              # 工程模板、工具链和最小工程结构资料
    Security/               # ECDC、CRC、RNG 和安全边界资料
    Ref/wch-dev-skill/      # WCH MCU AI 解读、recipes、API 和 pitfalls 的待消化来源
    wch-dev-skill-source-index.md     # wch-dev-skill 来源索引
    wch-dev-skill-digestion-plan.md  # wch-dev-skill Markdown 提取和归档计划
  HexBinStudio.ZIP          # 工具包
  WCHISPTool_CMD.ZIP        # WCH ISP 命令行工具
```

## 文档归档

根目录不再放置 RM / DS 手册，手册按用途归档：

| 目录 | 内容 |
|------|------|
| `Doc/DS/` | 芯片数据手册，例如 `CH32V002DS0.PDF`、`CH583DS1.PDF`、`CH595DS1.PDF` |
| `Doc/RM/` | 芯片参考手册，例如 `CH32V00XRM.PDF`、`CH32FV2x_V3xRM.PDF`、`CH32V407RM.PDF` |
| `Doc/Ref/wch-dev-skill/` | 待消化的 AI 解读资料，包含芯片族划分、场景 recipes、API 速查、常见坑和示例索引 |
| `Doc/wch-dev-skill-source-index.md` | `Doc/Ref/wch-dev-skill` 的 Markdown 来源索引和目标专题映射 |
| `Doc/wch-dev-skill-digestion-plan.md` | 将 `wch-dev-skill` 的 Markdown 资料提取到 `Doc` 专题目录的计划和 TODO |

HAL 统一时以 `Doc/DS`、`Doc/RM` 和官方 EVT 源码为事实来源；`Doc/Ref/wch-dev-skill` 仅作为待消化输入，提取后的专题 notes 逐步沉淀到 `Doc/Family`、`Doc/BLE`、`Doc/Core`、`Doc/HAL`、`Doc/ETH`、`Doc/USB`、`Doc/USBPD`、`Doc/HMI`、`Doc/IAP`、`Doc/RTOS`、`Doc/IO`、`Doc/Storage`、`Doc/System`、`Doc/Templates`、`Doc/Security` 等目录。

## wch-dev-skill 提取状态

首轮 Markdown 提取已完成以下专题。每个专题文档都包含来源路径和验证状态；当前结论仍需后续对照官方 EVT、RM、DS、启动文件、链接脚本和板级资料验证。

| 专题 | 目标文档 | 当前状态 |
|------|----------|----------|
| 芯片族路由 | `Doc/Family/family-routing.md` | 已提取，待 EVT/RM/DS 逐项验证 |
| BLE | `Doc/BLE/wch-ble-notes.md` | 已提取，待 BLE EVT 和协议栈验证 |
| Core / memory / startup / linker | `Doc/Core/wch-core-notes.md` | 已提取，待启动文件和链接脚本验证 |
| HAL 外设归一化 | `Doc/HAL/wch-hal-normalization.md` | 已提取，待 EVT API 和头文件验证 |
| Ethernet | `Doc/ETH/wch-ethernet-notes.md` | 已提取，待 WCHNET、MAC/PHY 和板级验证 |
| USB | `Doc/USB/wch-usb-notes.md` | 已提取，待 USB EVT、描述符和控制器细节验证 |
| USB-PD / Type-C / PIOC | `Doc/USBPD/wch-usbpd-notes.md` | 已提取，待 USB-PD EVT、芯片头文件和板级 VDD/CC 设计验证 |
| Display / HMI / specialty | `Doc/HMI/wch-hmi-specialty-notes.md` | 已提取，待 LCD/TouchKey/KEYSCAN/外部总线和板级时序验证 |
| IAP / OTA / bootloader | `Doc/IAP/wch-iap-ota-notes.md` | 已提取，待 Flash 布局、启动跳转和链接脚本验证 |
| RTOS | `Doc/RTOS/wch-rtos-notes.md` | 已提取，待 RTOS EVT、kernel config、tick 和栈/堆配置验证 |
| IO / audio / CAN / camera | `Doc/IO/wch-io-media-notes.md` | 已提取，待 CAN、I2S/SAI、DVP EVT、板级和外部器件验证 |
| Storage interfaces | `Doc/Storage/wch-storage-notes.md` | 已提取，待 SD/eMMC/QSPI/HSPI EVT、板级和器件数据手册验证 |
| System / analog / power utilities | `Doc/System/wch-system-analog-power-notes.md` | 已提取，待低功耗、RTC、watchdog、模拟和 CRC/RNG 细节验证 |
| Project templates | `Doc/Templates/wch-project-template-notes.md` | 已提取，待 MounRiver/Keil 工程、启动文件和链接脚本验证 |
| Security / crypto / CRC / RNG | `Doc/Security/wch-security-crypto-notes.md` | 已提取，待 ECDC、CRC、RNG、测试向量和安全需求验证 |

## HAL 统一思路

### 1. 先按芯片族建立事实表

| 芯片族 | 需要抽取的信息 |
|--------|----------------|
| CH5xx / CH57x / CH58x / CH59x | 内核类型、存储布局、BLE 栈入口、USB 资源、睡眠模式、寄存器命名和中断属性 |
| CH32V00x | 最小外设集合、时钟树、GPIO 复用、ADC/Timer/DMA/OPA/CMP、Flash 页大小 |
| CH32V1 / V2 / V3 | StdPeriphDriver API 差异、外设寄存器兼容性、USB/ETH/CAN/FSMC 能力矩阵 |
| CH32H | 高性能外设、FPU / 中断 / cache / 多核或高性能启动流程 |

### 2. 再抽象公共外设模型

优先统一稳定且跨系列普遍存在的外设：

- GPIO：端口、引脚、输入输出模式、上下拉、复用、速度。
- RCC / CLOCK：系统时钟源、PLL、总线分频、外设时钟使能。
- UART / USART：波特率、数据位、停止位、中断、DMA。
- SPI / I2C：主从模式、时钟、片选、传输 API。
- Timer / PWM：基础计数、输入捕获、输出比较、PWM、编码器。
- ADC：通道、采样周期、校准、触发源、DMA。
- USB / BLE / ETH / CAN：先保留芯片族专用层，再逐步提取公共描述。

### 3. 保留芯片族适配层

统一 HAL 不应抹平硬件差异。建议保留三层：

```text
CubeX metadata          # 芯片、封装、引脚、外设能力、时钟树和模板数据
Unified HAL API         # 面向用户和生成器的稳定接口
Family adapter          # CH5xx / CH32V00x / CH32V1 / CH32V2 / CH32V3 / CH32H 差异实现
Vendor EVT source       # 官方 EVT / StdPeriphDriver / BLE stack / USB stack
```

## CubeX 准备路线

1. 建立芯片族清单：从 `*EVT`、`Doc/DS`、`Doc/RM` 提取芯片型号、内核、Flash/RAM、封装和外设能力。
2. 建立外设能力矩阵：先覆盖 GPIO、RCC、UART、SPI、I2C、Timer、ADC，再扩展 USB、BLE、ETH、CAN、PIOC、LTDC。
3. 建立引脚和复用数据：按芯片型号和封装拆分，避免把同系列不同封装混为一谈。
4. 建立初始化模板：从 EVT 示例反推最小可运行工程、时钟初始化、调试串口、链接脚本和启动文件。
5. 建立适配层命名规范：统一外设名、实例名、IRQ 名、DMA 请求名和宏命名。
6. 建立生成器输出结构：CubeX 输出工程应能选择芯片、外设、引脚、时钟、RTOS 和示例模板。

## 使用建议

- 查芯片硬件能力时，先看 `Doc/DS` 和 `Doc/RM`。
- 查官方 API 和可运行初始化流程时，优先看对应 `*EVT/EXAM` 示例。
- 查待消化的 AI recipes、pitfalls 和 API 索引时，看 `Doc/Ref/wch-dev-skill`；查迁移进度和目标目录时，看 `Doc/wch-dev-skill-digestion-plan.md`。
- 做统一 HAL 时，不要直接把某一个系列的 StdPeriphDriver 当成全系列公共接口，应先建立能力矩阵和差异表。

## 后续整理重点

- 为 `CH5xx`、`CH32V00x`、`CH32V1`、`CH32V2`、`CH32V3`、`CH32H` 建立独立 family notes。
- 从官方 EVT 中抽取每个 family 的 `StdPeriphDriver` 文件清单、启动文件、链接脚本和最小 main 模板。
- 补齐 RM / DS 与 EVT 示例之间的对应关系。
- 逐步形成 `metadata/`、`hal/`、`templates/` 这类面向 CubeX 的新目录。

## 说明

本仓库目前仍处于资料整合与 HAL 抽象准备阶段。官方 EVT 和手册中的源码、协议栈、工具和文档版权归原作者或 WCH 所有；仓库中的统一 HAL 设计、索引和后续生成器元数据应独立维护，并尽量保持与官方资料可追溯。
