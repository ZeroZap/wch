#!/usr/bin/env python3
"""Generate the component-level KiCad 8 schematic for N1_Console.

The generator is deliberately schematic-first:
  - the MCU symbol is generated from hardware/pinmap.csv, so pin numbers cannot
    drift from the firmware;
  - every component is a real symbol (R/C/L/D/Q/SW/BZ/LED/FUSE + the ICs) and
    every pin gets a short wire + a global net label, so KiCad produces a
    complete netlist without a rat's nest of long wires;
  - the power tree, USB ESD, WS2812 level shifter, NOR/OLED pull-ups, reset and
    buzzer/IR drivers are included.

Outputs (hardware/kicad/):
  N1_Console.kicad_pro
  N1_Console.kicad_sch
  N1_Console.kicad_sym
  sym-lib-table
  README.md

Run:
  python tools/gen_kicad.py
  python tools/check_kicad.py
"""

import csv
import json
import os
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(HERE)
OUT = os.path.join(PROJ, "hardware", "kicad")
PINMAP = os.path.join(PROJ, "hardware", "pinmap.csv")

SHEET_UUID = str(uuid.uuid4())


def uid():
    return str(uuid.uuid4())


def q(s):
    return '"' + s.replace('"', '\\"') + '"'


def effect(size=1.27, justify=None, hide=False):
    j = ""
    if justify:
        j = " (justify %s)" % justify
    h = " hide" if hide else ""
    return "(effects (font (size %s %s))%s%s)" % (size, size, j, h)


def rect(x1, y1, x2, y2):
    return [
        "    (rectangle (start %.2f %.2f) (end %.2f %.2f)" % (x1, y1, x2, y2),
        "      (stroke (width 0.254) (type default)) (fill (type background)))",
    ]


def pind(num, name, x, y, angle, ptype="passive", length=2.54):
    return {
        "num": str(num),
        "name": name,
        "x": float(x),
        "y": float(y),
        "angle": int(angle),
        "type": ptype,
        "length": float(length),
    }


def pin_sexpr(p):
    return [
        "    (pin %s line (at %.2f %.2f %d) (length %.2f)"
        % (p["type"], p["x"], p["y"], p["angle"], p["length"]),
        "      (name %s %s)" % (q(p["name"]), effect(1.0)),
        "      (number %s %s))" % (q(p["num"]), effect(1.0)),
    ]


def make_2pin_vertical(n1="1", n2="2", w=2.032, h=5.08):
    return rect(-w / 2, h / 2, w / 2, -h / 2), [
        pind("1", n1, 0, h / 2 + 2.54, 270),
        pind("2", n2, 0, -h / 2 - 2.54, 90),
    ]


def make_2pin_horizontal(n1="1", n2="2", w=2.54, h=1.27):
    return rect(-w, -h, w, h), [
        pind("1", n1, -w - 2.54, 0, 0),
        pind("2", n2, w + 2.54, 0, 180),
    ]


def make_mos():
    return rect(-2.54, -3.81, 2.54, 3.81), [
        pind("1", "G", -5.08, 0, 0),
        pind("2", "S", 0, -6.35, 90),
        pind("3", "D", 0, 6.35, 270),
    ]


def make_connector(n):
    top = (n - 1) * 1.27
    body = rect(-2.54, top + 1.27, 2.54, -top - 1.27)
    pins = [pind(str(i + 1), str(i + 1), -5.08, top - i * 2.54, 0) for i in range(n)]
    return body, pins


