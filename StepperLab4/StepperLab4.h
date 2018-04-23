/*
  StepperLab3.h - Stepper Library to use with L293 chip.
  Created by Martin Nawrath and Andreas Muxel, 2008.2009,2010
  http://interface.khm.de
  Released into the public domain.
  
  StepperLab4.h
  Updated stepper library:
  - modified for STEP/DIR drivers
  - smooth acceleration/decceleration with Timer
  - position data stored in int32
  by Adam Orban, 2018
*/

#ifndef StepperLab4_h
#define StepperLab4_h



class StepperLab4
{
  public:
	// constructor
    StepperLab4();
	
	// attach
	void attach(bool step_inverz, bool dir_inverz);

	// sebesseg es gyorsulas beallitasa
	bool setSpeedAccel(int speed, int accel);

	// mozgatas inditasa
	void relativeSteps(long step);
	void absoluteSteps(long step);
	
	// megallitas lassulassal
	void soft_stop(void);
	// megallitas azonnal
	void hard_stop(void);
	
	// aktualis pozicio beallitasa
	void setRefPosition (long apos);

	// aktualis pozicio visszaolvasasa
	long getActualPosition(void);

	// mozgas inditasra kesz a tengely? (false - eppen mozog)
	bool stepReady(void);

  private:
   void _setupTimer();

};

#endif

