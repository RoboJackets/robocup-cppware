arm-none-eabi-objcopy -O binary build/Debug/motor-boards.elf build/Debug/motor-boards.bin
st-flash write build/Debug/motor-boards.bin 0x08000000