#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <stdlib.h>

#include "OWI_Config.h"
#include "OWIHighLevelFunctions.h"
#include "OWIBitFunctions.h"
#include "OWIcrc.h"

// DS18B20 commands
#define DS18B20_CONVERT_T                0x44
#define DS18B20_READ_SCRATCHPAD          0xbe

// output number for sensor
#define OWI_BUS OWI_PIN_0
// OWI port is set in OWI_CONFIG.h

/* maximum number of DS1820/DS18S20 connected to the bus */
#define MAX_DEVICES 2
static OWI_device rom_codes[MAX_DEVICES];

static unsigned char nowdig=1;
static unsigned char nowel=1;
static unsigned char digs[5];
static int number=0;
static int temp[2]={0,0};
static unsigned char devices;
static unsigned char mindig,maxdig;
static unsigned char nowTempId=0;
static int nowTempCounter=0;

#define mdig12_port PORTC
#define mdig1_pin 4
#define mdig2_pin 3

#define mdig3_port PORTD
#define mdig3_pin 7

#define mdig4_port PORTB
#define mdig4_pin 0

#define mae_port PORTB
#define ma_pin 1
#define mb_pin 2
#define mc_pin 3
#define md_pin 4
#define me_pin 5

#define mfgdp_port PORTC
#define mf_pin 0
#define mg_pin 1
#define dp_pin 2

void comp(void){
        char tmp[3];
        unsigned char k=1;
        unsigned char i;
        if(number>999){
            number=999;
        }
        if(number<-99){
            number=-99;
        }
        itoa(number,tmp, 10);
        if(number<0){k++;}
        if(number>=10 || number<=-10){k++;}
        if(number>=100){k++;}
        mindig=4-k;
        maxdig=3;
        for(i=1;i<=3;i++){
                digs[i]=0x00;
        }
        digs[4]=0x63;//01100011;    // C
        for(i=1;i<=k;i++){
                switch(tmp[i-1]){
                        case '-': digs[i+3-k]=0x40;//01000000;
                                break;
                        case '0': digs[i+3-k]=0x3f;//00111111;
                                break;
                        case '1': digs[i+3-k]=0x06;//00000110;
                                break;
                        case '2': digs[i+3-k]=0x5b;//01011011;
                                break;
                        case '3': digs[i+3-k]=0x4f;//01001111;
                                break;
                        case '4': digs[i+3-k]=0x66;//01100110;
                                break;
                        case '5': digs[i+3-k]=0x6d;//01101101;
                                break;
                        case '6': digs[i+3-k]=0x7d;//01111101;
                                break;
                        case '7': digs[i+3-k]=0x07;//00000111;
                                break;
                        case '8': digs[i+3-k]=0x7f;//01111111;
                                break;
                        case '9': digs[i+3-k]=0x6f;//01101111;
                                break;
                }
        }
}

// Timer 0 overflow interrupt service routine
ISR(TIMER0_OVF_vect){
    nowTempCounter++;
    if(nowTempCounter>=7800){   // 1 sec
        nowTempCounter=0;
        nowTempId++;
        if(nowTempId>1){
            nowTempId=0;
        }   
        number=temp[nowTempId];
        comp();
    }

    nowel++;
    if(nowel>7){
        nowel=1;
        nowdig++;
        if(nowdig>maxdig){
            nowdig=mindig;
        }

        // clear leds

        // Common CATHOD, Digital initial = 1, ABC..=0

        mdig12_port|=((1<<mdig1_pin)|(1<<mdig2_pin));
        mdig3_port|=(1<<mdig3_pin);
        mdig4_port|=(1<<mdig4_pin);

        mae_port&=(~(1<<ma_pin)|(1<<mb_pin)|(1<<mc_pin)|(1<<md_pin)|(1<<me_pin));
        mfgdp_port&=(~(1<<mf_pin)|(1<<mg_pin));
    }

    // select digit
    switch(nowdig){
    case 1: mdig12_port&=(~(1<<mdig1_pin));break;
    case 2: mdig12_port&=(~(1<<mdig2_pin));break;
    case 3: mdig3_port&=(~(1<<mdig3_pin));break;
    case 4: mdig4_port&=(~(1<<mdig4_pin));break;
    }

    if(nowel==1){
        unsigned char dig=digs[nowdig];

        // light up elements
        mae_port|=((((dig >> 0)&1)<<ma_pin)|
                   (((dig >> 1)&1)<<mb_pin)|
                   (((dig >> 2)&1)<<mc_pin)|
                   (((dig >> 3)&1)<<md_pin)|
                   (((dig >> 4)&1)<<me_pin)
                   );
        mfgdp_port|=((((dig >> 5)&1)<<mf_pin)|
                     (((dig >> 6)&1)<<mg_pin)
                     );
    }
}

void getds1820(unsigned char device){
      int tmp;
      tmp=ds1820_temperature_10(&rom_codes[device][0]);      
      if(tmp>-999){
        tmp=tmp/10;
        temp[device]=tmp;
      }  
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
        delay_ms(3000);        
        getds1820(1);        
        delay_ms(3000);        
      };
}
               
