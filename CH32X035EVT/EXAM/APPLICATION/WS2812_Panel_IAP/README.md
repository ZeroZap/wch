# WS2812_Panel_IAP — 蛇形 WS2812 灯带的 IAP 版本

`WS2812_Panel` 的应用层原样搬过来，接上 `IAP_USB_SIGNED` 的 boot：镜像链接在
`0x2800`（50 KB 槽），启动前由 boot 做 SHA-256 + HMAC 校验，支持从灯带 App
自己发起升级。

原 `WS2812_Panel` 目录**没有改动**，这里是一份独立拷贝。

## 和 `WS2812_Panel` 的差异

| 项 | `WS2812_Panel` | 这里 |
| --- | --- | --- |
| 链接地址 | `0x0000` / 62 KB | **`0x2800` / 50 KB**（前 10 KB 是 boot） |
| 封装 | 固定 LQFP48 | **LQFP48 / TSSOP20 二选一**（`-DWSP_BOARD=...`，见 `PINMUX.md`） |
| 启动方式 | 复位直接跑 | boot 校验通过后经软件中断跳转 |
| USB PID | `0xFE0C` | `0xFE0E`（boot 是 `0xFE0D`） |
| 升级 | 只能 WCH-Link | USB CDC 下发 + 可选 AES 加密 + 签名 |
| 新增命令 | — | `0x20 UPGRADE`：回 ACK 后写 flag 并复位进 boot |
| 其余 | — | 面板 / 特效 / 字库 / OSD / 外部 NOR / 按键 **完全不变** |

新增文件只有两个：`User/app_iap.c/.h`（写内部 flag 页 + 复位）。

## 封装选择（LQFP48 / CH32X033 TSSOP20）

一套源码编译期选封装，引脚定义集中在 `User/board_config.h`：

```sh
sh tools/build.sh              # LQFP48（默认）：WS2812 = PC7，保留 SDI 调试
sh tools/build.sh --tssop20    # TSSOP20：WS2812 = PC18，日志编译掉
```

两封装的差异只有两处：`PC7` 和 `PB10` 在 TSSOP20 上不存在。

| 版本 | WS2812 脚 | 调试串口 | flash | RAM |
| --- | --- | --- | --- | --- |
| LQFP48 | PC7（PIOC 重映射，SDI 保留） | PB10 | 26812 B | 8284 B |
| TSSOP20 | **PC18**（SDI 失效） | 无（`printf` 编译掉） | **19936 B** | 8144 B |

TSSOP20 的代价和应对（调试口、串口取舍）见 [`PINMUX.md`](PINMUX.md)。

## 构建

```sh
sh tools/build.sh                        # 默认几何 + LQFP48
sh tools/build.sh --tssop20              # CH32X033 TSSOP20
sh tools/build.sh -DPANEL_W=16 -DPANEL_H=32
sh tools/build.sh --no-pack              # 只出 elf/bin
sh tools/build.sh --standalone           # 链接在 0x0，纯调试用（不经过 boot）
```

`build/` 产物：

| 文件 | 用途 |
| --- | --- |
| `panel.elf/.bin/.map` | 应用本体，链接在 `0x2800` |
| `panel.img` | 签名镜像，给 USB 升级用 |
| `factory.bin` | **boot + app + 头页 + flag 页**，61952 B，调试器一次刷完 |
| `factory_payload.bin` / `factory_meta.bin` | 需要分区写入的烧录器用 |
| `panel_standalone.elf/.bin` | `--standalone` 时生成，链接在 `0x0` |

`factory.bin` 需要 `IAP_USB_SIGNED/build/boot/boot.bin` 已存在；没有就跳过并提示：

```sh
cd ../IAP_USB_SIGNED && sh tools/build.sh boot
```

## 三种烧录方式

```sh
# 1) 调试器，一次刷完（生产/换机也用它）
sh ../IAP_USB_SIGNED/tools/flash.sh factory build/factory.bin

# 2) 现场升级，不用调试器（app 已经在跑时最方便）
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM62 --reboot-to-boot
#   等 FE0E 变成 FE0D
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM61 --image build/panel.img

# 3) 调 app 本身：不经过 boot，直接在 0x0 跑，可随便单步/断点
sh tools/build.sh --standalone
#   用 WCH-Link 烧 build/panel_standalone.elf（会覆盖 boot，调完按 1) 刷回来）
```

