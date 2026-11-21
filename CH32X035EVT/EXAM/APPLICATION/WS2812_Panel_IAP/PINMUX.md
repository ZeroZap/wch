# WS2812_Panel_IAP — PinMux（LQFP48 / TSSOP20 双封装）

> 适用工程：`CH32X035EVT/EXAM/APPLICATION/WS2812_Panel_IAP`
> 引脚数据来自 [`PinMux/CH32X035_pins.csv`](../../../../PinMux/CH32X035_pins.csv) 的
> `LQFP48` / `TSSOP20` 两列（两封装同一颗 die，只是引出的脚不同）。
> 汇总表：[`PinMux/CH32X035_PinMux.xlsx`](../../../../PinMux/CH32X035_PinMux.xlsx)

## 1. 怎么选

一套源码，编译期选封装，默认 LQFP48：

```sh
sh tools/build.sh              # 或 --lqfp48：WS2812 走 PC7
sh tools/build.sh --tssop20    # CH32X033F8P6 / TSSOP20：WS2812 走 PC18
```

等价于 `-DWSP_BOARD=WSP_BOARD_TSSOP20`（数值 `2`）。引脚定义集中在
`User/board_config.h`，`tools/build.sh` 只负责传这个宏。

```c
#define WSP_BOARD_LQFP48        1
#define WSP_BOARD_TSSOP20       2
```

## 2. 引脚分配对照

| 功能 | LQFP48 引脚 | TSSOP20 引脚 | 端口 | 配置 |
| --- | --- | --- | --- | --- |
| WS2812 数据（PIOC IO0） | 19 | **19** | LQFP48: **PC7**<br>TSSOP20: **PC18** | `AF_PP` 50 MHz |
| 用户按键（低有效） | 10 | 6 | PA0 | `IPU` |
| SPI1 NOR CS | 12 | 8 | PA2 | `Out_PP`，上电拉高 |
| SPI1 NOR SCK | 15 | 11 | PA5 | `AF_PP`，12 MHz，模式 3 |
| SPI1 NOR MISO | 16 | 12 | PA6 | `IN_FLOATING` |
| SPI1 NOR MOSI | 17 | 13 | PA7 | `AF_PP` |
| USB FS D− | 32 | 17 | PC16 | `IN_FLOATING` |
| USB FS D+ | 33 | 18 | PC17 | `IPU` + `AFIO->CTLR` 1.5 k 上拉 |
| 调试串口 TX（可选） | 30 | — | PB10 | `AF_PP`，115200 8N1 |
| 1 ms 时基 | — | — | TIM3（内部） | 不占引脚 |
| 微秒延时 | — | — | SysTick（内部） | 不占引脚 |

TSSOP20 只有 20 个脚（GPIO 14 个）：`PB12 PC14 PC15 PC1 PA0 PA1 PA2 PA3 PA4 PA5 PA6 PA7 PB1 PC16 PC17 PC18 PC19`。
`PC7` 和 `PB10` **不存在**，这是两个封装唯一的实质差异。

## 3. TSSOP20 的两个硬约束

### 3.1 WS2812 只能在 PC18 / PC19，SDI 调试随之失效

TSSOP20 上没有 PC7，PIOC 的 `IO0/IO1` 只引出到 PC18 / PC19，
而这两根正是 SDI 调试口（`DIO` / `DCK`）。所以：

```c
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC18
```

`ws2812.c` 在初始化时会执行 `GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE)`，
**app 跑起来之后 WCH-Link 就接不上了**。应对方式：

| 场合 | 做法 |
| --- | --- |
| 生产 / 换机 | `flash.sh factory` 一次写完整片，不需要再连调试器 |
| 现场升级 | **按住按键 3 s** 进升级模式，再用 USB CDC（`iap_flash.py`），完全不碰调试口 |
| 有上位机 | 发 `0x20 UPGRADE`（`iap_flash.py --reboot-to-boot`），效果同上 |
| 需要单步调试 | 用 LQFP48 开发板 + `sh tools/build.sh --standalone`（链接在 `0x0`） |
| 必须在 TSSOP20 上调试 | 只能在上电瞬间抢连；或者把 PIOC 换成软件翻转（本工程未实现） |

### 3.2 没有调试串口

