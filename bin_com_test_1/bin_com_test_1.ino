#include <StepperLab4.h>
#include <BinComLib.h>

StepperLab4 myStepper;
BinComLib myCom;

int state = 1;

// allomas sorszam valtozok
byte station_nr = 0; //tengely szama (1..7 lehet!)
bool standalone = false; //standalone mod, csak a jog gombok mukodnek. akkor aktiv, ha a tengelyszam = 0

// parameterek taroloja
int minden_parameter[64];

void setup()
{
  DDRB = 0;           // PBx mindegyik bemenet!
  DDRC |= B00000111;  // PC0,1,2 kimenet: EN, DIR, STEP
  DDRD |= B01100000;  // PD5,6 kimenet: RUN, HBEAT
  DDRE |= B00000001;  // PE0 kimenet: SDIR
  
  PORTC |= B00000001; // EN kimenet bekapcsolasa
  
  // stepper init
  myStepper.attach(true, false);
  myStepper.setSpeedAccel(1000, 8000);

  // allomasszam beolvasasa
  if ((PINB & 1) != 0) station_nr = 4;
  if ((PINB & 2) != 0) station_nr = station_nr + 2;
  if ((PINB & 4) != 0) station_nr = station_nr + 1;
  if (station_nr == 0) standalone = true;

  // soros kommunikacio inicializalasa
  if (!standalone) myCom.init(station_nr);
}

void loop()
{
  
  // tesztmozgas oda-vissza
  switch (state)
  {
    case 1:
      if (!standalone) state = 300;
      break;
    case 300:
      myStepper.absoluteSteps(20000);
      state = 301;
      break;
    case 301:
      if (myStepper.stepReady()) state = 302;
      break;
    case 302:
      delay(500);
      state = 303;
      break;
    case 303:
      myStepper.absoluteSteps(0);
      state = 304;
      break;
    case 304:
      if (myStepper.stepReady()) state = 400;
      break;
    case 400:
      delay(500);
      state = 300;
      break;
  }

  if (standalone)
  {
    //UP gomb nyomva, szervo nem fut
    if (((PINB & B00001000) != 0) && (myStepper.stepReady())) myStepper.absoluteSteps(1000000);

    //DOWN gomb nyomva, szervo nem fut
    if (((PINB & B00010000) != 0) && (myStepper.stepReady())) myStepper.absoluteSteps(-1000000);

    //UP es DOWN gomb elengedve, de a szervo fut
    if (((PINB & B00001000) == 0) && ((PINB & B00010000) == 0) && (!myStepper.stepReady())) myStepper.soft_stop();
  }

  if (!standalone)
  {
    myCom.process((byte*)minden_parameter, 128, (byte*)minden_parameter, 128);
    myCom.send();
  }

}


















