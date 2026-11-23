# hardware/

本目录是 [`../HARDWARE.md`](../HARDWARE.md) 的机器可读副本：

| 文件         | 用途                                                                      |
| ------------ | ------------------------------------------------------------------------- |
| `pinmap.csv` | LQFP48 / TSSOP20 每个引脚的网络名、复用功能和硬件备注，可直接对照画原理图 |
| `BOM.csv`    | 全功能版 BOM，可在 Excel/LibreOffice 中打开                               |
| `kicad/`     | 生成的 KiCad 8 工程（`N1_Console.kicad_pro/.kicad_sch/.kicad_sym`）       |

KiCad 工程由 `tools/gen_kicad.py` 从 `pinmap.csv`/`BOM.csv` 生成，已经是
**器件级**原理图：CH32X035 48 脚符号、USB-C/ESD、F1/D1、ME6211 LDO、
AO3401 LED 电源开关、TP4056 + 电池保护 + MT3608 升压、74AHCT125 WS2812
电平转换、W25Q32、SSD1306、6 键、复位、蜂鸣器/红外 MOSFET 驱动，以及
全部 R/C/L/D/Q/SW/BZ/LED/FUSE。每个引脚都带短线 + 全局网络标签。

结构自检：`python tools/check_kicad.py`（括号、UUID、实例/标签数量）。
生成器还会检查“每个符号的每个引脚都有网络映射”，避免漏连。

`schematic` 目前以 `HARDWARE.md` 中的分模块文本原理图 + 网表为准：

1. 电源树（USB-C / 1S 锂电 / 3.3V LDO / 5V 升压）
2. MCU 引脚表（LQFP48 全功能、TSSOP20 精简）
3. WS2812 电平转换与储能
4. SPI NOR 卡带座
5. USB-C + ESD
6. 6 键
7. 蜂鸣器 + 38 kHz 红外发射共用 PA3
8. 红外接收、OLED、SWD/串口扩展
9. 关键网络表、BOM、PCB Layout、上板调试清单

> LQFP48 封装**没有**引出 PC0..PC5；不要照搬 QFN28/QFN20/TSSOP20 的引脚号。
> 电池电压使用内部 VREFINT，不占外部 ADC 脚。