`PB10` 未引出，而 TSSOP20 上其它可用的 UART TX 全部被占用：

| 候选 TX | 冲突 |
| --- | --- |
| PA2 (USART2_TX) | NOR CS |
| PA5 (USART4_TX) | NOR SCK |
| PA7 (USART1_TX remap) | NOR MOSI |
| PC16 / PC17 (USART4) | USB |
| PC18 (USART3_TX) | WS2812 |

所以 TSSOP20 版把日志编译掉（`WSP_DEBUG_UART = 0` → `printf` 变空宏），
既不会卡在永远不完成的发送上，也省了约 **6.9 KB** 的字符串：

| 版本 | text | data | flash 合计 | RAM（data+bss） |
| --- | --- | --- | --- | --- |
| LQFP48（16×16） | 26816 | 208 | 27024 B | 8284 B |
| TSSOP20（16×16） | 19996 | 80 | **20076 B** | 8144 B |

需要串口日志时，把 `WSP_DEBUG_UART` 改成 1 并自行挑一个能牺牲的脚。

## 4. 自检

引脚选择是纯编译期行为，可以用一个探针文件直接断言（推荐放进 CI）：

```c
#include "wsp_config.h"
_Static_assert(WS2812_PIOC_PIN == WS2812_PIOC_PC18, "tssop20 must use PC18");
```

```sh
# TSSOP20 应通过
riscv-none-embed-gcc -fsyntax-only -march=rv32imacxw -mabi=ilp32 \
  -IUser -I../../SRC/Core -I../../SRC/Debug -I../../SRC/Peripheral/inc \
  -DWSP_BOARD=WSP_BOARD_TSSOP20 probe.c
```

本工程实测：

- `-DWSP_BOARD=WSP_BOARD_LQFP48` → `WS2812_PIOC_PIN == 1`（PC7）
- `-DWSP_BOARD=WSP_BOARD_TSSOP20` → `WS2812_PIOC_PIN == 2`（PC18）
- TSSOP20 却断言 PC7 → 编译失败（符合预期）

### 验证状态

| 版本 | 状态 |
| --- | --- |
| LQFP48（CH32X035C-R0-1V2） | ✅ 已上板：factory.bin 刷入后 `FE0E` 出现、灯带正常、USB 升级通过 |
| TSSOP20（CH32X033） | ⚠️ 仅编译期验证：两种几何都链接通过、`_Static_assert` 断言通过、镜像 19.9 / 20.0 KB；**暂无实物，未上板** |

拿到 TSSOP20 板子后的首次上板步骤：

1. `sh tools/build.sh --tssop20` → `build/factory.bin`
2. `sh ../IAP_USB_SIGNED/tools/flash.sh factory build/factory.bin`
3. 万用表/示波器量 **PC18（TSSOP20 第 19 脚）**：应有 800 kHz、高电平 5V 经电平转换后的单线波形
4. 灯带不亮时先查：PC18 波形 → WS2812 电平/VCC → 颜色顺序宏
5. USB 侧确认 `1A86:FE0E` 出现，再跑一次 `iap_flash.py --image build/panel.img` 验证升级闭环

```sh
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COMx --info   # 看 state / slot
```

## 5. Flash 预算

两封装共用同一份分区表（`shared/img_hdr.h`）：

```
0x0000..0x27FF  boot 10 KB
0x2800..0xEFFF  app  50 KB
0xF000 / 0xF100 镜像头 / flag 页
0xF200..0xF7FF  1.5 KB 用户区
```

**CH32X033 与 CH32X035 各级封装都是 62 KB 可用 Flash（已确认）**，两个封装共用
同一份分区表，boot 的校验范围不会因为换封装而对不上。OpenOCD 的探测结果也是
`flash size = 62kbytes`。

> 只有换成 Flash 更小的别的型号（例如 32 KB 的变体）才需要同步改
> `shared/img_hdr.h` 的 `IMG_SLOT_SIZE` 和两个 `Link.ld`；
> `Ld/Link.ld` 里的 `ASSERT` 和 `LENGTH` 会先报错，不会悄悄越界。

TSSOP20 版镜像只有 20.1 KB，50 KB 槽的余量约 30 KB，够放更大的点阵或字库。
