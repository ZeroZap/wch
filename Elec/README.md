# 电气特性维护（按 MCU 型号）

本目录维护 **“一个 MCU 型号一份电气特性表”**，与 `PinMux/` 的组织方式一致。
数据来自 `Doc/DS/*.PDF` 的电气特性/参数章节，每个数值都带数据手册页码；
Flash 的页/块/最小写入粒度在数据手册未写明时，补充引用对应 EVT Flash 库头文件。

当前覆盖与 `PinMux/` 相同的 17 个型号；`CH32V103EVT`、`CH32V20xEVT`、`CH32H417TEVT` 因 `Doc/DS` 中没有对应数据手册，暂未列入。

| 型号 | 数据手册 | 内核 | 最高主频 | Flash | SRAM | 供电 | 低功耗模式 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CH32X035 | `CH32X035DS0.PDF` | RISC-V4C | 48 | 62 KB | 20 KB | 2.0–5.5 V（USB/PD 3.0–5.3 V；ADC 2.5–5.5 V） | 睡眠 / 停止 / 待机 |
| CH32X315 | `CH32X315DS0.PDF` | RISC-V3F | 480（内核）/ 313（系统） | 480 KB | 64 KB | 2.8–3.6 V（VDDK 1.17–1.27 V） | 睡眠 / 停止 |
| CH32L103 | `CH32L103DS0.PDF` | RISC-V4C | 96 | 64 KB | 20 KB | 1.8–3.6 V | 睡眠 / 停止 / 待机 |
| CH32M030 | `CH32M030DS0.PDF` | RISC-V3B | 72 | 64 KB（带 ECC） | 12 KB | VHV 4.0–29 V；VDD8 4.0–10.5 V；VDD33 3.1–3.5 V | 睡眠 / 停止 / 待机 |
| CH32V002 | `CH32V002DS0.PDF` | RISC-V2C | 48 | 16 KB | 4 KB | 2.0–5.5 V（ADC 2.4–5.5 V） | 睡眠 / 待机（无停止） |
| CH32V006 | `CH32V006DS0.PDF` | RISC-V2C | 48 | 62 KB | 8 KB | 2.0–5.5 V（ADC 2.4–5.5 V） | 睡眠 / 待机（无停止） |
| CH32V007 | `CH32V007DS0.PDF` | RISC-V2C | 48 | 62 KB | 8 KB | 2.0–5.5 V（ADC 2.4–5.5 V） | 睡眠 / 待机（无停止） |
| CH32V307 | `CH32V307DS0.PDF` | RISC-V4F | 144 | 480 KB（最大） | 128 KB（最大） | 2.4–3.6 V（VLEVEL=0 可低至 1.8 V） | 睡眠 / 停止 / 待机 |
| CH32V407 | `CH32V407DS0.PDF` | RISC-V3V | 200（默认）/ 240（LDO_VDDK=100） | 992 KB | 200 KB | 2.9–3.6 V（VDDK 1.2 V；V467 另有 VDD18 1.8 V） | 睡眠 / 停止 / 待机 |
| CH569 | `CH569DS1.PDF` | RISC-V3A | 120 | 448 KB CodeFlash + 32 KB DataFlash | 32/64/96 KB RAMX + 16 KB RAMS | VDDLDO/VDDIO 2.3–3.6 V；V12CORE 1.15–1.30 V；V33USB/V33GX 3.0–3.6 V | 空闲 / 暂停 / 睡眠 |
| CH572 | `CH572DS1.PDF` | RISC-V3C | 100 | 240 KB CodeFlash + 8 KB InfoFlash | 12 KB | V5 4.5–5.3 V 或 VDD33 2.0–3.6 V | 空闲 / 暂停 / 睡眠 / 下电 |
| CH573 | `CH573DS1.PDF` | RISC-V3A | 60 | 448 KB CodeFlash + 32 KB DataFlash | 18 KB（16 + 2） | 1.7–3.6 V（CH573X）；2.3–3.6 V（其它） | 空闲 / 暂停 / 睡眠 / 下电 |
| CH583 | `CH583DS1.PDF` | RISC-V4A | 80 | 448 KB CodeFlash + 32 KB DataFlash | 32 KB（30 + 2） | 1.75–3.6 V（CH583M）；2.3–3.6 V（其它） | 空闲 / 暂停 / 睡眠 / 下电 |
| CH585 | `CH585DS1.PDF` | RISC-V3C | 78 | 448 KB CodeFlash + 32 KB DataFlash | 128 KB（96 + 32） | 1.85–3.6 V（USB 3.15–3.45 V） | 空闲 / 暂停 / 睡眠 / 下电 |
| CH592 | `CH592DS1.PDF` | RISC-V4C | 80 | 448 KB CodeFlash + 32 KB DataFlash | 26 KB（24 + 2） | 1.75–3.6 V（X/F/D、CH591F）；2.3–3.6 V（A、CH591D/R） | 空闲 / 暂停 / 睡眠 / 下电 |
| CH595 | `CH595DS1.PDF` | RISC-V3C | 80 | 240 KB CodeFlash | 32 KB（24 + 8） | 1.7–3.6 V | 空闲 / 暂停 / 睡眠 / 下电 |
| CH596 | `CH596DS1.PDF` | RISC-V3C | 80 | 240 KB CodeFlash | 32 KB（24 + 8） | 1.2–1.8 V（VDD15 典型 1.5 V） | 空闲 / 暂停 / 睡眠 / 下电 |