def make_ic(left, right, top=(), bottom=(), width=15.24, spacing=5.08):
    n = max(len(left), len(right), len(top), len(bottom), 2)
    height = max(10.16, (n - 1) * spacing + 5.08)
    body = rect(-width / 2, height / 2, width / 2, -height / 2)
    pins = []
    num = 1
    for i, nm in enumerate(left):
        y = (len(left) - 1) * spacing / 2 - i * spacing
        pins.append(pind(str(num), nm, -width / 2 - 2.54, y, 0))
        num += 1
    for i, nm in enumerate(right):
        y = (len(right) - 1) * spacing / 2 - i * spacing
        pins.append(pind(str(num), nm, width / 2 + 2.54, y, 180))
        num += 1
    for i, nm in enumerate(top):
        x = (len(top) - 1) * spacing / 2 - i * spacing
        pins.append(pind(str(num), nm, x, height / 2 + 2.54, 270))
        num += 1
    for i, nm in enumerate(bottom):
        x = (len(bottom) - 1) * spacing / 2 - i * spacing
        pins.append(pind(str(num), nm, x, -height / 2 - 2.54, 90))
        num += 1
    return body, pins


def make_mcu(pin_rows):
    body = rect(-30.48, 46.99, 30.48, -46.99)
    pins = []
    for i, r in enumerate(pin_rows[:24]):
        pins.append(
            pind(r["pin"], r["name"].replace("/", "_"), -33.02, 43.18 - i * 3.81, 0)
        )
    for i, r in enumerate(pin_rows[24:48]):
        pins.append(
            pind(r["pin"], r["name"].replace("/", "_"), 33.02, 43.18 - i * 3.81, 180)
        )
    return body, pins


# ---------------------------------------------------------------------------
# symbol registry
# ---------------------------------------------------------------------------
SYMBOLS = {}


def add_symbol(name, ref, value, body, pins, footprint="", lib=None):
    SYMBOLS[name] = {
        "ref": ref,
        "value": value,
        "body": body,
        "pins": pins,
        "footprint": footprint,
        "lib": lib or ("N1:" + name),
    }


