/*
Battery application is a CLI  that demonstrates battery charging from photovoltaic on the alternat power input.
Copyright (C) 2020 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Note the library files are LGPL, e.g., you need to publish changes of them but can derive from this 
source and copyright or distribute as you see fit (it is Zero Clause BSD).

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/

#include <stdbool.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "../lib/uart0_bsd.h"
#include "../lib/parse.h"
#include "../lib/timers_bsd.h"
#include "../lib/adc_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/rpu_mgr_callback.h"
#include "../lib/io_enum_bsd.h"
#include "../Uart/id.h"
#include "../Adc/analog.h"
#include "../DayNight/day_night.h"
#include "main.h"
#include "battery.h"

#define ADC_DELAY_MILSEC 50UL
static unsigned long adc_started_at;

#define DAYNIGHT_BLINK 500UL
static unsigned long daynight_status_blink_started_at;

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;
static char rpu_addr;
static uint8_t rpu_addr_is_fake;
uint8_t manager_status;

void ProcessCmd()
{ 
    if ( (strcmp_P( command, PSTR("/id?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        Id("Battery"); 
    }
    if ( (strcmp_P( command, PSTR("/analog?")) == 0) && ( (arg_count >= 1 ) && (arg_count <= 5) ) )
    {
        Analog(5000UL); 
    }
    if ( (strcmp_P( command, PSTR("/day?")) == 0) && ( (arg_count == 0 ) ) )
    {
        dnReport(5000UL); 
    }
    if ( (strcmp_P( command, PSTR("/dnmthresh?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        dnMorningThreshold(); // set daynight state machine daynight_morning_threshold
    }
    if ( (strcmp_P( command, PSTR("/dnethresh?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        dnEveningThreshold(); // set daynight state machine daynight_evening_threshold
    }
    if ( (strcmp_P( command, PSTR("/dnmdebounc?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        dnMorningDebounce(); // set daynight state machine daynight_morning_debounce
    }
    if ( (strcmp_P( command, PSTR("/dnedebounc?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        dnEveningDebounce(); // set daynight state machine daynight_evening_debounce
    }
    if ( (strcmp_P( command, PSTR("/bm")) == 0) && ( (arg_count == 0 ) ) )
    {
        EnableBatMngCntl(); 
    }
    if ( (strcmp_P( command, PSTR("/bmcntl?")) == 0) && ( (arg_count == 0 ) ) )
    {
        ReportBatMngCntl(5000UL);  
    }
    if ( (strcmp_P( command, PSTR("/bmlow?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        BatMngLowLimit();  // set battery manager battery_low_limit
    }
    if ( (strcmp_P( command, PSTR("/bmhigh?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        BatMngHighLimit(); // set battery manager battery_high_limit
    }
    if ( (strcmp_P( command, PSTR("/bmhost?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        BatMngHostLimit(); // set battery manager battery_host_limit
    }
}

// these functions can be registered as callbacks 
// so the manager can update the application over i2c
void daynight_state_event(uint8_t daynight_state_from_mgr)
{
    daynight_state = daynight_state_from_mgr;
}

// Don't printf inside the callback since it is in i2c's ISR context.
// Why? The UART can/will block and lock up everything.
uint8_t day_work_flag;
void day_work_event(uint8_t data)
{
    day_work_flag = 1;
}

uint8_t night_work_flag;
void night_work_event(uint8_t data)
{
    night_work_flag = 1;
}

void battery_state_event(uint8_t batmgr_state_from_mgr)
{
    bm_state = (BATTERYMGR_STATE_t)batmgr_state_from_mgr;
}

void register_manager_callbacks(void)
{
    twi0_registerOnDayNightStateCallback(daynight_state_event);
    twi0_registerOnDayWorkCallback(day_work_event);
    twi0_registerOnNightWorkCallback(night_work_event);
    twi0_registerOnBatMgrStateCallback(battery_state_event); // register but not enabled in setup (done in battery.c)
}

void setup(void) 
{
    ioDir(MCU_IO_CS0_EN, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS0_EN, LOGIC_LEVEL_HIGH);

    ioDir(MCU_IO_CS1_EN, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS1_EN, LOGIC_LEVEL_HIGH);

    // Initialize Timers, ADC, and clear bootloader, Arduino does these with init() in wiring.c
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    uart0_init(0,0); // bootloader may have the UART enabled, a zero baudrate will disconnect it.

    // put ADC in Auto Trigger mode and fetch an array of channels
    enable_ADC_auto_conversion(BURST_MODE);
    adc_started_at = milliseconds();

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);
    
    /* Clear and setup the command buffer, (probably not needed at this point) */
    initCommandBuffer();

    // manager delays (blocks) for 50mSec after power up so i2c is not running yet
    _delay_ms(60); 

    /* Initialize I2C */
    twi0_slaveAddress(I2C0_APP_ADDR);
    twi0_registerSlaveTxCallback(transmit_i2c_event); // called when manager wants data returned (and I need to transmit it)
    twi0_registerSlaveRxCallback(receive_i2c_event); // called when manager has an event to send (and I need to receive data)
    twi0_init(100000UL, TWI0_PINS_PULLUP);

    // Enable global interrupts to start TIMER0 and UART ISR's
    sei(); 
    
    blink_started_at = milliseconds();
    daynight_status_blink_started_at = milliseconds();
    
     // manager will broadcast normal mode on DTR pair of mulit-drop
    rpu_addr = i2c_get_Rpu_address(); 
    if (mgr_twiErrorCode) manager_status = mgr_twiErrorCode;
    
    // default address, since RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        rpu_addr_is_fake = 1;
    }

    // managers default debounce is 20 min (e.g. 1,200,000 milliseconds) but to test this I want less
    TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
    while (loop_state != TWI0_LOOP_STATE_DONE)
    {
        i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_EVENING_DEBOUNCE+RW_WRITE_BIT,18000UL,&loop_state); // 18 sec
    }
    loop_state = TWI0_LOOP_STATE_INIT;
    while (loop_state != TWI0_LOOP_STATE_DONE)
    {
        i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_MORNING_DEBOUNCE+RW_WRITE_BIT,18000UL,&loop_state);
    }

    // ALT_V reading of analogRead(ALT_V)*5.0/1024.0*(11/1) where 40 is about 2.1V
    // 80 is about 4.3V, 160 is about 8.6V, 320 is about 17.18V
    // manager uses int bellow and adc from ALT_V divider to check threshold. 
    loop_state = TWI0_LOOP_STATE_INIT;
    while (loop_state != TWI0_LOOP_STATE_DONE)
    {
        i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_EVENING_THRESHOLD+RW_WRITE_BIT,40,&loop_state);
    }
    loop_state = TWI0_LOOP_STATE_INIT;
    while (loop_state != TWI0_LOOP_STATE_DONE)
    {
        i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_MORNING_THRESHOLD+RW_WRITE_BIT,80,&loop_state);
    }

    // register manager callbacks
    // then enable the manager as i2c master to send updates to the application
    register_manager_callbacks();
    i2c_daynight_cmd(I2C0_APP_ADDR); // this will also cause a callback (poke) to be generated that updates daynight_state
    i2c_battery_cmd(I2C0_APP_ADDR,CB_ROUTE_BM_STATE,0); // this will setup callback but but bm will be off.
}

