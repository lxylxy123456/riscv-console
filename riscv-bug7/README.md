## `riscv-bug7`
* Firmware
* Timer interrupts are disabled (by setting time compare to 0xffffffffffffffff)
* Only catching CMD interrupt and cartridge interrupt
* When interrupt received, prints interrupt information on the screen
	* Column 30 - 37 is mcause
	* Column 40 - 47 is interrupt pending & (~video interrupt),
	  if mcause = machine external, 
	* Column 50 - 57 is cartridge status, if video is pending
* Similar to bug6, press `u` to jump to 0x20000000

## `riscv-bug7c`
* Simply a cartridge that changes the first byte of video interrupt

## Experiments
* Test interrupt catching
	1. Load Firmware = `riscv-bug7`, PWR
	2. Press CMD, screen shows `8000000b  00000004`
	3. Press REM, screen shows `8000000b  00000001  00000000`
	4. Insert `riscv-bug7c`, screen shows `8000000b  00000001  20000001`
* Test cartridge content
	1. (Continue from previous experiment)
	1. Press `u`, the first character on screen ('H' of 'Hello World') will be
		constantly changing
	1. At this time, pressing CMD should still show `8000000b  00000004`
* Test remove cartridge
	1. (Continue from previous experiment)
	1. Press REM, the first expected line is `8000000b  00000001  00000000`,
		but actually it is `00000002`
