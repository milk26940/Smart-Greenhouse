# Serial CLI

## Install

```powershell
pip install pyserial
```

## Usage

```powershell
python tools/serial_cli.py list
python tools/serial_cli.py monitor COM3 115200
python tools/serial_cli.py send COM3 115200 "AT`r`n"
python tools/serial_cli.py log COM3 115200 logs/uart.txt
python tools/serial_cli.py capture COM10 115200 5 logs/uboot.txt
python tools/serial_cli.py --encoding gbk monitor COM3 115200
```

## Notes

- `monitor` prints serial output until interrupted
- `log` prints serial output and appends the same content to the target file
- `capture` prints serial output, appends to the target file, and exits after the requested number of seconds
- `send` writes the provided text bytes and exits
