/*****************************************************
This program was produced by the
CodeWizardAVR V1.24.6 Professional
Automatic Program Generator
© Copyright 1998-2005 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com
e-mail:office@hpinfotech.com

Project : 
Version : 
Date    : 18.12.2007
Author  : F4CG                            
Company : F4CG                            
Comments: 


Chip type           : ATmega8
Program type        : Application
Clock frequency     : 8,000000 MHz
Memory model        : Small
External SRAM size  : 0
Data Stack size     : 256
*****************************************************/

#include <mega8.h>
#include <stdlib.h>
#include <ds1820.h>
#include <stdio.h>
#include <delay.h>

#asm
    .equ __w1_port=0x12 ;PORTD
    .equ __w1_bit=0
#endasm
/* quartz crystal frequency [Hz] */
#define xtal 8000000L
/* Baud rate */
#define baud 9600
/* maximum number of DS1820/DS18S20 connected to the bus */
#define MAX_DEVICES 2
unsigned char rom_codes[MAX_DEVICES][9];

unsigned char nowdig=1;
unsigned char nowel=1;
unsigned char digs[5];
int number=0;
unsigned char devices;
unsigned char mindig,maxdig;

#define mdig1 PORTC.4
#define mdig2 PORTC.3
#define mdig3 PORTD.7
#define mdig4 PORTB.0
#define ma PORTB.1
#define mb PORTB.2
#define mc PORTB.3
#define md PORTB.4
#define me PORTB.5
#define mf PORTC.0
#define mg PORTC.1
#define dp PORTC.2

void comp(void){
        char tmp[3];
        unsigned char k=1;
        unsigned char i;
        itoa(number,tmp);         
        if(number<0){k++;}
        if(number>=10 || number<=-10){k++;}
        if(number>=100){k++;}
        mindig=4-k;
        maxdig=3;
        for(i=1;i<=3;i++){
                digs[i]=0x00;
        }
        digs[4]=0b01100011;
        for(i=1;i<=k;i++){
                switch(tmp[i-1]){
                        case '-': digs[i+3-k]=0b01000000;
                                break;
                        case '0': digs[i+3-k]=0b00111111;
                                break;
                        case '1': digs[i+3-k]=0b00000110;
                                break;
                        case '2': digs[i+3-k]=0b01011011;
                                break;
                        case '3': digs[i+3-k]=0b01001111;
                                break;
                        case '4': digs[i+3-k]=0b01100110;
                                break;
                        case '5': digs[i+3-k]=0b01101101;
                                break;
                        case '6': digs[i+3-k]=0b01111101;
                                break;
                        case '7': digs[i+3-k]=0b00000111;
                                break;
                        case '8': digs[i+3-k]=0b01111111;
                                break;
                        case '9': digs[i+3-k]=0b01101111;
                                break;
                }
        }
}

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
// Place your code here
// ÎÁÙÈÉ ÊÀÒÎÄ, ÄÈÃ â èñõîäíîì = 1, ÀÁÑ..=0

/*           
        mdig1=1;
        mdig2=1;
        mdig3=1;
        mdig4=1; 
        ma=0;
        mb=0;
        mc=0;
        md=0;
        me=0;
        mf=0;
        mg=0;
*/
        nowel++;
        if(nowel==8){
                nowel=1;
                nowdig++;     
                if(nowdig==maxdig+1){
                  nowdig=mindig;
                }    
                ///
                //mdig1=1;
                //mdig2=1;
                mdig3=1;
                //mdig4=1; 
                /*
                ma=0;
                mb=0;
                mc=0;
                md=0;
                me=0;
                mf=0;
                mg=0;                
                */
                PORTB=0x01;
                PORTC=0x18;
                ///
        }
        switch(nowdig){
                case 1: mdig1=0;
                        break;
                case 2: mdig2=0;
                        break;
                case 3: mdig3=0;
                        break;
                case 4: mdig4=0;
                        break;
        }             
