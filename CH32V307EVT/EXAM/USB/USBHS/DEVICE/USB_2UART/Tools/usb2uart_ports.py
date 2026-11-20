#!/usr/bin/env python3
"""Discover and monitor the eight USB_2UART CDC ports.

The mapping uses the stable USB interface number, not the assigned serial-port
name or enumeration order. Windows, Linux, and macOS are supported without
third-party Python dependencies.
"""

from __future__ import annotations

import argparse
import json
import os
import plistlib
import re
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Callable, Iterable, Iterator, Mapping, Sequence

USB_VID = "1A86"
USB_PID = "FE18"
EXPECTED_PORT_COUNT = 8

_MI_MAP: dict[str, tuple[str, str, int]] = {
    "00": ("A", "USART1", 1),
    "02": ("B", "USART2", 2),
    "04": ("C", "USART3", 3),
    "06": ("D", "UART4", 4),
    "08": ("E", "UART5", 5),
    "0A": ("F", "UART6", 6),
    "0C": ("G", "UART7", 7),
    "0E": ("H", "UART8", 8),
}

_INSTANCE_RE = re.compile(
    rf"^USB\\VID_{USB_VID}&PID_{USB_PID}&MI_([0-9A-F]{{2}})\\(.+)$",
    re.IGNORECASE,
)
_COM_RE = re.compile(r"\((COM\d+)\)\s*$", re.IGNORECASE)

_POWERSHELL_QUERY = rf"""
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [Console]::OutputEncoding
$ErrorActionPreference = 'Stop'
$devices = Get-CimInstance Win32_SerialPort |
    Where-Object {{ $_.PNPDeviceID -match '^USB\\VID_{USB_VID}&PID_{USB_PID}&MI_' }} |
    ForEach-Object {{
        [PSCustomObject]@{{
            FriendlyName = $_.Name
            InstanceId = $_.PNPDeviceID
            PortName = $_.DeviceID
        }}
    }}
@($devices) | ConvertTo-Json -Compress
"""


class DiscoveryError(RuntimeError):
    """Raised when serial-port discovery cannot be completed."""


@dataclass(frozen=True, order=True)
class Usb2UartPort:
    """One logical USB_2UART port."""

    index: int
    channel: str
    uart: str
    com: str
    mi: str
    instance_id: str
    friendly_name: str
    device_key: str

    @property
    def name(self) -> str:
        return f"USB2UART-{self.channel}"

    def to_dict(self) -> dict[str, str | int]:
        data = asdict(self)
        data["name"] = self.name
        return data


def _normalize_records(payload: object) -> list[dict[str, object]]:
    if payload is None:
        return []
    if isinstance(payload, dict):
        return [payload]
    if isinstance(payload, list) and all(isinstance(item, dict) for item in payload):
        return payload
    raise DiscoveryError("PowerShell returned an unexpected JSON structure")


def parse_pnp_records(records: Iterable[Mapping[str, object]]) -> list[Usb2UartPort]:
    """Convert Windows PnP records into stable A-H port mappings."""

    ports: list[Usb2UartPort] = []
    seen_channels: set[str] = set()

    for record in records:
        instance_id = str(record.get("InstanceId") or "")
        match = _INSTANCE_RE.match(instance_id)
        if not match:
            continue

        mi = match.group(1).upper()
        mapping = _MI_MAP.get(mi)
        if mapping is None:
            continue

        channel, uart, index = mapping
        friendly_name = str(record.get("FriendlyName") or "")
        port_name = str(record.get("PortName") or "").upper()
        if not re.fullmatch(r"COM\d+", port_name):
            com_match = _COM_RE.search(friendly_name)
            port_name = com_match.group(1).upper() if com_match else ""
        if not port_name:
            continue

        if channel in seen_channels:
            raise DiscoveryError(
                f"found duplicate channel {channel}; multiple identical boards may be connected"
            )
        seen_channels.add(channel)
        ports.append(
            Usb2UartPort(
                index=index,
                channel=channel,
                uart=uart,
                com=port_name,
                mi=mi,
                instance_id=instance_id,
                friendly_name=friendly_name,
                device_key=match.group(2),
            )
        )

    return sorted(ports)


def _discover_windows(timeout: float) -> list[Usb2UartPort]:
    command = [
        "powershell.exe",
        "-NoLogo",
        "-NoProfile",
        "-NonInteractive",
        "-ExecutionPolicy",
        "Bypass",
        "-Command",
        _POWERSHELL_QUERY,
    ]
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            encoding="utf-8-sig",
            errors="replace",
            timeout=timeout,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise DiscoveryError(f"failed to run PowerShell: {exc}") from exc

    if result.returncode != 0:
        message = result.stderr.strip() or result.stdout.strip() or "unknown error"
        raise DiscoveryError(f"PowerShell PnP query failed: {message}")

    output = result.stdout.strip()
    if not output:
        return []
    try:
        records = _normalize_records(json.loads(output))
    except json.JSONDecodeError as exc:
        raise DiscoveryError(f"PowerShell returned invalid JSON: {exc}") from exc
    return parse_pnp_records(records)


