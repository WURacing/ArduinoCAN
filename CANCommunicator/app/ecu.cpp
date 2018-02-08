//
// Created by Connor Monahan on 2/7/18.
//

#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include "ecu.h"

volatile struct ecu_t ecu_values;
static MCP_CAN *can;

const float RPM_SCALE = .39063;
const float ENG_LOAD_SCALE = .0015259;
const float ENG_THROTTLE_SCALE = .0015259;
const float ANALOG_SCALE = .00007782;
const float O2_SCALE = .00390625;
const float SPEED_SCALE = .00390625;
const float IGN_SCALE = .35156;
const float BATT_VOLTAGE_SCALE = .0002455;

static void convert_ecu_values()
{
    ecu_values.rpm = RPM_SCALE * ecu_values.rpm_raw;
    ecu_values.load = ENG_LOAD_SCALE * ecu_values.load_raw;
    ecu_values.throttle = ENG_THROTTLE_SCALE * ecu_values.throttle_raw;
    ecu_values.o2 = O2_SCALE * ecu_values.o2_raw;
    ecu_values.vehicleSpeed = SPEED_SCALE * ecu_values.speed_raw;
    ecu_values.volts = BATT_VOLTAGE_SCALE * ecu_values.volts_raw;
}

void ecu_init(MCP_CAN *can_bus)
{
    can = can_bus;
    memset((void *) &ecu_values, 0, sizeof(ecu_values));
    convert_ecu_values();
}

void parse_can_message(uint32_t id, uint8_t len, uint8_t buf[])
{
    if (len != 8) {
        return;
    }
    if (id == 0x01F0A000) {
        ecu_values.rpm_raw = buf[0] << 8 | buf[1];
        ecu_values.load_raw = buf[2] << 8 | buf[3];
        ecu_values.throttle_raw = buf[4] << 8 | buf[5];
        // air temp = buf[6];
        ecu_values.coolantC = buf[7];
    } else if (id == 0x01F0A001) {
        // ADC
    } else if (id == 0x01F0A002) {
        // ADC
    } else if (id == 0x01F0A003) {
        ecu_values.o2_raw = buf[0];
        // o2 #2 = buf[1];
        ecu_values.speed_raw = buf[2] << 8 | buf[3];
        ecu_values.gear = buf[4];
        // ign timing = buf[5];
        ecu_values.volts_raw = buf[6] << 8 | buf[7];
    } else {
        return;
    }
    convert_ecu_values();
}

ISR(SPI_STC_vect)
{
    uint32_t id = 0;
    uint8_t len = 0;
    uint8_t buf[8];

    while (CAN_MSGAVAIL == can->checkReceive())
    {
        // read data,  len: data length, buf: data buf
        can->readMsgBuf(&id, &len, buf);
        printf("Got data\n");
        if ((id & 0x80000000) == 0x80000000) {
            printf("Extended ID: 0x%.8lX  DLC: %1d  Data:", (id & 0x1FFFFFFF), len);
        } else {
            printf("Standard ID: 0x%.3lX       DLC: %1d  Data:", static_cast<unsigned long>(id), len);
        }
        if ((id & 0x40000000) == 0x40000000) {    // Determine if message is a remote request frame.
            printf(" REMOTE REQUEST FRAME");
        } else {
            for (uint8_t i = 0; i < len; i++) {
                printf(" 0x%.2X", buf[i]);
            }
        }
        puts("");

    }
}
