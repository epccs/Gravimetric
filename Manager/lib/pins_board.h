/*
Pin definitions for manager used with Digital IO library
Copyright (C) 2019 Ronald Sutherland
 
All rights reserved, specifically, the right to distribute is withheld. Subject to your compliance 
with these terms, you may use this software and any derivatives exclusively with Ronald Sutherland 
products. It is your responsibility to comply with third party license terms applicable to your use 
of third party software (including open source software) that accompany Ronald Sutherland software.

THIS SOFTWARE IS SUPPLIED BY RONALD SUTHERLAND "AS IS". NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
FOR A PARTICULAR PURPOSE.

IN NO EVENT WILL RONALD SUTHERLAND BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF RONALD SUTHERLAND
HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
THE FULLEST EXTENT ALLOWED BY LAW, RONALD SUTHERLAND'S TOTAL LIABILITY ON ALL
CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO RONALD SUTHERLAND FOR THIS
SOFTWARE.
 */
#ifndef Pins_Board_h
#define Pins_Board_h

// ADC channels 0..7
#define ALT_I 0
#define ALT_V 1
#define TX_nRE 2
#define RX_DE 3
#define SDA0 4
#define SCL0 5
#define PWR_I 6
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