def _read_text(path: Path) -> str | None:
    try:
        return path.read_text(encoding="ascii").strip()
    except (OSError, UnicodeError):
        return None


def _discover_linux(sys_class_tty: Path = Path("/sys/class/tty")) -> list[Usb2UartPort]:
    """Discover CDC ACM ports from Linux sysfs."""

    try:
        entries = list(sys_class_tty.iterdir())
    except OSError as exc:
        raise DiscoveryError(f"failed to read Linux sysfs: {exc}") from exc

    ports: list[Usb2UartPort] = []
    seen_channels: set[str] = set()
    for entry in entries:
        if not entry.name.startswith("ttyACM"):
            continue
        try:
            device_path = (entry / "device").resolve(strict=True)
        except OSError:
            continue

        interface_path: Path | None = None
        usb_device_path: Path | None = None
        for candidate in (device_path, *device_path.parents):
            if interface_path is None and (candidate / "bInterfaceNumber").is_file():
                interface_path = candidate
            if (
                _read_text(candidate / "idVendor") or ""
            ).lower() == USB_VID.lower() and (
                _read_text(candidate / "idProduct") or ""
            ).lower() == USB_PID.lower():
                usb_device_path = candidate
                break
        if interface_path is None or usb_device_path is None:
            continue

        mi = (_read_text(interface_path / "bInterfaceNumber") or "").upper()
        mapping = _MI_MAP.get(mi)
        if mapping is None:
            continue
        channel, uart, index = mapping
        if channel in seen_channels:
            raise DiscoveryError(
                f"found duplicate channel {channel}; multiple identical boards may be connected"
            )
        seen_channels.add(channel)
        serial = _read_text(usb_device_path / "serial") or usb_device_path.name
        product = _read_text(usb_device_path / "product") or "USB_2UART"
        ports.append(
            Usb2UartPort(
                index=index,
                channel=channel,
                uart=uart,
                com=f"/dev/{entry.name}",
                mi=mi,
                instance_id=str(interface_path),
                friendly_name=f"{product} ({entry.name})",
                device_key=serial,
            )
        )
    return sorted(ports)


def _iter_ioreg_nodes(value: object) -> Iterator[dict[str, object]]:
    if isinstance(value, dict):
        yield value
        for child in value.get("IORegistryEntryChildren", []):
            yield from _iter_ioreg_nodes(child)
    elif isinstance(value, list):
        for item in value:
            yield from _iter_ioreg_nodes(item)


def _as_int(value: object) -> int | None:
    if isinstance(value, int):
        return value
    if isinstance(value, bytes):
        return int.from_bytes(value, byteorder="little")
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError:
            return None
    return None


def parse_ioreg_tree(payload: object) -> list[Usb2UartPort]:
    """Convert a macOS IORegistry USB tree into stable A-H mappings."""

    ports: list[Usb2UartPort] = []
    seen_channels: set[str] = set()
    for device in _iter_ioreg_nodes(payload):
        if _as_int(device.get("idVendor")) != int(USB_VID, 16) or _as_int(
            device.get("idProduct")
        ) != int(USB_PID, 16):
            continue

        device_key = str(
            device.get("USB Serial Number")
            or device.get("locationID")
            or device.get("IORegistryEntryName")
            or "unknown"
        )
        product = str(device.get("USB Product Name") or "USB_2UART")
        for interface in _iter_ioreg_nodes(device.get("IORegistryEntryChildren", [])):
            interface_number = _as_int(interface.get("bInterfaceNumber"))
            if interface_number is None:
                continue
            mi = f"{interface_number:02X}"
            mapping = _MI_MAP.get(mi)
            if mapping is None:
                continue
            callout = next(
                (
                    str(node["IOCalloutDevice"])
                    for node in _iter_ioreg_nodes(interface)
                    if node.get("IOCalloutDevice")
                ),
                "",
            )
            if not callout:
                continue
            channel, uart, index = mapping
            if channel in seen_channels:
                raise DiscoveryError(
                    f"found duplicate channel {channel}; multiple identical boards may be connected"
                )
            seen_channels.add(channel)
            ports.append(
                Usb2UartPort(
                    index=index,
                    channel=channel,
                    uart=uart,
                    com=callout,
                    mi=mi,
                    instance_id=str(
                        interface.get("IORegistryEntryName") or f"interface-{mi}"
                    ),
                    friendly_name=f"{product} ({Path(callout).name})",
                    device_key=device_key,
                )
            )
    return sorted(ports)


def _discover_macos(timeout: float) -> list[Usb2UartPort]:
    command = ["ioreg", "-a", "-p", "IOUSB", "-w", "0"]
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            timeout=timeout,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise DiscoveryError(f"failed to run ioreg: {exc}") from exc
    if result.returncode != 0:
        message = result.stderr.decode(errors="replace").strip() or "unknown error"
        raise DiscoveryError(f"ioreg query failed: {message}")
    try:
        payload = plistlib.loads(result.stdout)
    except (plistlib.InvalidFileException, ValueError) as exc:
        raise DiscoveryError(
            f"ioreg returned invalid property-list data: {exc}"
        ) from exc
    return parse_ioreg_tree(payload)


