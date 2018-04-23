/*
  
  BinComLib.h
  
  Binaris kommunikacio PC es Arduino kozott
  
  by Adam Orban, 2018
  
*/

#ifndef BinComLib_h
#define BinComLib_h



class BinComLib
{
  public:
	// constructor
    BinComLib();
	
	// konyvtar inicializalas
	// allomas_szam: 1..7 kozott kell legyen
	void init(byte _allomas_szam);

	// bejovo adatfeldolgozas, minden ciklusban meg kell hivni
	void process(byte *_olvashato_adattomb, byte _olvashato_hossz, byte *_irhato_adattomb, byte _irhato_hossz);

	// kimenp adatkuldes, minden ciklusban meg kell hivni
	void send();
};

#endif

