//
// Created by Connor Monahan on 2/1/18.
//

#include <avr/io.h>
#include <uart.h>
#include <softuart.h>
#include <mcp_can.hpp>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>
#include "ecu.h"
#include "xbee.h"
#include "../gps/Adafruit_GPS.h"

void * operator new(size_t size)
{
    return malloc(size);
}

void operator delete(void * ptr)
{
    free(ptr);
}


MCP_CAN *can;
Adafruit_GPS *gps;
uint32_t rxId;
uint8_t len = 0;
uint8_t rxBuf[8];

volatile int adc0, adc1, adc2, adc5;

void readGPSint(int)
{
    gps->read();
}

void initIO()
{
    // enable serial console debugging
    uart_init();
#ifdef DEBUG_EN
    stdout = stderr = stdin = &uart_io;
    printf("UART online\n");
#endif

    // bring can controller online
    can = new MCP_CAN(0);
    if (can->begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        printf("Successfully initialized CAN.\n");
        can->setMode(MCP_NORMAL);
    } else {
        fprintf(stderr, "Failed to initialize CAN!\n");
        can = nullptr;
    }
    ecu_init(can);

    // bring gps input online
    softuart_init();
    sei();
    gps = new Adafruit_GPS(&softuart_io);
    softuart_add_callback(&readGPSint);

    // bring ADC online
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // prescaler value of 128 (125KHz)
    ADMUX |= _BV(REFS0); // 5V ref
    ADCSRA |= _BV(ADEN) | _BV(ADIE) | _BV(ADSC); // enable, interrupt, start conv
}

void loop()
{
    _delay_us(200);
    xbee_packet_t packet;
    xbee_packet_init(&packet);
    xbee_packet_copy_from_ecu(&packet, const_cast<ecu_t *>(&ecu_values));
    packet.rl_sensor = htons(adc0);
    packet.rr_sensor = htons(adc1);
    packet.back_brake = htons(adc2);
    packet.front_brake = htons(adc5);

    if (gps->newNMEAreceived() && gps->parse(gps->lastNMEA())) {
        packet.lat_deg = gps->latitude_fixed;
        packet.long_deg = gps->longitude_fixed;
    }
    packet.timestamp = htonll(gps->day * 86400000 + gps->hour * 3600000 + gps->minute * 60000 + gps->seconds * 1000 + gps->milliseconds);

#ifdef DEBUG_EN
    printf("Current time is %d:%d:%d.\n", gps->hour, gps->minute, gps->seconds);
    printf("Car goes %d fast.\n", (int)ecu_values.rpm);
    printf("C0: %d C1: %d C2: %d C5: %d\n", adc0, adc1, adc2, adc5);
#else
    send_packet(&uart_io, &packet, sizeof(xbee_packet_t));
#endif

    while (can && can->checkReceive() == CAN_MSGAVAIL) {
        can->readMsgBuf(&rxId, &len, rxBuf);
#ifdef DEBUG_EN
        if ((rxId & 0x80000000) == 0x80000000) {
            printf("Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
        } else {
            printf("Standard ID: 0x%.3lX       DLC: %1d  Data:", static_cast<unsigned long>(rxId), len);
        }
        if ((rxId & 0x40000000) == 0x40000000) {    // Determine if message is a remote request frame.
            printf(" REMOTE REQUEST FRAME");
        } else {
            for (uint8_t i = 0; i < len; i++) {
                printf(" 0x%.2X", rxBuf[i]);
            }
        }
        puts("");
#endif
        parse_can_message(rxId, len, rxBuf);
    }
}

int main()
{
    initIO();
    while (1) {
        loop();
    }
}

ISR(ADC_vect)
{
    int val, adc;
    val = ADC;
    adc = ADMUX & 0x0f;
    ADMUX	&=	0xf0;
    switch (adc) {
        case 0:
            ADMUX |= 0x01;
            adc0 = val;
            break;
        case 1:
            ADMUX |= 0x02;
            adc1 = val;
            break;
        case 2:
            ADMUX |= 0x05;
            adc2 = val;
            break;
        case 5:
            adc5 = val;
        default:
            break;
    }
    ADCSRA |= _BV(ADSC);
}
