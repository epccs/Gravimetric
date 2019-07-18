#define FUNC_READ 1
#define FUNC_WRITE 1
/*
Serial bootloader
Copyright 2017 by Ronald Sutherland 

This program is free software; you can redistribute it
and/or modify it under the terms of the GNU General
Public License as published by the Free Software 
Foundation; either version 2 of the License, or
(at your option) any later version. 

This program is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public
License for more details. 

You should have received a copy of the GNU General
Public License along with this program; if not, write 
to the Free Software Foundation, Inc., 
59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 

Licence can be viewed at
http://www.fsf.org/licenses/gpl.txt

Based on optiboot which is GPL and 
Copyright 2013-2015 by Bill Westfield. 
Copyright 2010 by Peter Knight. 

For the original version see https://github.com/majekw/optiboot
*/

/*  Optional   
BIGBOOT: Build a 1k bootloader, not 512 bytes. This turns on extra functionality. 
BAUD: Set bootloader baud rate.
SOFT_UART: Use AVR305 soft-UART instead of hardware UART.
LED_START_FLASHES: Number of LED flashes on bootup.
LED_DATA_FLASH: Flash LED when transferring data.
SUPPORT_EEPROM: Support reading and writing from EEPROM. This is not used by Arduino, so off by default. 
TIMEOUT_MS: Bootloader timeout period, in milliseconds. 500,1000,2000,4000,8000 supported. 
 UART: UART number (0..n) for devices with more than one hardware uart (644P, 1284P, etc) 
*/

/*  fuses could be set in the code rather than makefile https://www.avrfreaks.net/forum/how-embed-fuses-c-xmega
FUSES = {
          .low =      0xFF,
          .high =     0xDE,
          .extended = 0xFD,
        };
*/

#define OPTIBOOT_MAJVER 6
#define OPTIBOOT_MINVER 2

unsigned const int __attribute__((section(".version"))) 
optiboot_version = 256*(OPTIBOOT_MAJVER) + OPTIBOOT_MINVER;

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

/* Note that optiboot has its own version of "boot.h" */
#include "boot.h"
#include "pin_defs.h"

/* stk500.h contains the constant definitions for the stk500v1 protocol used by avrdude */
#include "stk500.h"

#ifndef LED_START_FLASHES
#define LED_START_FLASHES 0
#endif

/* set the UART baud rate defaults */
#ifndef BAUD
#if F_CPU >= 8000000L
#define BAUD   115200L // Highest rate Avrdude win32 will support
#elif F_CPU >= 1000000L
#define BAUD   9600L   // 19200 also supported, but with significant error
#elif F_CPU >= 128000L
#define BAUD   4800L   // Good for 128kHz internal RC
#else
#define BAUD 1200L     // Good even at 32768Hz
#endif
#endif

#ifndef UART
#define UART 0
#endif

#define BAUD_SETTING (( (F_CPU + BAUD * 4L) / ((BAUD * 8L))) - 1 )
#define BAUD_ACTUAL (F_CPU/(8 * ((BAUD_SETTING)+1)))
#if BAUD_ACTUAL <= BAUD
  #define BAUD_ERROR (( 100*(BAUD - BAUD_ACTUAL) ) / BAUD)
  #if BAUD_ERROR >= 5
    #error BAUD error greater than -5%
  #elif BAUD_ERROR >= 2
    #warning BAUD error greater than -2%
  #endif
#else
  #define BAUD_ERROR (( 100*(BAUD_ACTUAL - BAUD) ) / BAUD)
  #if BAUD_ERROR >= 5
    #error BAUD error greater than 5%
  #elif BAUD_ERROR >= 2
    #warning BAUD error greater than 2%
  #endif
#endif