void blink_mgr_status(void)
{
    // bm_state bypass, routes PWM to blink the status LED
    if ((bm_state == BATTERYMGR_STATE_PWM_MODE_ON) || (bm_state == BATTERYMGR_STATE_PWM_MODE_OFF))
    {
        if (bm_state == BATTERYMGR_STATE_PWM_MODE_ON)
        {
            ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_HIGH);
        }
        else
        {
            ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_LOW);
        }
        return;
    }

    unsigned long kRuntime = elapsed(&blink_started_at);

    // normal, all is fine
    if ( kRuntime > BLINK_DELAY)
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // next toggle 
        blink_started_at += BLINK_DELAY; 
    }

    // blink fast if address is fake
    if ( rpu_addr_is_fake && (kRuntime > (BLINK_DELAY/4) ) )
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // set for next toggle 
        blink_started_at += BLINK_DELAY/4; 
    }

    // blink very fast if manager_status bits 0 or 2 (DTR), 1 (twi), 6 (daynight) are set
    if ( (manager_status & ((1<<0) | (1<<1) | (1<<2) | (1<<6)) ) && \
        (kRuntime > (BLINK_DELAY/8) ) )
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // set for next toggle 
        blink_started_at += BLINK_DELAY/8; 
    }
}

void blink_daynight_state(void)
{
    unsigned long kRuntime = elapsed(&daynight_status_blink_started_at);
    switch (daynight_state)
    {
    case DAYNIGHT_STATE_START:
    case DAYNIGHT_STATE_DAY:
    case DAYNIGHT_STATE_DAYWORK:
        ioWrite(MCU_IO_CS1_EN, LOGIC_LEVEL_HIGH);
        break;
    case DAYNIGHT_STATE_NIGHTWORK:
    case DAYNIGHT_STATE_NIGHT:
        ioWrite(MCU_IO_CS1_EN, LOGIC_LEVEL_LOW);
        break;
    case DAYNIGHT_STATE_EVENING_DEBOUNCE:
    case DAYNIGHT_STATE_MORNING_DEBOUNCE:
        if (kRuntime > (DAYNIGHT_BLINK/2) )
        {
            ioToggle(MCU_IO_CS1_EN);

            // set for next toggle 
            daynight_status_blink_started_at += DAYNIGHT_BLINK/2;
        }
        break;
    case DAYNIGHT_STATE_FAIL:
        if (kRuntime > (DAYNIGHT_BLINK/8) )
        {
            ioToggle(MCU_IO_CS1_EN);
            
            // set for next toggle 
            daynight_status_blink_started_at += DAYNIGHT_BLINK/8; 
        }
        break;

    default:
        break;
    }
}

