import io
import os
import tempfile
import unittest
from unittest import mock

from tools import serial_lib


class FakePort:
    def __init__(self, device, description="USB Serial", hwid="HWID"):
        self.device = device
        self.description = description
        self.hwid = hwid


class FakeSerial:
    def __init__(self, chunks=None):
        self.chunks = list(chunks or [])
        self.written = []
        self.closed = False

    @property
    def in_waiting(self):
        if not self.chunks:
            return 0
        return len(self.chunks[0])

    def read(self, size=1):
        if not self.chunks:
            return b""
        return self.chunks.pop(0)

    def write(self, data):
        self.written.append(data)
        return len(data)

    def close(self):
        self.closed = True


class SerialLibTests(unittest.TestCase):
    def test_list_ports_returns_structured_data(self):
        port_provider = mock.Mock(return_value=[FakePort("COM3", "ST-Link", "USB VID:PID=0483:374B")])

        ports = serial_lib.list_ports(port_provider=port_provider)

        self.assertEqual(
            ports,
            [
                {
                    "device": "COM3",
                    "description": "ST-Link",
                    "hwid": "USB VID:PID=0483:374B",
                }
            ],
        )

    def test_send_text_writes_encoded_bytes(self):
        fake_serial = FakeSerial()

        serial_lib.send_text(fake_serial, "AT\r\n", encoding="utf-8")

        self.assertEqual(fake_serial.written, [b"AT\r\n"])

    def test_stream_to_outputs_prints_and_logs(self):
        fake_serial = FakeSerial([b"hello ", b"world"])
        stdout = io.StringIO()
        temp_dir = tempfile.mkdtemp()
        log_path = os.path.join(temp_dir, "uart.txt")

        serial_lib.stream_to_outputs(
            fake_serial,
            stdout=stdout,
            log_path=log_path,
            encoding="utf-8",
            iterations=3,
        )

        self.assertEqual(stdout.getvalue(), "hello world")
        with open(log_path, "r", encoding="utf-8") as handle:
            self.assertEqual(handle.read(), "hello world")

    def test_stream_to_outputs_stops_at_deadline(self):
        fake_serial = FakeSerial([b"boot\n"])
        stdout = io.StringIO()
        clock = mock.Mock(side_effect=[0.0, 0.0, 0.2, 0.2, 1.2])

        serial_lib.stream_to_outputs(
            fake_serial,
            stdout=stdout,
            encoding="utf-8",
            deadline_seconds=1.0,
            time_provider=clock,
            sleep_interval=0,
        )

        self.assertEqual(stdout.getvalue(), "boot\n")


if __name__ == "__main__":
    unittest.main()
