import argparse
import os
import sys

if __package__ in (None, ""):
    sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
    from tools import serial_lib
else:
    from tools import serial_lib


def build_parser():
    parser = argparse.ArgumentParser(description="Local serial debugging CLI")
    parser.add_argument("--encoding", default="utf-8", help="Text encoding for send/monitor/log")

    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("list", help="List serial ports")

    monitor_parser = subparsers.add_parser("monitor", help="Monitor serial output")
    monitor_parser.add_argument("port")
    monitor_parser.add_argument("baudrate", type=int)

    send_parser = subparsers.add_parser("send", help="Send text to serial port")
    send_parser.add_argument("port")
    send_parser.add_argument("baudrate", type=int)
    send_parser.add_argument("message")

    log_parser = subparsers.add_parser("log", help="Log serial output to file")
    log_parser.add_argument("port")
    log_parser.add_argument("baudrate", type=int)
    log_parser.add_argument("output_path")

    capture_parser = subparsers.add_parser("capture", help="Capture serial output for a fixed duration")
    capture_parser.add_argument("port")
    capture_parser.add_argument("baudrate", type=int)
    capture_parser.add_argument("seconds", type=float)
    capture_parser.add_argument("output_path")

    return parser


def main(
    argv=None,
    stdout=None,
    stderr=None,
    port_provider=None,
    serial_opener=None,
    stream_runner=None,
):
    stdout = stdout or sys.stdout
    stderr = stderr or sys.stderr
    runner = stream_runner or serial_lib.stream_to_outputs

    try:
        args = build_parser().parse_args(argv)
        if args.command == "list":
            for item in serial_lib.list_ports(port_provider=port_provider):
                stdout.write(f'{item["device"]}\t{item["description"]}\t{item["hwid"]}\n')
            return 0

        handle = serial_lib.open_serial(
            args.port,
            args.baudrate,
            serial_opener=serial_opener,
        )
        try:
            if args.command == "send":
                serial_lib.send_text(handle, args.message, encoding=args.encoding)
            elif args.command == "monitor":
                runner(handle, stdout=stdout, encoding=args.encoding)
            elif args.command == "log":
                runner(handle, stdout=stdout, log_path=args.output_path, encoding=args.encoding)
            elif args.command == "capture":
                runner(
                    handle,
                    stdout=stdout,
                    log_path=args.output_path,
                    encoding=args.encoding,
                    deadline_seconds=args.seconds,
                )
            return 0
        finally:
            handle.close()
    except KeyboardInterrupt:
        stderr.write("Interrupted.\n")
        return 130
    except Exception as exc:
        stderr.write(f"Error: {exc}\n")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
