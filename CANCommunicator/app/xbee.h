//
// Created by Connor Monahan on 2/7/18.
//

#ifndef AVR_CPP_EXAMPLE_XBEE_H
#define AVR_CPP_EXAMPLE_XBEE_H


#include <stdio.h>
#include "ecu.h"
#include "endian.h"


#define XBEE_PACKET_MAGIC 0x93
#define XBEE_PACKET_VER 0x01
struct __attribute__ ((__packed__)) xbee_packet_t {
    // don't change the order of this as i aligned it by hand :)
    uint8_t magic;
    uint8_t version;
    int8_t coolantC;
    uint8_t o2_raw;
    uint16_t gear;
    uint16_t rpm_raw;
    uint16_t load_raw;
    uint16_t throttle_raw;
    uint16_t speed_raw;
    uint16_t volts_raw;
    int32_t lat_deg;
    int32_t long_deg;
    uint16_t rl_sensor;
    uint16_t rr_sensor;
    uint16_t front_brake;
    uint16_t back_brake;
    uint32_t timestamp;
};

void xbee_packet_init(struct xbee_packet_t *packet);
void xbee_packet_copy_from_ecu(struct xbee_packet_t *packet, struct ecu_t *ecu_data);

void send_packet(FILE *xbee, void *packet, size_t len);


#endif //AVR_CPP_EXAMPLE_XBEE_H
