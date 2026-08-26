# Hardware Compatibility & Safety Check Policy

1. **Mandatory Hardware Pre-Check**:
   - Whenever the user mentions, asks about, or requests to connect any new hardware device (Microcontroller, Stepper Driver, Motor, Rotary/Linear Encoder, Sensor, Power Supply, Level Shifter, Module), the agent MUST immediately cross-check electrical and operational compatibility with currently active hardware in the project (recorded in `HARDWARE_DATABASE.md` and `WIRING.md`).

2. **Immediate Alert on Incompatibility ("KHÔNG ĐƯỢC SỬ DỤNG")**:
   - If the device is electrically incompatible (e.g. 5V/24V high voltage into 3.3V GPIO, 3-phase motor into 2-phase driver, missing VMOT capacitor, open-collector without pull-up, overcurrent, non-isolated direct connection), the agent MUST IMMEDIATELY display a prominent warning at the top of the reply:
     ```markdown
     ⛔ CẢNH BÁO: [TÊN THIẾT BỊ] KHÔNG ĐƯỢC SỬ DỤNG TRỰC TIẾP!
     ```
   - Provide the exact technical failure mechanism (e.g., destroyed GPIO, back-EMF MOSFET breakdown, positive feedback oscillation, destroyed bootloader).
   - Specify the required protection circuitry (Level Shifter, Optocoupler, Zener diode, Voltage Divider, Flyback Diode, decoupling capacitor) or provide a 100% safe compatible alternative.
