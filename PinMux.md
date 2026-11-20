> **说明**：本文件是手工整理的板级引脚备注。
> 按 MCU 型号维护的引脚复用矩阵（Excel）在 [`PinMux/`](PinMux/README.md)：
> 已覆盖 17 个型号（CH32X035/X315/L103/M030/V002/V006/V007/V307/V407、
> CH569/CH572/CH573/CH583/CH585/CH592/CH595/CH596），每个 demo 目录下另有自己的
> `PINMUX.md`，只记录该 demo 的引脚分配、接线与自检。

### CH32V407VET6

| 引脚号 | 引脚名称        | 类型/说明                      |
| :----- | :-------------- | :----------------------------- |
| 1      | PE2             | 通用 I/O（GPIO）               |
| 2      | PE3             | 通用 I/O（GPIO）               |
| 3      | PE4             | 通用 I/O（GPIO）               |
| 4      | PE5             | 通用 I/O（GPIO）               |
| 5      | PE6             | 通用 I/O（GPIO）               |
| 6      | VBAT            | ==后备电池供电==               |
| 7      | PC13/TAMPER-RTC | GPIO / 入侵检测 / RTC          |
| 8      | PC14/OSC32_IN   | ==GPIO / 32.768 kHz 晶振输入== |
| 9      | PC15/OSC32_OUT  | ==GPIO / 32.768 kHz 晶振输出== |
| 10     | VSS             | ==数字地==                     |
| 11     | VDD             | ==数字电源==                   |
| 12     | OSC_IN          | ==主晶振输入（8 MHz 等==）     |
| 13     | OSC_OUT         | ==主晶振输出==                 |
| 14     | NRST            | ==复位信号输入（低有效）==     |
| 15     | PC0/ADC10       | ==ETH/LINK_LED==               |
| 16     | PC1/ADC11       | ==ETH/ACT_LED==                |
| 17     | PC2/ADC12       | GPIO / ADC 输入通道 12         |
| 18     | PC3/ADC13       | GPIO / ADC 输入通道 13         |
| 19     | VSSA            | ==模拟地==                     |
| 20     | VREF-           | ==ADC 负参考电压==             |
| 21     | VREF+           | ==ADC 正参考电压==             |
| 22     | VDDA            | ==模拟电源==                   |
| 23     | PA0/WKUP/ADC0   | GPIO / 唤醒 / ADC 输入通道 0   |
| 24     | PA1/ADC1        | GPIO / ADC 输入通道 1          |
| 25     | PA2/ADC2        | GPIO / ADC 输入通道 2          |
| 26     | PA3/ADC3        | GPIO / ADC 输入通道 3          |
| 27     | VSS             | ==数字地==                     |
| 28     | VDD             | ==数字电源==                   |
| 29     | PA4/ADC4        | DAC1                           |
| 30     | PA5/ADC5        | NOR_CLK / 后面要更换/DAC2      |
| 31     | PA6/ADC6        | NOR_DO(IO1)                    |
| 32     | PA7/ADC7        | NOR_DI(IO0)                    |
| 33     | PC4/ADC14       | GPIO / ADC 输入通道 14         |
| 34     | PC5/ADC15       | GPIO / ADC 输入通道 15         |
| 35     | PB0/ADC8        | GPIO / ADC 输入通道 8          |
| 36     | PB1/ADC9        | GPIO / ADC 输入通道 9          |
| 37     | PB2/BOOT1       | GPIO / BOOT1 引脚              |
| 38     | PE7             | 通用 I/O（GPIO）               |
| 39     | PE8             | 通用 I/O（GPIO）               |
| 40     | PE9             | 内核电                         |
| 41     | PE10            | 地                             |
| 42     | PE11            | 通用 I/O（GPIO）               |
| 43     | PE12            | 通用 I/O（GPIO）               |
| 44     | PE13            | 通用 I/O（GPIO）               |
| 45     | PE14            | 通用 I/O（GPIO）               |
| 46     | PE15            | 通用 I/O（GPIO）               |
| 47     | PB10            | 通用 I/O（GPIO）               |
| 48     | PB11            | 通用 I/O（GPIO）               |
| 49     | VSS             | 通用 I/O（GPIO）               |
| 50     | VIO             | 通用 I/O（GPIO）               |
| 51     | PB12            | 通用 I/O（GPIO）               |
| 52     | PB13            | ==SD-DAT0==                    |
| 53     | PB14            | ==SD-DAT1D==                   |
| 54     | PB15            | 通用 I/O（GPIO）               |
| 55     | PD8             | 通用 I/O（GPIO）               |
| 56     | PD9             | 通用 I/O（GPIO）               |
| 57     | PD10            | 通用 I/O（GPIO）               |
| 58     | PD11            | 通用 I/O（GPIO）               |
| 59     | PD12            | 通用 I/O（GPIO）               |
| 60     | PD13            | 通用 I/O（GPIO）               |
| 61     | PD14            | 通用 I/O（GPIO）               |
| 62     | PD15            | 通用 I/O（GPIO）               |
| 63     | PC6/RXP         | ==ETH/RXP==                    |
| 64     | PC7/RXN         | ==ETH/RXN==                    |
| 65     | PC8/TXP         | ==ETH/TXP / /SD-DAT0==         |
| 66     | PC9/TXN         | ==ETH/TXN //SD-DAT1==          |
| 67     | PA8             | TIM1_CH1                       |
| 68     | PA9             | ==OTG_FS_VBUS==                |
| 69     | PA10            | ==OTG_FS_ID==                  |
| 70     | PA11/USB1DM     | ==OTG_FS_DM==                  |
| 71     | PA12/USB1DP     | ==OTG_FS_DP==                  |
| 72     | PA13/SWIO       | ==SWD==                        |
| 73     | NC              | ==NC==                         |
| 74     | VSS             | ==地==                         |
| 75     | VDD             | ==数字电==                     |
| 76     | PA14/SWCLK      | ==SWCLK==                      |
| 77     | PA15            | ==LED1==                       |
| 78     | PC10            | ==SD-DAT2==                    |
| 79     | PC11            | ==SD-CD/SD-DAT3==              |
| 80     | PC12            | ==SD-CMD==                     |
| 81     | PD0             | 通用 I/O（GPIO）               |
| 82     | PD1             | 通用 I/O（GPIO）               |
| 83     | PD2             | ==SD-CMD==                     |
| 84     | PD3             | 通用 I/O（GPIO）               |
| 85     | PD4             | 通用 I/O（GPIO）               |
| 86     | PD5             | 通用 I/O（GPIO）               |
| 87     | PD6             | 通用 I/O（GPIO）               |
| 88     | PD7             | ==SD-SW==                      |
| 89     | PB3             | ==SPI_CLK==                    |
| 90     | PB4             | ==LED2==                       |
| 91     | PB5             | 通用 I/O（GPIO）               |
| 92     | PB6/USB2DM      | ==HS_USB_DM==                  |
| 93     | PB7/USB2DP      | ==HS_USB_DP==                  |
| 94     | BOOT0           | ==BOOT0==                      |
| 95     | PB8             | 通用 I/O（GPIO）               |
| 96     | PB9             | 通用 I/O（GPIO）               |
| 97     | PE0             | 通用 I/O（GPIO）               |
| 98     | PE1             | 通用 I/O（GPIO）               |
| 99     | VSS             | ==数字地==                     |
| 100    | VDD             | ==数字电==                     |