## 1. Flash 粒度与寿命

| 型号 | 页 | 扇区 | 块 | 最小写入 | 擦写寿命 | 数据保持 |
| --- | --- | --- | --- | --- | --- | --- |
| CH32X035 | 256 B | 1024 B | DS 未给出 | 256 B | 300K @25 ℃；100K @70 ℃ | 20 年 @25 ℃；10 年 @70 ℃ |
| CH32X315 | 256 B | DS 未给出 | 32768 B | 256 B | 50K @25 ℃ | 20 年 |
| CH32L103 | 256 B | 2048 B | DS 未给出 | 256 B | 300K @25 ℃；100K @70 ℃ | 20 年 @25 ℃；10 年 @70 ℃ |
| CH32M030 | 128 B | DS 未给出 | DS 未给出 | 128 B | 10K–50K @25 ℃；1K–5K @125 ℃（抽测，非担保） | 10 年 |
| CH32V002 | 256 B | 1024 B | DS 未给出 | 256 B | 300K @25 ℃；100K @70 ℃ | 20 年 @25 ℃；10 年 @70 ℃ |
| CH32V006 | 256 B | 1024 B | DS 未给出 | 256 B | 300K @25 ℃；100K @70 ℃ | 20 年 @25 ℃；10 年 @70 ℃ |
| CH32V007 | 256 B | 1024 B | DS 未给出 | 256 B | 300K @25 ℃；100K @70 ℃ | 20 年 @25 ℃；10 年 @70 ℃ |
| CH32V307 | 256 B | 4096 B | DS 未给出 | 256 B | 10K（最小）/ 80K（实测抽测，非担保） | 20 年 |
| CH32V407 | 256 B | 4096 B | 32768 B | 256 B | 50K @25 ℃ | 20 年 |
| CH569 | 256 B | DS 未给出 | 4096 B | 4 B（CodeFlash）；1 B（DataFlash） | DS 未给出 | DS 未给出 |
| CH572 | 256 B | DS 未给出 | 4096 B | 4 B | 100K（最小）/ 2000K（抽测）@5–45 ℃；50K/500K @-40–85 ℃ | 20 年 |
| CH573 | 256 B | DS 未给出 | 4096 B | 4 B | 100K（最小）/ 1000K（抽测）@5–45 ℃；50K/200K @-40–85 ℃ | 20 年 |
| CH583 | 256 B | DS 未给出 | 4096 B | 4 B | 100K（最小）/ 1000K（抽测）@5–45 ℃；50K/200K @-40–85 ℃ | 20 年 |
| CH585 | 256 B | DS 未给出 | 4096 B | 4 B | 100K（最小）/ 1000K（抽测）@5–45 ℃；50K/200K @-40–85 ℃ | 20 年 |
| CH592 | 256 B | DS 未给出 | 4096 B | 4 B | 100K（最小）/ 1000K（抽测）@5–45 ℃；50K/200K @-40–85 ℃ | 20 年 |
| CH595 | 256 B | 2048 B | DS 未给出 | 4 B（字）；页写 256 B | 100K（最小）/ 800K（抽测）@5–45 ℃；30K/100K @-40–85 ℃ | 10 年 |
| CH596 | 256 B | 2048 B | DS 未给出 | 4 B（字）；页写 256 B | 100K（最小）/ 800K（抽测）@5–45 ℃；30K/100K @-40–85 ℃ | 10 年 |

