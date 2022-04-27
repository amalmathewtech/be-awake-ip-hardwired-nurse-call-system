cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(main C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project


add_executable(main
    main.c  dhcp.c socket.c w5100s.c wizchip_conf.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(main pico_stdlib hardware_spi hardware_dma)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(main)

# Disable usb output, Enable uart output
pico_enable_stdio_usb(main 0)
pico_enable_stdio_uart(main 1)