def build_symbols(pin_rows):
    b, p = make_2pin_vertical()
    add_symbol("R", "R", "R", b, p, "Resistor_SMD:R_0402_1005Metric", "Device:R")
    b, p = make_2pin_vertical(w=3.0, h=5.08)
    add_symbol("C", "C", "C", b, p, "Capacitor_SMD:C_0402_1005Metric", "Device:C")
    b, p = make_2pin_vertical(w=3.0, h=5.08)
    add_symbol(
        "CP", "C", "C", b, p, "Capacitor_SMD:CP_Elec_6.3x7.7", "Device:C_Polarized"
    )
    b, p = make_2pin_vertical(w=3.0, h=5.08)
    add_symbol("L", "L", "L", b, p, "Inductor_SMD:L_Bourns_SRR1260", "Device:L")
    b, p = make_2pin_horizontal("K", "A")
    add_symbol("D", "D", "D", b, p, "Diode_SMD:D_SOD-323", "Device:D")
    b, p = make_2pin_horizontal("K", "A")
    add_symbol("DSCH", "D", "D_Schottky", b, p, "Diode_SMD:D_SMA", "Device:D_Schottky")
    b, p = make_2pin_horizontal("K", "A")
    add_symbol("LED", "D", "LED", b, p, "LED_SMD:LED_0805_2012Metric", "Device:LED")
    b, p = make_2pin_horizontal("1", "2")
    add_symbol("FUSE", "F", "Fuse", b, p, "Fuse:Fuse_1206_3216Metric", "Device:Fuse")
    b, p = make_2pin_horizontal("1", "2")
    add_symbol(
        "SW",
        "SW",
        "SW_Push",
        b,
        p,
        "Button_Switch_SMD:SW_SPST_TL3342",
        "Switch:SW_Push",
    )
    b, p = make_2pin_vertical(w=5.08, h=5.08)
    add_symbol(
        "BZ", "BZ", "Buzzer", b, p, "Buzzer_Beeper:Buzzer_12x9.5RM7.6", "Device:Buzzer"
    )
    b, p = make_mos()
    add_symbol(
        "QN", "Q", "Q_NMOS_GSD", b, p, "Package_TO_SOT_SMD:SOT-23", "Device:Q_NMOS_GSD"
    )
    b, p = make_mos()
    add_symbol(
        "QP", "Q", "Q_PMOS_GSD", b, p, "Package_TO_SOT_SMD:SOT-23", "Device:Q_PMOS_GSD"
    )

    b, p = make_ic(["VIN", "GND", "EN"], ["NC", "VOUT"], width=15.24)
    add_symbol(
        "LDO5",
        "U",
        "ME6211C33",
        b,
        p,
        "Package_TO_SOT_SMD:SOT-23-5",
        "Regulator_Linear:ME6211C33M5G",
    )

    b, p = make_ic(
        ["TEMP", "PROG", "GND", "VCC"], ["BAT", "STDBY", "CHRG", "CE"], width=17.78
    )
    add_symbol(
        "TP4056",
        "U",
        "TP4056",
        b,
        p,
        "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
        "Battery_Management:TP4056",
    )

    b, p = make_ic(["SW", "GND", "FB"], ["EN", "IN", "NC"], width=15.24)
    add_symbol(
        "MT3608",
        "U",
        "MT3608",
        b,
        p,
        "Package_TO_SOT_SMD:SOT-23-6",
        "Regulator_Switching:MT3608",
    )

    b, p = make_ic(["B+", "B-"], ["P+", "P-"], width=15.24)
    add_symbol("BATPROT", "U", "DW01A+FS8205", b, p, "")

    b, p = make_ic(["I/O1", "GND", "I/O2"], ["I/O2", "VBUS", "I/O1"], width=17.78)
    add_symbol("USBLC6", "U", "USBLC6-2SC6", b, p, "Package_TO_SOT_SMD:SOT-23-6")

    b, p = make_ic(["/OE", "A", "GND"], ["Y", "VCC"], width=15.24)
    add_symbol("AHCT125", "U", "74AHCT1G125", b, p, "Package_TO_SOT_SMD:SOT-23-5")

    b, p = make_ic(["CS", "DO", "WP", "GND"], ["DI", "CLK", "HOLD", "VCC"], width=17.78)
    add_symbol(
        "NOR8",
        "U",
        "W25Q32",
        b,
        p,
        "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
        "Memory_Flash:W25Q32JVSS",
    )

    b, p = make_ic(["VBUS", "GND", "D+"], ["D-", "CC1", "CC2"], width=17.78)
    add_symbol(
        "USBC", "J", "USB_C", b, p, "Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12"
    )

    for n in (2, 3, 4, 6):
        b, p = make_connector(n)
        add_symbol(
            "Conn_%d" % n,
            "J",
            "Conn_%d" % n,
            b,
            p,
            "Connector_PinHeader_2.54mm:PinHeader_01x%02d_P2.54mm_Vertical" % n,
            "Connector_Generic:Conn_01x%02d" % n,
        )

    b, p = make_mcu(pin_rows)
    add_symbol(
        "MCU_CH32X035",
        "U",
        "CH32X035C-R0-1V2",
        b,
        p,
        "Package_QFP:LQFP-48_7x7mm_P0.5mm",
        "N1:MCU_CH32X035",
    )


# ---------------------------------------------------------------------------
# S-expression writers
# ---------------------------------------------------------------------------
def symbol_def(lib_name, inner_name, ref, value, body, pins, footprint=""):
    lines = []
    lines.append(
        "  (symbol %s (pin_names (offset 0.508)) (in_bom yes) (on_board yes)"
        % q(lib_name)
    )
    lines.append('    (property "Reference" %s (at 0 0 0) %s)' % (q(ref), effect()))
    lines.append('    (property "Value" %s (at 0 0 0) %s)' % (q(value), effect()))
    lines.append(
        '    (property "Footprint" %s (at 0 0 0) %s)'
        % (q(footprint), effect(hide=True))
    )
    lines.append('    (property "Datasheet" "" (at 0 0 0) %s)' % effect(hide=True))
    lines.append("    (symbol %s" % q(inner_name + "_0_1"))
    lines += body
    lines.append("    )")
    lines.append("    (symbol %s" % q(inner_name + "_1_1"))
    for p in pins:
        lines += pin_sexpr(p)
    lines.append("    )")
    lines.append("  )")
    return "\n".join(lines)