> 为什么调试器不能"只刷 app"：WCH 的 OpenOCD flash 驱动在 `program` 前会**整片擦除**
> （实测：只写 `0x0` 的 boot 也会把 `0xF000` 的头页/flag 冲掉）。所以调试器侧永远用
> `factory.bin`；只想换应用就走 USB。

## 进入升级模式的唯一入口：命令

App 里**没有**按键升级手势，升级只从 USB 命令进入：

```sh
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM62 --reboot-to-boot
# app 回 ACK -> 写 0xF100 的 UPGRADE flag -> 复位 -> 出现 1A86:FE0D
```

或者主机直接用帧格式发 `0x20`（`A5 5A 20 00 00 <crc8>`），效果一样。
按键仍然只管"切特效 / 切模式"。

**上电按住按键（PA0）→ 停在 boot**，这是不依赖上位机的兜底入口：

```
断电 -> 按住 PA0 -> 上电 -> 松开
   boot 读到 PA0 为低 -> 不跳 app -> 出现 1A86:FE0D
```

正常上电（不按键）时 boot 只多花约 10 ms 做这次判定。相关宏在
`IAP_USB_SIGNED/boot/User/boot_info.h`：`BOOT_BUTTON_STAY`（默认 1）、
`BOOT_BUTTON_PORT` / `BOOT_BUTTON_PIN`（默认 PA0）。

boot 的其它判断还有：flag 页是 `UPGRADE`、或者镜像校验没过；
以及正常启动时"USB 枚举后 1.2 s（`BOOT_STAY_WINDOW_MS`）内收到 `BEGIN`"。
三层加起来，app 起不来时总能救回来，不需要调试器。

## 协议

帧格式和 boot 完全一致（`A5 5A | CMD | LEN | payload | CRC8`），原有命令全部保留，
新增一条：

| CMD | 说明 |
| --- | --- |
| `0x20 UPGRADE` | 无 payload；先 ACK 再写 `0xF100` 的 `UPGRADE` flag 并复位，boot 看到后停在升级模式 |

因为格式一致，`tools/wsp_cdc.py` 和 `iap_flash.py --reboot-to-boot` 都能直接用。

## 尺寸（16×16 默认几何）

| 封装 | flash（`.bin`） | 占 50 KB 槽 | RAM（data+bss，含 2K 栈） | 占 16 KB |
| --- | --- | --- | --- | --- |
| LQFP48 | 26812 B | 52% | 8284 B | 51% |
| TSSOP20 | 19936 B | 39% | 8144 B | 50% |

16×32 几何：LQFP48 26908 B / RAM 11356 B；TSSOP20 约 20 KB / RAM 11216 B，均在预算内。
`Ld/Link.ld` 里有 `ASSERT(_ebss <= ...)`，溢出会直接编译失败。

## 注意

- **TSSOP20 上 app 会关掉 SDI**（WS2812 必须用 PC18/PC19）：调试请在 LQFP48 板上
  用 `--standalone`；上板更新一律走 USB 或 `factory.bin`。
- **Flash 62 KB**：CH32X033 / CH32X035 各级封装一致，两个封装共用同一份分区表
  （boot 10 KB / app 50 KB / 头页 0xF000 / flag 页 0xF100）。
- **PIOC 占 RAM 高 4 KB**（`0x20004000..0x20004FFF`）：boot 不碰这块，跳转后由 app
  重新初始化，两者互不影响。
- **外部 NOR 与内部 flag 页无关**：`norflash.c` 走 SPI 存图片，`app_iap_request()`
  只动内部 `0xF100`。
- **升级耗时**：26.8 KB ≈ 27 扇区擦除（110 ms）+ 105 页编程 + 传输 ~1.4 s，
  `panel.img` 全流程约 1.5 s（`DATA ≈ 19.5 kB/s`）。
- key 变了要重新打包：`iap_pack.py` 默认读 `IAP_USB_SIGNED/boot/User/img_key.c`，
  和 boot 用的是同一份；用了 `-D` 覆盖时两边都要传同一套 key。
