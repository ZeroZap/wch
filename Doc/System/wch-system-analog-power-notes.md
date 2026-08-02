# WCH 系统、模拟、电源与实用外设笔记

本文从 `Doc/Ref/wch-dev-skill` 中提取低功耗、看门狗、RTC、比较器、DAC、OPA、CRC、RNG 及相关实用外设的指导，整理为适用于本仓库未来 HAL 元数据和模板的规则。

官方 EVT 示例、RM、DS、模拟电气规范、电路板原理图及当前仓库源码仍是最终依据。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/lowpower.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/lowpower.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/watchdog.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/watchdog.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/watchdog.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/rtc_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/rtc_clock.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/cmp.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/compare.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/opa_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/opa_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/opa_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/dac_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/dac_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/dac_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/crc_calculation.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`

## 涵盖范围

| 主题 | 来源笔记中的系列 | 主要约束 |
|---|---|---|
| 低功耗 | CH57x、CH58x/CH59x、CH56x、CH561/CH563、CH32V、CH32X、CH32F、CH32H | 唤醒源、时钟恢复、调试行为、BLE/USB/ETH 时序及稳压器状态因系列而异。 |
| 看门狗 | CH32V 低成本/通用系列、CH32H、CH5xx 陷阱 | 必须明确超时时钟源和喂狗职责；处理得当时看门狗可从挂起中恢复，处理不当则可能使更新后的设备无法使用。 |
| RTC | CH32V 通用系列、CH32H、BLE 系列 API 参考 | 必须按芯片选择 LSE/LSI 源、备份域、日历格式、闹钟/防拆/唤醒及校准。 |
| 比较器 / CMP | CH57x 和 CH5xx 来源笔记；CH32 低成本模拟外设资料 | 输入路由、参考源、迟滞、中断清除及模拟电源取决于电路板。 |
| OPA | CH32V 低成本系列、CH32F、CH32H | 必须关注引脚路由、增益模式、校准及模拟电源约束。 |
| DAC | CH32V 通用系列、CH32F、CH32H | 必须关注输出缓冲、触发、DMA、参考电压及引脚冲突。 |
| CRC / RNG | CH32H CRC/RNG、CH32F RNG 来源笔记 | 必须明确多项式/种子/输出格式及熵就绪状态。 |

## 低功耗规则

低功耗支持应建模为系统策略，而不是单个 `sleep()` 辅助函数。

规则：

- 为每款芯片记录模式类型、唤醒源、唤醒延迟、保留的 RAM、时钟恢复路径及外设保持状态。
- 仅当所选唤醒策略允许时，才在休眠前禁用外设或使其静止。
- BLE 项目必须保持协议栈时序及 TMOS/BLE 休眠规则；参见 `Doc/BLE/wch-ble-notes.md`。
- USB 项目必须考虑挂起/恢复及远程唤醒时序。
- Ethernet 项目应明确链路、PHY、DMA 描述符及网络栈状态能否在低功耗期间保持。
- 调试探针和 UART 日志可能阻止或干扰低功耗测量；应分别记录调试模式与生产模式的假设。
- 应在实际电路板上测量电流，因为上拉电阻、LED、PHY、外部 Flash、传感器及稳压器通常是 MCU 休眠电流的主要来源。

## 看门狗规则

看门狗的管理职责必须明确且确定。

规则：

- 根据复位策略和时钟可用性选择独立看门狗或窗口看门狗。
- 在元数据中保存看门狗时钟源、预分频器、重装载值/窗口值、超时时间及复位/中断行为。
- 由一个指定的管理者喂狗，例如监督任务或主循环健康检查。
- 不要从无关的 ISR 中喂狗，否则会掩盖主应用程序中的死锁。
- 在 IAP/OTA 擦除/编程期间，应在经过验证的安全点维护看门狗，或按照引导加载程序策略禁用/重新配置看门狗。
- 如果看门狗一经启用只能保持到复位，应将其公开为项目级不可逆运行时决策。

## RTC 规则

RTC 启动涉及时钟域和备份域。

规则：

- 明确选择 LSE、LSI 或其他低速时钟源，并确认电路板是否装有相应晶振。
- 对于有此要求的系列，应在更改 RTC 时钟配置前解锁备份域。
- 仅当电源域保持供电时，才跨复位保留备份寄存器和时间状态。
- 日历格式、纪元、时区策略及夏令时处理不应放入底层 RTC 驱动程序。
- 闹钟、周期唤醒、防拆及校准功能应在元数据中作为独立能力表示。

## 模拟外设规则

比较器规则：

- 按目标系列的 CMP/OPA API 和 EVT 示例决定时钟、电源、配置及使能步骤，不规定跨系列固定顺序；有的 API 将配置与使能分开，有的寄存器/API 在初始化过程中同时写入工作状态。
- 按系列要求将输入引脚配置为模拟模式或浮空模式。
- 明确设置正输入、负输入、参考源、迟滞/滤波及输出极性。
- 只有目标 API 明确提供独立 `Cmd`/`Enable` 操作时，才要求在配置完成后单独使能；不得把某一系列的 `CMP_Init()` 后接 `CMP_Enable()` 固化为所有系列模板。
- 使用系列特定的写入语义清除比较器标志；某些 8051 风格的标志通过写入 `0` 清除。
- 将有噪声的模拟阈值用于唤醒/复位决策前，应通过软件或硬件进行去抖或滤波。

OPA 规则：

- 将 OPA 视为具有增益、输入、输出、校准及稳定时间的模拟路由模块。
- 检查其与 ADC、比较器、DAC、GPIO 的引脚冲突以及封装上的可用性。
- 未经 DS 验证，不要假定其具备轨到轨特性、特定带宽或负载驱动能力。

DAC 规则：

- 记录参考电压、分辨率、输出缓冲设置、触发源、DMA 支持及输出引脚。
- 当稳定输出需要定时器触发 DMA 时，不要用阻塞循环生成波形。
- 在电路板元数据中记录 DAC 输出稳定时间及外部负载。

## CRC 与 RNG 规则

CRC 规则：

- 保存多项式、初始值、输入位序、输出位序及最终异或值，使软件 CRC 与硬件 CRC 一致。
- 协议 CRC 配置应与固件映像 CRC 配置分开。
- 将 CRC 用于启动验证前，应对照目标 CRC 外设确认字节/字的送入顺序。

RNG 规则：

- 读取随机数据前启用所需时钟和熵源。
- 等待就绪/数据有效标志，并处理错误标志。
- 未经调理和安全审查，不要将未经验证的 RNG 输出直接用于加密密钥。
- 测试模式或伪随机源应与真实硬件 RNG 元数据分开。

## 当前 EVT 验证摘要

| 系列/主题 | 当前仓库证据 | 已核对结论 |
|---|---|---|
| CH32V103 STOP | `CH32V103EVT/EXAM/PWR/Stop_Mode/User/main.c` | 示例先使能 APB1 PWR 时钟，再调用双参数 `PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI)`。 |
| CH32M030 STOP | `CH32M030EVT/EXAM/PWR/Stop_Mode/User/main.c` | 示例使用单参数 `PWR_EnterSTOPMode(PWR_STOPEntry_WFI/WFE)`，没有稳压器参数；WFE 分支唤醒后显式调用 `SystemInit()`。因此不能复用 V103 的 STOP 调用签名和恢复模板。 |
| CH32M030 CMP | `CH32M030EVT/EXAM/OPA/CMP/User/main.c`、`CH32M030EVT/EXAM/OPA/CMP_IRQ/User/main.c` | CMP 位于 OPA/OPCM 模块；示例使能 `RCC_HBPeriph_OPCM`，配置 `CMP3_InitTypeDef` 后调用 `CMP3_Init()` 和独立的 `CMP3_Cmd(ENABLE)`，中断示例还使用 OPA IRQ 和 CMP 标志 API。 |
| CH32V407 OPA | `CH32V407EVT/EXAM/OPA/OPA/User/main.c` | 当前示例使用无实例参数的 `OPA_Init(&OPA_InitStructure)` 和 `OPA_Cmd(ENABLE)`；不能套用 H417 的 `OPA_Init(OPAx, ...)` 形式。 |
| CH32V407 DAC DMA | `CH32V407EVT/EXAM/DAC/DAC_DMA/User/main.c` | 示例以 TIM8 TRGO 触发 DAC1，DMA2 Channel3 循环搬运，并分别使能 `DAC_Cmd()` 与 `DAC_DMACmd()`。 |
| CH32V407 RNG | `CH32V407EVT/EXAM/RNG/RNG/User/main.c` | 示例调用 `RNG_Cmd(ENABLE)`，等待 `RNG_FLAG_DRDY` 后通过 `RNG_GetRandomNumber()` 读取。该示例只验证 API 路径，不构成密码学质量证明。 |

以上“验证”表示已对照当前仓库 EVT 源码确认 API/示例行为，不表示已在芯片实物上测量，也不替代 RM、DS 和电气验证。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| 未设置唤醒源便进入低功耗模式 | 设备永远无法唤醒 | 休眠前配置并测试唤醒源。 |
| 唤醒后未恢复时钟树 | UART、USB、BLE 或定时器以错误速度运行 | 唤醒时重新初始化或验证时钟。 |
| 从 ISR 中喂狗 | 已死锁的应用程序看似健康 | 仅由经过健康检查的管理者喂狗。 |
| 假定存在 RTC 晶振 | RTC 不运行或严重漂移 | 验证电路板时钟源及回退策略。 |
| 比较器输入仍为数字输出/上拉模式 | 阈值错误或无中断 | 使用芯片要求的模拟/浮空模式。 |
| 忽略 OPA/DAC 引脚冲突 | 模拟输出丢失或失真 | 预留引脚并检查封装映射。 |
| CRC 多项式不匹配 | 固件或数据包验证失败 | 保存完整 CRC 配置，而非仅保存位宽。 |
| 忽略 RNG 就绪/错误标志 | 产生重复或无效的随机数据 | 等待有效数据并处理错误。 |

## 元数据检查清单

未来的系统/模拟/电源模板应明确表示以下字段：

- 低功耗模式、保持域、唤醒源、时钟恢复、调试行为及电路板实测电流说明。
- 看门狗类型、时钟源、超时/窗口、喂狗管理者、复位/中断模式及引导加载程序/更新策略。
- RTC 时钟源、备份域行为、日历能力、闹钟/唤醒/防拆支持及校准。
- 模拟模块：比较器、OPA、DAC、ADC 交叉关联、引脚、参考源、校准、触发/DMA 及封装冲突。
- 实用模块：CRC 配置、RNG 就绪/错误策略、安全假设及固件验证用途。
- 验证来源：准确的 EVT 示例、RM/DS 章节、原理图及测量结果。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮已抽查当前仓库 V103/M030 STOP、M030 CMP 以及 V407 OPA、DAC DMA、RNG EVT 示例，确认上表中的 API 签名、调用路径和系列差异。
- 尚未对照芯片实物、RM、DS、电路板原理图、模拟测量、低功耗电流测量或安全验证；唤醒后的完整时钟/外设状态、模拟范围、电气性能、CRC/RNG 质量及引脚封装约束仍须针对目标硬件验证。
