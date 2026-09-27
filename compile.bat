@echo off
setlocal

qmk compile -c -kb handwired/replicazeron/stm32f103 -km vial -e REPLICAZERON_STM32_DIRECTINPUT=yes
if errorlevel 1 exit /b %errorlevel%
copy /y handwired_replicazeron_stm32f103_vial.bin handwired_replicazeron_stm32f103_directinput_vial.bin >nul

qmk compile -c -kb handwired/replicazeron/stm32f103 -km vial
if errorlevel 1 exit /b %errorlevel%

qmk compile -c -kb handwired/replicazeron/rp2040 -km vial
exit /b %errorlevel%
