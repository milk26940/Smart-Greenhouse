import io
import tempfile
import unittest
from unittest import mock

from tools import serial_cli


class SerialCliTests(unittest.TestCase):
    def test_list_command_prints_available_ports(self):
        stdout = io.StringIO()

        exit_code = serial_cli.main(
            ["list"],
            stdout=stdout,
            port_provider=lambda: [{"device": "COM3", "description": "ST-Link", "hwid": "USB"}],
        )

        self.assertEqual(exit_code, 0)
        self.assertIn("COM3", stdout.getvalue())

    def test_send_command_opens_port_and_sends_message(self):
        opener = mock.Mock()
        fake_serial = mock.Mock()
        opener.return_value = fake_serial

        exit_code = serial_cli.main(
            ["send", "COM3", "115200", "AT\r\n"],
            serial_opener=opener,
        )

        self.assertEqual(exit_code, 0)
        opener.assert_called_once()
        fake_serial.write.assert_called_once_with(b"AT\r\n")
        fake_serial.close.assert_called_once()

    def test_log_command_streams_to_file(self):
        opener = mock.Mock()
        fake_serial = mock.Mock()
        opener.return_value = fake_serial
        runner = mock.Mock()
        temp_dir = tempfile.mkdtemp()
        log_path = f"{temp_dir}/uart.txt"

        exit_code = serial_cli.main(
            ["log", "COM3", "115200", log_path],
            serial_opener=opener,
            stream_runner=runner,
        )

        self.assertEqual(exit_code, 0)
        runner.assert_called_once()
        _, kwargs = runner.call_args
        self.assertEqual(kwargs["log_path"], log_path)
        fake_serial.close.assert_called_once()

    def test_capture_command_passes_deadline_and_log_path(self):
        opener = mock.Mock()
        fake_serial = mock.Mock()
        opener.return_value = fake_serial
        runner = mock.Mock()
        temp_dir = tempfile.mkdtemp()
        log_path = f"{temp_dir}/uboot.txt"

        exit_code = serial_cli.main(
            ["capture", "COM10", "115200", "5", log_path],
            serial_opener=opener,
            stream_runner=runner,
        )

        self.assertEqual(exit_code, 0)
        runner.assert_called_once()
        _, kwargs = runner.call_args
        self.assertEqual(kwargs["deadline_seconds"], 5.0)
        self.assertEqual(kwargs["log_path"], log_path)
        fake_serial.close.assert_called_once()


if __name__ == "__main__":
    unittest.main()