#if (F_CPU + BAUD * 4L) / (BAUD * 8L) - 1 > 250
#error Unachievable baud rate (too slow) BAUD 
#endif // baud rate slow check
#if (F_CPU + BAUD * 4L) / (BAUD * 8L) - 1 < 3
#if BAUD_ERROR != 0 // permit high bitrates (ie 1Mbps@16MHz) if error is zero
#error Unachievable baud rate (too fast) BAUD 
#endif
#endif // baud rate fastn check

/* Watchdog settings */
#define WATCHDOG_OFF    (0)
#define WATCHDOG_16MS   (_BV(WDE))
#define WATCHDOG_32MS   (_BV(WDP0) | _BV(WDE))
#define WATCHDOG_64MS   (_BV(WDP1) | _BV(WDE))
#define WATCHDOG_125MS  (_BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_250MS  (_BV(WDP2) | _BV(WDE))
#define WATCHDOG_500MS  (_BV(WDP2) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_1S     (_BV(WDP2) | _BV(WDP1) | _BV(WDE))
#define WATCHDOG_2S     (_BV(WDP2) | _BV(WDP1) | _BV(WDP0) | _BV(WDE))
#ifndef __AVR_ATmega8__
#define WATCHDOG_4S     (_BV(WDP3) | _BV(WDE))
#define WATCHDOG_8S     (_BV(WDP3) | _BV(WDP0) | _BV(WDE))
#endif


/*
 * We can never load flash with more than 1 page at a time, so we can save
 * some code space on parts with smaller pagesize by using a smaller int.
 */
#if SPM_PAGESIZE > 255
typedef uint16_t pagelen_t ;
#define GETLENGTH(len) len = getch()<<8; len |= getch()
#else
typedef uint8_t pagelen_t;
#define GETLENGTH(len) (void) getch() /* skip high byte */; len = getch()
#endif


/* Function Prototypes
 * The main() function is in init9, which removes the interrupt vector table
 * we don't need. It is also 'OS_main', which means the compiler does not
 * generate any entry or exit code itself (but unlike 'naked', it doesn't
 * supress some compile-time options we want.)
 */

void pre_main(void) __attribute__ ((naked)) __attribute__ ((section (".init8")));
int main(void) __attribute__ ((OS_main)) __attribute__ ((section (".init9")));

void __attribute__((noinline)) putch(char);
uint8_t __attribute__((noinline)) getch(void);
void __attribute__((noinline)) verifySpace();
void __attribute__((noinline)) watchdogConfig(uint8_t x);

static inline void getNch(uint8_t);
#if LED_START_FLASHES > 0
static inline void flash_led(uint8_t);
#endif
static inline void watchdogReset();
static inline void writebuffer(int8_t memtype, uint8_t *mybuff,
			       uint16_t address, pagelen_t len);
static inline void read_mem(uint8_t memtype,
			    uint16_t address, pagelen_t len);
static void __attribute__((noinline)) do_spm(uint16_t address, uint8_t command, uint16_t data);

#ifdef SOFT_UART
void uartDelay() __attribute__ ((naked));
#endif
void appStart(uint8_t rstFlags) __attribute__ ((naked));

/*
 * RAMSTART should be self-explanatory.  It's bigger on parts with a
 * lot of peripheral registers.  Let 0x100 be the default
 * Note that RAMSTART (for optiboot) need not be exactly at the start of RAM.
 */
#if !defined(RAMSTART)  // newer versions of gcc avr-libc define RAMSTART
#define RAMSTART 0x100
#if defined (__AVR_ATmega644P__)
// correct for a bug in avr-libc
#undef SIGNATURE_2
#define SIGNATURE_2 0x0A
#elif defined(__AVR_ATmega1280__)
#undef RAMSTART
#define RAMSTART (0x200)
#endif
#endif

// Make sure all variants gets the correct device signature
#if defined(__AVR_ATmega88__)
	#undef SIGNATURE_2
	#define SIGNATURE_2 0x0A
#elif defined(__AVR_ATmega88p__)
	#undef SIGNATURE_2
	#define SIGNATURE_2 0x0F
