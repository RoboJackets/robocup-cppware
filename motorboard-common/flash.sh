set -e
cube-cmake --build build/Debug --
arm-none-eabi-objcopy -O binary build/Debug/motorboard-common.elf build/Debug/motorboard-common.bin
st-flash --reset write build/Debug/motorboard-common.bin 0x08000000