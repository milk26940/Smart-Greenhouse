# Serial CLI Design

## Goal

Provide a local Python serial debugging CLI for this STM32 project so terminal workflows can list ports, monitor logs, send commands, and capture serial output to files.

## Scope

This design adds host-side tooling only. It does not modify STM32 firmware, Keil project files, or board-side serial protocol behavior.

## Commands

- `list`
  - Enumerate available serial ports with device name, description, and hardware ID when available.
- `monitor <port> <baudrate>`
  - Open the serial port and continuously print decoded incoming data to stdout until interrupted.
- `send <port> <baudrate> <message>`
  - Open the port, send the provided text bytes, optionally read a short reply window, print any reply, then exit.
- `log <port> <baudrate> <output_path>`
  - Open the port, continuously print decoded incoming data to stdout, and append the same text to the output file until interrupted.

## Architecture

Two Python files keep responsibilities clear:

- `tools/serial_lib.py`
  - Port listing
  - Serial open/configuration
  - Streaming read loop
  - Send helper
  - Shared decoding/error handling
- `tools/serial_cli.py`
  - Argument parsing
  - Command dispatch
  - Exit codes and user-facing messages

## Data Handling

- Default text encoding is `utf-8`
- `--encoding` can override decoding, including `gbk`
- Incoming bytes are decoded with replacement for invalid byte sequences so monitoring does not crash on partial or noisy output
- `send` writes raw encoded bytes from the provided text argument

## Error Handling

- Invalid arguments are handled by argparse usage output
- Port open failures return a non-zero exit code with a clear stderr message
- Missing directories for `log` output are created automatically
- `KeyboardInterrupt` cleanly closes the port and exits without stack traces

## Testing

Use Python `unittest` and `unittest.mock`:

- Test command parsing and dispatch
- Test log file creation and append behavior
- Test send behavior against a fake serial object
- Test read loop decoding and output forwarding
- Test list formatting with mocked `serial.tools.list_ports`

## Dependencies

- Python 3
- `pyserial`