#elif defined(__AVR_ATmega168__)
	#undef SIGNATURE_2
	#define SIGNATURE_2 0x06
#elif defined(__AVR_ATmega168p__)
	#undef SIGNATURE_2
	#define SIGNATURE_2 0x0B
#elif defined(__AVR_ATmega328__)
	#undef SIGNATURE_2
	#define SIGNATURE_2 0x14
#elif defined(__AVR_ATmega328p__)
	#undef SIGNATURE_2
	#define SIGNATURE_2 0x0F	
#endif	
	

/* C zero initialises all global variables. However, that requires */
/* These definitions are NOT zero initialised, but that doesn't matter */
/* This allows us to drop the zero init code, saving us memory */
#define buff    ((uint8_t*)(RAMSTART))

/* Virtual boot partition support */
#ifdef VIRTUAL_BOOT_PARTITION
#define rstVect0_sav (*(uint8_t*)(RAMSTART+SPM_PAGESIZE*2+4))
#define rstVect1_sav (*(uint8_t*)(RAMSTART+SPM_PAGESIZE*2+5))
#define saveVect0_sav (*(uint8_t*)(RAMSTART+SPM_PAGESIZE*2+6))
#define saveVect1_sav (*(uint8_t*)(RAMSTART+SPM_PAGESIZE*2+7))
// Vector to save original reset jump:
//   SPM Ready is least probably used, so it's default
//   if not, use old way WDT_vect_num,
//   or simply set custom save_vect_num in Makefile using vector name
//   or even raw number.
#if !defined (save_vect_num)
#if defined (SPM_RDY_vect_num)
#define save_vect_num (SPM_RDY_vect_num)
#elif defined (SPM_READY_vect_num)
#define save_vect_num (SPM_READY_vect_num)
#elif defined (WDT_vect_num)
#define save_vect_num (WDT_vect_num)
#else
#error Cant find SPM or WDT interrupt vector for this CPU
#endif
#endif //save_vect_num
// check if it's on the same page (code assumes that)
#if (SPM_PAGESIZE <= save_vect_num)
#error Save vector not in the same page as reset!
#endif
#if FLASHEND > 8192
// AVRs with more than 8k of flash have 4-byte vectors, and use jmp.
//  We save only 16 bits of address, so devices with more than 128KB
//  may behave wrong for upper part of address space.
#define rstVect0 2
#define rstVect1 3
#define saveVect0 (save_vect_num*4+2)
#define saveVect1 (save_vect_num*4+3)
#define appstart_vec (save_vect_num*2)
#else
// AVRs with up to 8k of flash have 2-byte vectors, and use rjmp.
#define rstVect0 0
#define rstVect1 1
#define saveVect0 (save_vect_num*2)
#define saveVect1 (save_vect_num*2+1)
#define appstart_vec (save_vect_num)
#endif
#else
#define appstart_vec (0)
#endif // VIRTUAL_BOOT_PARTITION

/* everything that needs to run VERY early */
void pre_main(void) {
  // Allow convenient way of calling do_spm function - jump table,
  //   so entry to this function will always be here, indepedent of compilation,
  //   features etc
  asm volatile (
    "	rjmp	1f\n"
    /*"	rjmp	do_spm\n" */ // RSS: not useful and makes hacking to predictable.
    "1:\n"
  );
}


/* main program starts here */
int main(void) {
  uint8_t ch;

  /*
   * Making these local and in registers prevents the need for initializing
   * them, and also saves space because code no longer stores to memory.
   * (initializing address keeps the compiler happy, but isn't really
   *  necessary, and uses 4 bytes of flash.)
   */
  register uint16_t address = 0;
  register pagelen_t  length;

  // After the zero init loop, this is the first code to run.
  //
  // This code makes the following assumptions:
  //  No interrupts will execute
  //  SP points to RAMEND
  //  r1 contains zero
  //
  // If not, uncomment the following instructions:
  // cli();
  asm volatile ("clr __zero_reg__");
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega16__)
  SP=RAMEND;  // This is done by hardware reset
