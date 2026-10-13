# Experimental serial bridge

Requires `pip install pyserial`. `BridgeClient` supports STM32 USB CDC on the normal firmware serial port.

```python
from client import BridgeClient
import time

with BridgeClient('COM5') as bridge:
    bridge.set_mode('INPUT')
    bridge.input('RELOAD', True)
    time.sleep(0.1)
    bridge.input('RELOAD', False)
```

**Implemented:** remote logical input mode and direct SD-backed AUDIO/LIGHT/STOP commands, 1.5 s heartbeat timeout, source-switch input reset.

**Not implemented yet:** SD upload/deployment, LED frame streaming, editor GUI integration, binary framing/CRC/ACK sequencing, telemetry, and PC-side FSM execution. This is a partial prototype, not the complete three-mode bridge. Serial commands are ASCII lines; diagnostic log messages coexist on the same port. No STM32 build or hardware validation was performed.