def discover_ports(*, timeout: float = 20.0) -> list[Usb2UartPort]:
    """Query connected USB_2UART ports using the current OS backend."""

    if os.name == "nt":
        return _discover_windows(timeout)
    if sys.platform.startswith("linux"):
        return _discover_linux()
    if sys.platform == "darwin":
        return _discover_macos(timeout)
    raise DiscoveryError(f"unsupported operating system: {sys.platform}")


def resolve_port(
    selector: str, ports: Sequence[Usb2UartPort] | None = None
) -> Usb2UartPort:
    """Resolve A-H, USB2UART-A, UART1-8, MI_xx, or COMx to one port."""

    available = list(ports) if ports is not None else discover_ports()
    token = selector.strip().upper().replace("_", "-")

    for port in available:
        aliases = {
            port.channel,
            port.name,
            port.uart,
            f"UART{port.index}",
            f"MI-{port.mi}",
            port.com.upper(),
        }
        if token in aliases:
            return port
    raise DiscoveryError(f"port selector {selector!r} is not connected")


def wait_for_ports(
    count: int = EXPECTED_PORT_COUNT,
    *,
    timeout: float | None = None,
    interval: float = 0.5,
    discover: Callable[[], list[Usb2UartPort]] = discover_ports,
) -> list[Usb2UartPort]:
    """Wait until at least ``count`` mapped ports are present."""

    deadline = None if timeout is None else time.monotonic() + timeout
    while True:
        ports = discover()
        if len(ports) >= count:
            return ports
        if deadline is not None and time.monotonic() >= deadline:
            raise DiscoveryError(
                f"timed out waiting for {count} ports; currently found {len(ports)}"
            )
        time.sleep(interval)


def watch_ports(
    *,
    interval: float = 1.0,
    discover: Callable[[], list[Usb2UartPort]] = discover_ports,
) -> Iterator[list[Usb2UartPort]]:
    """Yield the full port list initially and whenever it changes."""

    previous: tuple[tuple[str, str, str], ...] | None = None
    while True:
        ports = discover()
        signature = tuple((port.channel, port.com, port.instance_id) for port in ports)
        if signature != previous:
            previous = signature
            yield ports
        time.sleep(interval)


def _print_table(ports: Sequence[Usb2UartPort]) -> None:
    print(f"USB_2UART: {len(ports)}/{EXPECTED_PORT_COUNT} ports connected")
    print("NAME        PORT                  UART     USB IF  STATUS")
    print("----------  --------------------  -------  ------  ---------")
    by_channel = {port.channel: port for port in ports}
    for mi, (channel, uart, _) in _MI_MAP.items():
        port = by_channel.get(channel)
        if port is None:
            print(f"USB2UART-{channel:<1}  {'-':<20}  {uart:<7}  MI_{mi}   missing")
        else:
            print(
                f"{port.name:<10}  {port.com:<20}  {port.uart:<7}  MI_{port.mi}   connected"
            )


def _print_json(ports: Sequence[Usb2UartPort]) -> None:
    print(json.dumps([port.to_dict() for port in ports], ensure_ascii=False, indent=2))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Discover USB_2UART A-H ports by USB interface number."
    )
    parser.add_argument(
        "--json", action="store_true", help="print machine-readable JSON"
    )
    parser.add_argument("--watch", action="store_true", help="monitor hot-plug changes")
    parser.add_argument(
        "--interval", type=float, default=1.0, help="poll interval in seconds"
    )
    parser.add_argument(
        "--wait",
        nargs="?",
        type=int,
        const=EXPECTED_PORT_COUNT,
        metavar="COUNT",
        help="wait for COUNT ports (default: 8)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=None,
        help="wait timeout in seconds; default is unlimited",
    )
    parser.add_argument(
        "--resolve",
        metavar="PORT",
        help="print the serial port for A-H, UART1-8, MI_xx, USB2UART-X, or OS path",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.interval <= 0:
        print("error: --interval must be greater than zero", file=sys.stderr)
        return 2

    try:
        if args.watch:
            for ports in watch_ports(interval=args.interval):
                if args.json:
                    _print_json(ports)
                else:
                    print(time.strftime("\n[%Y-%m-%d %H:%M:%S]"))
                    _print_table(ports)
            return 0

        ports = (
            wait_for_ports(args.wait, timeout=args.timeout, interval=args.interval)
            if args.wait is not None
            else discover_ports()
        )
        if args.resolve:
            port = resolve_port(args.resolve, ports)
            if args.json:
                print(json.dumps(port.to_dict(), ensure_ascii=False, indent=2))
            else:
                print(port.com)
        elif args.json:
            _print_json(ports)
        else:
            _print_table(ports)
        return 0 if len(ports) == EXPECTED_PORT_COUNT else 1
    except DiscoveryError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