## 2. 低功耗电流与唤醒时间

| 型号 | 低功耗模式 | 最低典型电流 | 唤醒时间 |
| --- | --- | --- | --- |
| CH32X035 | 睡眠 / 停止 / 待机 | 51 uA（Standby，IWDG/AWU 关闭） | Sleep 1 us / Stop 10 us / Standby 10 us |
| CH32X315 | 睡眠 / 停止 | 0.79 mA（Stop，LDO 低功耗） | Sleep 0.48 us / Stop 6.6–22.05 us |
| CH32L103 | 睡眠 / 停止 / 待机 | 0.37 uA（Standby，RAM 不保持，LDO 0 档） | Sleep 0.2 us / Stop 13 us / Standby 72 us |
| CH32M030 | 睡眠 / 停止 / 待机 | 74 uA（Standby，VDD8 = 5 V） | Sleep 24 us / Stop 255 us / Standby 260 us |
| CH32V002 | 睡眠 / 待机（无停止） | 17.5 uA（Standby，IWDG/LSI 关闭） | Sleep 10 us / Standby 250 us |
| CH32V006 | 睡眠 / 待机（无停止） | 10.2 uA（V006 Standby，IWDG/LSI 关闭） | Sleep 10 us / Standby 250 us |
| CH32V007 | 睡眠 / 待机（无停止） | 10.2 uA（Standby，IWDG/LSI 关闭） | Sleep 10 us / Standby 250 us |
| CH32V307 | 睡眠 / 停止 / 待机 | 1.0 uA（Standby，所有 RAM 不带电） | Sleep 2.4 us / Stop 23.1–76.7 us / Standby 8.9 ms |
| CH32V407 | 睡眠 / 停止 / 待机 | 1.5 uA（Standby，所有 RAM 不带电） | Sleep 2.4 us / Stop 23.1–76.7 us / Standby 18.5 ms |
| CH569 | 空闲 / 暂停 / 睡眠 | 1.3 mA（Sleep） | Idle 0.5 us / Pause 27 us / Sleep TSUHSE+30 us |
| CH572 | 空闲 / 暂停 / 睡眠 / 下电 | 0.3 uA（Shutdown，仅 PMU） | Idle 1 us / Halt 200+80 us / Sleep 200+300 us / Shutdown 200+1 ms |
| CH573 | 空闲 / 暂停 / 睡眠 / 下电 | 0.3 uA（Shutdown，仅 PMU） | Idle 1 us / Halt 200+80 us / Sleep 200+300 us / Shutdown 200+1 ms |
| CH583 | 空闲 / 暂停 / 睡眠 / 下电 | 0.2 uA（Shutdown，仅 PMU） | Idle 1 us / Halt 200+80 us / Sleep 200+300 us / Shutdown 200+1 ms |
| CH585 | 空闲 / 暂停 / 睡眠 / 下电 | 0.65 uA（Shutdown，仅 PMU） | Idle 1 us / Halt 81 us / Sleep 301 us / Shutdown 1.001 ms |
| CH592 | 空闲 / 暂停 / 睡眠 / 下电 | 0.3 uA（Shutdown，多组合下限） | Idle 1 us / Halt 200+80 us / Sleep 200+300 us / Shutdown 200+1 ms |
| CH595 | 空闲 / 暂停 / 睡眠 / 下电 | 0.85 uA（Shutdown，仅 PMU） | Idle 1 us / Halt 200+80 us / Sleep 200+300 us / Shutdown 200+1 ms |
| CH596 | 空闲 / 暂停 / 睡眠 / 下电 | 0.85 uA（Shutdown，仅 PMU） | Idle 1 us / Halt 200+80 us / Sleep 200+300 us / Shutdown 200+1 ms |

