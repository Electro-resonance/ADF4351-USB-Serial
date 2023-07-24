# Programming the ADF4351 STM32F103CB from a RPI

The following instructions allow programming the firmware.elf file to the STMF103CB using an ST-LINK-V2 adapter and a 4-wire link connected to the ADF4351 board with the GND,3.3V, SWDIO and SWCLK lines.

Based on instructions from:

https://mensi.ch/blog/articles/bare-metal-c-programming-the-blue-pill-part-1 

https://mensi.ch/blog/articles/bare-metal-c-programming-the-blue-pill-part-2

```console
mkdir ~/proj
cd ~/proj
git clone http://openocd.zylin.com/openocd
cd openocd/
./bootstrap
./configure --enable-stlink
make -j 4
sudo make install

cd ..
git clone https://github.com/Electro-resonance/ADF4351-USB-Serial
cd ADF4351-USB-Serial/firmware

sudo openocd -f adf4351_openocd.cfg -c "program firmware.elf verify reset exit"
```

# Terminal output during programming
The folloing shows the messages seen whilst programming the STMF103CB:

```console
cd ~/proj/ADF4351-USB-Serial/firmware
sudo openocd -f adf4351_openocd.cfg -c "program firmware.elf verify reset exit"

Open On-Chip Debugger 0.12.0+dev-00274-g7023deb06 (2023-07-24-21:02)
Licensed under GNU GPL v2
For bug reports, read
        http://openocd.org/doc/doxygen/bugs.html
WARNING: interface/stlink-v2.cfg is deprecated, please switch to interface/stlink.cfg
Info : The selected transport took over low-level target control. The results might differ compared to plain JTAG/SWD
Info : clock speed 1000 kHz
Info : STLINK V2J25S4 (API v2) VID:PID 0483:3748
Info : Target voltage: 3.250331
Info : [stm32f1x.cpu] Cortex-M3 r1p1 processor detected
Info : [stm32f1x.cpu] target has 6 breakpoints, 4 watchpoints
Info : starting gdb server for stm32f1x.cpu on 3333
Info : Listening on port 3333 for gdb connections
[stm32f1x.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x08001240 msp: 0x20005000
** Programming Started **
Info : device id = 0x20036410
Info : flash size = 128 KiB
Warn : Adding extra erase range, 0x0800f510 .. 0x0800f7ff
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
shutdown command invoked
```