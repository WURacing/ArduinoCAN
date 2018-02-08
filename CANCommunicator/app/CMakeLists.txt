#####################################################################
# simple AVR executable
#####################################################################
add_avr_executable(app main.cpp ecu.cpp xbee.cpp)

#####################################################################
# libraries to link to
#####################################################################
target_link_libraries(app-${AVR_MCU}.elf uart-${AVR_MCU} spi-${AVR_MCU} mcp2515-${AVR_MCU} gps-${AVR_MCU})