## 3. 重点差异（先看这里）

* **CH32X035 Flash 是 256 B 整页编程，不是字节编程。** 需要先在页缓冲中填满 256 B，再调用 `FLASH_ProgramPage_Fast()` 整页提交；升级/参数存储的掉电保护必须按整页设计。
* **CH32M030 Flash 页只有 128 B，寿命 10K–50K（25 ℃）/1K–5K（125 ℃），保持 10 年**，且寿命为抽测值、非担保。
* **CH32X315 没有待机模式**，最低是 Stop 的 0.79–0.87 mA；不能把它当 uA 级低功耗 MCU。
* **CH32V002/CH32V006/CH32V007 只有睡眠和待机，没有停止模式**；待机约 10–18 uA。
* **CH32V307/CH32V407 的 Flash 寿命表注明“实测抽测，非担保”**；V307 为 10K 最小/80K 抽测，V407 为 50K。两者擦除/编程时间和待机唤醒时间也明显更长。
* **CH5xx 系列 CodeFlash 按字（4 B）编程、256 B/页写、4 KB/块擦**；DataFlash 才支持按字节读写。`TSUHSE` 参与暂停/睡眠/下电唤醒时间计算。
* **CH595/CH596 数据保持只有 10 年**，-40–85 ℃ 擦写寿命最低 30K；明显低于 CH573/CH583/CH592 的 20 年。
* **CH569 数据手册没有 Flash 擦写寿命/编程时间**；只有 EVT `ISPEM569.h` 给出 4 B 最小写、256 B 页写、4 KB 块擦，可靠性指标需向原厂确认。

### CH32X035 Flash 写入方式（重点）

* 编程单位：256 B/页；擦除单位：页 256 B 或扇区 1 KB。
* 典型流程：`FLASH_Unlock_Fast()` → `FLASH_BufReset()` → 64 次 `FLASH_BufLoad(addr + 4*i, word)` → `FLASH_ProgramPage_Fast(addr)`。
* 不支持按字节写入；参数存储不要按 EEPROM 逐字节写，必须凑满 256 B 页或做读-改-写。

## 4. 各型号注意事项

### CH32X035

* Flash 只能按 256 B 页编程，不能按字节写入；升级/掉电保护必须以整页为单位设计。
* 待机模式下 IWDG 或 AWU 开启后电流由约 51 uA 升到约 530 uA，低功耗预算要区分。
* 使用 USB/PD 时 VDD 下限提高到 3.0 V，不能按 2.0 V 设计。

### CH32X315

* 没有待机模式，最低功耗是 Stop 的约 0.79 mA，不能当 uA 级低功耗 MCU 使用。
* 480 MHz 内核 / 313 MHz 系统是默认档；400/625 MHz 性能模式仅适合 TA ≤ 70 ℃ 且散热良好。
* DS 未给出 Flash 扇区大小，只给出扇区与 32 KB 块擦除时间。

### CH32L103

* Stop 电流强依赖 RAM 保持、LDO_EC、LPDS 配置，4.08–34.63 uA 都是同一 STOP 模式的不同配置。
* Standby 的 0.37 uA 是最优配置（RAM 不保持、LDO 0 档），实际项目要加 RAM 保持与 LSI 电流。
* CC1_PD/CC2_PD 打开时，部分型号会额外增加约 5 uA。

### CH32M030

* Flash 寿命只有 10K–50K（25 ℃）和 1K–5K（125 ℃），且为抽测值，不能按 300K 级 Flash 设计。
* Flash 页只有 128 B，擦除/编程时间约 4.5 ms，频繁参数存储要控制写次数。
* 带高压调压器，Stop/Standby 仍在 74–133 uA，不能与 3.3 V 低功耗 MCU 的 uA 级待机直接比较。

### CH32V002

* 只有睡眠和待机，没有停止模式；需要 STOP 级功耗时应选其它型号。
* 待机电流约 17.5–18.5 uA，明显高于 CH32L103/CH32V307 的 uA 级待机。

### CH32V006

