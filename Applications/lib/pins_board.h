/*
 * Pin definitions for Gravimetric used with pin_num.h Digital IO library
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

//Analog channels and DIO have same number on this board (4..7 are test points)
#define ADC0 0
#define ADC1 1
#define ADC2 2
#define ADC3 3
#define ADC4 4
#define ADC5 5
#define ADC6 6
#define ADC7 7
// on ^0 channels 4..7 were connected to ALT_V,ALT_I,PWR_I,PWR_V. They are test points now.

// 22mA Current Source Enable
#define CS0_EN 8
#define CS4_EN 9

// not connected
#define NC10 10

// 22mA Current Source Enable
#define CS2_EN 11

// SPI 
#define nSS 12
#define MOSI 13

// ICP3 pin reads start event inverted from 100 Ohm termination
#define ICP3_MOSI 13

// SPI
#define MISO 14
#define SCK 15

// I2C
#define SCL0 16
#define SDA0 17

// 22mA Current Source Enable for Gravimetric diversion control
#define CS_FAST 18

// ICP4 pin reads stop event inverted from 100 Ohm termination
#define ICP4 19

// 17mA Current Source Enable
#define CS_ICP4 20
#define CS_ICP3 21

// 22mA Current Source Enable for Gravimetric diversion control
#define CS_DIVERSION 22

// 22mA Current Source Enable
#define CS3_EN 23

// UART2 is for user
#define RX2 24
#define TX2 25

// I2C
#define SDA1 26
#define SCL1 27

// UART0 is for rpubus communication
#define RX0 28 
#define TX0 29

// UART1 is for user
#define RX1 30
#define TX1 31

// 22mA Current Source Enable
#define CS1_EN 32

// 17mA Current Source Enable
#define CS_ICP1 33

// ICP1 pin reads inverted from the 100 Ohm termination
#define ICP1 34

// ALTernate power ENable for solar panel power
#define NC35 35

#endif // Pins_Board_h
