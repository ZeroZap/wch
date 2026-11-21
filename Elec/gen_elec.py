#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gen_elec.py - WCH electrical-characteristics workbook generator.

For every supported MCU this script produces

    <MODEL>_elec.csv          long table: category / item / condition / value
    <MODEL>_Electrical.xlsx   workbook: 说明 / 汇总 / 低功耗 / 唤醒 / Flash / 其他
    elec_all.csv              all models in one long table
    Elec_All.xlsx             all models in one workbook
    elec.json                 machine-readable data for CubeX / HAL metadata
    README.md                 regenerated index and comparison tables

The source values live in ``elec_data.py``.  Every row carries the datasheet
page it was read from; ``check`` verifies that the referenced page still
contains the expected chapter keyword.

Commands
    python gen_elec.py list
    python gen_elec.py show [MODEL ...]
    python gen_elec.py survey [MODEL ...]
    python gen_elec.py pages MODEL KEYWORD [PAGE_FROM PAGE_TO]
    python gen_elec.py check
    python gen_elec.py build [MODEL ...]
    python gen_elec.py all   [MODEL ...]

Dependencies
    build : openpyxl
    pages/check : pypdf
"""

import argparse
import csv
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DS_DIR = os.path.join(HERE, "..", "Doc", "DS")
INDEX = os.path.join(HERE, "README.md")
ALL_CSV = os.path.join(HERE, "elec_all.csv")
ALL_XLSX = os.path.join(HERE, "Elec_All.xlsx")
ELEC_JSON = os.path.join(HERE, "elec.json")

sys.path.insert(0, HERE)
import elec_data  # noqa: E402

MODELS = elec_data.MODELS
MODEL_MAP = {m["id"]: m for m in MODELS}
SCHEMA_VERSION = elec_data.SCHEMA_VERSION

CSV_FIELDS = [
    "model",
    "category",
    "item",
    "mode",
    "symbol",
    "condition",
    "min",
    "typ",
    "max",
    "unit",
    "source_page",
    "note",
]

SUMMARY_FIELDS = [
    "型号",
    "系列",
    "内核",
    "最高主频",
    "Flash",
    "SRAM",
    "供电",
    "TA",
    "ESD",
    "低功耗模式",
    "最低典型电流",
    "唤醒时间",
    "Flash 页",
    "Flash 扇区",
    "擦写寿命",
    "数据保持",
]

FLASH_FIELDS = [
    ("page_bytes", "页大小", "page", "B"),
    ("sector_bytes", "扇区大小", "sector", "B"),
    ("block_bytes", "块大小", "block", "B"),
    ("min_write_bytes", "最小写入单位", "min_write", ""),
    ("program_time", "页编程时间", "tprog", ""),
    ("page_erase_time", "页擦除时间", "terase_page", ""),
    ("sector_erase_time", "扇区擦除时间", "terase_sector", ""),
    ("block_erase_time", "块擦除时间", "terase_block", ""),
    ("mass_erase_time", "整片擦除时间", "terase_mass", ""),
    ("endurance", "擦写寿命", "NEND", ""),
    ("retention", "数据保持", "tRET", ""),
]


# ---------------------------------------------------------------------------
# row assembly
# ---------------------------------------------------------------------------
def _fmt(v):
    return "" if v is None else str(v)


def _num(v):
    try:
        float(str(v))
        return True
    except (TypeError, ValueError):
        return False


def flash_display(model):
    if model.get("flash_label"):
        return model["flash_label"]
    v = str(model.get("flash_kb", ""))
    return (v + " KB") if _num(v) else v


def sram_display(model):
    if model.get("sram_label"):
        return model["sram_label"]
    v = str(model.get("sram_kb", ""))
    return (v + " KB") if _num(v) else v


def bytes_display(value):
    v = _fmt(value)
    if not v:
        return v
    return (v + " B") if _num(v) else v


def flash_rows(model):
    """Yield flash rows from the structured ``flash`` block."""
    f = model["flash"]
    page = f.get("source_page", "")
    note = f.get("note", "")
    for key, item, symbol, unit in FLASH_FIELDS:
        value = f.get(key, "")
        if value == "":
            continue
        if key == "min_write_bytes" and _num(value):
            unit = "B"
        yield {
            "model": model["id"],
            "category": "flash",
            "item": item,
            "mode": "",
            "symbol": symbol,
            "condition": "",
            "min": "",
            "typ": value,
            "max": "",
            "unit": unit,
            "source_page": page,
            "note": "",
        }
    if note:
        yield {
            "model": model["id"],
            "category": "flash",
            "item": "编程粒度/注意事项",
            "mode": "",
            "symbol": "note",
            "condition": "",
            "min": "",
            "typ": note,
            "max": "",
            "unit": "",
            "source_page": page,
            "note": "",
        }


def model_rows(model, with_model=True):
    """Yield every CSV row for one model."""
    rows = []
    rows.extend(flash_rows(model))
    for r in model["low_power"]:
        rows.append(
            {
                "model": model["id"],
                "category": "low_power",
                "item": "低功耗电流",
                "mode": r["mode"],
                "symbol": "IDD",
                "condition": "；".join(
                    x for x in (r.get("clock", ""), r.get("condition", "")) if x
                ),
                "min": _fmt(r.get("min")),
                "typ": _fmt(r.get("typ")),
                "max": _fmt(r.get("max")),
                "unit": r.get("unit", ""),
                "source_page": r.get("page", ""),
                "note": r.get("note", ""),
            }
        )
    for r in model["wakeup"]:
        rows.append(
            {
                "model": model["id"],
                "category": "wakeup",
                "item": "低功耗唤醒时间",
                "mode": r["mode"],
                "symbol": "tWU",
                "condition": r.get("condition", ""),
                "min": _fmt(r.get("min")),
                "typ": _fmt(r.get("typ")),
                "max": _fmt(r.get("max")),
                "unit": r.get("unit", ""),
                "source_page": r.get("page", ""),
                "note": r.get("note", ""),
            }
        )
    for r in model.get("other", []):
        rows.append(
            {
                "model": model["id"],
                "category": r.get("category", "other"),
                "item": r.get("item", ""),
                "mode": "",
                "symbol": r.get("symbol", ""),
                "condition": r.get("condition", ""),
                "min": _fmt(r.get("min")),
                "typ": _fmt(r.get("typ")),
                "max": _fmt(r.get("max")),
                "unit": r.get("unit", ""),
                "source_page": r.get("page", ""),
                "note": r.get("note", ""),
            }
        )
    if with_model:
        return rows
    return [{k: v for k, v in row.items() if k != "model"} for row in rows]


def summary_row(model):
    f = model["flash"]
    return {
        "型号": model["id"],
        "系列": model.get("family", ""),
        "内核": model.get("core", ""),
        "最高主频": model.get("max_mhz", ""),
        "Flash": flash_display(model),
        "SRAM": sram_display(model),
        "供电": model.get("vdd", ""),
        "TA": model.get("ta", ""),
        "ESD": model.get("esd", ""),
        "低功耗模式": model.get("modes", ""),
        "最低典型电流": model.get("lp_min", ""),
        "唤醒时间": model.get("wake_summary", ""),
        "Flash 页": bytes_display(f.get("page_bytes")) or "DS 未给出",
        "Flash 扇区": bytes_display(f.get("sector_bytes")) or "DS 未给出",
        "擦写寿命": f.get("endurance", "DS 未给出"),
        "数据保持": f.get("retention", "DS 未给出"),
    }


# ---------------------------------------------------------------------------
# writers
# ---------------------------------------------------------------------------
def write_csv(path, rows, fields=CSV_FIELDS):
    with open(path, "w", newline="", encoding="utf-8-sig") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)
    return path


def write_json(path):
    payload = {
        "schema_version": SCHEMA_VERSION,
        "source": "Doc/DS/*.PDF + EVT Flash headers; see each model flash.source_page/note",
        "models": MODELS,
    }
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(payload, fh, ensure_ascii=False, indent=2, sort_keys=False)
        fh.write("\n")
    return path


def _xlsx_styles():
    from openpyxl.styles import Alignment, Border, Font, PatternFill, Side

    return {
        "head_fill": PatternFill("solid", fgColor="1F4E79"),
        "head_font": Font(color="FFFFFF", bold=True),
        "sub_fill": PatternFill("solid", fgColor="DDEBF7"),
        "warn_fill": PatternFill("solid", fgColor="FFF2CC"),
        "border": Border(
            left=Side(style="thin", color="B0B0B0"),
            right=Side(style="thin", color="B0B0B0"),
            top=Side(style="thin", color="B0B0B0"),
            bottom=Side(style="thin", color="B0B0B0"),
        ),
        "alignment": Alignment(vertical="top", wrap_text=True),
    }


def _write_table(ws, headers, rows, widths, styles, start_row=1, title=None):
    from openpyxl.utils import get_column_letter

    row = start_row
    if title:
        ws.cell(row=row, column=1, value=title)
        ws.cell(row=row, column=1).font = styles["head_font"]
        ws.cell(row=row, column=1).fill = styles["head_fill"]
        ws.merge_cells(
            start_row=row, start_column=1, end_row=row, end_column=len(headers)
        )
        row += 1
    for c, h in enumerate(headers, start=1):
        cell = ws.cell(row=row, column=c, value=h)
        cell.fill = styles["head_fill"]
        cell.font = styles["head_font"]
        cell.alignment = styles["alignment"]
        cell.border = styles["border"]
    header_row = row
    for r in rows:
        row += 1
        for c, h in enumerate(headers, start=1):
            cell = ws.cell(row=row, column=c, value=r.get(h, ""))
            cell.alignment = styles["alignment"]
            cell.border = styles["border"]
    for i, w in enumerate(widths, start=1):
        ws.column_dimensions[get_column_letter(i)].width = w
    ws.freeze_panes = ws.cell(row=header_row + 1, column=1)
    if rows:
        ws.auto_filter.ref = "%s%d:%s%d" % (
            "A",
            header_row,
            get_column_letter(len(headers)),
            row,
        )
    return header_row


def _sheet_rows_for_model(model, category):
    rows = []
    for r in model_rows(model, with_model=False):
        if r["category"] != category:
            continue
        rows.append(
            {
                "模式": r["mode"],
                "符号": r["symbol"],
                "条件": r["condition"],
                "最小值": r["min"],
                "典型值": r["typ"],
                "最大值": r["max"],
                "单位": r["unit"],
                "数据手册页": r["source_page"],
                "备注": r["note"],
            }
        )
    return rows


def build_model_xlsx(model, path):
    from openpyxl import Workbook

    st = _xlsx_styles()
    wb = Workbook()
    ws = wb.active
    ws.title = "说明"

    info = [
        ("%s 电气特性维护表" % model["id"], "title"),
        ("", ""),
        ("数据来源", "h2"),
        ("Doc/DS/%s" % model["pdf"], ""),
        ("Flash 粒度补充来源", model["flash"].get("source_page", "")),
        ("", ""),
        ("概要", "h2"),
        (
            "内核 / 最高主频",
            "%s / %s MHz" % (model.get("core", ""), model.get("max_mhz", "")),
        ),
        ("Flash / SRAM", "%s / %s" % (flash_display(model), sram_display(model))),
        ("供电 / 温度", "%s / %s" % (model.get("vdd", ""), model.get("ta", ""))),
        ("低功耗模式", model.get("modes", "")),
        ("最低典型电流", model.get("lp_min", "")),
        ("唤醒时间", model.get("wake_summary", "")),
        (
            "Flash 页 / 扇区",
            "%s / %s"
            % (
                bytes_display(model["flash"].get("page_bytes")) or "DS 未给出",
                bytes_display(model["flash"].get("sector_bytes")) or "DS 未给出",
            ),
        ),
        (
            "擦写寿命 / 数据保持",
            "%s / %s"
            % (
                model["flash"].get("endurance", "DS 未给出"),
                model["flash"].get("retention", "DS 未给出"),
            ),
        ),
        ("", ""),
        ("注意事项", "h2"),
    ]
    r = 1
    for text, kind in info:
        cell = ws.cell(row=r, column=1, value=text)
        if kind == "title":
            cell.font = st["head_font"]
            cell.fill = st["head_fill"]
            ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=8)
        elif kind == "h2":
            from openpyxl.styles import Font

            cell.font = Font(bold=True, color="1F4E79")
            cell.fill = st["sub_fill"]
            ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=8)
        elif kind:
            vcell = ws.cell(row=r, column=2, value=kind)
            vcell.alignment = st["alignment"]
            ws.merge_cells(start_row=r, start_column=2, end_row=r, end_column=8)
        r += 1
    for c in model.get("cautions", []):
        cell = ws.cell(row=r, column=1, value="• " + c)
        cell.fill = st["warn_fill"]
        cell.alignment = st["alignment"]
        ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=8)
        r += 1
    ws.column_dimensions["A"].width = 30
    for col in "BCDEFGH":
        ws.column_dimensions[col].width = 18

    ws = wb.create_sheet("汇总")
    summary_headers = SUMMARY_FIELDS
    row = summary_row(model)
    _write_table(
        ws,
        summary_headers,
        [row],
        [14, 14, 14, 18, 18, 14, 34, 24, 18, 22, 26, 34, 12, 14, 30, 24],
        st,
    )

    for name, cat, widths in (
        ("低功耗", "low_power", [10, 12, 42, 12, 12, 12, 10, 10, 34]),
        ("唤醒", "wakeup", [10, 12, 42, 12, 12, 12, 10, 10, 34]),
        ("Flash", "flash", [22, 14, 12, 30, 20, 10, 16, 30]),
    ):
        ws = wb.create_sheet(name)
        rows = _sheet_rows_for_model(model, cat)
        headers = [
            "模式",
            "符号",
            "条件",
            "最小值",
            "典型值",
            "最大值",
            "单位",
            "数据手册页",
            "备注",
        ]
        if cat == "flash":
            # Flash rows do not have a useful mode; keep the item name.
            item_by_symbol = {sym: item for _, item, sym, _ in FLASH_FIELDS}
            headers = [
                "项目",
                "符号",
                "典型值",
                "单位",
                "数据手册页",
                "备注",
            ]
            rows = [
                {
                    "项目": item_by_symbol.get(x["符号"], "编程粒度/注意事项"),
                    "符号": x["符号"],
                    "典型值": "" if x["符号"] == "note" else x["典型值"],
                    "单位": x["单位"],
                    "数据手册页": x["数据手册页"],
                    "备注": x["典型值"] if x["符号"] == "note" else x["备注"],
                }
                for x in rows
            ]
            widths = [24, 18, 34, 10, 12, 36]
        _write_table(ws, headers, rows, widths, st)

    ws = wb.create_sheet("其他")
    rows = []
    for r0 in model.get("other", []):
        rows.append(
            {
                "分类": r0.get("category", ""),
                "项目": r0.get("item", ""),
                "符号": r0.get("symbol", ""),
                "条件": r0.get("condition", ""),
                "最小值": _fmt(r0.get("min")),
                "典型值": _fmt(r0.get("typ")),
                "最大值": _fmt(r0.get("max")),
                "单位": r0.get("unit", ""),
                "数据手册页": r0.get("page", ""),
                "备注": r0.get("note", ""),
            }
        )
    _write_table(
        ws,
        [
            "分类",
            "项目",
            "符号",
            "条件",
            "最小值",
            "典型值",
            "最大值",
            "单位",
            "数据手册页",
            "备注",
        ],
        rows,
        [12, 26, 18, 34, 12, 12, 12, 8, 12, 30],
        st,
    )

    wb.save(path)
    return path


def build_all_xlsx(path):
    from openpyxl import Workbook

    st = _xlsx_styles()
    wb = Workbook()
    ws = wb.active
    ws.title = "说明"

    lines = [
        ("WCH MCU 电气特性汇总", "title"),
        ("", ""),
        ("数据来源", "h2"),
        ("Doc/DS/*.PDF 电气特性/参数章节；Flash 粒度补充自各 EVT Flash 库头文件。", ""),
        ("每个型号的详细页见 <MODEL>_Electrical.xlsx；每个数值都带数据手册页码。", ""),
        ("", ""),
        ("口径", "h2"),
        ("典型值 = datasheet 的 typ 列；未给 typ 的保留 min/max。", ""),
        ("“抽测/非担保”保留原文，不能当保证值使用。", ""),
        ("低功耗电流均为 datasheet 测试条件（VDD、IO 配置、外设开关）下的值。", ""),
        ("", ""),
        ("重点", "h2"),
        ("CH32X035 Flash 为 256 B 整页编程，不支持字节编程。", ""),
        (
            "CH32M030 Flash 为 128 B 页，擦写寿命 10K–50K（25 ℃）/1K–5K（125 ℃），保持 10 年。",
            "",
        ),
        ("CH32X315 无待机模式，Stop 典型 0.79–0.87 mA。", ""),
        ("CH32V002/006/007 只有睡眠/待机，无停止模式。", ""),
        ("CH595/CH596 数据保持仅 10 年，-40–85 ℃ 擦写寿命最低 30K。", ""),
        ("CH569 数据手册未给 Flash 擦写寿命/编程时间，只有 EVT 库给出页/块粒度。", ""),
    ]
    r = 1
    for text, kind in lines:
        cell = ws.cell(row=r, column=1, value=text)
        if kind == "title":
            cell.font = st["head_font"]
            cell.fill = st["head_fill"]
            ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=10)
        elif kind == "h2":
            from openpyxl.styles import Font

            cell.font = Font(bold=True, color="1F4E79")
            cell.fill = st["sub_fill"]
            ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=10)
        else:
            cell.alignment = st["alignment"]
            ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=10)
        r += 1
    ws.column_dimensions["A"].width = 40
    for col in "BCDEFGHIJ":
        ws.column_dimensions[col].width = 14

    ws = wb.create_sheet("汇总")
    rows = [summary_row(m) for m in MODELS]
    _write_table(
        ws,
        SUMMARY_FIELDS,
        rows,
        [14, 16, 14, 20, 18, 14, 36, 26, 20, 24, 28, 36, 14, 16, 34, 26],
        st,
    )

    for name, cat, widths in (
        ("低功耗", "low_power", [14, 10, 12, 46, 10, 10, 10, 8, 10, 34]),
        ("唤醒", "wakeup", [14, 10, 12, 46, 10, 10, 10, 8, 10, 34]),
        ("Flash", "flash", [14, 22, 14, 30, 18, 10, 14, 30]),
    ):
        ws = wb.create_sheet(name)
        rows = []
        for m in MODELS:
            for r0 in model_rows(m, with_model=False):
                if r0["category"] != cat:
                    continue
                rows.append(
                    {
                        "型号": m["id"],
                        "项目": r0["item"],
                        "模式": r0["mode"],
                        "符号": r0["symbol"],
                        "条件": r0["condition"],
                        "最小值": r0["min"],
                        "典型值": r0["typ"],
                        "最大值": r0["max"],
                        "单位": r0["unit"],
                        "数据手册页": r0["source_page"],
                        "备注": r0["note"],
                    }
                )
        if cat == "flash":
            headers = ["型号", "项目", "符号", "典型值", "单位", "数据手册页", "备注"]
            rows = [
                {
                    "型号": x["型号"],
                    "项目": x["项目"],
                    "符号": x["符号"],
                    "典型值": "" if x["符号"] == "note" else x["典型值"],
                    "单位": x["单位"],
                    "数据手册页": x["数据手册页"],
                    "备注": x["典型值"] if x["符号"] == "note" else x["备注"],
                }
                for x in rows
            ]
            widths = [14, 24, 18, 34, 10, 12, 40]
        else:
            headers = [
                "型号",
                "模式",
                "符号",
                "条件",
                "最小值",
                "典型值",
                "最大值",
                "单位",
                "数据手册页",
                "备注",
            ]
        _write_table(ws, headers, rows, widths, st)

    ws = wb.create_sheet("其他")
    rows = []
    for m in MODELS:
        for r0 in m.get("other", []):
            rows.append(
                {
                    "型号": m["id"],
                    "分类": r0.get("category", ""),
                    "项目": r0.get("item", ""),
                    "符号": r0.get("symbol", ""),
                    "条件": r0.get("condition", ""),
                    "最小值": _fmt(r0.get("min")),
                    "典型值": _fmt(r0.get("typ")),
                    "最大值": _fmt(r0.get("max")),
                    "单位": r0.get("unit", ""),
                    "数据手册页": r0.get("page", ""),
                    "备注": r0.get("note", ""),
                }
            )
    _write_table(
        ws,
        [
            "型号",
            "分类",
            "项目",
            "符号",
            "条件",
            "最小值",
            "典型值",
            "最大值",
            "单位",
            "数据手册页",
            "备注",
        ],
        rows,
        [14, 12, 28, 18, 34, 12, 12, 12, 8, 12, 30],
        st,
    )

    wb.save(path)
    return path


def _md(s):
    return str(s).replace("|", "\\|").replace("\n", " ")


def build_readme():
    lines = []
    lines.append("# 电气特性维护（按 MCU 型号）")
    lines.append("")
    lines.append(
        "本目录维护 **“一个 MCU 型号一份电气特性表”**，与 `PinMux/` 的组织方式一致。"
    )
    lines.append(
        "数据来自 `Doc/DS/*.PDF` 的电气特性/参数章节，每个数值都带数据手册页码；"
    )
    lines.append(
        "Flash 的页/块/最小写入粒度在数据手册未写明时，补充引用对应 EVT Flash 库头文件。"
    )
    lines.append("")
    lines.append(
        "当前覆盖与 `PinMux/` 相同的 17 个型号；`CH32V103EVT`、`CH32V20xEVT`、"
        "`CH32H417TEVT` 因 `Doc/DS` 中没有对应数据手册，暂未列入。"
    )
    lines.append("")
    lines.append(
        "| 型号 | 数据手册 | 内核 | 最高主频 | Flash | SRAM | 供电 | 低功耗模式 |"
    )
    lines.append("| --- | --- | --- | --- | --- | --- | --- | --- |")
    for m in MODELS:
        lines.append(
            "| %s | `%s` | %s | %s | %s | %s | %s | %s |"
            % (
                m["id"],
                m["pdf"],
                m.get("core", ""),
                m.get("max_mhz", ""),
                flash_display(m),
                sram_display(m),
                _md(m.get("vdd", "")),
                _md(m.get("modes", "")),
            )
        )
    lines.append("")
    lines.append("## 1. Flash 粒度与寿命")
    lines.append("")
    lines.append("| 型号 | 页 | 扇区 | 块 | 最小写入 | 擦写寿命 | 数据保持 |")
    lines.append("| --- | --- | --- | --- | --- | --- | --- |")
    for m in MODELS:
        f = m["flash"]
        page = ("%s B" % f["page_bytes"]) if f.get("page_bytes") else "DS 未给出"
        sector = ("%s B" % f["sector_bytes"]) if f.get("sector_bytes") else "DS 未给出"
        block = ("%s B" % f["block_bytes"]) if f.get("block_bytes") else "DS 未给出"
        minw = f.get("min_write_bytes") or "DS 未给出"
        lines.append(
            "| %s | %s | %s | %s | %s | %s | %s |"
            % (
                m["id"],
                page,
                sector,
                block,
                _md(bytes_display(minw)),
                _md(f.get("endurance", "DS 未给出")),
                _md(f.get("retention", "DS 未给出")),
            )
        )
    lines.append("")
    lines.append("## 2. 低功耗电流与唤醒时间")
    lines.append("")
    lines.append("| 型号 | 低功耗模式 | 最低典型电流 | 唤醒时间 |")
    lines.append("| --- | --- | --- | --- |")
    for m in MODELS:
        lines.append(
            "| %s | %s | %s | %s |"
            % (
                m["id"],
                _md(m.get("modes", "")),
                _md(m.get("lp_min", "")),
                _md(m.get("wake_summary", "")),
            )
        )
    lines.append("")
    lines.append("## 3. 重点差异（先看这里）")
    lines.append("")
    highlights = [
        "**CH32X035 Flash 是 256 B 整页编程，不是字节编程。** 需要先在页缓冲中填满 256 B，再调用 `FLASH_ProgramPage_Fast()` 整页提交；升级/参数存储的掉电保护必须按整页设计。",
        "**CH32M030 Flash 页只有 128 B，寿命 10K–50K（25 ℃）/1K–5K（125 ℃），保持 10 年**，且寿命为抽测值、非担保。",
        "**CH32X315 没有待机模式**，最低是 Stop 的 0.79–0.87 mA；不能把它当 uA 级低功耗 MCU。",
        "**CH32V002/CH32V006/CH32V007 只有睡眠和待机，没有停止模式**；待机约 10–18 uA。",
        "**CH32V307/CH32V407 的 Flash 寿命表注明“实测抽测，非担保”**；V307 为 10K 最小/80K 抽测，V407 为 50K。两者擦除/编程时间和待机唤醒时间也明显更长。",
        "**CH5xx 系列 CodeFlash 按字（4 B）编程、256 B/页写、4 KB/块擦**；DataFlash 才支持按字节读写。`TSUHSE` 参与暂停/睡眠/下电唤醒时间计算。",
        "**CH595/CH596 数据保持只有 10 年**，-40–85 ℃ 擦写寿命最低 30K；明显低于 CH573/CH583/CH592 的 20 年。",
        "**CH569 数据手册没有 Flash 擦写寿命/编程时间**；只有 EVT `ISPEM569.h` 给出 4 B 最小写、256 B 页写、4 KB 块擦，可靠性指标需向原厂确认。",
    ]
    for h in highlights:
        lines.append("* " + h)
    lines.append("")
    lines.append("### CH32X035 Flash 写入方式（重点）")
    lines.append("")
    lines.append("* 编程单位：256 B/页；擦除单位：页 256 B 或扇区 1 KB。")
    lines.append(
        "* 典型流程：`FLASH_Unlock_Fast()` → `FLASH_BufReset()` → 64 次 `FLASH_BufLoad(addr + 4*i, word)` → `FLASH_ProgramPage_Fast(addr)`。"
    )
    lines.append(
        "* 不支持按字节写入；参数存储不要按 EEPROM 逐字节写，必须凑满 256 B 页或做读-改-写。"
    )
    lines.append("")
    lines.append("## 4. 各型号注意事项")
    lines.append("")
    for m in MODELS:
        lines.append("### %s" % m["id"])
        lines.append("")
        for c in m.get("cautions", []):
            lines.append("* " + c)
        lines.append("")
    lines.append("## 5. 文件说明")
    lines.append("")
    lines.append("| 文件 | 内容 |")
    lines.append("| --- | --- |")
    lines.append("| `elec_data.py` | 人工复核后的数据源，每个数值带数据手册页码 |")
    lines.append("| `gen_elec.py` | 生成/检查脚本 |")
    lines.append(
        "| `<MODEL>_elec.csv` | 单型号长表：分类/项目/条件/最小值/典型值/最大值/单位/页码/备注 |"
    )
    lines.append(
        "| `<MODEL>_Electrical.xlsx` | 单型号工作簿：说明/汇总/低功耗/唤醒/Flash/其他 |"
    )
    lines.append("| `elec_all.csv` | 全部型号长表，便于 grep/diff |")
    lines.append("| `Elec_All.xlsx` | 全部型号汇总工作簿 |")
    lines.append("| `elec.json` | 机器可读数据，供 CubeX / HAL 元数据使用 |")
    lines.append("")
    lines.append("## 6. 生成 / 检查")
    lines.append("")
    lines.append("```bash")
    lines.append("cd Elec")
    lines.append("python gen_elec.py list                 # 查看已配置型号")
    lines.append("python gen_elec.py show CH32X035        # 打印单型号摘要")
    lines.append(
        "python gen_elec.py survey             # 定位各手册的电气/Flash/低功耗/唤醒页"
    )
    lines.append("python gen_elec.py pages CH32X035 擦写   # 查看 PDF 命中页，人工复核")
    lines.append("python gen_elec.py check                # 校验型号/页码/关键字")
    lines.append(
        "python gen_elec.py all                  # 生成全部 CSV/XLSX/JSON/README"
    )
    lines.append("python gen_elec.py build CH32X035       # 只重建一个型号")
    lines.append("```")
    lines.append("")
    lines.append(
        "依赖：`pip install openpyxl pypdf`（`build` 只需要 openpyxl，`pages/check` 需要 pypdf）。"
    )
    lines.append("")
    lines.append("## 7. 新增一个型号")
    lines.append("")
    lines.append("1. 把数据手册放到 `Doc/DS/`；")
    lines.append(
        "2. 在 `elec_data.py` 的 `MODELS` 中增加一条记录，填写 `pdf`、概要、低功耗、唤醒、Flash、其他；"
    )
    lines.append(
        "3. 每个数值填写 `page`（数据手册页码）；Flash 粒度若来自 EVT 库，在 `flash.note` 中写清头文件；"
    )
    lines.append(
        "4. 运行 `python gen_elec.py check`，再 `python gen_elec.py all <MODEL>`；"
    )
    lines.append("5. 人工复核 CSV/XLSX，特别是“抽测/非担保”、Typ/Min/Max 和温度条件。")
    lines.append("")
    lines.append("## 8. 提取质量与口径")
    lines.append("")
    lines.append(
        "* **典型值是 datasheet 的 typ 列**；没有 typ 的保留 min/max，不自行取中值。"
    )
    lines.append("* **“抽测/实测/非担保”原文保留**，这类寿命数据不能当保证值使用。")
    lines.append(
        "* **低功耗电流与测试条件绑定**：VDD、IO 上下拉、外设时钟开关、RAM 保持、LDO 档位不同都会改变结果；表中 `condition` 列保留了这些条件。"
    )
    lines.append(
        "* **唤醒时间**：CH5xx 的暂停/睡眠/下电唤醒以 `TSUHSE` 为基准，`TSUHSE` 典型 200 us；CH32 的待机唤醒可能包含代码加载时间（V307 8.9 ms、V407 18.5 ms）。"
    )
    lines.append(
        "* **Flash 粒度**：CH32X035 为 256 B 页；CH32M030 为 128 B 页；CH5xx CodeFlash 为 4 B 字/256 B 页写、4 KB 块擦；CH569 为 4 B 字/256 B 页写、4 KB 块擦。"
    )
    lines.append(
        "* **数据手册缺项**保留为“DS 未给出”，不用同类芯片猜测补齐；需要时查 RM/EVT 或向原厂确认。"
    )
    lines.append("")
    lines.append("## 9. 复核建议")
    lines.append("")
    lines.append(
        "1. 先看每个型号的 `说明` 页和 `Flash` 页，确认页/扇区/块粒度与项目写入方式一致。"
    )
    lines.append(
        "2. 低功耗选型时同时看 `低功耗` 与 `唤醒`，不要只比较最低电流；CH5xx 还要加 `TSUHSE` 和射频占空比。"
    )
    lines.append(
        "3. 高可靠/频繁参数存储场景，优先使用“保证值”而不是“抽测值”，并按温度降额。"
    )
    lines.append(
        "4. 任何与官方最新版数据手册不一致的地方，以最新版 DS 为准，并回改 `elec_data.py` 后重新生成。"
    )
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("生成命令：`python gen_elec.py all`；数据源：`elec_data.py`。")
    lines.append("")
    with open(INDEX, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    return INDEX


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------
def selected(argv):
    if not argv:
        return MODELS
    out = []
    for name in argv:
        key = name.upper()
        if key not in MODEL_MAP:
            raise SystemExit(
                "unknown model: %s\nknown: %s" % (name, ", ".join(MODEL_MAP))
            )
        out.append(MODEL_MAP[key])
    return out


def cmd_list(_args):
    print(
        "%-10s %-24s %-36s %-16s %-40s"
        % ("model", "pdf", "flash", "page/sector", "endurance")
    )
    for m in MODELS:
        f = m["flash"]
        print(
            "%-10s %-24s %-36s %-16s %-40s"
            % (
                m["id"],
                m["pdf"],
                flash_display(m),
                "%s/%s"
                % (
                    bytes_display(f.get("page_bytes")) or "-",
                    bytes_display(f.get("sector_bytes")) or "-",
                ),
                (f.get("endurance", "") or "-")[:40],
            )
        )


def cmd_show(args):
    for m in selected(args):
        print("=" * 78)
        print("%s  %s" % (m["id"], m.get("core", "")))
        print(
            "  %s, Flash: %s, SRAM: %s, %s"
            % (
                m.get("max_mhz", ""),
                flash_display(m),
                sram_display(m),
                m.get("vdd", ""),
            )
        )
        print("  low power : %s" % m.get("lp_min", ""))
        print("  wake      : %s" % m.get("wake_summary", ""))
        f = m["flash"]
        print(
            "  flash     : page=%s, sector=%s, block=%s"
            % (
                bytes_display(f.get("page_bytes")) or "-",
                bytes_display(f.get("sector_bytes")) or "-",
                bytes_display(f.get("block_bytes")) or "-",
            )
        )
        print("  endurance : %s" % f.get("endurance", ""))
        print("  retention : %s" % f.get("retention", ""))
        if m.get("cautions"):
            print("  cautions  :")
            for c in m["cautions"]:
                print("    - " + c)


def cmd_pages(args):
    if len(args) < 2:
        raise SystemExit("usage: pages MODEL KEYWORD [PAGE_FROM PAGE_TO]")
    model = selected([args[0]])[0]
    keyword = args[1]
    lo = int(args[2]) if len(args) > 2 else 1
    hi = int(args[3]) if len(args) > 3 else 10**9
    try:
        from pypdf import PdfReader
    except ImportError:
        raise SystemExit("pypdf is required: pip install pypdf")
    path = os.path.join(DS_DIR, model["pdf"])
    reader = PdfReader(path)
    hits = 0
    for idx, page in enumerate(reader.pages, start=1):
        if idx < lo or idx > hi:
            continue
        text = page.extract_text() or ""
        if keyword in text:
            hits += 1
            print("\n===== %s PAGE %d =====" % (model["id"], idx))
            print(text)
    if not hits:
        print("no page contains %r in %s" % (keyword, model["pdf"]))


def _page_text(model, page):
    from pypdf import PdfReader

    reader = PdfReader(os.path.join(DS_DIR, model["pdf"]))
    if not isinstance(page, int) or page < 1 or page > len(reader.pages):
        return ""
    return reader.pages[page - 1].extract_text() or ""


def cmd_survey(args):
    """Locate candidate electrical-characteristics pages in each datasheet."""
    try:
        from pypdf import PdfReader
    except ImportError:
        raise SystemExit("pypdf is required: pip install pypdf")
    groups = {
        "电气特性章节": (
            "电气特性",
            "第 19章 参数",
            "第 20章 参数",
            "第 22章 参数",
            "第 23章 参数",
        ),
        "Flash/寿命": (
            "闪存存储器特性",
            "闪存存储器寿命",
            "擦写次数",
            "擦写寿命",
            "Endurance",
        ),
        "低功耗电流": (
            "停止模式",
            "待机模式",
            "睡眠模式",
            "下电模式",
            "暂停模式",
            "空闲模式",
        ),
        "唤醒时间": ("唤醒的时间", "唤醒时间", "TWAK"),
    }
    for m in selected(args):
        reader = PdfReader(os.path.join(DS_DIR, m["pdf"]))
        n = len(reader.pages)
        anchors = []
        fp = m["flash"].get("source_page")
        if isinstance(fp, int):
            anchors.append(fp)
        for block in ("low_power", "wakeup"):
            for r in m.get(block, []):
                if isinstance(r.get("page"), int):
                    anchors.append(r["page"])
        anchor = min(anchors) if anchors else max(1, n - 40)
        lo = max(1, anchor - 12)
        hi = min(n, anchor + 18)
        hits = {k: [] for k in groups}
        for idx, page in enumerate(reader.pages, start=1):
            if idx < lo or idx > hi:
                continue
            text = page.extract_text() or ""
            for key, words in groups.items():
                if any(w in text for w in words):
                    hits[key].append(idx)
        print("### %s  (%s, %d pages, scan %d-%d)" % (m["id"], m["pdf"], n, lo, hi))
        for key, pages in hits.items():
            shown = ", ".join(str(p) for p in pages[:16])
            more = " ..." if len(pages) > 16 else ""
            print("  %-12s %s%s" % (key, shown, more))


def cmd_check(_args):
    try:
        from pypdf import PdfReader  # noqa: F401
    except ImportError:
        raise SystemExit("pypdf is required: pip install pypdf")
    errors = []
    warnings = []
    ids = set()
    for m in MODELS:
        mid = m["id"]
        if mid in ids:
            errors.append("%s: duplicate model id" % mid)
        ids.add(mid)
        path = os.path.join(DS_DIR, m["pdf"])
        if not os.path.exists(path):
            errors.append("%s: missing datasheet %s" % (mid, m["pdf"]))
            continue
        for key in ("flash", "low_power", "wakeup"):
            if key not in m:
                errors.append("%s: missing key %s" % (mid, key))
        reader = PdfReader(path)
        n = len(reader.pages)
        checks = []
        fpage = m["flash"].get("source_page")
        if isinstance(fpage, int):
            checks.append((fpage, "Flash", ("存储器特性", "闪存", "Flash", "参数")))
        for r in m["low_power"]:
            if isinstance(r.get("page"), int):
                checks.append((r["page"], "low_power", ("电流", "功耗", "电源")))
                break
        for r in m["wakeup"]:
            if isinstance(r.get("page"), int):
                checks.append((r["page"], "wakeup", ("唤醒", "时间参数", "时间")))
                break
        for page, kind, keys in checks:
            if page < 1 or page > n:
                errors.append(
                    "%s: %s page %d out of range 1..%d" % (mid, kind, page, n)
                )
                continue
            text = reader.pages[page - 1].extract_text() or ""
            if not any(k in text for k in keys):
                warnings.append(
                    "%s: %s page %d does not contain any of %s"
                    % (mid, kind, page, "/".join(keys))
                )
        for r in m.get("other", []):
            p = r.get("page")
            if isinstance(p, int) and (p < 1 or p > n):
                errors.append(
                    "%s: other row %r page %d out of range"
                    % (mid, r.get("item", ""), p)
                )
    for w in warnings:
        print("WARN  " + w)
    for e in errors:
        print("ERROR " + e)
    print(
        "%d models, %d error(s), %d warning(s)"
        % (len(MODELS), len(errors), len(warnings))
    )
    return 1 if errors else 0


def cmd_build(args):
    models = selected(args)
    write_json(ELEC_JSON)
    all_rows = []
    for m in models:
        rows = list(model_rows(m, with_model=True))
        all_rows.extend(rows)
        csv_path = os.path.join(HERE, "%s_elec.csv" % m["id"])
        write_csv(csv_path, rows)
        xlsx_path = os.path.join(HERE, "%s_Electrical.xlsx" % m["id"])
        build_model_xlsx(m, xlsx_path)
        print("wrote %s" % os.path.basename(csv_path))
        print("wrote %s" % os.path.basename(xlsx_path))
    if len(models) == len(MODELS):
        write_csv(ALL_CSV, all_rows)
        build_all_xlsx(ALL_XLSX)
        build_readme()
        print("wrote %s" % os.path.basename(ALL_CSV))
        print("wrote %s" % os.path.basename(ALL_XLSX))
        print("wrote %s" % os.path.basename(ELEC_JSON))
        print("wrote %s" % os.path.basename(INDEX))
    else:
        print("partial build: combined files/README not regenerated")


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    sub = parser.add_subparsers(dest="cmd")
    p = sub.add_parser("list", help="list configured models")
    p.set_defaults(func=cmd_list)
    p = sub.add_parser("show", help="print model summary")
    p.add_argument("models", nargs="*")
    p.set_defaults(func=cmd_show)
    p = sub.add_parser("pages", help="dump PDF pages containing a keyword")
    p.add_argument("args", nargs="+")
    p.set_defaults(func=cmd_pages)
    p = sub.add_parser("survey", help="locate candidate electrical pages in datasheets")
    p.add_argument("models", nargs="*")
    p.set_defaults(func=cmd_survey)
    p = sub.add_parser("check", help="validate model ids, pages and keywords")
    p.set_defaults(func=cmd_check)
    p = sub.add_parser("build", help="build CSV/XLSX/JSON/README")
    p.add_argument("models", nargs="*")
    p.set_defaults(func=cmd_build)
    p = sub.add_parser("all", help="same as build (all models)")
    p.add_argument("models", nargs="*")
    p.set_defaults(func=cmd_build)
    ns = parser.parse_args(argv)
    if not ns.cmd:
        parser.print_help()
        return 0
    return ns.func(ns.models if hasattr(ns, "models") else []) or 0


if __name__ == "__main__":
    sys.exit(main())
