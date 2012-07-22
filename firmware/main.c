/* Name: main.c
 * Project: hid-solar
 * Author: Florian Knodt, Christian Starkjohann
 * Copyright: (c) 2012 by Florian Knodt, 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <string.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */

uchar buffer[7];
static int dataSent;

 
/* ADC initialisieren */
void ADC_Init(void) {
 
  uint16_t result;
 
  // internal reference voltage
  ADMUX = (1<<REFS0);
  
  // single conversion
  ADCSRA = (1<<ADPS1) | (1<<ADPS0);     // prescaler
  ADCSRA |= (1<<ADEN);                  // ADC on

  ADCSRA |= (1<<ADSC);                  // Dummy readout
  while (ADCSRA & (1<<ADSC) ) {}        // 
  result = ADCW;
}
 
/* ADC single */
uint16_t ADC_Read( uint8_t channel )
{
  // channel
  ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
  ADCSRA |= (1<<ADSC);            // single conversion
  while (ADCSRA & (1<<ADSC) ) {}  // wait
  return ADCW;                    // return
}
 
/* ADC multi (average) */
uint16_t ADC_Read_Avg( uint8_t channel, uint8_t nsamples )
{
  uint32_t sum = 0;
  uint8_t i;
 
  for (i = 0; i < nsamples; ++i ) {
    sum += ADC_Read( channel );
  }
 
  return (uint16_t)( sum / nsamples );
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[22] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x80,                    //   REPORT_COUNT (128)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of 128
 * opaque data bytes.
 */

/* The following variables store the status of the current data transfer */
static uchar    currentAddress;
static uchar    bytesRemaining;

/* ------------------------------------------------------------------------- */

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionRead(uchar *data, uchar len)
{
    /*if(len > bytesRemaining)
        len = bytesRemaining;
    //eeprom_read_block(data, (uchar *)0 + currentAddress, len);
    strcpy(data, buffer);
    currentAddress += len;
    bytesRemaining -= len;
    return len;*/
    uchar i;

    for(i = 0; dataSent < 1024 && i < len && i < 6; i++, dataSent++)
        data[i] = buffer[i];

    // terminate the string if it's the last byte sent
    if(i && dataSent == 1024)
        data[i-1] = 0; // NULL

    return i; // equals the amount of bytes written
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionWrite(uchar *data, uchar len)
{
    wdt_enable(WDTO_1S);
    while(true) {} //Reset
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* HID class request */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* since we have only one report type, we can ignore the report-ID */
            bytesRemaining = 128;
            currentAddress = 0;
            return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
            /* since we have only one report type, we can ignore the report-ID */
            bytesRemaining = 128;
            currentAddress = 0;
            return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
        }
    }else{
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

int main(void)
{
uchar   i;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    uint16_t adcval;
    DDRC |= (1 << DDC0) | (1 << DDC1);
    DDRB |= (1 << DDB3);
    buffer[0]=0x00;
    buffer[1]=0x00;
    buffer[2]=0x00;
    buffer[3]=0x00;
    buffer[4]=0x00;
    buffer[5]=0x00;
    buffer[6]=0x00;
    ADC_Init();
    odDebugInit();
    DBG1(0x00, 0, 0);       /* debug output: main starts */
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();
    DBG1(0x01, 0, 0);       /* debug output: main loop starts */
    for(;;){                /* main event loop */
        DBG1(0x02, 0, 0);   /* debug output: main loop iterates */
        wdt_reset();
        adcval = ADC_Read_Avg(2, 4);    //Amp1 -> Load
        buffer[0] = (uint8_t)adcval; 
        buffer[1] = (adcval >> 8 );
        adcval = ADC_Read_Avg(3, 4);    //Amp2 -> Battery
        if(adcval > 512) {              //Charging
                PORTC |= (1 << PC0); 
        }else{                          //Discharging
                PORTC &= ~(1 << PC0);
        }
        buffer[2] = (uint8_t)adcval; 
        buffer[3] = (adcval >> 8 );
        adcval = ADC_Read_Avg(4, 4);    //Voltage -> Battery
        if(adcval <= 794) {             //11V - switch to AC
                PORTC |= (1 << PC1); 
                PORTB |= (1 << PB3);
        }else if(adcval >= 866) {       //12V - back to solar
                PORTC &= ~(1 << PC1);
                PORTB &= ~(1 << PB3);
        }
        buffer[4] = (uint8_t)adcval; 
        buffer[5] = (adcval >> 8 );
        usbPoll();
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