#endif

  /*
   * Protect as much from MCUSR as possible for application
   * and still skip bootloader if not necessary
   * 
   * Code by MarkG55
   * see discusion in https://github.com/Optiboot/optiboot/issues/97
   */
#if !defined(__AVR_ATmega16__)
  ch = MCUSR;
#else
  ch = MCUCSR;
#endif
  // Skip all logic and run bootloader if MCUSR is cleared (application request)
  if (ch != 0) {
    /*
     * To run the boot loader, External Reset Flag must be set.
     * If not, we could make shortcut and jump directly to application code.
     * Also WDRF set with EXTRF is a result of Optiboot timeout, so we
     * shouldn't run bootloader in loop :-) That's why:
     *  1. application is running if WDRF is cleared
     *  2. we clear WDRF if it's set with EXTRF to avoid loops
     * One problematic scenario: broken application code sets watchdog timer 
     * without clearing MCUSR before and triggers it quickly. But it's
     * recoverable by power-on with pushed reset button.
     */
    if ((ch & (_BV(WDRF) | _BV(EXTRF))) != _BV(EXTRF)) { 
      if (ch & _BV(EXTRF)) {
        /*
         * Clear WDRF because it was most probably set by wdr in bootloader.
         * It's also needed to avoid loop by broken application which could
         * prevent entering bootloader.
         * '&' operation is skipped to spare few bytes as bits in MCUSR
         * can only be cleared.
         */
#if !defined(__AVR_ATmega16__)
        MCUSR = ~(_BV(WDRF));  
#else
        MCUCSR = ~(_BV(WDRF));  
#endif
      }
      appStart(ch);
    }
  }
  
#if LED_START_FLASHES > 0
  // Set up Timer 1 for timeout counter
  TCCR1B = _BV(CS12) | _BV(CS10); // div 1024
#endif

#ifndef SOFT_UART
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega16__)
  UCSRA = _BV(U2X); //Double speed mode USART
  UCSRB = _BV(RXEN) | _BV(TXEN);  // enable Rx & Tx
  UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);  // config USART; 8N1
  UBRRL = (uint8_t)( (F_CPU + BAUD * 4L) / (BAUD * 8L) - 1 );
#else
  UART_SRA = _BV(U2X0); //Double speed mode USART0
  UART_SRB = _BV(RXEN0) | _BV(TXEN0);
  UART_SRC = _BV(UCSZ00) | _BV(UCSZ01);
  UART_SRL = (uint8_t)( (F_CPU + BAUD * 4L) / (BAUD * 8L) - 1 );
#endif
#endif

  // Set up watchdog to trigger after 1s
  watchdogConfig(WATCHDOG_2S);

#if (LED_START_FLASHES > 0) || defined(LED_DATA_FLASH)
  /* Set LED pin as output */
  LED_DDR |= _BV(LED);
#endif

#ifdef SOFT_UART
  /* Set TX pin as output */
  UART_DDR |= _BV(UART_TX_BIT);
#endif

#if LED_START_FLASHES > 0
  /* Flash onboard LED to signal entering of bootloader */
  flash_led(LED_START_FLASHES * 2);
