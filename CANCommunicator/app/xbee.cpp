//
// Created by Connor Monahan on 2/7/18.
//

#include <string.h>
#include "xbee.h"

void send_packet(FILE *xbee, void *packet, size_t len)
{
    uint16_t *p = static_cast<uint16_t *>(packet);
    for (size_t i = 0; i < len; i++) {
        fputc(*(p+i), xbee);
    }
}

void xbee_packet_init(struct xbee_packet_t *packet)
{
    memset(packet, 0, sizeof(xbee_packet_t));
    packet->magic = XBEE_PACKET_MAGIC;
    packet->version = XBEE_PACKET_VER;
}

void xbee_packet_copy_from_ecu(struct xbee_packet_t *packet, struct ecu_t *ecu_data)
{
    packet->rpm_raw = htons(ecu_data->rpm_raw);
    packet->load_raw = htons(ecu_data->load_raw);
    packet->throttle_raw = htons(ecu_data->throttle_raw);
    packet->coolantC = ecu_data->coolantC;
    packet->o2_raw = ecu_data->o2_raw;
    packet->speed_raw = htons(ecu_data->speed_raw);
    packet->gear = ecu_data->gear;
    packet->volts_raw = htons(ecu_data->volts_raw);
}
