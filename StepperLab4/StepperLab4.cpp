/*
  StepperLab3.cpp - Stepper Library to use with L293 or other H-Bridge chip.
  Created by Martin Nawrath, KHM 2010.
  http://interface.khm.de
  Released into the public domain.
  
  StepperLab4.cpp
  Updated stepper library:
  - modified for STEP/DIR drivers
  - smooth acceleration/decceleration with Timer
  - position data stored in int32
  by Adam Orban, 2018
  
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "StepperLab4.h"

bool _dir_inverz;   // DIR pin sima vagy negalt uzemu
bool _step_inverz;  // STEP pin sima vagy negalt uzemu (true - negalt)

static volatile byte _f_int;   //teszteli, hogy fut-e a megszakitas. a megszakitas vegen 1-re van allitva
static volatile byte _f_ramp;  //0-uresjarat, 1-gyorsulunk, 2-konstans sebesseg, 3-lassulunk

static volatile unsigned long _ddsreg; // szamlalo, hogy mikor jon a kovetkezo motor leptetes

static volatile long _absoluteStep; // mozgatas celpozicioja
static volatile long _currentStep;  // mozgatas aktualis pozicioja
static volatile long _differenceSteps ; // hatralevo lepesek szama a celpozicioig

static volatile bool _stepReadyStatus; //false - mozog a tengely, true - all a tengely

static volatile int _dspeed; //pillanatnyi sebesseg
static volatile int _setSpeed; //megcelzott max. sebesseg

unsigned long _rampSteps; // hany lepesbol all a gyorsulasi ciklus
unsigned long _rampThres; //a mozgasi tartomany fele, ha idaig nem sikerult felgyorsulni, akkor lassulasba kell atvaltani!
unsigned long _stepsMade; //hany lepest tettunk meg a mozgas kezdete ota

unsigned int _hbeat_led = 10000;

// constructor
StepperLab4::StepperLab4(){};
// attach
void StepperLab4::attach(bool step_inverz, bool dir_inverz)
{

	//pinMode(A1, OUTPUT);  //PC1 - DIR
	//pinMode(A2, OUTPUT);  //PC2 - STEP
	//pinMode(PD5, OUTPUT); //RUN led
	//pinMode(PD6, OUTPUT); //HBEAT led
	
	DDRC |= B00000110;	//PC1 (DIR) es PC2 (STEP) output
	DDRD |= B01100000;	//PD5 (RUN led) es PD6 (HBEAT led) output	
	
	// init variables
	_dir_inverz = dir_inverz;
	_step_inverz = step_inverz;
	
	_currentStep = 0;
	_absoluteStep = 0;
	
	_stepReadyStatus = true;
	
	StepperLab4::_setupTimer();
	
}
//****************************************************************************
void StepperLab4::_setupTimer() {
    
  // TIMER2 - lepteto impulzus idozito
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  // 50000 Hz (20000000/((49+1)*8))
  OCR2A = 49;
  // CTC
  TCCR2A |= (1 << WGM21);
  // Prescaler 8
  TCCR2B |= (1 << CS21);
  // Output Compare Match A Interrupt Enable
  TIMSK2 |= (1 << OCIE2A);

}


//****************************************************************************
// Sebesseg es gyorsulas beallitasa
// Speed - sebesseg 100..10000 step/sec kozott
// Accel - gyorsulas 100..30000 step/sec2 kozott
bool StepperLab4::setSpeedAccel(int speed, int accel)
{
	//var, amig az aktualis interrupt befejezodik
	_f_int=0;
	while(_f_int==0); // critical section, wait untlt interrupt is done
	//ha eppen fut a szervo, akkor nem lehet allitani: kilep!
	if (_stepReadyStatus == false) return false;
	//ha a sebesseg tul kicsi, akkor is hiba van
	if (speed < 100) return false;
	//ha a sebesseg tul nagy, akkor is hiba van
	if (speed > 10000) return false;
	//ha a gyorsulas tul kicsi, akkor is hiba van
	if (accel < 100) return false;
	//ha a gyorsulas tul nagy, akkor is hiba van
	if (accel > 30000) return false;
	
	_setSpeed = speed;
	
	unsigned int szamlalo = 2500000 / accel;
	szamlalo--;
	
	// TIMER1 - gyorsulas idozito
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // 20000000/((szamlalo+1)*8)
    OCR1A = szamlalo;
    // CTC
    TCCR1B |= (1 << WGM12);
    // Prescaler 8
    TCCR1B |= (1 << CS11);
    // Output Compare Match A Interrupt Enable
    TIMSK1 |= (1 << OCIE1A);
	
	return true;
}
//****************************************************************************
// set relative steps
void StepperLab4::relativeSteps(long step){

	_f_int=0;
	while(_f_int==0); // critical section, wait untlt interrupt is done
	
	_absoluteStep = _absoluteStep + step;
	_stepReadyStatus = false;
	
	_dspeed = 1;
	_stepsMade=0;
	
	_rampThres = abs(_absoluteStep - _currentStep) / 2;
	_rampSteps = 0;
	_f_ramp=1;

}
//****************************************************************************
// set absolute steps
void StepperLab4::absoluteSteps(long step){
	// critical section, wait untlt interrupt is done

	_f_int=0;
	 while(_f_int==0);
	
	_absoluteStep = step;
	_stepReadyStatus = false; 
	
	_dspeed = 1;
	_stepsMade=0;
	
	_rampThres = abs(_absoluteStep - _currentStep) / 2;
	_rampSteps = 0;
	_f_ramp=1;
	
}

//****************************************************************************
// set reference position
void  StepperLab4::setRefPosition(long apos){
	
	_f_int=0;
	while(_f_int==0);
	
	_f_ramp = 0;
	_dspeed = 1;
	_stepsMade=0;
	_rampSteps = 0;
	
	_absoluteStep = apos;
	_currentStep = apos;
	
}
//****************************************************************************
// get actual position
long StepperLab4::getActualPosition(void){
    _f_int=0;
    while(_f_int==0); // critical section, wait untlt interrupt is done
	
	return _currentStep;
}
//****************************************************************************
// get status step ready
bool StepperLab4::stepReady(void){

	_f_int=0;
	while(_f_int==0); // critical section, wait untlt interrupt is done
	
	return _stepReadyStatus;
}

// megallitas lassulassal
void StepperLab4::soft_stop(void)
{
	_f_int=0;
	while(_f_int==0); // critical section, wait untlt interrupt is done
	
	//mar allunk, nincs miert lassitani
	if (_stepReadyStatus == true) return;
	
	//ha eppen gyorsulunk...
	if (_f_ramp == 1)
	{
		if (_absoluteStep > _currentStep)
			_absoluteStep = _currentStep + _stepsMade;
		else
			_absoluteStep = _currentStep - _stepsMade;
		_f_ramp = 3;
	}
	
	//ha teljes sebesseggel haladunk...
	if (_f_ramp == 2)
	{
		if (_absoluteStep > _currentStep)
			_absoluteStep = _currentStep + _rampSteps;
		else
			_absoluteStep = _currentStep - _rampSteps;
		_f_ramp = 3;
	}
	
	//ha lassulunk: nincs teendo, mindjart megall magatol!
	if (_f_ramp == 3) return;
	
}
	
// megallitas azonnal
void StepperLab4::hard_stop(void)
{
	_f_int=0;
    while(_f_int==0); // critical section, wait untlt interrupt is done
	
	//mar allunk, nincs miert lassitani
	if (_stepReadyStatus == true) return;
	
	_absoluteStep = _currentStep;
	_f_ramp = 0;
}

//****************************************************************************
//****************************************************************************
//****************************************************************************
//****************************************************************************

SIGNAL (TIMER1_COMPA_vect) {
  if (_f_ramp == 0) return;
  if (_f_ramp == 2) return;
  // gyorsulunk, de meg nem ertuk el a celsebesseget
  if ((_f_ramp == 1) && (_dspeed < _setSpeed)) _dspeed++; 
  // gyorsulunk, es pont elertuk a celsebesseget
  if ((_f_ramp == 1) && (_dspeed >= _setSpeed))
  {
	  _f_ramp = 2;
	  _rampSteps = _stepsMade;
  } 
  // lassulunk, de meg nem ertuk el az 1 step/sec sebesseget
  if ((_f_ramp == 3) && (_dspeed > 1)) _dspeed--;
  // lassulunk, es elertuk az 1 step/sec sebesseget
  if ((_f_ramp == 3) && (_dspeed <= 1)) _f_ramp = 0;
  
}

SIGNAL (TIMER2_COMPA_vect) 
{ 
	
// szamlalo novelese az aktualis sebesseggel
_ddsreg= _ddsreg+_dspeed;

// STEP pin kikapcsolasa
if (_step_inverz) PORTC |= B00000100; else PORTC &= B11111011;

// HBeat led kezelese
_hbeat_led--;
if (_hbeat_led == 0) 
{	
	PORTD ^= B01000000;
	_hbeat_led = 10000;
}

if (_ddsreg >= 50000)  
{ 
	_ddsreg=0;
	// milyen messze vagyunk a celtol
	_differenceSteps = _absoluteStep - _currentStep;

	// meg nem ertuk el a celt, ezert mozogni kell valamerre
	if(_differenceSteps != 0)
	{
		// lepesszamlalo inkrementalasa
		_stepsMade++;
		
		// gyorsulas-lassulas kezelese
		if (_f_ramp!= 0)
		{
			switch (_f_ramp)
			{
				case 1:
					// nem sikerult felgyorsulni teljesen, de mar feltavolsagnal vagyunk:
					// lassulasba kell atvaltani
					if (_stepsMade > _rampThres && _rampThres !=0 ) _f_ramp=3;
					break;
					
				case 2:
					// lassulasi szakasz kezdetenek figyelese
					if (abs(_differenceSteps)  <= _rampSteps) _f_ramp=3;
					break;
					
			}
	
		}
		// mozgunk, ezert a ready jelet hamisba allitjuk
		_stepReadyStatus = false; 
		// RUN led kigyujtasa
		PORTD |= B00100000;
		
        // attol fuggoen, hogy elore vagy hatra mozgunk, a DIR pin beallitasa
		if (_differenceSteps < 0)
		{
			PORTC |= B00000010;
			_currentStep--;
		}
		else
		{
			PORTC &= B11111101;
			_currentStep++;
		}
		
		// STEP pin beallitasa
		if (_step_inverz) PORTC &= B11111011; else PORTC |= B00000100;
		
		//utolso lepes utan gyorsan visszajelezni a veget
		if (abs(_differenceSteps) == 1)
		{
			_stepReadyStatus = true;
			// RUN led eloltasa
			PORTD &= B11011111;
		}

	}
	else
	{  // nincs tobb lepes, azaz befejezodott a mozgas!
		_stepReadyStatus = true;
		// RUN led eloltasa
		PORTD &= B11011111;
	} 
	
} 

_f_int=1;  // interrupt befejezodott jel beallitasa

}



