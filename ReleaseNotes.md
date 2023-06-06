## SiS Uart Update Pen Firmware Tool

### Release Notes

#### V0.2.0

2023-4-19
-First Version

#### V0.3.0

2023-6-7
-Modify the Special Update Flag to "7501".
-Fix update flow when used -ba, -b arguments.
-Support only update parameters (-p argument).
-Check PKGID, and bypass PKGID (if set -jcp argument).
-Reserve Calibration settings. (-rcal)
-Update Firmware Info of 0xa00040a0 and 0xa001e000
-Display Update Firmware Historyof 0xa00040a0
