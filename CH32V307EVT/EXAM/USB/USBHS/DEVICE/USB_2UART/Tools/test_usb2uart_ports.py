import tempfile
import unittest
from pathlib import Path
from unittest import mock

import usb2uart_ports
from usb2uart_ports import (
    DiscoveryError,
    _discover_linux,
    discover_ports,
    parse_ioreg_tree,
    parse_pnp_records,
    resolve_port,
)


class Usb2UartPortTests(unittest.TestCase):
    def setUp(self):
        self.records: list[dict[str, object]] = [
            {
                "FriendlyName": f"USB Serial Device (COM{20 + index})",
                "InstanceId": (f"USB\\VID_1A86&PID_FE18&MI_{mi}\\6&12345678&0&0000"),
                "PortName": f"COM{20 + index}",
            }
            for index, mi in enumerate(("00", "02", "04", "06", "08", "0A", "0C", "0E"))
        ]

    def test_parse_all_ports(self):
        ports = parse_pnp_records(self.records)
        self.assertEqual([port.channel for port in ports], list("ABCDEFGH"))
        self.assertEqual(ports[0].com, "COM20")
        self.assertEqual(ports[7].uart, "UART8")

    def test_friendly_name_com_fallback(self):
        self.records[0]["PortName"] = None
        ports = parse_pnp_records(self.records)
        self.assertEqual(ports[0].com, "COM20")

    def test_resolve_aliases(self):
        ports = parse_pnp_records(self.records)
        expected = ports[2]
        for selector in ("C", "usb2uart-c", "USART3", "UART3", "MI_04", "COM22"):
            self.assertEqual(resolve_port(selector, ports), expected)

    def test_duplicate_channel_rejected(self):
        with self.assertRaises(DiscoveryError):
            parse_pnp_records([self.records[0], self.records[0]])

    def test_linux_sysfs_discovery(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            tty_root = root / "class" / "tty"
            tty_entry = tty_root / "ttyACM0"
            interface = tty_entry / "device"
            interface.mkdir(parents=True)
            (tty_entry / "idVendor").write_text("1a86\n", encoding="ascii")
            (tty_entry / "idProduct").write_text("fe18\n", encoding="ascii")
            (tty_entry / "serial").write_text("BOARD-1\n", encoding="ascii")
            (tty_entry / "product").write_text("USB_2UART\n", encoding="ascii")
            (interface / "bInterfaceNumber").write_text("02\n", encoding="ascii")

            ports = _discover_linux(tty_root)

        self.assertEqual(len(ports), 1)
        self.assertEqual(ports[0].channel, "B")
        self.assertEqual(ports[0].com, "/dev/ttyACM0")
        self.assertEqual(ports[0].device_key, "BOARD-1")

    def test_parse_macos_ioreg_tree(self):
        payload = [
            {
                "IORegistryEntryName": "USB_2UART",
                "idVendor": 0x1A86,
                "idProduct": 0xFE18,
                "USB Serial Number": "BOARD-1",
                "IORegistryEntryChildren": [
                    {
                        "IORegistryEntryName": "USB2UART-C",
                        "bInterfaceNumber": 4,
                        "IORegistryEntryChildren": [
                            {"IOCalloutDevice": "/dev/cu.usbmodem12303"}
                        ],
                    }
                ],
            }
        ]

        ports = parse_ioreg_tree(payload)

        self.assertEqual(len(ports), 1)
        self.assertEqual(ports[0].channel, "C")
        self.assertEqual(ports[0].com, "/dev/cu.usbmodem12303")
        self.assertEqual(ports[0].device_key, "BOARD-1")

    def test_discover_dispatches_linux_backend(self):
        expected = parse_pnp_records([self.records[0]])
        with (
            mock.patch.object(usb2uart_ports.os, "name", "posix"),
            mock.patch.object(usb2uart_ports.sys, "platform", "linux"),
            mock.patch.object(
                usb2uart_ports, "_discover_linux", return_value=expected
            ) as backend,
        ):
            self.assertEqual(discover_ports(), expected)
            backend.assert_called_once_with()

    def test_unsupported_platform(self):
        with (
            mock.patch.object(usb2uart_ports.os, "name", "posix"),
            mock.patch.object(usb2uart_ports.sys, "platform", "freebsd"),
        ):
            with self.assertRaisesRegex(DiscoveryError, "unsupported operating system"):
                discover_ports()


if __name__ == "__main__":
    unittest.main()
