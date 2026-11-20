# PinMux 维护（按 MCU 型号 + 按 demo）

本目录维护 **“一个 MCU 型号一张表”** 的引脚复用资源。
demo 目录内的 `PINMUX.md` 只描述该 demo 自己的接线、电气与自检步骤。

| 型号 | 数据手册 | 引脚行数 | GPIO/模拟 | 解析方式 | 工作簿 |
| --- | --- | --- | --- | --- | --- |
| CH32X035 | `CH32X035DS0.PDF` | 65 | 60 | 主功能+重映射 | `CH32X035_PinMux.xlsx` |
| CH32X315 | `CH32X315DS0.PDF` | 70 | 64 | 复用+描述 | `CH32X315_PinMux.xlsx` |
| CH32L103 | `CH32L103DS0.PDF` | 41 | 32 | 主功能+重映射 | `CH32L103_PinMux.xlsx` |
| CH32M030 | `CH32M030DS0.PDF` | 42 | 37 | 主功能+重映射 | `CH32M030_PinMux.xlsx` |
| CH32V002 | `CH32V002DS0.PDF` | 21 | 18 | 主功能+重映射 | `CH32V002_PinMux.xlsx` |
| CH32V006 | `CH32V006DS0.PDF` | 32 | 30 | 主功能+重映射 | `CH32V006_PinMux.xlsx` |
| CH32V007 | `CH32V007DS0.PDF` | 34 | 31 | 主功能+重映射 | `CH32V007_PinMux.xlsx` |
| CH32V307 | `CH32V307DS0.PDF` | 82 | 76 | 主功能+重映射 | `CH32V307_PinMux.xlsx` |
| CH32V407 | `CH32V407DS0.PDF` | 89 | 74 | 主功能+重映射 | `CH32V407_PinMux.xlsx` |
| CH569 | `CH569DS1.PDF` | 61 | 51 | 主功能+重映射 | `CH569_PinMux.xlsx` |
| CH572 | `CH572DS1.PDF` | 18 | 13 | 复用+描述 | `CH572_PinMux.xlsx` |
| CH573 | `CH573DS1.PDF` | 32 | 24 | 复用+描述 | `CH573_PinMux.xlsx` |
| CH583 | `CH583DS1.PDF` | 49 | 43 | 复用+描述 | `CH583_PinMux.xlsx` |
| CH585 | `CH585DS1.PDF` | 48 | 41 | 复用+描述 | `CH585_PinMux.xlsx` |
| CH592 | `CH592DS1.PDF` | 30 | 23 | 复用+描述 | `CH592_PinMux.xlsx` |
| CH595 | `CH595DS1.PDF` | 29 | 22 | 复用+描述 | `CH595_PinMux.xlsx` |
| CH596 | `CH596DS1.PDF` | 17 | 14 | 复用+描述 | `CH596_PinMux.xlsx` |

## demo 汇总

| demo | MCU | 引脚数 | 来源 | 目录 |
| --- | --- | --- | --- | --- |
| WS2812_Panel | CH32X035 | 9 | PINMUX.md | `CH32X035EVT/EXAM/APPLICATION/WS2812_Panel` |
| SPI_FLASH (vendor) | CH32X035 | 5 | 内置示例 | `CH32X035EVT/EXAM/SPI/SPI_FLASH` |
| WS2812_LED (vendor) | CH32X035 | 3 | 内置示例 | `CH32X035EVT/EXAM/APPLICATION/WS2812_LED` |
| PIOC 1_Wire (vendor) | CH32X035 | 3 | 内置示例 | `CH32X035EVT/EXAM/PIOC/1_Wire` |
| USB CDC (vendor) | CH32X035 | 4 | 内置示例 | `CH32X035EVT/EXAM/USB/USBFS/DEVICE/SimulateCDC` |

## 生成 / 更新

```bash
cd PinMux
python gen_pinmux.py list          # 查看已配置型号
python gen_pinmux.py scan          # 扫描各 demo 的 PINMUX.md -> demos.json
python gen_pinmux.py all           # 全部型号：extract + scan + build
python gen_pinmux.py all CH32X035  # 只处理某个型号
```

依赖：`pip install pypdf openpyxl`。

## 约定

* 默认复用：AF 名称不带后缀，如 `TX1`、`CK1`、`MCO`、`PIOC_IO0`
* 重映射：AF 名称带选项后缀 `_N`，如 `TX1_2`、`PIOC_IO0_1`
* 封装/型号列头每个型号不同（封装名或型号名），工作簿 `说明` 页保留了原始列头，
  `PinList` 的 `封装/型号引脚串` 为原始值，按列头顺序阅读。
* CH32X035 额外保留 7 个具名封装列，并带有 `WS2812_Panel` demo 详细页。

## 每个 demo 的引脚分配（PINMUX.md）

型号级工作簿记录全量引脚；**demo 级只记录自己用到的引脚**，并且单向汇总：

1. 在 demo 目录写 `PINMUX.md`（可复制 `PinMux/PINMUX.template.md`）：
   引脚表 / 接线网表 / 复用冲突 / 电气要点 / 上电自检；
2. 运行 `python gen_pinmux.py scan`，生成 `demos.json`（demo 名 + MCU + 引脚表）；
3. `build` 时按 MCU 型号把 demo 写入对应工作簿的 `DemoUsage`，
   带“配置/方向/备注”的 demo 还会自动生成一个同名详细工作表；
4. 改了 `PINMUX.md` 只需重新 `scan` + `build`，不再手改 Excel。

## 新增一个型号

1. 把数据手册放到 `Doc/DS/`；
2. 在 `gen_pinmux.py` 的 `CHIPS` 中加一行（型号、PDF、`af`/`list`、列头说明）；
3. `python gen_pinmux.py all <MODEL>`，然后人工复核 CSV；
4. 在工作簿 `DemoUsage` 增加该型号上的 demo 列。

## 提取质量

数据来自各自数据手册的“引脚定义”表，自动解析后**建议人工复核**：

* 多型号/多封装列的对齐（`封装/型号引脚串`）；
* 手册中存在第二张引脚表时（小封装/另一型号），只保留列数最多的一条，
  其余列可能丢失；
* CH32X315 的 `引脚功能` 列带 `(AFn)` 标记，归一到“默认/重映射”意义有限；
* 电源引脚允许重复行（多个 GND/VDD 物理引脚），个别型号可能残留重复的 VDD 行；
* 手册脚注（内部短接、BOOT 引脚、复用优先级、USB 注意事项）未收录，
  需要时补到 `DemoUsage` 或 demo 的 `PINMUX.md`。

板级备注（特定开发板接线）仍保留在仓库根目录 `PinMux.md`。
