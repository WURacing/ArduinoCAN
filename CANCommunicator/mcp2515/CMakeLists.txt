#####################################################################
# simple AVR library
#####################################################################
add_avr_library(mcp2515 mcp_can_dfs.h mcp_can.cpp mcp_can.hpp)
avr_target_link_libraries(mcp2515 spi)

#add_dependencies(mcp2515 spi)


