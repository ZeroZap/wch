#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gen_pinmux.py - WCH PinMux workbook generator (multi model).

For every supported MCU this script produces

    <MODEL>_pins.csv       pin matrix, extracted from Doc/DS/*.PDF
    <MODEL>_PinMux.xlsx    workbook: 说明 / PinList / DemoUsage

and a regenerated index (README.md).

Commands
    python gen_pinmux.py list                 # show configured models
    python gen_pinmux.py extract [MODEL ...]  # PDF  -> CSV
    python gen_pinmux.py scan                 # demo PINMUX.md -> demos.json
    python gen_pinmux.py build   [MODEL ...]  # CSV + demos.json -> XLSX + index
    python gen_pinmux.py all     [MODEL ...]  # extract + scan + build

Two datasheet layouts are supported:

    af   : 引脚编号... 引脚名称 引脚类型 [I/O电平] 主功能 默认复用功能 重映射功能
           (CH32X035/L103/V002/V006/V007/V307/V407/M030, CH569 ...)
    list : 引脚编号... 引脚名称 引脚类型 复用功能 功能描述
           (CH57x/CH58x/CH59x, CH32X315 ...)

Notes
    * package / model columns are kept as one raw string per pin
      ("封装/型号引脚串"), because the column headers differ per datasheet
      (packages, or model names such as CH585C/CH585F);
      CH32X035 additionally keeps the named package columns.
    * default AF = token without "_N" suffix, remap = token with "_N".
"""

import argparse
import csv
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DS_DIR = os.path.join(HERE, "..", "Doc", "DS")
INDEX = os.path.join(HERE, "README.md")
DEMOS_JSON = os.path.join(HERE, "demos.json")
REPO_ROOT = os.path.dirname(HERE)

# ---------------------------------------------------------------------------
# model registry
# ---------------------------------------------------------------------------
CHIPS = [
    # id            pdf                    layout  column header (informative)
    (
        "CH32X035",
        "CH32X035DS0.PDF",
        "af",
        "LQFP64M / LQFP48 / QFN28 / QSOP28 / QFN20 / TSSOP20 / QFN12",
    ),
    ("CH32X315", "CH32X315DS0.PDF", "list", "CH32X315CCU6 / WCU6 / MCU6"),
    ("CH32L103", "CH32L103DS0.PDF", "af", "L103F8P6 / F8U6 / G8R6 / K8U / C8T6"),
    (
        "CH32M030",
        "CH32M030DS0.PDF",
        "af",
        "QSOP28 / QFN32 / QFN48 / QFN48X7_A / LQFP48",
    ),
    ("CH32V002", "CH32V002DS0.PDF", "af", "SOP8 / QFN12 / SOP16 / QFN20 / TSSOP20"),
    ("CH32V006", "CH32V006DS0.PDF", "af", "V006D8U7 / F8P7 / F8U7 / E8R / K8U7"),
    ("CH32V007", "CH32V007DS0.PDF", "af", "V007E8R / K8U7"),
    (
        "CH32V307",
        "CH32V307DS0.PDF",
        "af",
        "TSSOP20 / QFN28 / LQFP48(V305) / LQFP48(V303) / LQFP64M / QFN68 / LQFP100",
    ),
    (
        "CH32V407",
        "CH32V407DS0.PDF",
        "af",
        "CH32V467RET6 / V407RET6 / V467WEU6 / V407WEU6 / V467VET6 / V407VET6",
    ),
    ("CH569", "CH569DS1.PDF", "af", "CH569 / CH565"),
    ("CH572", "CH572DS1.PDF", "list", "CH572Q / CH572R / CH572D"),
    ("CH573", "CH573DS1.PDF", "list", "QFN32 / QFN28"),
    ("CH583", "CH583DS1.PDF", "list", "QFN48 / QFN28"),
    ("CH585", "CH585DS1.PDF", "list", "CH585C / F / M / D"),
    (
        "CH592",
        "CH592DS1.PDF",
        "list",
        "CH592X / CH592F/A / CH591F / CH592D / CH591D / CH591R",
    ),
    ("CH595", "CH595DS1.PDF", "list", "CH595X / CH595F / CH595D"),
    ("CH596", "CH596DS1.PDF", "list", "CH596D"),
]

CHIP_MAP = {
    c[0]: {"id": c[0], "pdf": c[1], "layout": c[2], "columns": c[3]} for c in CHIPS
}

PIN_NAMES = (
    r"P[A-F]\d{1,2}|GND|VDD[A-Z0-9]*|VSS[A-Z0-9]*|VBAT|VSW|VDCID|VDCIA|"
    r"VINTA|VIN5|V12[A-Z0-9]*|V33[A-Z0-9]*|VIO[A-Z0-9]*|VHV|"
    r"ANT|X32M[IO]|XI|XO|UD[+-]|SSTX[A-B]|SSRX[A-B]|USB[A-Z0-9]*"
)
TYPE_RE = r"I/O/A|I/O|I/A|O/A|电源|P|A|I|O"

CSV_FIELDS = [
    "pin",
    "type",
    "main",
    "af_default",
    "af_remap",
    "af_raw",
    "pins_raw",
    "desc",
]

HEADER_NOISE = re.compile(
    r"(引脚\s*编号|引脚\s*名称|引脚\s*类型|引脚功能|主功能|默认复用功能|"
    r"重映射功能|复用功能|功能描述|I/O\s*电平|I/O\s*特性|在前优先)"
)


# ---------------------------------------------------------------------------
# extraction
# ---------------------------------------------------------------------------
def _clean_text(pdf_path, max_pages=60):
    """Return a dict page_index -> cleaned text for pages that may hold a table."""
    from pypdf import PdfReader

    reader = PdfReader(pdf_path)
    pages = {}
    n = min(max_pages, len(reader.pages))
    for i in range(n):
        raw = reader.pages[i].extract_text() or ""
        if not raw:
            continue

        keep = []
        for line in raw.split("\n"):
            s = line.strip()
            if not s:
                continue
            if "wch.cn" in s or "数据手册" in s:
                continue
            if re.match(r"^V\d(\.\d+)?\b", s):
                continue
            if HEADER_NOISE.fullmatch(s):
                continue
            keep.append(s)
        text = " ".join(keep)
        text = re.sub(r"[（(]\d+[）)]", "", text)  # footnote markers
        text = re.sub(r"\s+", " ", text)
        pages[i] = text
    return pages


def _row_re(layout):
    # package / model cells, pin name (possibly with a -suffix), pin type
    # a package column may also be "无" (not bonded on that package)
    return re.compile(
        r"(?<![\w/])(?:(?:-|\d{1,3}|无) ){1,10}"
        r"(?P<name>" + PIN_NAMES + r")(?:-[A-Za-z0-9]+(?:\d)?)* "
        r"(?P<type>" + TYPE_RE + r")(?= )"
    )


def _numeric_cells(prefix):
    return [t for t in prefix.strip().split(" ") if t]


def parse_chip(chip, max_pages=60):
    pdf = os.path.join(DS_DIR, chip["pdf"])
    pages = _clean_text(pdf, max_pages)
    row = _row_re(chip["layout"])

    # choose the contiguous page group that holds the most rows
    hits = [i for i, t in pages.items() if len(row.findall(t)) >= 3]
    if not hits:
        return []

    groups, cur = [], [hits[0]]
    for i in hits[1:]:
        if i - cur[-1] <= 1:
            cur.append(i)
        else:
            groups.append(cur)
            cur = [i]
    groups.append(cur)

    def group_rows(g):
        return sum(len(row.findall(pages[i])) for i in g)

    best = max(groups, key=group_rows)
    text = " ".join(pages[i] for i in best)

    matches = list(row.finditer(text))
    rows = []
    for idx, m in enumerate(matches):
        end = matches[idx + 1].start() if idx + 1 < len(matches) else len(text)
        block = text[m.start() : end]

        name = m.group("name")
        cells = _numeric_cells(block[: m.start("name") - m.start()])

        # tokens after "pin type"
        rest = block[m.end() - m.start() :].strip()
        toks = rest.split(" ")

        # optional I/O level column (FT / MV/PD / MVO / ...) then main function
        main = ""
        if toks and re.match(r"^" + PIN_NAMES, toks[0] + " "):
            main = toks.pop(0)
        elif len(toks) >= 2 and re.match(r"^" + PIN_NAMES, toks[1]):
            toks.pop(0)  # level token
            main = toks.pop(0)

        # AF tokens until a footnote / description starts
        af_toks = []
        desc = []
        for t in toks:
            t = t.strip("/")
            if not t:
                continue
            if t == "-" or re.search(r"[^\x00-\x7f]", t):
                if re.search(r"[^\x00-\x7f]", t):
                    desc.append(t)
                break
            if re.fullmatch(r"\d{1,3}", t) or re.fullmatch(r"V\d(\.\d+)?", t):
                continue
            if re.match(r"^注\s*\d", t):
                break
            af_toks.append(t)

        # merge tokens torn apart by a line wrap: "CTS3_" + "3"
        merged = []
        i = 0
        while i < len(af_toks):
            t = af_toks[i]
            if t.endswith("_") and i + 1 < len(af_toks) and af_toks[i + 1].isdigit():
                t = t + af_toks[i + 1]
                i += 1
            merged.append(t)
            i += 1

        af = "/".join(merged)
        parts = [t for t in re.split(r"[/\s]+", af) if t]
        default, remap = [], []
        for t in parts:
            if t == name or t == main:
                continue
            if re.search(r"_\d+$", t):
                remap.append(t)
            else:
                default.append(t)

        rows.append(
            {
                "pin": name,
                "type": m.group("type"),
                "main": main,
                "af_default": "/".join(default),
                "af_remap": "/".join(remap),
                "af_raw": af,
                "pins_raw": "/".join(cells),
                "desc": " ".join(desc)[:120],
            }
        )

    # Some datasheets repeat part of the pin table (second chapter / two
    # column layout).  Rows of the main table carry all package cells, the
    # repeated "small package" table only 1~2 of them.  Use the most common
    # cell count of the main table as reference.
    from collections import Counter

    counts = Counter(len(r["pins_raw"].split("/")) for r in rows)
    if os.environ.get("PINMUX_DEBUG"):
        print("   cell counts:", dict(counts))
    if counts:
        mode_cells = counts.most_common(1)[0][0]
        if mode_cells >= 3:
            rows = [r for r in rows if len(r["pins_raw"].split("/")) >= mode_cells - 1]

    # keep only the richest row per pin: some datasheets contain two pin
    # tables (e.g. 2-package and 4-package variants) that list the same pin
    from collections import defaultdict

    per_pin = defaultdict(list)
    for r in rows:
        per_pin[r["pin"]].append(r)
    rows = []
    for rs in per_pin.values():
        best = max(len(r["pins_raw"].split("/")) for r in rs)
        rows.extend(r for r in rs if len(r["pins_raw"].split("/")) == best)

    # keep the first occurrence of identical rows and, for GPIO-like pins,
    # only one row per pin (some datasheets contain two pin tables for
    # different package variants).  Power pins may legitimately repeat.
    power_prefix = (
        "GND",
        "VDD",
        "VSS",
        "VB",
        "VS",
        "VSW",
        "VDC",
        "VIN",
        "V12",
        "V33",
        "VIO",
        "VREF",
        "VHV",
    )
    seen = set()
    seen_gpio = set()
    uniq = []
    for r in rows:
        key = (r["pin"], r["pins_raw"])
        if key in seen:
            continue
        seen.add(key)

        if r["pin"].startswith(power_prefix):
            uniq.append(r)
            continue

        if r["pin"] in seen_gpio:
            continue
        seen_gpio.add(r["pin"])
        uniq.append(r)

    return uniq


# ---------------------------------------------------------------------------
# CH32X035: verified parser with named package columns (kept from v1)
# ---------------------------------------------------------------------------
PACKAGES_X035 = ["LQFP64M", "LQFP48", "QFN28", "QSOP28", "QFN20", "TSSOP20", "QFN12"]
X035_FIELDS = [
    "pin",
    "type",
    "main",
    "af_default",
    "af_remap",
    "af_raw",
    "LQFP64M",
    "LQFP48",
    "QFN28",
    "QSOP28",
    "QFN20",
    "TSSOP20",
    "QFN12",
    "pins_raw",
    "desc",
]


def parse_x035():
    rows = parse_chip(CHIP_MAP["CH32X035"], max_pages=45)
    out = []
    for r in rows:
        cells = r["pins_raw"].split("/")
        cells += ["-"] * (7 - len(cells))
        item = {
            k: r[k]
            for k in (
                "pin",
                "type",
                "main",
                "af_default",
                "af_remap",
                "af_raw",
                "pins_raw",
                "desc",
            )
        }
        for i, pkg in enumerate(PACKAGES_X035):
            item[pkg] = cells[i] if i < len(cells) else "-"
        out.append(item)
    return out


# ---------------------------------------------------------------------------
# demo usage (CH32X035)
# ---------------------------------------------------------------------------
DEMOS = [
    {
        "name": "WS2812_Panel",
        "path": "CH32X035EVT/EXAM/APPLICATION/WS2812_Panel",
        "pins": {
            "PC7": "WS2812 DIN (PIOC_IO0, remap, SDI kept)",
            "PA0": "KEY (IPU, active low)",
            "PA2": "NOR /CS (GPIO out)",
            "PA5": "NOR SCK (SPI1)",
            "PA6": "NOR MISO (SPI1)",
            "PA7": "NOR MOSI (SPI1)",
            "PB10": "USART1 TX (debug, 115200)",
            "PC16": "USB D- (reserved, phase 5)",
            "PC17": "USB D+ (reserved, phase 5)",
        },
    },
    {
        "name": "SPI_FLASH (vendor)",
        "path": "CH32X035EVT/EXAM/SPI/SPI_FLASH",
        "pins": {
            "PA2": "NOR /CS",
            "PA5": "NOR SCK",
            "PA6": "NOR MISO",
            "PA7": "NOR MOSI",
            "PB10": "USART1 TX",
        },
    },
    {
        "name": "WS2812_LED (vendor)",
        "path": "CH32X035EVT/EXAM/APPLICATION/WS2812_LED",
        "pins": {
            "PA7": "WS2812 DIN (SPI1 mode)",
            "PB9": "WS2812 DIN (TIM1_CH1 PWM mode)",
            "PB10": "USART1 TX",
        },
    },
    {
        "name": "PIOC 1_Wire (vendor)",
        "path": "CH32X035EVT/EXAM/PIOC/1_Wire",
        "pins": {
            "PC18": "WS2812 / DS1820 (PIOC_IO0, SDI disabled)",
            "PC19": "WS2812 / DS1820 (PIOC_IO1, SDI disabled)",
            "PC7": "WS2812 (PIOC_IO0 remap, alternative)",
        },
    },
    {
        "name": "USB CDC (vendor)",
        "path": "CH32X035EVT/EXAM/USB/USBFS/DEVICE/SimulateCDC",
        "pins": {
            "PC16": "USB D-",
            "PC17": "USB D+",
            "PB10": "USART1 TX (debug)",
            "PA2": "USART2 TX (emulated CDC)",
        },
    },
]


# ---------------------------------------------------------------------------
# demo PINMUX.md  ->  demos.json  ->  DemoUsage sheet
# ---------------------------------------------------------------------------
PIN_RE = re.compile(r"^(?:" + PIN_NAMES + r")$")


def _md_tables(text):
    """yield (header, rows) for every markdown table in text."""
    lines = text.splitlines()
    i = 0
    while i + 1 < len(lines):
        head = lines[i].strip()
        sep = lines[i + 1].strip()
        if head.startswith("|") and re.match(r"^\|[\s:\-|]+\|$", sep):
            header = [c.strip() for c in head.strip("|").split("|")]
            rows = []
            j = i + 2
            while j < len(lines) and lines[j].strip().startswith("|"):
                rows.append([c.strip() for c in lines[j].strip().strip("|").split("|")])
                j += 1
            yield header, rows
            i = j
        else:
            i += 1


def _cell(value):
    return re.sub(r"[`*]", "", value or "").strip()


def parse_demo_pinmux(path):
    """Parse the pin table of one demo PINMUX.md.

    Expected header (columns may be a subset / different order):
        引脚 | 信号 | GPIO 配置 | 方向 | 备注
    Returns None when no suitable table is found.
    """
    with open(path, encoding="utf-8") as f:
        text = f.read()

    mcu = None
    for cid in CHIP_MAP:
        if re.search(r"\b%s\b" % re.escape(cid), text):
            mcu = cid
            break

    for header, rows in _md_tables(text):
        head = [_cell(h) for h in header]
        if not head or "引脚" not in head[0]:
            continue
        if not any(("信号" in h) or ("用途" in h) or ("功能" in h) for h in head[1:]):
            continue

        def col(*names):
            for k, h in enumerate(head):
                if any(n in h for n in names):
                    return k
            return None

        c_sig = col("信号", "用途", "功能")
        c_cfg = col("配置")
        c_dir = col("方向")
        c_note = col("备注", "说明")

        out = []
        for row in rows:
            cells = [_cell(c) for c in row]
            if not cells:
                continue
            pin = cells[0]
            if not PIN_RE.match(pin):
                continue
            out.append(
                {
                    "pin": pin,
                    "signal": cells[c_sig]
                    if c_sig is not None and c_sig < len(cells)
                    else "",
                    "config": cells[c_cfg]
                    if c_cfg is not None and c_cfg < len(cells)
                    else "",
                    "dir": cells[c_dir]
                    if c_dir is not None and c_dir < len(cells)
                    else "",
                    "note": cells[c_note]
                    if c_note is not None and c_note < len(cells)
                    else "",
                }
            )

        if out:
            return {"mcu": mcu, "rows": out}

    return None


def scan_demos():
    """Walk the repo for demo PINMUX.md files and write demos.json."""
    skip = {".git", ".dsh-tmp", "obj", "PinMux"}
    found = {}

    for root, dirs, files in os.walk(REPO_ROOT):
        dirs[:] = [d for d in dirs if d not in skip]
        if root == REPO_ROOT:
            continue  # repo level notes (PinMux.md) are board notes
        lower = {f.lower(): f for f in files}
        if "pinmux.md" not in lower:
            continue
        path = os.path.join(root, lower["pinmux.md"])
        data = parse_demo_pinmux(path)
        if not data:
            print("  skip (no pin table):", os.path.relpath(path, REPO_ROOT))
            continue

        name = os.path.basename(root)
        if name in found:
            name = "%s/%s" % (os.path.basename(os.path.dirname(root)), name)

        found[name] = {
            "mcu": data["mcu"],
            "dir": os.path.relpath(root, REPO_ROOT).replace(os.sep, "/"),
            "rows": data["rows"],
        }
        print(
            "  %-24s %-10s %d pins  (%s)"
            % (name, data["mcu"] or "?", len(data["rows"]), found[name]["dir"])
        )

    with open(DEMOS_JSON, "w", encoding="utf-8") as f:
        json.dump(found, f, ensure_ascii=False, indent=2)
        f.write("\n")
    print("demos.json:", len(found), "demo(s)")
    return found


def load_demos():
    """Load demos.json; fall back to the built-in vendor example list."""
    if os.path.exists(DEMOS_JSON):
        with open(DEMOS_JSON, encoding="utf-8") as f:
            raw = json.load(f)
        out = []
        for name, d in raw.items():
            pins = {}
            for row in d.get("rows", []):
                pins[row["pin"]] = row.get("signal") or row.get("note") or ""
            out.append(
                {
                    "name": name,
                    "mcu": d.get("mcu"),
                    "dir": d.get("dir", ""),
                    "source": "PINMUX.md",
                    "pins": pins,
                    "rows": d.get("rows", []),
                }
            )

        # keep the hand written vendor examples that have no PINMUX.md
        names = {d["name"] for d in out}
        for d in DEMOS:
            if d["name"] in names:
                continue
            out.append(
                {
                    "name": d["name"],
                    "mcu": "CH32X035",
                    "dir": d.get("path", ""),
                    "source": "内置示例",
                    "pins": d["pins"],
                    "rows": [],
                }
            )
        return out

    return [
        {
            "name": d["name"],
            "mcu": "CH32X035",
            "dir": d.get("path", ""),
            "pins": d["pins"],
            "rows": [],
        }
        for d in DEMOS
    ]


# ---------------------------------------------------------------------------
# writers
# ---------------------------------------------------------------------------
def write_csv(chip_id, rows, fields):
    path = os.path.join(HERE, "%s_pins.csv" % chip_id)
    with open(path, "w", newline="", encoding="utf-8-sig") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    return path


def read_csv(path):
    with open(path, newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def build_xlsx(chip, rows, fields, demos=None):
    from openpyxl import Workbook
    from openpyxl.styles import Alignment, Border, Font, PatternFill, Side
    from openpyxl.utils import get_column_letter

    head_fill = PatternFill("solid", fgColor="1F4E79")
    head_font = Font(color="FFFFFF", bold=True)
    sub_fill = PatternFill("solid", fgColor="DDEBF7")
    thin = Side(style="thin", color="B0B0B0")
    border = Border(left=thin, right=thin, top=thin, bottom=thin)

    def header(ws, ncols, row=1):
        for c in range(1, ncols + 1):
            cell = ws.cell(row=row, column=c)
            cell.fill = head_fill
            cell.font = head_font
            cell.alignment = Alignment(
                horizontal="center", vertical="center", wrap_text=True
            )
            cell.border = border

    def widths(ws, values):
        for i, w in enumerate(values, start=1):
            ws.column_dimensions[get_column_letter(i)].width = w

    wb = Workbook()

    ws = wb.active
    ws.title = "说明"
    gpio = sum(
        1
        for r in rows
        if r["type"].startswith("I/O") or r["type"] in ("I/A", "O/A", "A", "O", "I")
    )
    info = [
        ("%s PinMux 维护表" % chip["id"], "title"),
        ("", ""),
        ("数据来源", "h2"),
        ("Doc/DS/%s（引脚定义表）" % chip["pdf"], ""),
        (
            "解析方式：%s"
            % (
                "主功能 + 默认/重映射"
                if chip["layout"] == "af"
                else "复用功能 + 功能描述"
            ),
            "",
        ),
        ("封装/型号列：%s" % chip["columns"], ""),
        ("", ""),
        ("统计", "h2"),
        ("引脚行数：%d（其中 GPIO/模拟类 %d）" % (len(rows), gpio), ""),
        ("", ""),
        ("使用约定", "h2"),
        ("默认复用 = AF 名称不带后缀（TX1 / CK1 / MCO）", ""),
        ("重映射   = AF 名称带选项后缀 _N（TX1_2 / PIOC_IO0_1）", ""),
        ("封装/型号引脚串 = 表中各封装列的原始值，列顺序见上", ""),
        ("", ""),
        ("维护", "h2"),
        ("DemoUsage 每列一个 demo；新增 demo 加一列并在 demo 目录补 PINMUX.md。", ""),
        ("重新执行 build 会覆盖本文件；长期维护以 xlsx 为准并保留 CSV 备份。", ""),
        ("CSV 提取结果建议人工复核（尤其封装列与多型号列）。", ""),
    ]
    for i, (text, kind) in enumerate(info, start=1):
        cell = ws.cell(row=i, column=1, value=text)
        if kind == "title":
            cell.font = Font(size=14, bold=True)
        elif kind == "h2":
            cell.font = Font(bold=True, color="1F4E79")
    ws.column_dimensions["A"].width = 110

    ws = wb.create_sheet("PinList")
    head = [
        "引脚",
        "类型",
        "主功能",
        "默认复用",
        "重映射",
        "复用/功能原始串",
        "封装/型号引脚串",
        "功能描述",
    ] + [f for f in fields if f not in CSV_FIELDS]
    ws.append(head)
    for r in rows:
        extra = [r.get(f, "") for f in fields if f not in CSV_FIELDS]
        pins_raw = r.get("pins_raw") or "/".join(extra)
        ws.append(
            [
                r["pin"],
                r["type"],
                r["main"],
                r["af_default"],
                r["af_remap"],
                r["af_raw"],
                pins_raw,
                r.get("desc", ""),
            ]
            + extra
        )
    header(ws, len(head))
    widths(ws, [10, 8, 10, 34, 44, 44, 22, 46] + [9] * (len(head) - 8))
    ws.freeze_panes = "A2"
    ws.auto_filter.ref = "A1:%s%d" % (get_column_letter(len(head)), ws.max_row)
    for row in ws.iter_rows(min_row=2):
        for cell in row:
            cell.alignment = Alignment(vertical="top", wrap_text=True)
            cell.border = border

    # DemoUsage
    ws = wb.create_sheet("DemoUsage")
    chip_demos = [
        d for d in (demos or []) if (d.get("mcu") or "CH32X035") == chip["id"]
    ]
    demos = chip_demos
    head = ["引脚"] + [d["name"] for d in demos] + ["说明"]
    ws.append(head)
    if demos:
        seen = set()
        for d in demos:
            seen.update(d["pins"].keys())
        for r in rows:
            if r["pin"] in seen:
                ws.append(
                    [r["pin"]] + [d["pins"].get(r["pin"], "") for d in demos] + [""]
                )
        for pin in sorted(seen - {r["pin"] for r in rows}):
            ws.append(
                [pin]
                + [d["pins"].get(pin, "") for d in demos]
                + ["datasheet 表中未找到，请核对"]
            )
    else:
        ws.append(["", "（暂无 demo：在 demo 目录添加 PINMUX.md 后运行 scan）"])
    header(ws, len(head))
    widths(ws, [10] + [26] * max(1, len(demos)) + [26])
    ws.freeze_panes = "B2"
    for row in ws.iter_rows(min_row=2):
        for cell in row:
            cell.alignment = Alignment(vertical="top", wrap_text=True)
            cell.border = border
            if cell.value:
                cell.fill = sub_fill
        row[0].font = Font(bold=True)

    # one detailed sheet per demo that carries a full pin table
    for d in chip_demos:
        rows_detail = d.get("rows") or []
        if not rows_detail:
            continue
        if not any(
            r.get("config") or r.get("dir") or r.get("note") for r in rows_detail
        ):
            continue

        sheet_name = re.sub(r"[\\/*?:\[\]]", "_", d["name"])[:31]
        if sheet_name in wb.sheetnames:
            continue

        ws = wb.create_sheet(sheet_name)
        ws.append(["引脚", "信号", "GPIO 配置", "方向", "备注"])
        for r in rows_detail:
            ws.append(
                [
                    r.get("pin", ""),
                    r.get("signal", ""),
                    r.get("config", ""),
                    r.get("dir", ""),
                    r.get("note", ""),
                ]
            )
        header(ws, 5)
        widths(ws, [10, 14, 18, 8, 56])
        ws.freeze_panes = "A2"
        for row in ws.iter_rows(min_row=2):
            for cell in row:
                cell.border = border
                cell.alignment = Alignment(vertical="top", wrap_text=True)

    path = os.path.join(HERE, "%s_PinMux.xlsx" % chip["id"])
    wb.save(path)
    return path, len(rows), gpio


def write_index(stats):
    lines = [
        "# PinMux 维护（按 MCU 型号 + 按 demo）",
        "",
        "本目录维护 **“一个 MCU 型号一张表”** 的引脚复用资源。",
        "demo 目录内的 `PINMUX.md` 只描述该 demo 自己的接线、电气与自检步骤。",
        "",
        "| 型号 | 数据手册 | 引脚行数 | GPIO/模拟 | 解析方式 | 工作簿 |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for s in stats:
        lines.append(
            "| %s | `%s` | %d | %d | %s | `%s_PinMux.xlsx` |"
            % (
                s["id"],
                s["pdf"],
                s["rows"],
                s["gpio"],
                "主功能+重映射" if s["layout"] == "af" else "复用+描述",
                s["id"],
            )
        )

    demos = load_demos()
    if demos:
        lines += [
            "",
            "## demo 汇总",
            "",
            "| demo | MCU | 引脚数 | 来源 | 目录 |",
            "| --- | --- | --- | --- | --- |",
        ]
        for d in demos:
            lines.append(
                "| %s | %s | %d | %s | `%s` |"
                % (
                    d["name"],
                    d.get("mcu") or "?",
                    len(d.get("pins") or {}),
                    d.get("source", ""),
                    d.get("dir") or "",
                )
            )

    lines += [
        "",
        "## 生成 / 更新",
        "",
        "```bash",
        "cd PinMux",
        "python gen_pinmux.py list          # 查看已配置型号",
        "python gen_pinmux.py scan          # 扫描各 demo 的 PINMUX.md -> demos.json",
        "python gen_pinmux.py all           # 全部型号：extract + scan + build",
        "python gen_pinmux.py all CH32X035  # 只处理某个型号",
        "```",
        "",
        "依赖：`pip install pypdf openpyxl`。",
        "",
        "## 约定",
        "",
        "* 默认复用：AF 名称不带后缀，如 `TX1`、`CK1`、`MCO`、`PIOC_IO0`",
        "* 重映射：AF 名称带选项后缀 `_N`，如 `TX1_2`、`PIOC_IO0_1`",
        "* 封装/型号列头每个型号不同（封装名或型号名），工作簿 `说明` 页保留了原始列头，",
        "  `PinList` 的 `封装/型号引脚串` 为原始值，按列头顺序阅读。",
        "* CH32X035 额外保留 7 个具名封装列，并带有 `WS2812_Panel` demo 详细页。",
        "",
        "## 每个 demo 的引脚分配（PINMUX.md）",
        "",
        "型号级工作簿记录全量引脚；**demo 级只记录自己用到的引脚**，并且单向汇总：",
        "",
        "1. 在 demo 目录写 `PINMUX.md`（可复制 `PinMux/PINMUX.template.md`）：",
        "   引脚表 / 接线网表 / 复用冲突 / 电气要点 / 上电自检；",
        "2. 运行 `python gen_pinmux.py scan`，生成 `demos.json`（demo 名 + MCU + 引脚表）；",
        "3. `build` 时按 MCU 型号把 demo 写入对应工作簿的 `DemoUsage`，",
        "   带“配置/方向/备注”的 demo 还会自动生成一个同名详细工作表；",
        "4. 改了 `PINMUX.md` 只需重新 `scan` + `build`，不再手改 Excel。",
        "",
        "## 新增一个型号",
        "",
        "1. 把数据手册放到 `Doc/DS/`；",
        "2. 在 `gen_pinmux.py` 的 `CHIPS` 中加一行（型号、PDF、`af`/`list`、列头说明）；",
        "3. `python gen_pinmux.py all <MODEL>`，然后人工复核 CSV；",
        "4. 在工作簿 `DemoUsage` 增加该型号上的 demo 列。",
        "",
        "## 提取质量",
        "",
        "数据来自各自数据手册的“引脚定义”表，自动解析后**建议人工复核**：",
        "",
        "* 多型号/多封装列的对齐（`封装/型号引脚串`）；",
        "* 手册中存在第二张引脚表时（小封装/另一型号），只保留列数最多的一条，",
        "  其余列可能丢失；",
        "* CH32X315 的 `引脚功能` 列带 `(AFn)` 标记，归一到“默认/重映射”意义有限；",
        "* 电源引脚允许重复行（多个 GND/VDD 物理引脚），个别型号可能残留重复的 VDD 行；",
        "* 手册脚注（内部短接、BOOT 引脚、复用优先级、USB 注意事项）未收录，",
        "  需要时补到 `DemoUsage` 或 demo 的 `PINMUX.md`。",
        "",
        "板级备注（特定开发板接线）仍保留在仓库根目录 `PinMux.md`。",
        "",
    ]
    with open(INDEX, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------
def models(args):
    if not args:
        return [c[0] for c in CHIPS]
    out = []
    for a in args:
        key = a.upper()
        if key not in CHIP_MAP:
            print("unknown model:", a)
            sys.exit(2)
        out.append(key)
    return out


def do_extract(ids):
    stats = []
    for cid in ids:
        chip = CHIP_MAP[cid]
        if cid == "CH32X035":
            rows, fields = parse_x035(), X035_FIELDS
        else:
            rows, fields = (
                parse_chip(chip),
                [
                    "pin",
                    "type",
                    "main",
                    "af_default",
                    "af_remap",
                    "af_raw",
                    "pins_raw",
                    "desc",
                ],
            )
        path = write_csv(cid, rows, fields)
        gpio = sum(
            1
            for r in rows
            if r["type"].startswith("I/O") or r["type"] in ("I/A", "O/A", "A", "O", "I")
        )
        print(
            "%-10s %-28s rows=%-3d gpio=%-3d"
            % (cid, os.path.basename(path), len(rows), gpio)
        )
        stats.append(
            {
                "id": cid,
                "pdf": chip["pdf"],
                "rows": len(rows),
                "gpio": gpio,
                "layout": chip["layout"],
            }
        )
    return stats


def do_build(ids, stats=None):
    if stats is None:
        stats = []
        for cid in ids:
            try:
                rows = read_csv(os.path.join(HERE, "%s_pins.csv" % cid))
            except FileNotFoundError:
                continue
            gpio = sum(
                1
                for r in rows
                if r["type"].startswith("I/O")
                or r["type"] in ("I/A", "O/A", "A", "O", "I")
            )
            stats.append(
                {
                    "id": cid,
                    "pdf": CHIP_MAP[cid]["pdf"],
                    "rows": len(rows),
                    "gpio": gpio,
                    "layout": CHIP_MAP[cid]["layout"],
                }
            )
    demos = load_demos()
    for cid in ids:
        chip = CHIP_MAP[cid]
        fields = X035_FIELDS if cid == "CH32X035" else CSV_FIELDS
        rows = read_csv(os.path.join(HERE, "%s_pins.csv" % cid))
        path, n, gpio = build_xlsx(chip, rows, fields, demos)
        print(
            "%-10s %-28s rows=%-3d gpio=%-3d" % (cid, os.path.basename(path), n, gpio)
        )
    # index covers every CSV present
    all_stats = []
    for cid in [c[0] for c in CHIPS]:
        p = os.path.join(HERE, "%s_pins.csv" % cid)
        if not os.path.exists(p):
            continue
        rows = read_csv(p)
        gpio = sum(
            1
            for r in rows
            if r["type"].startswith("I/O") or r["type"] in ("I/A", "O/A", "A", "O", "I")
        )
        all_stats.append(
            {
                "id": cid,
                "pdf": CHIP_MAP[cid]["pdf"],
                "rows": len(rows),
                "gpio": gpio,
                "layout": CHIP_MAP[cid]["layout"],
            }
        )
    write_index(all_stats)
    print("index:", INDEX)


def main(argv=None):
    ap = argparse.ArgumentParser(description="WCH PinMux generator")
    sub = ap.add_subparsers(dest="cmd")

    sub.add_parser("list", help="list configured models")

    ex = sub.add_parser("extract", help="PDF -> CSV")
    ex.add_argument("models", nargs="*")

    sub.add_parser("scan", help="demo PINMUX.md -> demos.json")

    bd = sub.add_parser("build", help="CSV + demos.json -> XLSX + index")
    bd.add_argument("models", nargs="*")

    al = sub.add_parser("all", help="extract + scan + build")
    al.add_argument("models", nargs="*")

    args = ap.parse_args(argv)

    if args.cmd == "list":
        for c in CHIPS:
            print("%-10s %-22s %-4s %s" % (c[0], c[1], c[2], c[3]))
        return 0

    if args.cmd == "extract":
        stats = do_extract(models(args.models))
        return 0

    if args.cmd == "scan":
        scan_demos()
        return 0

    if args.cmd == "build":
        do_build(models(args.models))
        return 0

    if args.cmd == "all":
        ids = models(args.models)
        stats = do_extract(ids)
        scan_demos()
        do_build(ids, stats)
        return 0

    ap.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
