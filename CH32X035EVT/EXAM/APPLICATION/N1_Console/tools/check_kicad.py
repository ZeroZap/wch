#!/usr/bin/env python3
"""Lightweight structural check for the generated KiCad project.

KiCad itself is the final authority; this catches unbalanced S-expressions,
duplicate UUIDs and missing lib_symbols before opening the project.
"""

import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(HERE)
BASE = os.path.join(PROJ, "hardware", "kicad")


def balanced(text):
    depth = 0
    in_str = False
    esc = False
    for ch in text:
        if in_str:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
        else:
            if ch == '"':
                in_str = True
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth < 0:
                    return False, depth
    return depth == 0 and not in_str, depth


def uuids(text):
    out = []
    i = 0
    while True:
        i = text.find('(uuid "', i)
        if i < 0:
            break
        j = text.find('"', i + 7)
        out.append(text[i + 7 : j])
        i = j + 1
    return out


def main():
    with open(os.path.join(BASE, "N1_Console.kicad_pro"), encoding="utf-8") as fh:
        json.load(fh)

    ok = True
    for name in ("N1_Console.kicad_sch", "N1_Console.kicad_sym"):
        path = os.path.join(BASE, name)
        with open(path, encoding="utf-8") as fh:
            text = fh.read()
        good, depth = balanced(text)
        print(
            "%-24s %s (%d bytes, depth=%d)"
            % (name, "balanced" if good else "UNBALANCED", len(text), depth)
        )
        ok = ok and good

    with open(os.path.join(BASE, "N1_Console.kicad_sch"), encoding="utf-8") as fh:
        sch = fh.read()

    u = uuids(sch)
    print("uuids                    %d (%d unique)" % (len(u), len(set(u))))
    ok = ok and len(u) == len(set(u))

    lib_ids = set(re.findall(r'\(lib_id "([^"]+)"', sch))
    missing = sorted(x for x in lib_ids if ('(symbol "%s"' % x) not in sch)
    print("lib_ids                  %d" % len(lib_ids))
    if missing:
        print("missing lib_symbols      %s" % missing)
        ok = False

    print("instances                %d" % sch.count("(symbol (lib_id "))
    print("global labels            %d" % sch.count("(global_label "))
    print("wires                    %d" % sch.count("(wire "))
    print("no_connect               %d" % sch.count("(no_connect "))

    print("OK" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