void adc_burst(void)
{
    unsigned long kRuntime= elapsed(&adc_started_at);
    if ((kRuntime) > ((unsigned long)ADC_DELAY_MILSEC))
    {
        enable_ADC_auto_conversion(BURST_MODE);
        adc_started_at += ADC_DELAY_MILSEC; 
    } 
}

int main(void) 
{
    setup();

    while(1) 
    { 
        // use LED to show manager status
        blink_mgr_status();

        // use LED to show daynight_state
        blink_daynight_state();

        // delay between ADC burst
        adc_burst();

        // check if character is available to assemble a command, e.g. non-blocking
        if ( (!command_done) && uart0_available() ) // command_done is an extern from parse.h
        {
            // get a character from stdin and use it to assemble a command
            AssembleCommand(getchar());

            // address is an ascii value, warning: a null address would terminate the command string. 
            StartEchoWhenAddressed(rpu_addr);
        }
        
        // check if a character is available, and if so flush transmit buffer and stop any command that is running.
        // A multi-drop bus can have another device start transmitting after getting an address byte so
        // the first byte is used as a warning, it is the onlly chance to detect a possible collision.
        if ( command_done && uart0_available() )
        {
            // dump the transmit buffer to limit a collision 
            uart0_empty(); 
            initCommandBuffer();
        }
        
        // finish echo of the command line befor starting a reply (or the next part of a reply)
        if ( command_done && uart0_availableForWrite() )
        {
            if ( !echo_on  )
            { // this happons when the address did not match 
                initCommandBuffer();
            }
            else
            {
                if (command_done == 1)  
                {
                    findCommand();
                    command_done = 10;
                }
                
                // do not overfill the serial buffer since that blocks looping, e.g. process a command in 32 byte chunks
                if ( (command_done >= 10) && (command_done < 250) )
                {
                     ProcessCmd();
                }
                else 
                {
                    initCommandBuffer();
                }
            }
        }
    }        
    return 0;
}