#endif

  /* Forever loop: exits by causing WDT reset */
  for (;;) {
    /* get character from UART */
    ch = getch();

    if(ch == STK_GET_PARAMETER) {
      unsigned char which = getch();
      verifySpace();
      /*
       * Send optiboot version as "SW version"
       * Note that the references to memory are optimized away.
       */
      if (which == STK_SW_MINOR) {
	  putch(optiboot_version & 0xFF);
      } else if (which == STK_SW_MAJOR) {
	  putch(optiboot_version >> 8);
      } else {
	/*
	 * GET PARAMETER returns a generic 0x03 reply for
         * other parameters - enough to keep Avrdude happy
	 */
	putch(0x03);
      }
    }
    else if(ch == STK_SET_DEVICE) {
      // SET DEVICE is ignored
      getNch(20);
    }
    else if(ch == STK_SET_DEVICE_EXT) {
      // SET DEVICE EXT is ignored
      getNch(5);
    }
    else if(ch == STK_LOAD_ADDRESS) {
      // LOAD ADDRESS
      uint16_t newAddress;
      newAddress = getch();
      newAddress = (newAddress & 0xff) | (getch() << 8);
#ifdef RAMPZ
      // Transfer top bit to LSB in RAMPZ
      if (newAddress & 0x8000) {
        RAMPZ |= 0x01;
      }
      else {
        RAMPZ &= 0xFE;
      }
#endif
      newAddress += newAddress; // Convert from word address to byte address
      address = newAddress;
      verifySpace();
    }
    else if(ch == STK_UNIVERSAL) {
#ifdef RAMPZ
      // LOAD_EXTENDED_ADDRESS is needed in STK_UNIVERSAL for addressing more than 128kB
      if ( AVR_OP_LOAD_EXT_ADDR == getch() ) {
        // get address
        getch();  // get '0'
        RAMPZ = (RAMPZ & 0x01) | ((getch() << 1) & 0xff);  // get address and put it in RAMPZ
        getNch(1); // get last '0'
        // response
        putch(0x00);
      }
      else {
        // everything else is ignored
        getNch(3);
        putch(0x00);
      }
#else
      // UNIVERSAL command is ignored
      getNch(4);
      putch(0x00);
#endif
    }
    /* Write memory, length is big endian and is in bytes */
    else if(ch == STK_PROG_PAGE) {
      // PROGRAM PAGE - we support flash programming only, not EEPROM
      uint8_t desttype;
      uint8_t *bufPtr;
      pagelen_t savelength;

      GETLENGTH(length);
      savelength = length;
      desttype = getch();

      // read a page worth of contents
      bufPtr = buff;
      do *bufPtr++ = getch();
      while (--length);

      // Read command terminator, start reply
      verifySpace();

#ifdef VIRTUAL_BOOT_PARTITION
#if FLASHEND > 8192
/*
 * AVR with 4-byte ISR Vectors and "jmp"
 * WARNING: this works only up to 128KB flash!
 */
      if (address == 0) {
	// This is the reset vector page. We need to live-patch the
	// code so the bootloader runs first.
	//
	// Save jmp targets (for "Verify")
	rstVect0_sav = buff[rstVect0];
	rstVect1_sav = buff[rstVect1];
	saveVect0_sav = buff[saveVect0];
	saveVect1_sav = buff[saveVect1];

        // Move RESET jmp target to 'save' vector
        buff[saveVect0] = rstVect0_sav;
        buff[saveVect1] = rstVect1_sav;

        // Add jump to bootloader at RESET vector
        // WARNING: this works as long as 'main' is in first section
        buff[rstVect0] = ((uint16_t)main) & 0xFF;
        buff[rstVect1] = ((uint16_t)main) >> 8;
      }

#else
/*
 * AVR with 2-byte ISR Vectors and rjmp
 */
      if ((uint16_t)(void*)address == rstVect0) {
        // This is the reset vector page. We need to live-patch
        // the code so the bootloader runs first.
        //
        // Move RESET vector to 'save' vector
	// Save jmp targets (for "Verify")
	rstVect0_sav = buff[rstVect0];
	rstVect1_sav = buff[rstVect1];
	saveVect0_sav = buff[saveVect0];
	saveVect1_sav = buff[saveVect1];

	// Instruction is a relative jump (rjmp), so recalculate.
	uint16_t vect=(rstVect0_sav & 0xff) | ((rstVect1_sav & 0x0f)<<8); //calculate 12b displacement
	vect = (vect-save_vect_num) & 0x0fff; //substract 'save' interrupt position and wrap around 4096
        // Move RESET jmp target to 'save' vector
        buff[saveVect0] = vect & 0xff;
        buff[saveVect1] = (vect >> 8) | 0xc0; //
        // Add rjump to bootloader at RESET vector
        vect = ((uint16_t)main) &0x0fff; //WARNIG: this works as long as 'main' is in first section
        buff[0] = vect & 0xFF; // rjmp 0x1c00 instruction
	buff[1] = (vect >> 8) | 0xC0;
      }
#endif // FLASHEND
#endif // VBP

      writebuffer(desttype, buff, address, savelength);


    }
    /* Read memory block mode, length is big endian.  */
    else if(ch == STK_READ_PAGE) {
      uint8_t desttype;
      GETLENGTH(length);

      desttype = getch();

      verifySpace();

      read_mem(desttype, address, length);
    }

    /* Get device signature bytes  */
    else if(ch == STK_READ_SIGN) {
      // READ SIGN - return what Avrdude wants to hear
      verifySpace();
      putch(SIGNATURE_0);
      putch(SIGNATURE_1);
      putch(SIGNATURE_2);
    }
    else if (ch == STK_LEAVE_PROGMODE) { /* 'Q' */
      // Adaboot no-wait mod
      watchdogConfig(WATCHDOG_16MS);
      verifySpace();
    }
    else {
      // This covers the response to commands like STK_ENTER_PROGMODE
      verifySpace();
    }
    putch(STK_OK);
  }
}

