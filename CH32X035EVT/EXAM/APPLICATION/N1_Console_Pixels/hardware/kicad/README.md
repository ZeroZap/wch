# N1 Console KiCad project

Component-level, schematic-first KiCad project generated from
`../pinmap.csv`, `../HARDWARE.md` and `../BOM.csv`.

The files use the KiCad 8 format (`version 20231120`). KiCad 8, 9 and 10
open them directly; on first save KiCad offers to upgrade the format, which
is expected and harmless. Symbols carry explicit `(instances ...)` blocks so
references (U1, R10, ...) stay annotated when opening in newer versions.

| File | Purpose |
| --- | --- |
| `N1_Console.kicad_pro` | project |
| `N1_Console.kicad_sch` | MCU + power tree + USB/ESD + WS2812 + NOR/OLED + buttons + audio/IR |
| `N1_Console.kicad_sym` | generated symbols (`MCU_CH32X035`, `AHCT125`, `USBLC6`, `USBC`, `BATPROT`, plus R/C/D/Q/SW fallbacks) |
| `sym-lib-table` | maps the `N1` library nickname to the `.kicad_sym` |

Passives and common parts use standard KiCad library IDs (`Device:R`, `Device:C`,
`Device:C_Polarized`, `Device:L`, `Device:D`, `Device:D_Schottky`, `Device:LED`,
`Device:Fuse`, `Device:Buzzer`, `Device:Q_NMOS_GSD`, `Device:Q_PMOS_GSD`,
`Switch:SW_Push`, `Connector_Generic:Conn_01xNN`, `Memory_Flash:W25Q32JVSS`,
`Regulator_Linear:ME6211C33M5G`, `Battery_Management:TP4056`,
`Regulator_Switching:MT3608`). The schematic embeds simple `lib_symbols`
definitions so it opens even without a populated global library; pin numbers
match the standard symbols, so "Update symbols from library" is safe.

Custom logical blocks: `N1:MCU_CH32X035`, `N1:AHCT125` (single-gate
74AHCT1G125), `N1:USBLC6` (6-pin ESD block), `N1:USBC` (6-pin USB-C block),
`N1:BATPROT` (DW01A+FS8205 block). Verify their pinouts against the
datasheets before layout.

## What is included

- CH32X035 LQFP48 symbol, all 48 pins generated from `../pinmap.csv`
- USB-C input, F1, D1, ME6211 3.3 V LDO, 5 V LED rail P-MOS, bulk caps
- 1S LiPo protection block, TP4056 charger, MT3608 boost + feedback divider
- USB-C CC 5.1k, 22R series resistors, USBLC6 ESD
- 74AHCT1G125 (SOT-23-5) WS2812 level shifter + 330R + panel connector + bulk caps
- W25Q32 NOR socket, CS pull-up, decoupling, card-detect switch
- SSD1306 OLED header + 4.7k I2C pull-ups
- 6 buttons, reset RC + key
- buzzer + IR LED MOSFET drivers, IR receiver supply filter
- debug/SWD/UART header

Every pin has a short wire and a **global net label**, so the netlist is
complete without drawing long wires. Components are placed in a functional
grid; rearrange or draw explicit wires for a pretty print before layout.

## Open

1. Open `N1_Console.kicad_pro`.
2. Accept symbol rescue/remap if KiCad asks; the schematic embeds `lib_symbols`.
3. Run **Inspect -> Electrical Rules Checker**. Expect:
   - no missing connections (global labels define the nets);
   - power-flag / pin-type warnings until you add `PWR_FLAG` symbols or set
     the LDO/connector power pins to `power_out` (schematic polish).
4. Verify IC pinouts against the datasheets. `TP4056`, `MT3608`, `USBLC6`,
   `DW01A+FS8205`, `74AHCT125` and the USB-C connector are drawn as logical
   blocks with standard pin names; the footprints are set but the symbols are
   intentionally simple.

## Make the PCB

1. Finish/verify the schematic.
2. **Tools -> Update PCB from Schematic** (F8).
3. Place: LQFP48, USB-C, NOR socket, buttons, OLED header, WS2812 connector,
   power inductors/caps.
4. Follow `../HARDWARE.md` section 10:
   - WS2812 5V/GND >= 1.5 mm, 470 uF near J2, 330R at the MCU side;
   - USB D+/D- 90 ohm differential, 22R close to MCU;
   - NOR SPI short, /CS pull-up near socket;
   - IR LED loop small, C11 close; keep IR receiver away from buzzer;
   - LQFP48 exposed pad / thermal vias per datasheet.

> The MCU footprint is `Package_QFP:LQFP-48_7x7mm_P0.5mm`. Verify the exact
> CH32X035C-R0-1V2 package drawing before ordering.

## Regenerate

```sh
python tools/gen_kicad.py
python tools/check_kicad.py
```

