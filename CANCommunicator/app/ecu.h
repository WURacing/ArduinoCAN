//
// Created by Connor Monahan on 2/7/18.
//

#ifndef AVR_CPP_EXAMPLE_ECU_H
#define AVR_CPP_EXAMPLE_ECU_H

#include <mcp_can.hpp>


struct ecu_t {
    uint16_t rpm_raw;
    float rpm;
    uint16_t load_raw;
    float load;
    uint16_t throttle_raw;
    float throttle;
    int8_t coolantC;
    float coolantF;
    uint8_t o2_raw;
    float o2;
    uint16_t speed_raw;
    float vehicleSpeed;
    uint8_t gear;
    uint16_t volts_raw;
    float volts;
};

extern volatile struct ecu_t ecu_values;

void ecu_init(MCP_CAN *can_bus);

void parse_can_message(uint32_t id, uint8_t len, uint8_t buf[]);

#endif //AVR_CPP_EXAMPLE_ECU_H
