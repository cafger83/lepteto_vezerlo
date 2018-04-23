#include <StepperLab4.h>
#include <BinComLib.h>

StepperLab4 myStepper;
BinComLib myCom;

int state = 1;

// allomas sorszam valtozok
byte station_nr = 0; //tengely szama (1..7 lehet!)
bool standalone = false; //standalone mod, csak a jog gombok mukodnek. akkor aktiv, ha a tengelyszam = 0

// parameterek taroloja
struct {
    byte statusz_bitek_1;     // cim: 0
    byte statusz_bitek_2;     // cim: 1
    byte statusz_bitek_3;     // cim: 2
    byte statusz_bitek_4;     // cim: 3
    long aktualis_pozicio;    // cim: 4
    int aktualis_sebesseg;    // cim: 8
    int beallitott_sebesseg;  // cim: 10
    int beallitott_gyorsulas; // cim: 12
    int default_sebesseg;     // cim: 14
    int default_gyorsulas;    // cim: 16
    byte mode_EN;             // cim: 18
    byte mode_DIR_STEP;       // cim: 19
    byte mode_HOME_ORG;       // cim: 20
    byte mode_RDY;            // cim: 21
    byte mode_ALM;            // cim: 22
    byte mode_JOG;            // cim: 23
    long soft_limit_plusz;    // cim: 24
    long soft_limit_minusz;   // cim: 28
    int home_sebesseg;        // cim: 32
    int home_timeout;         // cim: 34
    byte PC_reserved_1[4];    // cim: 36
    byte PC_reserved_2[4];    // cim: 40
    int max_sebesseg;         // cim: 44
    int min_sebesseg;         // cim: 46
    int max_gyorsulas;        // cim: 48
    int min_gyorsulas;        // cim: 50
    byte fo_verzio;           // cim: 52
    byte al_verzio;           // cim: 53
    byte sorozat_szam;        // cim: 54
    byte atmega_id[9];        // cim: 55
} regiszter;

//byte regiszter[64];
byte bejovo_parancs[8];
byte kimeno_parancs[8];
byte parancs_hossz;

void setup()
{
  DDRB = 0;           // PBx mindegyik bemenet!
  DDRC |= B00000111;  // PC0,1,2 kimenet: EN, DIR, STEP
  DDRD |= B01100000;  // PD5,6 kimenet: RUN, HBEAT
  DDRE |= B00000001;  // PE0 kimenet: SDIR
  
  PORTC |= B00000001; // EN kimenet bekapcsolasa

  // regiszterbe az alapertekek beirasa
  regiszter.atmega_id[0] = DEVID0;
  regiszter.atmega_id[1] = DEVID1;
  regiszter.atmega_id[2] = DEVID2;
  regiszter.atmega_id[3] = DEVID3;
  regiszter.atmega_id[4] = DEVID4;
  regiszter.atmega_id[5] = DEVID5;
  regiszter.atmega_id[6] = DEVID6;
  regiszter.atmega_id[7] = DEVID7;
  regiszter.atmega_id[8] = DEVID8;
  
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

  regiszter.aktualis_pozicio = myStepper.getActualPosition();

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
    parancs_hossz = myCom.process((byte*)&regiszter, 64, (byte*)bejovo_parancs);
    if (parancs_hossz > 0) myCom.reply((byte*)kimeno_parancs, parancs_hossz);
    myCom.send();
  }

}


















