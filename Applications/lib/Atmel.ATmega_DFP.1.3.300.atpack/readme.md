# Atmel Packs (Atmel.ATmega_DFP.1.3.300.atpack)

downloaded from http://packs.download.atmel.com/

Note: I have remove everything but the 324pb support

# Usage

avr-gcc -mmcu=atmega324pb -B ../Atmel.ATmega_DFP.1.3.300.atpack/gcc/dev/atmega324pb/ -I ../Atmel.ATmega_DFP.1.3.300.atpack/include/

# Atmel toolchain

http://distribute.atmel.no/tools/opensource/Atmel-AVR-GNU-Toolchain/

I am just using avr-gcc packaged on Debian buster (e.g. 5.4.0 it is on Ubuntu 1804)

Welp; that "iom324pb.h" file is going to cause pain. 

Bits (e.g., RXCIE) in registers (e.g., UCSRnB) were defined for each register but for the 324pb you have to know it is a bit that is used the same way for multiple registers (e.g., RXCIEn is not defined).

MCUdude has added this to make it work in MightyCore.

```
#if defined(__AVR_ATmega324PB__)
  // USART
  #define MPCM0   MPCM
  #define U2X0    U2X
  #define UPE0    UPE
  #define DOR0    DOR
  #define FE0     FE
  #define UDRE0   UDRE
  #define TXC0    TXC
  #define RXC0    RXC
  #define TXB80   TXB8
  #define RXB80   RXB8
  #define UCSZ02  UCSZ2
  #define TXEN0   TXEN
  #define RXEN0   RXEN
  #define UDRIE0  UDRIE
  #define TXCIE0  TXCIE
  #define RXCIE0  RXCIE
  #define UCPOL0  UCPOL
  #define UCSZ00  UCSZ0
  #define UCSZ01  UCSZ1
  #define USBS0   USBS
  #define UPM00   UPM0
  #define UPM01   UPM1
  #define UMSEL00 UMSEL0
  #define UMSEL01 UMSEL1  
  #define MPCM1   MPCM
  #define U2X1    U2X
  #define UPE1    UPE
  #define DOR1    DOR
  #define FE1     FE
  #define UDRE1   UDRE
  #define TXC1    TXC
  #define RXC1    RXC
  #define TXB81   TXB8
  #define RXB81   RXB8
  #define UCSZ12  UCSZ2
  #define TXEN1   TXEN
  #define RXEN1   RXEN
  #define UDRIE1  UDRIE
  #define TXCIE1  TXCIE
  #define RXCIE1  RXCIE
  #define UCPOL1  UCPOL
  #define UCSZ10  UCSZ0
  #define UCSZ11  UCSZ1
  #define USBS1   USBS
  #define UPM10   UPM0
  #define UPM11   UPM1
  #define UMSEL10 UMSEL0
  #define UMSEL11 UMSEL1 
  #define MPCM2   MPCM
  #define U2X2    U2X
  #define UPE2    UPE
  #define DOR2    DOR
  #define FE2     FE
  #define UDRE2   UDRE
  #define TXC2    TXC
  #define RXC2    RXC
  #define TXB82   TXB8
  #define RXB82   RXB8
  #define UCSZ22  UCSZ2
  #define TXEN2   TXEN
  #define RXEN2   RXEN
  #define UDRIE2  UDRIE
  #define TXCIE2  TXCIE
  #define RXCIE2  RXCIE
  #define UCPOL2  UCPOL
  #define UCSZ20  UCSZ0
  #define UCSZ21  UCSZ1
  #define USBS2   USBS
  #define UPM20   UPM0
  #define UPM21   UPM1
  #define UMSEL20 UMSEL0
  #define UMSEL21 UMSEL1
  
  // i2c
  #define TWI_vect TWI0_vect
  #define TWI_vect_num TWI0_vect_num
  #define TWBR TWBR0
  #define TWSR TWSR0
  #define TWS3 TWS03
  #define TWS4 TWS04
  #define TWS5 TWS05
  #define TWS6 TWS06
  #define TWS7 TWS07
  #define TWAR TWAR0
  #define TWDR TWDR0
  #define TWD0 0
  #define TWD1 1
  #define TWD2 2
  #define TWD3 3
  #define TWD4 4
  #define TWD5 5
  #define TWD6 6
  #define TWD7 7
  #define TWCR  TWCR0
  #define TWAMR TWAMR0
  #define TWAM0 TWAM00
  #define TWAM1 TWAM01
  #define TWAM2 TWAM02
  #define TWAM3 TWAM03
  #define TWAM4 TWAM04
  #define TWAM5 TWAM05
  #define TWAM6 TWAM06

  // SPI
  #define SPI_STC_vect SPI0_STC_vect
  #define SPI_STC_vect_num SPI0_STC_vect_num
  #define SPCR SPCR0
  #define SPSR SPSR0
  #define SPDR SPDR0
  #define SPDRB0 0
  #define SPDRB1 1
  #define SPDRB2 2
  #define SPDRB3 3
  #define SPDRB4 4
  #define SPDRB5 5
  #define SPDRB6 6
  #define SPDRB7 7

#endif // 324PB defs
```