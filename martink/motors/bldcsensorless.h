/*
bldc sensorless driver 0x01

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef BLDCSENSORLESS_H_
#define BLDCSENSORLESS_H_

//fet driver port
#define BLDCSENSORLESS_FETUHDDR DDRD
#define BLDCSENSORLESS_FETUHPORT PORTD
#define BLDCSENSORLESS_FETUHINPUT PD2
#define BLDCSENSORLESS_FETULDDR DDRD
#define BLDCSENSORLESS_FETULPORT PORTD
#define BLDCSENSORLESS_FETULINPUT PD3
#define BLDCSENSORLESS_FETVHDDR DDRD
#define BLDCSENSORLESS_FETVHPORT PORTD
#define BLDCSENSORLESS_FETVHINPUT PD4
#define BLDCSENSORLESS_FETVLDDR DDRD
#define BLDCSENSORLESS_FETVLPORT PORTD
#define BLDCSENSORLESS_FETVLINPUT PD5
#define BLDCSENSORLESS_FETWHDDR DDRD
#define BLDCSENSORLESS_FETWHPORT PORTD
#define BLDCSENSORLESS_FETWHINPUT PD0
#define BLDCSENSORLESS_FETWLDDR DDRD
#define BLDCSENSORLESS_FETWLPORT PORTD
#define BLDCSENSORLESS_FETWLINPUT PD7

//fet driver port status
#define BLDCSENSORLESS_FETUL1 BLDCSENSORLESS_FETULPORT |=  (1<<BLDCSENSORLESS_FETULINPUT)
#define BLDCSENSORLESS_FETUL0 BLDCSENSORLESS_FETULPORT &= ~(1<<BLDCSENSORLESS_FETULINPUT)
#define BLDCSENSORLESS_FETUH1 BLDCSENSORLESS_FETUHPORT |=  (1<<BLDCSENSORLESS_FETUHINPUT)
#define BLDCSENSORLESS_FETUH0 BLDCSENSORLESS_FETUHPORT &= ~(1<<BLDCSENSORLESS_FETUHINPUT)
#define BLDCSENSORLESS_FETVL1 BLDCSENSORLESS_FETVLPORT |=  (1<<BLDCSENSORLESS_FETVLINPUT)
#define BLDCSENSORLESS_FETVL0 BLDCSENSORLESS_FETVLPORT &= ~(1<<BLDCSENSORLESS_FETVLINPUT)
#define BLDCSENSORLESS_FETVH1 BLDCSENSORLESS_FETVHPORT |=  (1<<BLDCSENSORLESS_FETVHINPUT)
#define BLDCSENSORLESS_FETVH0 BLDCSENSORLESS_FETVHPORT &= ~(1<<BLDCSENSORLESS_FETVHINPUT)
#define BLDCSENSORLESS_FETWL1 BLDCSENSORLESS_FETWLPORT |=  (1<<BLDCSENSORLESS_FETWLINPUT)
#define BLDCSENSORLESS_FETWL0 BLDCSENSORLESS_FETWLPORT &= ~(1<<BLDCSENSORLESS_FETWLINPUT)
#define BLDCSENSORLESS_FETWH1 BLDCSENSORLESS_FETWHPORT |=  (1<<BLDCSENSORLESS_FETWHINPUT)
#define BLDCSENSORLESS_FETWH0 BLDCSENSORLESS_FETWHPORT &= ~(1<<BLDCSENSORLESS_FETWHINPUT)

//running operations can also be implemented in 1 instruction, for example setting all a motor output on the sample port, a running instruction can be just a PORT setting
//running free spin
#define BLDCSENSORLESS_RUNS BLDCSENSORLESS_FETUH0;BLDCSENSORLESS_FETUL0; BLDCSENSORLESS_FETVH0;BLDCSENSORLESS_FETVL0; BLDCSENSORLESS_FETWH0;BLDCSENSORLESS_FETWL0; //00-00-00 (U-V-W)
//running break
#define BLDCSENSORLESS_RUNB BLDCSENSORLESS_FETUH0;BLDCSENSORLESS_FETUL1; BLDCSENSORLESS_FETVH0;BLDCSENSORLESS_FETVL1; BLDCSENSORLESS_FETWH0;BLDCSENSORLESS_FETWL1; //10-10-10 (U-V-W)
//running phases
#define BLDCSENSORLESS_RUN1 BLDCSENSORLESS_FETUH1;BLDCSENSORLESS_FETUL0; BLDCSENSORLESS_FETVH0;BLDCSENSORLESS_FETVL1; BLDCSENSORLESS_FETWH0;BLDCSENSORLESS_FETWL0; //10-01-00 (U-V-W)
#define BLDCSENSORLESS_RUN2 BLDCSENSORLESS_FETUH1;BLDCSENSORLESS_FETUL0; BLDCSENSORLESS_FETVH0;BLDCSENSORLESS_FETVL0; BLDCSENSORLESS_FETWH0;BLDCSENSORLESS_FETWL1; //10-00-01 (U-V-W)
#define BLDCSENSORLESS_RUN3 BLDCSENSORLESS_FETUH0;BLDCSENSORLESS_FETUL0; BLDCSENSORLESS_FETVH1;BLDCSENSORLESS_FETVL0; BLDCSENSORLESS_FETWH0;BLDCSENSORLESS_FETWL1; //00-10-01 (U-V-W)
#define BLDCSENSORLESS_RUN4 BLDCSENSORLESS_FETUH0;BLDCSENSORLESS_FETUL1; BLDCSENSORLESS_FETVH1;BLDCSENSORLESS_FETVL0; BLDCSENSORLESS_FETWH0;BLDCSENSORLESS_FETWL0; //01-10-00 (U-V-W)
#define BLDCSENSORLESS_RUN5 BLDCSENSORLESS_FETUH0;BLDCSENSORLESS_FETUL1; BLDCSENSORLESS_FETVH0;BLDCSENSORLESS_FETVL0; BLDCSENSORLESS_FETWH1;BLDCSENSORLESS_FETWL0; //01-00-10 (U-V-W)
#define BLDCSENSORLESS_RUN6 BLDCSENSORLESS_FETUH0;BLDCSENSORLESS_FETUL0; BLDCSENSORLESS_FETVH0;BLDCSENSORLESS_FETVL1; BLDCSENSORLESS_FETWH1;BLDCSENSORLESS_FETWL0; //00-01-10 (U-V-W)

//define positions for running phases depending on hall sensor/position CW
#define BLDCSENSORLESS_101CW BLDCSENSORLESS_RUN6
#define BLDCSENSORLESS_100CW BLDCSENSORLESS_RUN1
#define BLDCSENSORLESS_110CW BLDCSENSORLESS_RUN2
#define BLDCSENSORLESS_010CW BLDCSENSORLESS_RUN3
#define BLDCSENSORLESS_011CW BLDCSENSORLESS_RUN4
#define BLDCSENSORLESS_001CW BLDCSENSORLESS_RUN5
//define positions for running phases depending on hall sensor/position CCW
#define BLDCSENSORLESS_101CCW BLDCSENSORLESS_RUN5
#define BLDCSENSORLESS_100CCW BLDCSENSORLESS_RUN4
#define BLDCSENSORLESS_110CCW BLDCSENSORLESS_RUN3
#define BLDCSENSORLESS_010CCW BLDCSENSORLESS_RUN2
#define BLDCSENSORLESS_011CCW BLDCSENSORLESS_RUN1
#define BLDCSENSORLESS_001CCW BLDCSENSORLESS_RUN6

//startup delay, the startup delay for every step is given by startup delay array*LDCSENSORLESS_STARTUPDELAYMULT
#define BLDCSENSORLESS_STARTUPDELAYS {2000,1900,1800,1600,1400,1200,1000,800,600,550,500,480,460,440,420,400,380,360,355,350,345,340,335,330,325,320,315,310,305,300}
#define BLDCSENSORLESS_STARTUPCOMMUTATIONS 29 //number of startup commutations
#define BLDCSENSORLESS_STARTUPDELAYMULT 5 //startup delay multiplier

//direction definitions
#define BLDCSENSORLESS_DIRECTIONCW 1
#define BLDCSENSORLESS_DIRECTIONCCW 2

//emit startup sound
#define BLDCSENSORLESS_STARTUPSOUND 1

//freq = FCPU/1+(prescaler*ICR1)
//note: ICR1 can not be small, because we have some commands to execute during the interrupt routine
//20000Hz = 8000000Hz/(1+(1*399))
#define TIMER1_ICR1 199
#define TIMER1_PRESCALER (1<<CS11) //8 prescaler

//setup speed interval
#define BLDCSENSORLESS_SPEEDMIN 599 //minimum speed, setup ICR1 higher for minimum speed
#define BLDCSENSORLESS_SPEEDMAX 199 //maximum speed, setup ICR1 lower for minimum speed

#define BLDCSENSORLESS_ZCERRORS 100 //number of zc threshold reading error befor emit a motor startup

//comparator port
#define BLDCSENSORLESS_COMPDDR DDRD
#define BLDCSENSORLESS_COMPPORT PORTD
#define BLDCSENSORLESS_COMPINPUT PD6

//admux table
#define BLDCSENSORLESS_ADMUXCW {2,0,1,2,0,1}
#define BLDCSENSORLESS_ADMUXCCW {1,0,2,1,0,2}

//zc bemf threshold value
#define BLDCSENSORLESS_ZCTHRESHOLD 150

//adc definitions
#define BLDCSENSORLESS_ADCINCLUDE "adc.h" //include file
#define BLDCSENSORLESS_ADCSETCHANNEL(x) adc_setchannel(x) //set channel to read
#define BLDCSENSORLESS_ADCREADSEL adc_readsel() //read adc selected channel


//functions
extern uint16_t bldcsensorless_getspeed(void);
extern void bldcsensorless_setspeed(uint16_t speed);
extern uint8_t bldcsensorless_getdirection(void);
extern void bldcsensorless_setdirection(uint8_t direction);
extern void bldcsensorless_setstop(void);
extern void bldcsensorless_setstart(void);
extern void bldcsensorless_soundercyle(uint16_t ontime, uint16_t offtime);
extern void bldcsensorless_sounder(uint8_t repetitions, uint8_t duration, uint16_t ontime, uint16_t offtime);
extern void bldcsensorless_poweronsound(void);
extern void bldcsensorless_init(void);
extern void bldcsensorless_runstep(uint8_t step);
extern void bldcsensorless_startupmotor(void);

#endif
