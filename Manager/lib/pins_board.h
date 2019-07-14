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

#define DTR_RXD 0
#define DTR_TXD 1
#define HOST_nDTR 2
#define HOST_nRTS 3
#define RX_nRE 4
#define TX_DE 5
#define DTR_nRE 6
#define DTR_DE 7
#define SHUTDOWN 8

#define LED_BUILTIN 9

#define nSS 10
#define MOSI 11
#define MISO 12
#define SCK 13

#define HOST_nCTS 14
#define HOST_nDSR 15
#define TX_nRE 16
#define RX_DE 17

#define SDA 18
#define SCL 19

#endif // Pins_Board_h