def write_symbol_lib(path):
    parts = [
        "(kicad_symbol_lib (version 20231120) (generator kicad_symbol_editor)",
    ]
    for name, sd in SYMBOLS.items():
        parts.append(
            symbol_def(
                name,
                name,
                sd["ref"],
                sd["value"],
                sd["body"],
                sd["pins"],
                sd["footprint"],
            )
        )
    parts.append(")")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(parts) + "\n")


# ---------------------------------------------------------------------------
# schematic
# ---------------------------------------------------------------------------
class Sch:
    def __init__(self):
        self.items = []

    def add(self, s):
        self.items.append(s)

    def wire(self, x1, y1, x2, y2):
        self.add(
            "  (wire (pts (xy %.2f %.2f) (xy %.2f %.2f))\n"
            "    (stroke (width 0) (type default)) (uuid %s))"
            % (x1, y1, x2, y2, q(uid()))
        )

    def glabel(self, name, x, y, angle):
        just = "right" if angle in (180, 270) else "left"
        self.add(
            "  (global_label %s (shape bidirectional) (at %.2f %.2f %d)\n"
            "    (effects (font (size 1.27 1.27)) (justify %s)) (uuid %s))"
            % (q(name), x, y, angle, just, q(uid()))
        )

    def noconnect(self, x, y):
        self.add("  (no_connect (at %.2f %.2f) (uuid %s))" % (x, y, q(uid())))

    def text(self, s, x, y, size=1.6):
        self.add(
            "  (text %s (at %.2f %.2f 0) (effects (font (size %s %s)) "
            "(justify left bottom)) (uuid %s))" % (q(s), x, y, size, size, q(uid()))
        )

    def symbol(self, lib_id, ref, value, x, y, pins, footprint=""):
        self.add(
            "  (symbol (lib_id %s) (at %.2f %.2f 0) (unit 1)\n"
            "    (in_bom yes) (on_board yes) (dnp no) (uuid %s)"
            % (q(lib_id), x, y, q(uid()))
        )
        self.add(
            '    (property "Reference" %s (at %.2f %.2f 0) %s)'
            % (q(ref), x, y - 5, effect())
        )
        self.add(
            '    (property "Value" %s (at %.2f %.2f 0) %s)'
            % (q(value), x, y + 5, effect())
        )
        self.add(
            '    (property "Footprint" %s (at %.2f %.2f 0) %s)'
            % (q(footprint), x, y + 7, effect(hide=True))
        )
        for n in pins:
            self.add("    (pin %s (uuid %s))" % (q(str(n)), q(uid())))
        # KiCad 8+ stores per-symbol instance data here; without it newer
        # KiCad versions can fall back to "unannotated" references.
        self.add(
            "    (instances\n"
            '      (project "N1_Console"\n'
            "        (path %s\n"
            "          (reference %s) (unit 1)\n"
            "        )\n"
            "      )\n"
            "    )" % (q("/" + SHEET_UUID), q(ref))
        )
        self.add("  )")