/*
        switch(nowel){
                case 1: if(digs[nowdig] & 0b00000001){ma=1;}
                        break;
                case 2: if(digs[nowdig] & 0b00000010){mb=1;}
                        break;
                case 3: if(digs[nowdig] & 0b00000100){mc=1;}
                        break;
                case 4: if(digs[nowdig] & 0b00001000){md=1;}
                        break;
                case 5: if(digs[nowdig] & 0b00010000){me=1;}
                        break;
                case 6: if(digs[nowdig] & 0b00100000){mf=1;}
                        break;                           
                case 7: if(digs[nowdig] & 0b01000000){mg=1;}
                        break;
        }
*/             
        /// 
        if(nowel==1){  
                /*
                if(digs[nowdig] & 0b00000001){ma=1;}
                if(digs[nowdig] & 0b00000010){mb=1;}
                if(digs[nowdig] & 0b00000100){mc=1;}
                if(digs[nowdig] & 0b00001000){md=1;}
                if(digs[nowdig] & 0b00010000){me=1;}
                if(digs[nowdig] & 0b00100000){mf=1;}
                if(digs[nowdig] & 0b01000000){mg=1;}
                */
                PORTB|=(digs[nowdig]<<1);
                PORTC|=(digs[nowdig]>>5);
        }
        ///
}
      
// Declare your global variables here

void getds1820(unsigned char device){
      do{
              int temp;
              temp=ds1820_temperature_10(&rom_codes[device][0]);
              number=temp/10;  
      }while(number<-60);
      comp();
}

void main(void)
{
unsigned int ubrtmp;
/* initialize the UART's baud rate */
ubrtmp=xtal/16/baud-1;
UBRRL=ubrtmp/256;
UBRRH=ubrtmp;
/* initialize the UART control register
   TX enabled, no interrupts, 8 data bits */
UCSRA=0x00;
UCSRB=0x04;
UCSRC=0x06;
/* detect how many DS1820/DS18S20 devices
   are connected to the bus and
   store their ROM codes in the rom_codes array */
devices=w1_search(0xf0,rom_codes);
/* if no devices were detected then halt */
//if (devices==0) while (1); /* loop forever */

// Declare your local variables here

// Input/Output Ports initialization
// Port B initialization
// Func7=In Func6=In Func5=In Func4=Out Func3=Out Func2=Out Func1=Out Func0=Out 
// State7=T State6=T State5=T State4=1 State3=1 State2=1 State1=1 State0=0 
PORTB=0x1;
DDRB=0b00111111;

// Port C initialization
// Func6=In Func5=In Func4=In Func3=In Func2=Out Func1=Out Func0=Out 
// State6=T State5=T State4=T State3=T State2=1 State1=1 State0=1 
PORTC=0b00011000;
DDRC=0x1f;

// Port D initialization
// Func7=Out Func6=Out Func5=Out Func4=Out Func3=Out Func2=Out Func1=In Func0=In 
// State7=0 State6=0 State5=0 State4=1 State3=1 State2=1 State1=T State0=T 
PORTD=0x80;
DDRD=0x80;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 2000,000 kHz
TCCR0=0x02;
TCNT0=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer 1 Stopped
// Mode: Normal top=FFFFh
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer 1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=0x00;
TCCR1B=0x00;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer 2 Stopped
// Mode: Normal top=FFh
// OC2 output: Disconnected
ASSR=0x00;
TCCR2=0x00;
TCNT2=0x00;
OCR2=0x00;

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
MCUCR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization 
// TOIE0=1
TIMSK=0x01;

// Analog Comparator initialization
// Analog Comparator: Off
// Analog Comparator Input Capture by Timer/Counter 1: Off
ACSR=0x80;
SFIOR=0x00;

// ADC initialization
// ADC Clock frequency: 125,000 kHz
// ADC Voltage Reference: AREF pin
// Only the 8 most significant bits of
// the AD conversion result are used
ADMUX=0x00;
ADCSRA=0x00;

// Global enable interrupts
#asm("sei")

while (1)
      {
        getds1820(0);
        delay_ms(2000);
        getds1820(1);        
        delay_ms(2000);        
      };
}
               