/*
 * Pin definitions for manager used with pin_num.h Digital IO library
  Copyright (C) 2019 Ronald Sutherland
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef Pins_Board_h
#define Pins_Board_h

// ADC channels 0..7
#define ALT_I  0
#define ALT_V 1
#define TX_nRE 2
#define RX_DE 3
#define SDA0 4
#define SCL0 5
#define PWR_I  6
#define PWR_V 7

#define SHUTDOWN 8
#define PIPWR_EN 9
#define MGR_nSS 10
#define MGR_MOSI 11
#define ALT_EN 11
#define MGR_MISO 12
#define MGR_SCK 13
#define LED_BUILTIN 13

#define DTR_RXD 14
#define DTR_TXD 15
#define HOST_nCTS 16
#define HOST_nRTS 17
#define RX_nRE 18
#define TX_DE 19
#define DTR_nRE 20
#define DTR_DE 21

#define SDA1 22
#define SCL1 23

#endif // Pins_Board_h