void putch(char ch) {
#ifndef SOFT_UART
  while (!(UART_SRA & _BV(UDRE0)));
  UART_UDR = ch;
#else
  __asm__ __volatile__ (
    "   com %[ch]\n" // ones complement, carry set
    "   sec\n"
    "1: brcc 2f\n"
    "   cbi %[uartPort],%[uartBit]\n"
    "   rjmp 3f\n"
    "2: sbi %[uartPort],%[uartBit]\n"
    "   nop\n"
    "3: rcall uartDelay\n"
    "   rcall uartDelay\n"
    "   lsr %[ch]\n"
    "   dec %[bitcnt]\n"
    "   brne 1b\n"
    :
    :
      [bitcnt] "d" (10),
      [ch] "r" (ch),
      [uartPort] "I" (_SFR_IO_ADDR(UART_PORT)),
      [uartBit] "I" (UART_TX_BIT)
    :
      "r25"
  );
#endif
}

uint8_t getch(void) {
  uint8_t ch;

#ifdef LED_DATA_FLASH
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega16__)
  LED_PORT ^= _BV(LED);
#else
  LED_PIN |= _BV(LED);
#endif
#endif

#ifdef SOFT_UART
    watchdogReset();
  __asm__ __volatile__ (
    "1: sbic  %[uartPin],%[uartBit]\n"  // Wait for start edge
    "   rjmp  1b\n"
    "   rcall uartDelay\n"          // Get to middle of start bit
    "2: rcall uartDelay\n"              // Wait 1 bit period
    "   rcall uartDelay\n"              // Wait 1 bit period
    "   clc\n"
    "   sbic  %[uartPin],%[uartBit]\n"
    "   sec\n"
    "   dec   %[bitCnt]\n"
    "   breq  3f\n"
    "   ror   %[ch]\n"
    "   rjmp  2b\n"
    "3:\n"
    :
      [ch] "=r" (ch)
    :
      [bitCnt] "d" (9),
      [uartPin] "I" (_SFR_IO_ADDR(UART_PIN)),
      [uartBit] "I" (UART_RX_BIT)
    :
      "r25"
);
#else
  while(!(UART_SRA & _BV(RXC0)))
    ;
  if (!(UART_SRA & _BV(FE0))) {
      /*
       * A Framing Error indicates (probably) that something is talking
       * to us at the wrong bit rate.  Assume that this is because it
       * expects to be talking to the application, and DON'T reset the
       * watchdog.  This should cause the bootloader to abort and run
       * the application "soon", if it keeps happening.  (Note that we
       * don't care that an invalid char is returned...)
       */
    watchdogReset();
  }

  ch = UART_UDR;