### CH32V407VET6

| 引脚号 | 引脚名称        | 类型/说明                    |
| :----- | :-------------- | :--------------------------- |
| 1      | PE2             | 通用 I/O（GPIO）             |
| 2      | PE3             | 通用 I/O（GPIO）             |
| 3      | PE4             | 通用 I/O（GPIO）             |
| 4      | PE5             | 通用 I/O（GPIO）             |
| 5      | PE6             | 通用 I/O（GPIO）             |
| 6      | VBAT            | 后备电池供电                 |
| 7      | PC13/TAMPER-RTC | GPIO / 入侵检测 / RTC        |
| 8      | PC14/OSC32_IN   | GPIO / 32.768 kHz 晶振输入   |
| 9      | PC15/OSC32_OUT  | GPIO / 32.768 kHz 晶振输出   |
| 10     | VSS             | 数字地                       |
| 11     | VDD             | 数字电源                     |
| 12     | OSC_IN          | 主晶振输入（8 MHz 等）       |
| 13     | OSC_OUT         | 主晶振输出                   |
| 14     | NRST            | 复位信号输入（低有效）       |
| 15     | PC0/ADC10       | GPIO / ADC 输入通道 10       |
| 16     | PC1/ADC11       | GPIO / ADC 输入通道 11       |
| 17     | PC2/ADC12       | GPIO / ADC 输入通道 12       |
| 18     | PC3/ADC13       | GPIO / ADC 输入通道 13       |
| 19     | VSSA            | 模拟地                       |
| 20     | VREF-           | ADC 负参考电压               |
| 21     | VREF+           | ADC 正参考电压               |
| 22     | VDDA            | 模拟电源                     |
| 23     | PA0/WKUP/ADC0   | GPIO / 唤醒 / ADC 输入通道 0 |
| 24     | PA1/ADC1        | GPIO / ADC 输入通道 1        |
| 25     | PA2/ADC2        | GPIO / ADC 输入通道 2        |
| 26     | PA3/ADC3        | GPIO / ADC 输入通道 3        |
| 27     | VSS             | 数字地                       |
| 28     | VDD             | 数字电源                     |
| 29     | PA4/ADC4        | GPIO / ADC 输入通道 4        |
| 30     | PA5/ADC5        | GPIO / ADC 输入通道 5        |
| 31     | PA6/ADC6        | GPIO / ADC 输入通道 6        |
| 32     | PA7/ADC7        | GPIO / ADC 输入通道 7        |
| 33     | PC4/ADC14       | GPIO / ADC 输入通道 14       |
| 34     | PC5/ADC15       | GPIO / ADC 输入通道 15       |
| 35     | PB0/ADC8        | GPIO / ADC 输入通道 8        |
| 36     | PB1/ADC9        | GPIO / ADC 输入通道 9        |
| 37     | PB2/BOOT1       | GPIO / BOOT1 引脚            |
| 38     | PE8             | 通用 I/O（GPIO）             |
| 39     | PE9             | 通用 I/O（GPIO）             |
| 40     | VDDK            | 内核电                       |
| 41     | VSS             | 地                           |
| 42     | MDIRN           | 通用 I/O（GPIO）             |
| 43     | MDIRP           | 通用 I/O（GPIO）             |
| 44     | MDITN           | 通用 I/O（GPIO）             |
| 45     | MDITP           | 通用 I/O（GPIO）             |
| 46     | VDD             | 通用 I/O（GPIO）             |
| 47     | VSS             | 通用 I/O（GPIO）             |
| 48     | PE7             | 通用 I/O（GPIO）             |
| 49     | PB10            | 通用 I/O（GPIO）             |
| 50     | PB11            | 通用 I/O（GPIO）             |
| 51     | PD8             | 通用 I/O（GPIO）             |
| 52     | PB12            | 通用 I/O（GPIO）             |
| 53     | PB13            | 通用 I/O（GPIO）             |
| 54     | PB14            | 通用 I/O（GPIO）             |
| 55     | PB15            | 通用 I/O（GPIO）             |
| 56     | PD9             | 通用 I/O（GPIO）             |
| 57     | PD10            | 通用 I/O（GPIO）             |
| 58     | PD11            | 通用 I/O（GPIO）             |
| 59     | PD12            | 通用 I/O（GPIO）             |
| 60     | PD13            | 通用 I/O（GPIO）             |
| 61     | PD14            | 通用 I/O（GPIO）             |
| 62     | PD15            | 通用 I/O（GPIO）             |
| 63     | PC6             | 通用 I/O（GPIO）             |
| 64     | PC7             | 通用 I/O（GPIO）             |
| 65     | PC8             | 通用 I/O（GPIO）             |
| 66     | PC9             | 通用 I/O（GPIO）             |
| 67     | PA8             | 通用 I/O（GPIO）             |
| 68     | PA9             | 通用 I/O（GPIO）             |
| 69     | PA10            | 通用 I/O（GPIO）             |
| 70     | PA11            | 通用 I/O（GPIO）             |
| 71     | PA12            | 通用 I/O（GPIO）             |
| 72     | PA13/SWIO       | 通用 I/O（GPIO，JTAG/SWD）   |
| 73     | PE10            | 通用 I/O（GPIO）             |
| 74     | PE11            | 通用 I/O（GPIO）             |
| 75     | VDD             | 数字电                       |
| 76     | PA14/SWCLK      | 通用 I/O（GPIO，JTAG/Sck）   |
| 77     | PA15            | 通用 I/O（GPIO）             |
| 78     | PC10            | 通用 I/O（GPIO）             |
| 79     | PC11            | 通用 I/O（GPIO）             |
| 80     | PC12            | 通用 I/O（GPIO）             |
| 81     | PD0             | 通用 I/O（GPIO）             |
| 82     | PD1             | 通用 I/O（GPIO）             |
| 83     | PD2             | 通用 I/O（GPIO）             |
| 84     | PD3             | 通用 I/O（GPIO）             |
| 85     | PD4             | 通用 I/O（GPIO）             |
| 86     | PD5             | 通用 I/O（GPIO）             |
| 87     | PD6             | 通用 I/O（GPIO）             |
| 88     | PD7             | 通用 I/O（GPIO）             |
| 89     | PB3             | 通用 I/O（GPIO）             |
| 90     | PB4             | 通用 I/O（GPIO）             |
| 91     | PB5             | 通用 I/O（GPIO）             |
| 92     | PB6             | 通用 I/O（GPIO）             |
| 93     | PB7             | 通用 I/O（GPIO）             |
| 94     | PE12            | 通用 I/O（GPIO）             |
| 95     | PB8             | 通用 I/O（GPIO）             |
| 96     | PB9             | 通用 I/O（GPIO）             |
| 97     | PE0             | 通用 I/O（GPIO）             |
| 98     | PE1             | 通用 I/O（GPIO）             |
| 99     | VSS             | 数字地                       |
| 100    | VDD             | 数字电                       |
