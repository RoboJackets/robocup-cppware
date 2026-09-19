arm-none-eabi-objcopy -O binary build/Debug/motor-board.elf build/Debug/motor-board.bin
st-flash write build/Debug/motor-board.bin 0x08000000