#endif

#ifdef LED_DATA_FLASH
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega16__)
  LED_PORT ^= _BV(LED);
#else
  LED_PIN |= _BV(LED);
#endif
#endif

  return ch;
}

#ifdef SOFT_UART
// AVR305 equation: #define UART_B_VALUE (((F_CPU/BAUD)-23)/6)
// Adding 3 to numerator simulates nearest rounding for more accurate baud rates
#define UART_B_VALUE (((F_CPU/BAUD)-20)/6)
#if UART_B_VALUE > 255
#error Baud rate too slow for soft UART
#endif

void uartDelay() {
  __asm__ __volatile__ (
    "ldi r25,%[count]\n"
    "1:dec r25\n"
    "brne 1b\n"
    "ret\n"
    ::[count] "M" (UART_B_VALUE)
  );
}
#endif

void getNch(uint8_t count) {
  do getch(); while (--count);
  verifySpace();
}

void verifySpace() {
  if (getch() != CRC_EOP) {
    watchdogConfig(WATCHDOG_16MS);    // shorten WD timeout
    while (1)			      // and busy-loop so that WD causes
      ;				      //  a reset and app start.
  }
  putch(STK_INSYNC);
}

#if LED_START_FLASHES > 0
void flash_led(uint8_t count) {
  do {
    TCNT1 = -(F_CPU/(1024*16));
    TIFR1 = _BV(TOV1);
    while(!(TIFR1 & _BV(TOV1)));
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega16__)
    LED_PORT ^= _BV(LED);
#else
    LED_PIN |= _BV(LED);
#endif
    watchdogReset();
  } while (--count);
}
#endif

// Watchdog functions. These are only safe with interrupts turned off.
void watchdogReset() {
  __asm__ __volatile__ (
    "wdr\n"
  );
}

void watchdogConfig(uint8_t x) {
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = x;
}

void appStart(uint8_t rstFlags) {
  // save the reset flags in the designated register
  //  This can be saved in a main program by putting code in .init0 (which
  //  executes before normal c init code) to save R2 to a global variable.
  __asm__ __volatile__ ("mov r2, %0\n" :: "r" (rstFlags));

  watchdogConfig(WATCHDOG_OFF);
  // Note that appstart_vec is defined so that this works with either
  // real or virtual boot partitions.
  __asm__ __volatile__ (
    // Jump to 'save' or RST vector
    "ldi r30,%[rstvec]\n"
    "clr r31\n"
    "ijmp\n"::[rstvec] "M"(appstart_vec)
  );
}

/*
 * void writebuffer(memtype, buffer, address, length)
 */
static inline void writebuffer(int8_t memtype, uint8_t *mybuff,
			       uint16_t address, pagelen_t len)
{
    switch (memtype) {
    case 'E': // EEPROM
#if defined(SUPPORT_EEPROM) || defined(BIGBOOT)
        while(len--) {
	    eeprom_write_byte((uint8_t *)(address++), *mybuff++);
        }
#else
	/*
	 * On systems where EEPROM write is not supported, just busy-loop
	 * until the WDT expires, which will eventually cause an error on
	 * host system (which is what it should do.)
	 */
	while (1)
	    ; // Error: wait for WDT
#endif
	break;
    default:  // FLASH
	/*
	 * Default to writing to Flash program memory.  By making this
	 * the default rather than checking for the correct code, we save
	 * space on chips that don't support any other memory types.
	 */
	{
	    // Copy buffer into programming buffer
	    uint8_t *bufPtr = mybuff;
	    uint16_t addrPtr = (uint16_t)(void*)address;

	    /*
	     * Start the page erase and wait for it to finish.  There
	     * used to be code to do this while receiving the data over
	     * the serial link, but the performance improvement was slight,
	     * and we needed the space back.
	     */
	    do_spm((uint16_t)(void*)address,__BOOT_PAGE_ERASE,0);

	    /*
	     * Copy data from the buffer into the flash write buffer.
	     */
	    do {
		uint16_t a;
		a = *bufPtr++;
		a |= (*bufPtr++) << 8;
		do_spm((uint16_t)(void*)addrPtr,__BOOT_PAGE_FILL,a);
		addrPtr += 2;
	    } while (len -= 2);

	    /*
	     * Actually Write the buffer to flash (and wait for it to finish.)
	     */
	    do_spm((uint16_t)(void*)address,__BOOT_PAGE_WRITE,0);
	} // default block
	break;
    } // switch
}