def load_pins():
    rows = []
    with open(PINMAP, newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            if row["package"] != "LQFP48":
                continue
            rows.append(row)
    rows.sort(key=lambda r: int(r["pin"]))
    return rows


def C(ref, sym, value, x, y, nets, fp=""):
    return (ref, sym, value, fp, x, y, nets)


# Component placement: 12 columns x 7 rows on A0.  The MCU is the large block
# at the bottom; everything else is grouped by function row.
COMPONENTS = [
    # row 0: USB input / 3V3 LDO / LED rail
    C(
        "J1",
        "USBC",
        "USB_C",
        50,
        90,
        {
            "1": "USB_VBUS",
            "2": "GND",
            "3": "USB_DP_CON",
            "4": "USB_DM_CON",
            "5": "CC1",
            "6": "CC2",
        },
    ),
    C("F1", "FUSE", "1.5A", 150, 90, {"1": "USB_VBUS", "2": "USB_FUSED"}),
    C("D1", "DSCH", "SS34", 250, 90, {"1": "VSYS_5V", "2": "USB_FUSED"}),
    C(
        "U2",
        "LDO5",
        "ME6211C33",
        350,
        90,
        {"1": "VSYS_5V", "2": "GND", "3": "VSYS_5V", "4": "NC", "5": "VDD_3V3"},
    ),
    C("C1", "C", "10uF", 450, 90, {"1": "VSYS_5V", "2": "GND"}),
    C("C2", "C", "100nF", 550, 90, {"1": "VSYS_5V", "2": "GND"}),
    C("C3", "C", "1uF", 650, 90, {"1": "VDD_3V3", "2": "GND"}),
    C("C4", "C", "10uF", 750, 90, {"1": "VDD_3V3", "2": "GND"}),
    C("Q1", "QP", "AO3401", 850, 90, {"1": "PWR_EN", "2": "VSYS_5V", "3": "V5_LED"}),
    C("R23", "R", "100k", 950, 90, {"1": "PWR_EN", "2": "GND"}),
    C("C5", "CP", "470uF", 1050, 90, {"1": "V5_LED", "2": "GND"}),
    C("C6", "C", "100nF", 1150, 90, {"1": "V5_LED", "2": "GND"}),
    # row 1: battery / charger / boost
    C("J5", "Conn_2", "BATTERY", 50, 210, {"1": "VBAT_RAW", "2": "GND_BAT"}),
    C(
        "U8",
        "BATPROT",
        "DW01A+FS8205",
        150,
        210,
        {"1": "VBAT_RAW", "2": "GND_BAT", "3": "VBAT", "4": "GND"},
    ),
    C(
        "U4",
        "TP4056",
        "TP4056",
        250,
        210,
        {
            "1": "GND",
            "2": "TP_PROG",
            "3": "GND",
            "4": "VSYS_5V",
            "5": "VBAT",
            "6": "NC",
            "7": "NC",
            "8": "VSYS_5V",
        },
    ),
    C("R24", "R", "1.2k", 350, 210, {"1": "TP_PROG", "2": "GND"}),
    C("C15", "C", "10uF", 450, 210, {"1": "VSYS_5V", "2": "GND"}),
    C("C16", "C", "10uF", 550, 210, {"1": "VBAT", "2": "GND"}),
    C(
        "U3",
        "MT3608",
        "MT3608",
        650,
        210,
        {
            "1": "BOOST_SW",
            "2": "GND",
            "3": "BOOST_FB",
            "4": "VBAT",
            "5": "VBAT",
            "6": "NC",
        },
    ),
    C("L1", "L", "22uH", 750, 210, {"1": "VBAT", "2": "BOOST_SW"}),
    C("D2", "DSCH", "SS34", 850, 210, {"1": "VSYS_5V", "2": "BOOST_OUT"}),
    C("C17", "C", "22uF", 950, 210, {"1": "BOOST_OUT", "2": "GND"}),
    C("R25", "R", "100k", 1050, 210, {"1": "BOOST_OUT", "2": "BOOST_FB"}),
    C("R26", "R", "13.6k", 1150, 210, {"1": "BOOST_FB", "2": "GND"}),
    # row 2: USB data / WS2812 level shifter
    C(
        "U9",
        "USBLC6",
        "USBLC6-2SC6",
        50,
        330,
        {
            "1": "USB_DP_CON",
            "2": "GND",
            "3": "USB_DM_CON",
            "4": "USB_DM_CON",
            "5": "USB_FUSED",
            "6": "USB_DP_CON",
        },
    ),
    C("R12", "R", "5.1k", 150, 330, {"1": "CC1", "2": "GND"}),
    C("R13", "R", "5.1k", 250, 330, {"1": "CC2", "2": "GND"}),
    C("R14", "R", "22R", 350, 330, {"1": "USB_DP_CON", "2": "USB_DP"}),
    C("R15", "R", "22R", 450, 330, {"1": "USB_DM_CON", "2": "USB_DM"}),
    C(
        "U5",
        "AHCT125",
        "74AHCT1G125",
        550,
        330,
        {"1": "GND", "2": "WS2812_DIN", "3": "GND", "4": "WS2812_LS", "5": "V5_LED"},
    ),
    C("R10", "R", "330R", 650, 330, {"1": "WS2812_LS", "2": "WS2812_PANEL_DIN"}),
    C(
        "J2",
        "Conn_3",
        "WS2812_PANEL",
        750,
        330,
        {"1": "V5_LED", "2": "WS2812_PANEL_DIN", "3": "GND"},
    ),
    C("C18", "C", "100nF", 850, 330, {"1": "V5_LED", "2": "GND"}),
    # row 3: NOR cartridge / OLED
    C(
        "U6",
        "NOR8",
        "W25Q32",
        50,
        450,
        {
            "1": "NOR_CS",
            "2": "NOR_MISO",
            "3": "VDD_3V3",
            "4": "GND",
            "5": "NOR_MOSI",
            "6": "NOR_SCK",
            "7": "VDD_3V3",
            "8": "VDD_3V3",
        },
    ),
    C("R11", "R", "10k", 150, 450, {"1": "NOR_CS", "2": "VDD_3V3"}),
    C("C10", "C", "100nF", 250, 450, {"1": "VDD_3V3", "2": "GND"}),
    C("SW7", "SW", "CARD_DETECT", 350, 450, {"1": "CARD_DET", "2": "GND"}),
    C(
        "J3",
        "Conn_4",
        "OLED_SSD1306",
        450,
        450,
        {"1": "VDD_3V3", "2": "GND", "3": "OLED_SCL", "4": "OLED_SDA"},
    ),
    C("R21", "R", "4.7k", 550, 450, {"1": "OLED_SCL", "2": "VDD_3V3"}),
    C("R22", "R", "4.7k", 650, 450, {"1": "OLED_SDA", "2": "VDD_3V3"}),
    C("C14", "C", "100nF", 750, 450, {"1": "VDD_3V3", "2": "GND"}),
    # row 4: buttons / reset / MCU decoupling
    C("SW1", "SW", "BTN_UP", 50, 570, {"1": "BTN_UP", "2": "GND"}),
    C("SW2", "SW", "BTN_DOWN", 150, 570, {"1": "BTN_DOWN", "2": "GND"}),
    C("SW3", "SW", "BTN_LEFT", 250, 570, {"1": "BTN_LEFT", "2": "GND"}),
    C("SW4", "SW", "BTN_RIGHT", 350, 570, {"1": "BTN_RIGHT", "2": "GND"}),
    C("SW5", "SW", "BTN_A", 450, 570, {"1": "BTN_A", "2": "GND"}),
    C("SW6", "SW", "BTN_B", 550, 570, {"1": "BTN_B", "2": "GND"}),
    C("R9", "R", "10k", 650, 570, {"1": "RESET_N", "2": "VDD_3V3"}),
    C("C9", "C", "100nF", 750, 570, {"1": "RESET_N", "2": "GND"}),
    C("SW9", "SW", "RESET", 850, 570, {"1": "RESET_N", "2": "GND"}),
    C("C7", "C", "100nF", 950, 570, {"1": "VDD_3V3", "2": "GND"}),
    C("C8", "C", "10uF", 1050, 570, {"1": "VDD_3V3", "2": "GND"}),
    # row 5: buzzer / IR transmitter / IR receiver
    C("Q2", "QN", "AO3400", 50, 690, {"1": "IR_GATE", "2": "GND", "3": "IR_LED_K"}),
    C("R16", "R", "1k", 150, 690, {"1": "BUZZ_IR", "2": "IR_GATE"}),
    C("D5", "LED", "IR_940nm", 250, 690, {"1": "IR_LED_K", "2": "IR_LED_A"}),
    C("R17", "R", "12R", 350, 690, {"1": "V5_LED", "2": "IR_LED_A"}),
    C("C11", "CP", "100uF", 450, 690, {"1": "V5_LED", "2": "GND"}),
    C("Q3", "QN", "AO3400", 550, 690, {"1": "BZ_GATE", "2": "GND", "3": "BZ_NEG"}),
    C("R18", "R", "0R", 650, 690, {"1": "BUZZ_IR", "2": "BZ_PRE"}),
    C("R19", "R", "1k", 750, 690, {"1": "BZ_PRE", "2": "BZ_GATE"}),
    C("BZ1", "BZ", "MLT-7525", 850, 690, {"1": "V5_LED", "2": "BZ_NEG"}),
    C("D4", "D", "1N4148", 950, 690, {"1": "V5_LED", "2": "BZ_NEG"}),
    C("U7", "Conn_3", "IR_RX", 1050, 690, {"1": "IR_VCC", "2": "IR_RX", "3": "GND"}),
    C("R20", "R", "47R", 1150, 690, {"1": "V5_LED", "2": "IR_VCC"}),
    # row 6: IR filter caps / debug header
    C("C12", "CP", "47uF", 50, 780, {"1": "IR_VCC", "2": "GND"}),
    C("C13", "C", "100nF", 150, 780, {"1": "IR_VCC", "2": "GND"}),
    C(
        "J4",
        "Conn_6",
        "DEBUG_SWD_UART",
        250,
        780,
        {
            "1": "VDD_3V3",
            "2": "GND",
            "3": "DBG_TX",
            "4": "DBG_RX",
            "5": "SWDIO",
            "6": "SWCLK",
        },
    ),
]

BLOCK_TITLES = [
    (40, 45, "POWER: USB-C -> FUSE -> D1 -> VSYS_5V -> LDO 3V3 / LED RAIL"),
    (40, 165, "BATTERY: 1S LiPo + DW01A/FS8205 + TP4056 + MT3608 BOOST"),
    (40, 285, "USB DATA ESD + WS2812 3V3->5V LEVEL SHIFT"),
    (40, 405, "SPI NOR CARTRIDGE + OLED I2C PULL-UPS"),
    (40, 525, "6 BUTTONS + RESET"),
    (40, 645, "BUZZER + 38kHz IR TX + IR RECEIVER FILTER"),
    (40, 735, "IR FILTER CAPS + DEBUG/SWD HEADER"),
    (500, 735, "CH32X035 LQFP48"),
]


def validate_components(pin_rows):
    """Catch a missing pin-to-net mapping before KiCad ever sees the file."""
    comps = list(COMPONENTS)
    comps.append(
        C(
            "U1",
            "MCU_CH32X035",
            "CH32X035C-R0-1V2",
            600,
            780,
            {r["pin"]: r["net"] for r in pin_rows},
            "Package_QFP:LQFP-48_7x7mm_P0.5mm",
        )
    )
    counts = {}

    for ref, sym, value, fp, x, y, nets in comps:
        sd = SYMBOLS[sym]
        pins = {p["num"] for p in sd["pins"]}
        missing = pins - set(nets)
        extra = set(nets) - pins
        if missing:
            raise SystemExit(
                "%s/%s missing nets for pins %s" % (ref, sym, sorted(missing))
            )
        if extra:
            raise SystemExit(
                "%s/%s unknown pins in net map: %s" % (ref, sym, sorted(extra))
            )
        for net in nets.values():
            if net != "NC":
                counts[net] = counts.get(net, 0) + 1

    dangling = {n: c for n, c in counts.items() if c < 2}
    if dangling:
        print("WARNING: nets with only one pin: %s" % dangling)
    print("nets     : %d" % len(counts))


def place(s, comp):
    ref, sym, value, fp, x, y, nets = comp
    sd = SYMBOLS[sym]
    s.symbol(
        sd["lib"],
        ref,
        value,
        x,
        y,
        [p["num"] for p in sd["pins"]],
        fp or sd["footprint"],
    )
    for p in sd["pins"]:
        px = x + p["x"]
        py = y + p["y"]
        net = nets.get(p["num"], "NC")
        if net == "NC":
            s.noconnect(px, py)
            continue
        lang = (p["angle"] + 180) % 360
        dx, dy = {0: (6.0, 0.0), 90: (0.0, 6.0), 180: (-6.0, 0.0), 270: (0.0, -6.0)}[
            lang
        ]
        lx, ly = px + dx, py + dy
        s.wire(px, py, lx, ly)
        s.glabel(net, lx, ly, lang)


def build_schematic(pin_rows):
    s = Sch()

    # MCU, generated from the same CSV as the firmware
    mcu_nets = {r["pin"]: r["net"] for r in pin_rows}
    place(
        s,
        C(
            "U1",
            "MCU_CH32X035",
            "CH32X035C-R0-1V2",
            600,
            780,
            mcu_nets,
            "Package_QFP:LQFP-48_7x7mm_P0.5mm",
        ),
    )

    for comp in COMPONENTS:
        place(s, comp)

    for x, y, title in BLOCK_TITLES:
        s.text(title, x, y, size=2.0)

    s.text(
        "Schematic-first KiCad project. Every pin has a short wire + global "
        "net label, so the netlist is complete. Arrange by block or draw "
        "explicit wires before Update PCB. Verify IC pinouts against the "
        "datasheets and HARDWARE.md.",
        40,
        825,
        size=1.4,
    )

    body = []
    body.append("(kicad_sch (version 20231120) (generator eeschema)")
    body.append("  (uuid %s)" % q(SHEET_UUID))
    body.append('  (paper "A0")')
    body.append("  (title_block")
    body.append('    (title "N1 Console - CH32X035 handheld")')
    body.append('    (date "2026-11-22")')
    body.append('    (rev "1.1")')
    body.append('    (comment 1 "Generated from HARDWARE.md / pinmap.csv / BOM.csv")')
    body.append("  )")

    body.append("  (lib_symbols")
    for name, sd in SYMBOLS.items():
        body.append(
            symbol_def(
                sd["lib"],
                sd["lib"].split(":", 1)[-1],
                sd["ref"],
                sd["value"],
                sd["body"],
                sd["pins"],
                sd["footprint"],
            )
        )
    body.append("  )")

    body += s.items
    body.append('  (sheet_instances (path "/" (page "1")))')
    body.append(")")

    return "\n".join(body) + "\n"


def write_project(path):
    pro = {
        "board": {},
        "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": []},
        "meta": {"filename": "N1_Console.kicad_pro", "version": 1},
        "net_settings": {},
        "pcbnew": {},
        "schematic": {"legacy_lib_dir": "", "legacy_lib_list": []},
        "sheets": [[SHEET_UUID, "N1_Console.kicad_sch"]],
        "text_variables": {},
    }
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(pro, fh, indent=2)
        fh.write("\n")