* 只有睡眠和待机，没有停止模式。
* CH32V005 待机约 17.5 uA，CH32V006 约 10.2 uA，选型时不要混用数据。
* Flash 按 256 B 页编程，扇区 1 KB。

### CH32V007

* 只有睡眠和待机，没有停止模式。
* CH32M007 变体带高压调节器/预驱，额外静态电流 41–185 uA，不能只看 MCU 内核电流。

### CH32V307

* Flash 页/扇区擦除时间 16–20 ms（页编程 2–2.5 ms），擦除比 V00x/L103 慢一个数量级，IAP/参数存储要留足时间。
* 待机唤醒 8.9 ms 包含代码加载时间，不能按纯 LDO 唤醒时间估算。
* Flash 寿命表注明实测非担保；高可靠场景建议按 10K 次最小值和降额设计。

### CH32V407

* 240 MHz 仅在 LDO_VDDK[2:0] = 100 时成立；默认档最高 200 MHz，不能直接按 240 MHz 做全温设计。
* Standby 唤醒 18.5 ms 包含 512 KB 代码加载时间，实际唤醒延迟与代码区大小强相关。
* Stop 电流 433/173 uA，属于高性能 MCU 水平，不适合 uA 级待机场景。

### CH569

* DS 没有 Flash 擦写寿命/编程时间，只有 EVT 库给出页/块粒度和最小写入单位；可靠性指标需向原厂确认。
* 低功耗是 mA 级（Sleep 1.3 mA），不是 BLE/低功耗 MCU 的 uA 级待机。
* DS 绝对最大值表没有 ESD 指标，只有 I/O 电平与电流。

### CH572

* 单一 5 V 供电时不支持暂停、睡眠、下电三种低功耗模式，只有 3.3 V 供电才能拿到 uA 级电流。
* 睡眠保持电流与 RAM12K、LSI、RTC 配置强相关，0.46–1.2 uA 是不同组合的区间。
* BLE 收发电流另计（接收 7.1 mA，0 dBm 发送 10.6 mA），不能与 MCU 睡眠电流相加后忽略射频占空比。

### CH573

* CH571 变体 Flash 总容量只有 256 KB，且 ADC 只支持 2.3 V 以上电源，不能直接套用 CH573 参数。
* CodeFlash 按 4 B 字编程、256 B 页写、4 KB 块擦；数据区要区分 DataFlash 与 CodeFlash。
* 睡眠保持电流与 RAM16K 是否保持强相关（RAM16K 额外 4.0–5.5 uA）。

### CH583

* CH582 与 CH583 的差别：CH582 没有 SPI1 主机，部分型号最低电源 2.3 V，不能混用 1.75 V 参数。
* CodeFlash 按 4 B 字编程，256 B 页写、4 KB 块擦；DataFlash 支持字节读写。
* 睡眠保持电流随 RAM30K/RAM2K、LSI、RTC 组合变化，0.7–2.8 uA 不是单一固定值。

### CH585

* 睡眠/下电电流明显高于 CH583/CH592，因为默认保持 RAM32K（2.6–8.2 uA）和更大 SRAM；要按实际保持 RAM 选配置。
* 使用 USB 时 VDD33 必须 3.15–3.45 V，不能按 1.85 V 低电压设计。
* CodeFlash 按 4 B 字编程，256 B 页写、4 KB 块擦；NFC/BLE 电流另计。

### CH592

* DS 表 20-3 中睡眠模式区间写 0.8–2.5 uA，但下一行“PMU+内核+RAM2K”写 0.7 uA；下电模式也类似（0.3–1.3 uA vs 0.4 uA），取值时保留这个差异。
* CH592A 工作温度可到 105 ℃，但供电下限 2.3 V，不能套用 X/F/D 的 1.75 V。
* CodeFlash 按 4 B 字编程，256 B 页写、4 KB 块擦；睡眠保持电流与 RAM24K 相关。

### CH595

* 数据保持只有 10 年，低于 CH573/CH583/CH592 的 20 年；高温 85 ℃ 下擦写寿命最低 30K。
* Flash 页 256 B、扇区 2 KB（EVT 库），DS 只给“字或页”编程时间。
* 睡眠/下电电流与 RAM8K/RAM24K、LCD、KEYSCAN 组合相关。

