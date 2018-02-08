//
//  uart.h
//  Index
//
//  Created by Connor Monahan on 11/17/17.
//  Copyright © 2017 Connor Monahan. All rights reserved.
//

#ifndef uart_h
#define uart_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

extern FILE uart_io;

void uart_init(void);
    
#ifdef __cplusplus
}
#endif

#endif /* uart_h */
