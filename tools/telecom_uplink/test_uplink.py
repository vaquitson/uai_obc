"""Check command bytes offline, or capture real datagrams with --udp."""
import functools
import operator
from pathlib import Path
import socket
import struct
import subprocess
import sys
import unittest


PROGRAM = str(Path(__file__).resolve().with_name("telecom_uplink"))
USE_UDP = "--udp" in sys.argv
if USE_UDP:
    sys.argv.remove("--udp")


class UplinkTest(unittest.TestCase):
    def capture(self, *payload_args):
        if not USE_UDP:
            result = subprocess.run(
                [PROGRAM, "--dry-run", "127.0.0.1", "43210", *payload_args],
                capture_output=True, text=True, timeout=5,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            return bytes.fromhex(result.stdout.split("  Hex:\n", 1)[1])
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as receiver:
            receiver.bind(("127.0.0.1", 0))
            receiver.settimeout(2)
            port = receiver.getsockname()[1]
            result = subprocess.run(
                [PROGRAM, "127.0.0.1", str(port), *payload_args],
                capture_output=True, text=True, timeout=5,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            packet, _ = receiver.recvfrom(65535)
            # One invocation must send exactly one command.
            receiver.settimeout(0.1)
            with self.assertRaises(socket.timeout):
                receiver.recvfrom(65535)
        return packet

    def check_packet(self, packet, ip, port="2234"):
        self.assertEqual(len(packet), 40)
        mid, sequence, length, fc, _ = struct.unpack("!HHHBB", packet[:8])
        self.assertEqual((mid, sequence, length, fc), (0x187A, 0xC000, 33, 2))
        self.assertEqual(functools.reduce(operator.xor, packet), 0xFF)
        self.assertEqual(packet[8:24], ip.encode("ascii").ljust(16, b"\0"))
        self.assertEqual(packet[24:40], port.encode("ascii").ljust(16, b"\0"))

    def test_default_payload(self):
        self.check_packet(self.capture(), "127.0.0.1")

    def test_custom_payload(self):
        self.check_packet(self.capture("192.168.1.50"), "192.168.1.50")
        self.check_packet(self.capture("192.168.1.50", "4567"), "192.168.1.50", "4567")

    def test_field_boundaries(self):
        self.check_packet(self.capture("255.255.255.255"), "255.255.255.255")
        self.check_packet(self.capture("1.1.1.1"), "1.1.1.1")
        self.check_packet(self.capture("255.255.255.255", "65535"), "255.255.255.255", "65535")
        self.check_packet(self.capture("10.0.0.1", "00001"), "10.0.0.1", "1")

    def test_invalid_arguments(self):
        cases = [[], ["127.0.0.1"], ["localhost", "1234"], ["::1", "1234"],
                 ["127.0.0.1", "1234", "999.1.1.1"],
                 ["127.0.0.1", "1234", ""],
                 ["127.0.0.1", "1234", "127.0.0.1", "2234", "extra"]]
        for port in ("", "0", "65536", "-1", "+1", "1.5", "12x", " 1234", "9" * 40):
            cases.append(["127.0.0.1", port])
            cases.append(["127.0.0.1", "1234", "127.0.0.1", port])
        for args in cases:
            with self.subTest(args=args):
                result = subprocess.run([PROGRAM, *args], capture_output=True, timeout=5)
                self.assertEqual(result.returncode, 2)

    def test_help(self):
        result = subprocess.run([PROGRAM, "--help"], capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
