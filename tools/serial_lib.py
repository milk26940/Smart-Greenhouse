import os
import time


def _default_port_provider():
    from serial.tools import list_ports

    return list_ports.comports()


def _default_serial_opener(port, baudrate, timeout=0.1):
    import serial

    return serial.Serial(port=port, baudrate=baudrate, timeout=timeout)


def list_ports(port_provider=None):
    provider = port_provider or _default_port_provider
    ports = []
    for item in provider():
        if isinstance(item, dict):
            ports.append(
                {
                    "device": item.get("device", ""),
                    "description": item.get("description", ""),
                    "hwid": item.get("hwid", ""),
                }
            )
            continue
        ports.append(
            {
                "device": getattr(item, "device", ""),
                "description": getattr(item, "description", ""),
                "hwid": getattr(item, "hwid", ""),
            }
        )
    return ports


def open_serial(port, baudrate, timeout=0.1, serial_opener=None):
    opener = serial_opener or _default_serial_opener
    return opener(port, baudrate, timeout=timeout)


def send_text(serial_handle, message, encoding="utf-8"):
    payload = message.encode(encoding)
    serial_handle.write(payload)


def stream_to_outputs(
    serial_handle,
    stdout,
    log_path=None,
    encoding="utf-8",
    iterations=None,
    deadline_seconds=None,
    time_provider=None,
    sleep_interval=0.05,
):
    log_handle = None
    clock = time_provider or time.monotonic
    deadline = None
    if deadline_seconds is not None:
        deadline = clock() + float(deadline_seconds)
    try:
        if log_path:
            os.makedirs(os.path.dirname(log_path), exist_ok=True)
            log_handle = open(log_path, "a", encoding=encoding)

        count = 0
        while iterations is None or count < iterations:
            if deadline is not None and clock() >= deadline:
                break
            waiting = getattr(serial_handle, "in_waiting", 0)
            chunk = serial_handle.read(waiting or 1)
            if chunk:
                text = chunk.decode(encoding, errors="replace")
                stdout.write(text)
                stdout.flush()
                if log_handle:
                    log_handle.write(text)
                    log_handle.flush()
            else:
                time.sleep(sleep_interval)
            count += 1
    finally:
        if log_handle:
            log_handle.close()