### CH596

* 1.5 V 核心/IO 供电，不能直接接 3.3 V；外部接口需要电平转换。
* 数据保持只有 10 年，高温 85 ℃ 下擦写寿命最低 30K。
* Flash 页 256 B、扇区 2 KB（按 CH595 同系列 EVT 库）；DS 只给“字或页”编程时间。

## 5. 文件说明

| 文件 | 内容 |
| --- | --- |
| `elec_data.py` | 人工复核后的数据源，每个数值带数据手册页码 |
| `gen_elec.py` | 生成/检查脚本 |
| `<MODEL>_elec.csv` | 单型号长表：分类/项目/条件/最小值/典型值/最大值/单位/页码/备注 |
| `<MODEL>_Electrical.xlsx` | 单型号工作簿：说明/汇总/低功耗/唤醒/Flash/其他 |
| `elec_all.csv` | 全部型号长表，便于 grep/diff |
| `Elec_All.xlsx` | 全部型号汇总工作簿 |
| `elec.json` | 机器可读数据，供 CubeX / HAL 元数据使用 |

## 6. 生成 / 检查

```bash
cd Elec
python gen_elec.py list                 # 查看已配置型号
python gen_elec.py show CH32X035        # 打印单型号摘要
python gen_elec.py survey             # 定位各手册的电气/Flash/低功耗/唤醒页
python gen_elec.py pages CH32X035 擦写   # 查看 PDF 命中页，人工复核
python gen_elec.py check                # 校验型号/页码/关键字
python gen_elec.py all                  # 生成全部 CSV/XLSX/JSON/README
python gen_elec.py build CH32X035       # 只重建一个型号
```

依赖：`pip install openpyxl pypdf`（`build` 只需要 openpyxl，`pages/check` 需要 pypdf）。

## 7. 新增一个型号

1. 把数据手册放到 `Doc/DS/`；
2. 在 `elec_data.py` 的 `MODELS` 中增加一条记录，填写 `pdf`、概要、低功耗、唤醒、Flash、其他；
3. 每个数值填写 `page`（数据手册页码）；Flash 粒度若来自 EVT 库，在 `flash.note` 中写清头文件；
4. 运行 `python gen_elec.py check`，再 `python gen_elec.py all <MODEL>`；
5. 人工复核 CSV/XLSX，特别是“抽测/非担保”、Typ/Min/Max 和温度条件。

## 8. 提取质量与口径

* **典型值是 datasheet 的 typ 列**；没有 typ 的保留 min/max，不自行取中值。
* **“抽测/实测/非担保”原文保留**，这类寿命数据不能当保证值使用。
* **低功耗电流与测试条件绑定**：VDD、IO 上下拉、外设时钟开关、RAM 保持、LDO 档位不同都会改变结果；表中 `condition` 列保留了这些条件。
* **唤醒时间**：CH5xx 的暂停/睡眠/下电唤醒以 `TSUHSE` 为基准，`TSUHSE` 典型 200 us；CH32 的待机唤醒可能包含代码加载时间（V307 8.9 ms、V407 18.5 ms）。
* **Flash 粒度**：CH32X035 为 256 B 页；CH32M030 为 128 B 页；CH5xx CodeFlash 为 4 B 字/256 B 页写、4 KB 块擦；CH569 为 4 B 字/256 B 页写、4 KB 块擦。
* **数据手册缺项**保留为“DS 未给出”，不用同类芯片猜测补齐；需要时查 RM/EVT 或向原厂确认。

## 9. 复核建议

1. 先看每个型号的 `说明` 页和 `Flash` 页，确认页/扇区/块粒度与项目写入方式一致。
2. 低功耗选型时同时看 `低功耗` 与 `唤醒`，不要只比较最低电流；CH5xx 还要加 `TSUHSE` 和射频占空比。
3. 高可靠/频繁参数存储场景，优先使用“保证值”而不是“抽测值”，并按温度降额。
4. 任何与官方最新版数据手册不一致的地方，以最新版 DS 为准，并回改 `elec_data.py` 后重新生成。

---

生成命令：`python gen_elec.py all`；数据源：`elec_data.py`。
