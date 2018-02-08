#ifndef SOFTUART_H
#define SOFTUART_H

#include <stdbool.h>
#include <stdio.h>

#if !defined(F_CPU)
    #warning "F_CPU not defined in makefile - now defined in softuart.h"
    #define F_CPU 3686400UL
#endif

#define SOFTUART_BAUD_RATE      9600

#if defined (__AVR_ATtiny25__) || defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
    #define SOFTUART_RXPIN   PINB
    #define SOFTUART_RXDDR   DDRB
    #define SOFTUART_RXBIT   PB0

    #define SOFTUART_TXPORT  PORTB
    #define SOFTUART_TXDDR   DDRB
    #define SOFTUART_TXBIT   PB1

    #define SOFTUART_T_COMP_LABEL      TIM0_COMPA_vect
    #define SOFTUART_T_COMP_REG        OCR0A
    #define SOFTUART_T_CONTR_REGA      TCCR0A
    #define SOFTUART_T_CONTR_REGB      TCCR0B
    #define SOFTUART_T_CNT_REG         TCNT0
    #define SOFTUART_T_INTCTL_REG      TIMSK

    #define SOFTUART_CMPINT_EN_MASK    (1 << OCIE0A)

    #define SOFTUART_CTC_MASKA         (1 << WGM01)
    #define SOFTUART_CTC_MASKB         (0)

    /* "A timer interrupt must be set to interrupt at three times 
       the required baud rate." */
    #define SOFTUART_PRESCALE (8)
    // #define SOFTUART_PRESCALE (1)

    #if (SOFTUART_PRESCALE == 8)
        #define SOFTUART_PRESC_MASKA         (0)
        #define SOFTUART_PRESC_MASKB         (1 << CS01)
    #elif (SOFTUART_PRESCALE==1)
        #define SOFTUART_PRESC_MASKA         (0)
        #define SOFTUART_PRESC_MASKB         (1 << CS00)
    #else 
        #error "prescale unsupported"
    #endif
#elif defined (__AVR_ATmega324P__) || defined (__AVR_ATmega324A__)  \
   || defined (__AVR_ATmega644P__) || defined (__AVR_ATmega644PA__) \
   || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328PA__) \
   || defined (__AVR_ATmega164P__) || defined (__AVR_ATmega164A__)

    #define SOFTUART_RXPIN   GPS_RX_PIN
    #define SOFTUART_RXDDR   GPS_RX_DDR
    #define SOFTUART_RXBIT   GPS_RX_BIT

    #define SOFTUART_TXPORT  GPS_TX_PORT
    #define SOFTUART_TXDDR   GPS_TX_DDR
    #define SOFTUART_TXBIT   GPS_TX_BIT

    #define SOFTUART_T_COMP_LABEL      TIMER0_COMPA_vect
    #define SOFTUART_T_COMP_REG        OCR0A
    #define SOFTUART_T_CONTR_REGA      TCCR0A
    #define SOFTUART_T_CONTR_REGB      TCCR0B
    #define SOFTUART_T_CNT_REG         TCNT0
    #define SOFTUART_T_INTCTL_REG      TIMSK0
    #define SOFTUART_CMPINT_EN_MASK    (1 << OCIE0A)
    #define SOFTUART_CTC_MASKA         (1 << WGM01)
    #define SOFTUART_CTC_MASKB         (0)

    /* "A timer interrupt must be set to interrupt at three times 
       the required baud rate." */
    #define SOFTUART_PRESCALE (8)
    // #define SOFTUART_PRESCALE (1)

    #if (SOFTUART_PRESCALE == 8)
        #define SOFTUART_PRESC_MASKA         (0)
        #define SOFTUART_PRESC_MASKB         (1 << CS01)
    #elif (SOFTUART_PRESCALE==1)
        #define SOFTUART_PRESC_MASKA         (0)
        #define SOFTUART_PRESC_MASKB         (1 << CS00)
    #else 
        #error "prescale unsupported"
    #endif
#else
    #error "no defintions available for this AVR"
#endif

#define SOFTUART_TIMERTOP ( F_CPU/SOFTUART_PRESCALE/SOFTUART_BAUD_RATE/3 - 1)

#if (SOFTUART_TIMERTOP > 0xff)
    #warning "Check SOFTUART_TIMERTOP: increase prescaler, lower F_CPU or use a 16 bit timer"
#endif

#define SOFTUART_IN_BUF_SIZE     32

#ifdef __cplusplus
extern "C" {
#endif

extern FILE softuart_io;

// Init the Software Uart
void softuart_init(void);

typedef void (*__su_callback)(int);

void softuart_add_callback(__su_callback cb);

// Clears the contents of the input buffer.
void softuart_flush_input_buffer( void );

// Tests whether an input character has been received.
bool softuart_kbhit(void );

// Reads a character from the input buffer, waiting if necessary.
int softuart_getchar(FILE* stream);

// To check if transmitter is busy
bool softuart_transmit_busy(void );

// Writes a character to the serial port.
int softuart_putchar(char ch, FILE* stream);

// Turns on the receive function.
void softuart_turn_rx_on( void );

// Turns off the receive function.
void softuart_turn_rx_off( void );

// Write a NULL-terminated string from RAM to the serial port
void softuart_puts( const char *s );

// Write a NULL-terminated string from program-space (flash) 
// to the serial port. example: softuart_puts_p(PSTR("test"))
void softuart_puts_p( const char *prg_s );
    
#ifdef __cplusplus
}
#endif


// Helper-Macro - "automatically" inserts PSTR
// when used: include avr/pgmspace.h before this include-file
#define softuart_puts_P(s___) softuart_puts_p(PSTR(s___))

#endif

