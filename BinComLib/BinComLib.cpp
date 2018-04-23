/*
  
  BinComLib.cpp
  
  Binaris kommunikacio PC es Arduino kozott
  
  by Adam Orban, 2018
  
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "BinComLib.h"

// soros kommunikacio valtozoi az interruptban
byte _station_nr_rotated;
byte _bejovo_adat;         //ez a bajt erkezett
byte _bejovo_adatok[19];   //ide teszem le a bejovo bajtokat: fejlec + 2x cim + 8x adat = 19 byte
byte _bejovo_mutato = 255; //hova irunk a bejovo pufferben, ha 255 - nem kell irni!
byte _bejovo_hossz;        //hany hasznos byte erkezik
volatile bool _bejovo_ready = false;  //van-e ervenyes adat a bejovo pufferben

// soros kommunikacio valtozoi a feldolgozasban
byte _adat_hossz; // hany bajttal kell majd dolgozni (1..8)
byte _kezdocim;   // adattomb hanyadik eleme (elso elem: 0)
bool _ir_olvas;   // true=iras, false=olvasas
byte _crc;		  // crc szamitashoz

// soros kommunikacio valtozoi a kuldesben
bool _kimeno_ready = true; //true, ha ures a kuldo puffer, false, ha folyamatban van a kuldes
byte _kimeno_adatok[19];
byte _kimeno_hossz;
byte _kimeno_mutato = 255;

BinComLib::BinComLib(){};

void BinComLib::init(byte _allomas_szam)
{
  // elforgatott allomasszam
  _station_nr_rotated = _allomas_szam << 4;

  // 115200 baud, 20Mhz clock, simple mode
  UBRR0 = 10;
  // double speed mode off
  UCSR0A &= ~(_BV(U2X0));
  // 8N1 data mode
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  // tx, rx enabled, rx interrupt enabled
  UCSR0B = ((1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0));
  
}

int BinComLib::process(byte *_regiszter_tomb, byte _regiszter_hossz, byte *_parancs_tomb)
{
  //megnezzuk, erkezett-e adatcsomag, es valaszt tudunk-e kuldeni, ha igen: mehet a feldolgozas
  if ((_bejovo_ready) && (_kimeno_ready))
  {
    //alapadatok kibanyaszasa
    if ((_bejovo_adatok[0] & B00001000) != 0) _ir_olvas = true; else _ir_olvas = false;
	_adat_hossz = (_bejovo_adatok[0] & B00000111) + 1;
	
    //ha parancs irasrol van szo, akkor az irhato tombbe az adatokat be kell paszirozni
    if (_ir_olvas)
    {

	  // feldolgozas: az adatokat beirjuk a tombbe
      for (byte _fut_f = 0; _fut_f < _adat_hossz; _fut_f++)
        _parancs_tomb[_fut_f] = ((_bejovo_adatok[(_fut_f << 1) + 1] & B00001111) << 4) + (_bejovo_adatok[(_fut_f << 1) + 2] & B00001111);

	  //CRC check kell meg ide!
	  
	  return _adat_hossz;

    } else

      //ha regiszter olvasasrol van szo, akkor az olvashato tombbol kinyomjuk az adatokat
    {
		
	  _kezdocim = ((_bejovo_adatok[1] & B00001111) << 4) + (_bejovo_adatok[2] & B00001111);
	  
	  // tombmeret tullepes ellenorzese
      int _vegso_meret = (int)_kezdocim + (int)_adat_hossz;
      if (_vegso_meret > _regiszter_hossz)
      {
        //eldobjuk, a master majd timeout-ra fut...
        _bejovo_ready = false;
        return 0;
      }

      //CRC check kell meg ide!

      // valasz kuldese
      _kimeno_adatok[0] = _bejovo_adatok[0] & B10001111; //ugyanaz mint ami erkezett, csak a celallomas cime 0;
      for (byte _fut_f = 0; _fut_f < _adat_hossz; _fut_f++)
      {
        _kimeno_adatok[(_fut_f << 1) + 1] = _regiszter_tomb[_kezdocim + _fut_f] >> 4;
        _kimeno_adatok[(_fut_f << 1) + 2] = _regiszter_tomb[_kezdocim + _fut_f] & B00001111;
      }
	  _kimeno_hossz = 3 + (_adat_hossz << 1);
	  _kimeno_mutato = 0;
	  
	  // kamu crc szamitas
	  _kimeno_adatok[_kimeno_hossz-2] = B00001111; //kamu crc ertek
	  _kimeno_adatok[_kimeno_hossz-1] = B00001111; //kamu crc ertek
	  
	  _kimeno_ready = false;
	  // RS485 EN bekapcsolasa
	  
    }

    _bejovo_ready = false;
  }

  return 0;
}

void BinComLib::reply(byte *_parancs_tomb, byte _parancs_hossz)
{
	if (_kimeno_ready)
	{
	  // fejlec generalasa
	  _kimeno_adatok[0] = B10001000 + (_parancs_hossz - 1);
	  
	  // adat darabolas es atmasolas
      for (byte _fut_f = 0; _fut_f < _parancs_hossz; _fut_f++)
      {
        _kimeno_adatok[(_fut_f << 1) + 1] = _parancs_tomb[_fut_f] >> 4;
        _kimeno_adatok[(_fut_f << 1) + 2] = _parancs_tomb[_fut_f] & B00001111;
      }
	  
	  // kuldesi parameterek atadasa
	  _kimeno_hossz = 3 + (_parancs_hossz << 1);
	  _kimeno_mutato = 0;
	  
	  // kamu crc szamitas
	  _kimeno_adatok[_kimeno_hossz-2] = B00001111; //kamu crc ertek
	  _kimeno_adatok[_kimeno_hossz-1] = B00001111; //kamu crc ertek
	  
	  _kimeno_ready = false;
	  // RS485 EN bekapcsolasa
	}
}

void BinComLib::send()
{
  // ha van mit kuldeni...
  if ((_kimeno_ready == false) && ((UCSR0A & 32) != 0))
  {
    if (_kimeno_mutato < _kimeno_hossz) UDR0 = _kimeno_adatok[_kimeno_mutato];
    if (_kimeno_mutato == _kimeno_hossz)
    {
      // RS485 EN kikapcsolasa;
      _kimeno_mutato = 254;
      _kimeno_ready = true;
    }
    _kimeno_mutato++;
  }
  return;
}

SIGNAL (USART_RX_vect)
{
  //bejovo byte beolvasasa
  _bejovo_adat = UDR0;

  //szures az allomasszamra, tobbi bajt eldobasa
  if ((_bejovo_adat & B01110000) != _station_nr_rotated) return;

  //elso bajt figyelese!
  if ((_bejovo_adat & B10000000) != 0)
  {
    _bejovo_ready = false;
    _bejovo_adatok[0] = _bejovo_adat;
    if ((_bejovo_adat & B00001000) != 0)
    {
      //parancs irast ker a master, jon 2*x bajt parameter, 2 bajt parancs es 2 bajt CRC
      _bejovo_hossz = (2 * (_bejovo_adat & B00000111)) + 4;
    } else
    {
      //regiszter olvasast ker a master, meg ket bajt fog jonni a cimmel es ketto CRC
      _bejovo_hossz = 4;
    }
    _bejovo_mutato = 0;
    return;
  }

  // masodik, vagy sokadik bajt erkezett
  if (_bejovo_mutato != 255)
  {
    _bejovo_mutato++;
    _bejovo_adatok[_bejovo_mutato] = _bejovo_adat;
  }

  // vege az uzenetnek? ha igen, akkor a kesz flag beallitasa
  if (_bejovo_mutato == _bejovo_hossz)
  {
    _bejovo_mutato = 255;
    _bejovo_ready = true;
  }

  return;
}

void debug_send(byte adatom)
{
  while ((UCSR0A & 32) == 0);
  UDR0 = adatom;
}