static inline void read_mem(uint8_t memtype, uint16_t address, pagelen_t length)
{
    uint8_t ch;

    switch (memtype) {

#if defined(SUPPORT_EEPROM) || defined(BIGBOOT)
    case 'E': // EEPROM
	do {
	    putch(eeprom_read_byte((uint8_t *)(address++)));
	} while (--length);
	break;
#endif
    default:
	do {
#ifdef VIRTUAL_BOOT_PARTITION
        // Undo vector patch in bottom page so verify passes
	    if (address == rstVect0) ch = rstVect0_sav;
	    else if (address == rstVect1) ch = rstVect1_sav;
	    else if (address == saveVect0) ch = saveVect0_sav;
	    else if (address == saveVect1) ch = saveVect1_sav;
	    else ch = pgm_read_byte_near(address);
	    address++;
#elif defined(RAMPZ)
	    // Since RAMPZ should already be set, we need to use EPLM directly.
	    // Also, we can use the autoincrement version of lpm to update "address"
	    //      do putch(pgm_read_byte_near(address++));
	    //      while (--length);
	    // read a Flash and increment the address (may increment RAMPZ)
	    __asm__ ("elpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
#else
	    // read a Flash byte and increment the address
	    __asm__ ("lpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
#endif
	    putch(ch);
	} while (--length);
	break;
    } // switch
}

/*
 * Separate function for doing spm stuff
 * It's needed for application to do SPM, as SPM instruction works only
 * from bootloader.
 *
 * How it works:
 * - do SPM
 * - wait for SPM to complete
 * - if chip have RWW/NRWW sections it does additionaly:
 *   - if command is WRITE or ERASE, AND data=0 then reenable RWW section
 *
 * In short:
 * If you play erase-fill-write, just set data to 0 in ERASE and WRITE
 * If you are brave, you have your code just below bootloader in NRWW section
 *   you could do fill-erase-write sequence with data!=0 in ERASE and
 *   data=0 in WRITE
 */
static void do_spm(uint16_t address, uint8_t command, uint16_t data) {
    // Do spm stuff
    asm volatile (
	"    movw  r0, %3\n"
        "    out %0, %1\n"
        "    spm\n"
        "    clr  r1\n"
        :
        : "i" (_SFR_IO_ADDR(__SPM_REG)),
          "r" ((uint8_t)command),
          "z" ((uint16_t)address),
          "r" ((uint16_t)data)
        : "r0"
    );

    // wait for spm to complete
    //   it doesn't have much sense for __BOOT_PAGE_FILL,
    //   but it doesn't hurt and saves some bytes on 'if'
    boot_spm_busy_wait();
#if defined(RWWSRE)
    // this 'if' condition should be: (command == __BOOT_PAGE_WRITE || command == __BOOT_PAGE_ERASE)...
    // but it's tweaked a little assuming that in every command we are interested in here, there
    // must be also SELFPRGEN set. If we skip checking this bit, we save here 4B
    if ((command & (_BV(PGWRT)|_BV(PGERS))) && (data == 0) ) {
      // Reenable read access to flash
      boot_rww_enable();
    }
#endif
}