def write_table(path):
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(
            "(sym_lib_table\n"
            "  (version 7)\n"
            '  (lib (name "N1")(type "KiCad")(uri "${KIPRJMOD}/N1_Console.kicad_sym")'
            '(options "")(descr "N1 console generated symbols"))\n'
            ")\n"
        )


def write_readme(path):
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("""# N1 Console KiCad project

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
""")
        fh.write("\n")


def main():
    os.makedirs(OUT, exist_ok=True)
    rows = load_pins()
    if len(rows) != 48:
        raise SystemExit("expected 48 LQFP48 pins, got %d" % len(rows))

    build_symbols(rows)
    validate_components(rows)

    write_symbol_lib(os.path.join(OUT, "N1_Console.kicad_sym"))
    with open(os.path.join(OUT, "N1_Console.kicad_sch"), "w", encoding="utf-8") as fh:
        fh.write(build_schematic(rows))
    write_project(os.path.join(OUT, "N1_Console.kicad_pro"))
    write_table(os.path.join(OUT, "sym-lib-table"))
    write_readme(os.path.join(OUT, "README.md"))

    print("wrote %s" % OUT)
    print("  symbols  : %d" % len(SYMBOLS))
    print("  parts    : %d" % (len(COMPONENTS) + 1))
    print("  N1_Console.kicad_pro")
    print("  N1_Console.kicad_sch")
    print("  N1_Console.kicad_sym")
    print("  sym-lib-table")
    print("  README.md")


if __name__ == "__main__":
    main()
