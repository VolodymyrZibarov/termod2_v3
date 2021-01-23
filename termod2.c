#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <stdlib.h>

#include "OWIPolled.h"
#include "OWIHighLevelFunctions.h"
#include "OWIBitFunctions.h"
#include "OWIcrc.h"

// DS18B20 commands
#define DS18B20_CONVERT_T                0x44
#define DS18B20_READ_SCRATCHPAD          0xbe

// output number for sensor
#define OWI_BUS OWI_PIN_0   // OWI port is set in OWIPolled.h


/* maximum number of DS1820/DS18S20 connected to the bus */
#define MAX_DEVICES 2
static OWI_device rom_codes[MAX_DEVICES];

static unsigned char nowdig=1;
static unsigned char showDigCounter=0;
static unsigned char digs[5]={0x00,0xff,0xff,0xff,0xff};    // using 1-4 indexes
static unsigned char mindig=1;
static unsigned char maxdig=4;
static int number=0;
static int temp[2]={0x7fff,0x7fff};
static unsigned char nowTempId=0;
static int tempIndexUpdateCounter=0;
static unsigned char elementsCountInDig=8;

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

//#define TESTING

#define SAME_BRIGHTNESS_ALWAYS_PROCESS_ALL_NUMBERS

#ifdef TESTING
    unsigned char testCounter=0;
#endif

#ifndef TESTING
    #define TEMP_CHANGE_PERIOD 7800
#else
    #define TEMP_CHANGE_PERIOD 1950 // 0.5 s
#endif
void comp(void){

#ifdef TESTING
    if(testCounter%1==0){
        digs[1]=digs[2];
        digs[2]=digs[3];
        digs[3]=digs[4];
    }
    digs[4]=0x00;
    for(unsigned char i=0;i<(testCounter%9);i++){
        digs[4]=(digs[4]<<1)|1;
    }
    testCounter++;
    return;
#endif

#ifndef SAME_BRIGHTNESS_ALWAYS_PROCESS_ALL_NUMBERS
    if(nowTempId==0){
        digs[4]=0x01;
        maxdig=4;
    }else{
        maxdig=3;
    }
#else
    digs[1]=0;
    digs[2]=0;
    //digs[3]=0;// this will be always set, we don't need to set it to 0
    if(nowTempId==0){
        digs[4]=0x01;   // upper dush '~' - for outdoor temperature
    }else{
        digs[4]=0x08;   // underscor '_' - for indoors temperatur
    }
#endif


    if(number==0x7fff){
#ifndef SAME_BRIGHTNESS_ALWAYS_PROCESS_ALL_NUMBERS
        mindig=2;
#endif
        digs[2]=0x40;
        digs[3]=0x40;
        return;
    }
    char tmp[4];
    tmp[0]=0;
    tmp[1]=0;
    tmp[2]=0;
    tmp[3]=0;
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
#ifndef SAME_BRIGHTNESS_ALWAYS_PROCESS_ALL_NUMBERS
    mindig=4-k;
#endif

    // LED names:
    //
    //    ---a---
    //   |       |
    //   f       b
    //   |       |
    //    ---g---
    //   |       |
    //   e       c
    //   |       |
    //    ---d---  dp

    // bit numbers:
    //
    //    ---0---
    //   |       |
    //   5       1
    //   |       |
    //    ---6---
    //   |       |
    //   4       2
    //   |       |
    //    ---3---  7


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
        default: digs[i+3-k]=0x08; //'_'
            break;
        }
    }
}

