/*
  
  BinComLib.h
  
  Binaris kommunikacio PC es Arduino kozott
  Fixen bedrotozva 0-as sorosport, 115200 baud, 8N1
  
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
	// regiszter tomb: ebben vannak a csak olvashato mezok
	// regiszter hossz: a regiszter tomb merete bajtban
	// parancs tomb: a fogadott parancsokat ebbe teszi a fuggveny, merete: 8 bajt
	// visszateresi ertek: ha nem erkezett parancs, akkor 0, ha erkezett parancs,
	// akkor a parancs tombben atadott bajtok szama (1..8)
	int process(byte *_regiszter_tomb, byte _regiszter_hossz, byte *_parancs_tomb);

	// kimeno adatfeldolgozas, akkor kell meghivni, ha egy parancsot meg akarunk valaszolni
	// parancs_tomb: a kuldendo parancsot tartalmazza, merete 8 bajt
	// parancs hossz: a kuldendo tombben levo hasznos bajtok szamat tartalmazza (1..8)
	void reply(byte *_parancs_tomb, byte _parancs_hossz);
	
	// kimeno adatkuldes, minden ciklusban meg kell hivni
	void send();
};

void debug_send(byte adatom);

#endif