void timer0OVF_func(){
    tempIndexUpdateCounter++;

    showDigCounter++;
    if(showDigCounter==elementsCountInDig){ // elementsCountInDig will be 8 at max
        // PWM this
        // if only one element (LED) of a 7-elemeent symbol is light-up currently, we powering it only 1/8 of our duty cycle, to not overcurrent the LED
        // and for symbols to maintain constant brightness independently of symbol shown
        mae_port&=~((1<<ma_pin)|(1<<mb_pin)|(1<<mc_pin)|(1<<md_pin)|(1<<me_pin));
        mfgdp_port&=~((1<<mf_pin)|(1<<mg_pin)|(1<<dp_pin));
    }
    if(showDigCounter==8){
        showDigCounter=0;
        nowdig++;
        if(nowdig>maxdig){
            if(tempIndexUpdateCounter>TEMP_CHANGE_PERIOD){
                tempIndexUpdateCounter=0;
                nowTempId++;
                if(nowTempId>1){
                    nowTempId=0;
                }
                number=temp[nowTempId];
                comp();
            }
            nowdig=mindig;
        }

        // clear leds

        // Common CATHOD, Digs initial = 1, ABC.. initial = 0

        mdig12_port|=((1<<mdig1_pin)|(1<<mdig2_pin));
        mdig3_port|=(1<<mdig3_pin);
        mdig4_port|=(1<<mdig4_pin);

        // not clearing elements pins here, because we cleared them before

        // select digit
        switch(nowdig){
        case 1: mdig12_port&=~(1<<mdig1_pin);break;
        case 2: mdig12_port&=~(1<<mdig2_pin);break;
        case 3: mdig3_port&=~(1<<mdig3_pin);break;
        case 4: mdig4_port&=~(1<<mdig4_pin);break;
        }

        unsigned char dig=digs[nowdig];

        if(dig!=0x00){
            // light up elements
            mae_port|=((((dig >> 0)&1)<<ma_pin)|
                       (((dig >> 1)&1)<<mb_pin)|
                       (((dig >> 2)&1)<<mc_pin)|
                       (((dig >> 3)&1)<<md_pin)|
                       (((dig >> 4)&1)<<me_pin)
                       );
            mfgdp_port|=((((dig >> 5)&1)<<mf_pin)|
                         (((dig >> 6)&1)<<mg_pin)|
                         (((dig >> 7)&1)<<dp_pin)
                         );

            elementsCountInDig=
                    ((dig >> 0)&1)+
                    ((dig >> 1)&1)+
                    ((dig >> 2)&1)+
                    ((dig >> 3)&1)+
                    ((dig >> 4)&1)+
                    ((dig >> 5)&1)+
                    ((dig >> 6)&1)+
                    ((dig >> 7)&1);
        }else{
            elementsCountInDig=0;
        }
    }
}

// Timer 0 overflow interrupt service routine
ISR(TIMER0_OVF_vect){
    timer0OVF_func();
}

void getds1820(unsigned char device){
    OWI_DetectPresence(OWI_BUS);
    OWI_MatchRom(rom_codes[device].id,OWI_BUS);
    OWI_SendByte(DS18B20_CONVERT_T ,OWI_BUS);
    // wait for temperature convertation to finish
    while (!OWI_ReadBit(OWI_BUS)){}

    OWI_DetectPresence(OWI_BUS);
    OWI_MatchRom(rom_codes[device].id,OWI_BUS);
    OWI_SendByte(DS18B20_READ_SCRATCHPAD, OWI_BUS);
    char scratchpad[2];
    scratchpad[0] = OWI_ReceiveByte(OWI_BUS);
    scratchpad[1] = OWI_ReceiveByte(OWI_BUS);
    // todo test for negative values
    cli();

    // we're using DS1820 here (NOT DS18B20).
    // For DS1820 temperature is represented in terms of 1/2 C (it's 1/16 for DS18B20)

    temp[device]=(*((int*)scratchpad))/2;

    sei();
}

int main(void)
{
    // not using UART
    UCSRA=0x20;
    UCSRB=0x00;
    UCSRC=0x86;

    PORTB=0x01;
    DDRB=0x3f;//0b00111111;

    PORTC=0x18; //0b00011000;
    DDRC=0x1f;

    PORTD=0x80;
    DDRD=0x80;

    // timer 0 - working, one ovf interrupt
    TCCR0=0x02; // prescaler: 1/8 => 1Mhz timer clock => 3906 overflows per second
    TCNT0=0x00;

    // timer 1 - off
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

    // timer 2 - off
    ASSR=0x00;
    TCCR2=0x00;
    TCNT2=0x00;
    OCR2=0x00;

    // no external interrupts
    MCUCR=0x00;

    // TOIE0=1
    TIMSK=0x01;

    // Input comparator - off
    ACSR=0x80;
    SFIOR=0x00;

    // ADC - off
    ADMUX=0x00;
    ADCSRA=0x00;

    _delay_ms(1000);

    // Global enable interrupts
    sei();

    OWI_Init(OWI_BUS);

    unsigned char devicesFound=0;
    while(1){
        unsigned char res=OWI_SearchDevices(rom_codes,2,OWI_BUS,&devicesFound);
        if(res!=SEARCH_SUCCESSFUL){
            temp[0]=333;
            temp[1]=-33;
            continue;
        }
        break;
    }

    while (1){
        for(unsigned char i=0;i<devicesFound;i++){
            getds1820(i);
            _delay_ms(5000);    // test temperature rarely to reduce sensor heating
        }
    };

    return 0;
}

