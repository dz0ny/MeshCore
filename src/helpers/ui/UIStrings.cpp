#include "UIStrings.h"
#include "../../examples/companion_radio/NodePrefs.h"

// Define all translation strings in PROGMEM to save RAM
// Each string table must have exactly STR_COUNT entries

// English strings
static const char str_en_0[] PROGMEM = "ON";
static const char str_en_1[] PROGMEM = "OFF";
static const char str_en_2[] PROGMEM = "N/A";
static const char str_en_3[] PROGMEM = "Not supported";
static const char str_en_4[] PROGMEM = "Settings";
static const char str_en_5[] PROGMEM = "* Connectivity";
static const char str_en_6[] PROGMEM = "* Sound";
static const char str_en_7[] PROGMEM = "* GPS";
static const char str_en_8[] PROGMEM = "* Location Advert";
static const char str_en_9[] PROGMEM = "* Privacy";
static const char str_en_10[] PROGMEM = "* Maintenance";
static const char str_en_11[] PROGMEM = "  Bluetooth";
static const char str_en_12[] PROGMEM = "  Buzzer";
static const char str_en_13[] PROGMEM = "  Key Press Buzzer";
static const char str_en_14[] PROGMEM = "  GPS";
static const char str_en_15[] PROGMEM = "  Broadcast Location";
static const char str_en_16[] PROGMEM = "  Movement Threshold";
static const char str_en_17[] PROGMEM = "  Update Frequency";
static const char str_en_18[] PROGMEM = "  Guaranteed Interval";
static const char str_en_19[] PROGMEM = "  Required Accuracy";
static const char str_en_20[] PROGMEM = "  Telemetry Share";
static const char str_en_21[] PROGMEM = "  Advertise Location";
static const char str_en_22[] PROGMEM = "  Clear Files";
static const char str_en_23[] PROGMEM = "  Language";
static const char str_en_24[] PROGMEM = "DENY";
static const char str_en_25[] PROGMEM = "FLAGS";
static const char str_en_26[] PROGMEM = "ALL";
static const char str_en_27[] PROGMEM = "1m";
static const char str_en_28[] PROGMEM = "2m";
static const char str_en_29[] PROGMEM = "5m";
static const char str_en_30[] PROGMEM = "10m";
static const char str_en_31[] PROGMEM = "15m";
static const char str_en_32[] PROGMEM = "BLE: OFF";
static const char str_en_33[] PROGMEM = "BLE: ON";
static const char str_en_34[] PROGMEM = "Key Buzz: ON";
static const char str_en_35[] PROGMEM = "Key Buzz: OFF";
static const char str_en_36[] PROGMEM = "LocAdv: ON";
static const char str_en_37[] PROGMEM = "LocAdv: OFF";
static const char str_en_38[] PROGMEM = "Distance: %dm";
static const char str_en_39[] PROGMEM = "Frequency: %ds";
static const char str_en_40[] PROGMEM = "Interval: %s";
static const char str_en_41[] PROGMEM = "Accuracy: %dm";
static const char str_en_42[] PROGMEM = "Telemetry: %s";
static const char str_en_43[] PROGMEM = "Advert Loc: ON";
static const char str_en_44[] PROGMEM = "Advert Loc: OFF";
static const char str_en_45[] PROGMEM = "Clearing files...";
static const char str_en_46[] PROGMEM = "Files cleared! Rebooting...";
static const char str_en_47[] PROGMEM = "Clear failed!";
static const char str_en_48[] PROGMEM = "Language: %s";
static const char str_en_49[] PROGMEM = "English";
static const char str_en_50[] PROGMEM = "Slovenian";
static const char str_en_51[] PROGMEM = "Croatian";
static const char str_en_52[] PROGMEM = "System Stats";
static const char str_en_53[] PROGMEM = "=== Runtime ===";
static const char str_en_54[] PROGMEM = "=== Heap ===";
static const char str_en_55[] PROGMEM = "=== Stack ===";
static const char str_en_56[] PROGMEM = "Uptime:";
static const char str_en_57[] PROGMEM = "Free:";
static const char str_en_58[] PROGMEM = "Min Free:";
static const char str_en_59[] PROGMEM = "Total:";
static const char str_en_60[] PROGMEM = "Usage:";
static const char str_en_61[] PROGMEM = "Free (words):";
static const char str_en_62[] PROGMEM = "Free (bytes):";
static const char str_en_63[] PROGMEM = "h";
static const char str_en_64[] PROGMEM = "m";
static const char str_en_65[] PROGMEM = "s";
static const char str_en_66[] PROGMEM = "B";
static const char str_en_67[] PROGMEM = "%";
static const char str_en_68[] PROGMEM = "Recent Contacts";
static const char str_en_69[] PROGMEM = "No nearby contacts";
static const char str_en_70[] PROGMEM = "No GPS fix";
static const char str_en_71[] PROGMEM = "Need GPS to show map";
static const char str_en_72[] PROGMEM = "d";
static const char str_en_73[] PROGMEM = "km";
static const char str_en_74[] PROGMEM = "Messages";
static const char str_en_75[] PROGMEM = "Messages %d/%d";
static const char str_en_76[] PROGMEM = "Options";
static const char str_en_77[] PROGMEM = "No saved messages";
static const char str_en_78[] PROGMEM = "Read";
static const char str_en_79[] PROGMEM = "Delete";
static const char str_en_80[] PROGMEM = "From: %s";
static const char str_en_81[] PROGMEM = "From: %s (h%d)";
static const char str_en_82[] PROGMEM = "Radio Stats";
static const char str_en_83[] PROGMEM = "Reports";
static const char str_en_84[] PROGMEM = "  Show GPS Info";
static const char str_en_85[] PROGMEM = "  Show Radio Stats";
static const char str_en_86[] PROGMEM = "  Show Telemetry";
static const char str_en_87[] PROGMEM = "  Show Debug Keys";
static const char str_en_88[] PROGMEM = "  Show System Stats";
static const char str_en_89[] PROGMEM = "[ GPS OFF ]";
static const char str_en_90[] PROGMEM = "Enable in Settings";
static const char str_en_91[] PROGMEM = "GPS: ERROR";
static const char str_en_92[] PROGMEM = "Can't access GPS";
static const char str_en_93[] PROGMEM = "GPS [FIX]";
static const char str_en_94[] PROGMEM = "GPS [SEARCH]";
static const char str_en_95[] PROGMEM = "lat";
static const char str_en_96[] PROGMEM = "lon";
static const char str_en_97[] PROGMEM = "alt";
static const char str_en_98[] PROGMEM = "acc";
static const char str_en_99[] PROGMEM = "Telemetry";
static const char str_en_100[] PROGMEM = "Battery";
static const char str_en_101[] PROGMEM = "Temperature";
static const char str_en_102[] PROGMEM = "Humidity";
static const char str_en_103[] PROGMEM = "Pressure";
static const char str_en_104[] PROGMEM = "Latitude";
static const char str_en_105[] PROGMEM = "Longitude";
static const char str_en_106[] PROGMEM = "No additional sensors";
static const char str_en_107[] PROGMEM = "detected";
static const char str_en_108[] PROGMEM = "C";
static const char str_en_109[] PROGMEM = "hPa";
static const char str_en_110[] PROGMEM = "< Connected >";
static const char str_en_111[] PROGMEM = "Pin: %d";
static const char str_en_112[] PROGMEM = "%d nearby";
static const char str_en_113[] PROGMEM = "No contacts";
static const char str_en_114[] PROGMEM = "Press to view";
static const char str_en_115[] PROGMEM = "No Messages";
static const char str_en_116[] PROGMEM = "%d unread";
static const char str_en_117[] PROGMEM = "%d total";
static const char str_en_118[] PROGMEM = "id";
static const char str_en_119[] PROGMEM = "share";
static const char str_en_120[] PROGMEM = "fix";
static const char str_en_121[] PROGMEM = "sats";
static const char str_en_122[] PROGMEM = "Send location";
static const char str_en_123[] PROGMEM = "press Enter";
static const char str_en_124[] PROGMEM = "long press";
static const char str_en_125[] PROGMEM = " to send";
static const char str_en_126[] PROGMEM = "hibernating...";
static const char str_en_127[] PROGMEM = "Hibernate";
static const char str_en_128[] PROGMEM = "Unread: %d";
static const char str_en_129[] PROGMEM = "Advert sent!";
static const char str_en_130[] PROGMEM = "Advert failed..";
static const char str_en_131[] PROGMEM = "Enter when pairing";

// Slovenian strings (Slovenscina)
static const char str_sl_0[] PROGMEM = "DA";
static const char str_sl_1[] PROGMEM = "NE";
static const char str_sl_2[] PROGMEM = "N/A";
static const char str_sl_3[] PROGMEM = "Ni podprto";
static const char str_sl_4[] PROGMEM = "Nastavitve";
static const char str_sl_5[] PROGMEM = "* Povezljivost";
static const char str_sl_6[] PROGMEM = "* Zvok";
static const char str_sl_7[] PROGMEM = "* GPS";
static const char str_sl_8[] PROGMEM = "* Oglas lokacije";
static const char str_sl_9[] PROGMEM = "* Zasebnost";
static const char str_sl_10[] PROGMEM = "* Vzdrzevanje";
static const char str_sl_11[] PROGMEM = "  Bluetooth";
static const char str_sl_12[] PROGMEM = "  Zvocnik";
static const char str_sl_13[] PROGMEM = "  Zvok ob pritisku";
static const char str_sl_14[] PROGMEM = "  GPS";
static const char str_sl_15[] PROGMEM = "  Oddaj lokacijo";
static const char str_sl_16[] PROGMEM = "  Prag premika";
static const char str_sl_17[] PROGMEM = "  Frekvenca posod.";
static const char str_sl_18[] PROGMEM = "  Zagotovl. interval";
static const char str_sl_19[] PROGMEM = "  Zahtev. natancnost";
static const char str_sl_20[] PROGMEM = "  Delj. telemetrija";
static const char str_sl_21[] PROGMEM = "  Objavi lokacijo";
static const char str_sl_22[] PROGMEM = "  Pocisti shrambo";
static const char str_sl_23[] PROGMEM = "  Jezik";
static const char str_sl_24[] PROGMEM = "ZAVRNI";
static const char str_sl_25[] PROGMEM = "ZASTAVICE";
static const char str_sl_26[] PROGMEM = "VSE";
static const char str_sl_27[] PROGMEM = "1m";
static const char str_sl_28[] PROGMEM = "2m";
static const char str_sl_29[] PROGMEM = "5m";
static const char str_sl_30[] PROGMEM = "10m";
static const char str_sl_31[] PROGMEM = "15m";
static const char str_sl_32[] PROGMEM = "BLE: NE";
static const char str_sl_33[] PROGMEM = "BLE: DA";
static const char str_sl_34[] PROGMEM = "Zvok: DA";
static const char str_sl_35[] PROGMEM = "Zvok: NE";
static const char str_sl_36[] PROGMEM = "OglasLok: DA";
static const char str_sl_37[] PROGMEM = "OglasLok: NE";
static const char str_sl_38[] PROGMEM = "Razdalja: %dm";
static const char str_sl_39[] PROGMEM = "Frekvenca: %ds";
static const char str_sl_40[] PROGMEM = "Interval: %s";
static const char str_sl_41[] PROGMEM = "Natancnost: %dm";
static const char str_sl_42[] PROGMEM = "Telemetrija: %s";
static const char str_sl_43[] PROGMEM = "Oglas lok: DA";
static const char str_sl_44[] PROGMEM = "Oglas lok: NE";
static const char str_sl_45[] PROGMEM = "Brisanje datotek...";
static const char str_sl_46[] PROGMEM = "Datoteke pobrisane! Ponovni zagon...";
static const char str_sl_47[] PROGMEM = "Brisanje ni uspelo!";
static const char str_sl_48[] PROGMEM = "Jezik: %s";
static const char str_sl_49[] PROGMEM = "Anglescina";
static const char str_sl_50[] PROGMEM = "Slovenscina";
static const char str_sl_51[] PROGMEM = "Hrvascina";
static const char str_sl_52[] PROGMEM = "Sistemske statistike";
static const char str_sl_53[] PROGMEM = "=== Cas delovanja ===";
static const char str_sl_54[] PROGMEM = "=== Kopica ===";
static const char str_sl_55[] PROGMEM = "=== Sklad ===";
static const char str_sl_56[] PROGMEM = "Cas delovanja:";
static const char str_sl_57[] PROGMEM = "Prosto:";
static const char str_sl_58[] PROGMEM = "Min prosto:";
static const char str_sl_59[] PROGMEM = "Skupaj:";
static const char str_sl_60[] PROGMEM = "Uporaba:";
static const char str_sl_61[] PROGMEM = "Prosto (besede):";
static const char str_sl_62[] PROGMEM = "Prosto (bajti):";
static const char str_sl_63[] PROGMEM = "h";
static const char str_sl_64[] PROGMEM = "m";
static const char str_sl_65[] PROGMEM = "s";
static const char str_sl_66[] PROGMEM = "B";
static const char str_sl_67[] PROGMEM = "%";
static const char str_sl_68[] PROGMEM = "Nedavni stiki";
static const char str_sl_69[] PROGMEM = "Ni bliznjih stikov";
static const char str_sl_70[] PROGMEM = "Ni GPS signala";
static const char str_sl_71[] PROGMEM = "Potreben GPS za zemljevid";
static const char str_sl_72[] PROGMEM = "d";
static const char str_sl_73[] PROGMEM = "km";
static const char str_sl_74[] PROGMEM = "Sporocila";
static const char str_sl_75[] PROGMEM = "Sporocila %d/%d";
static const char str_sl_76[] PROGMEM = "Moznosti";
static const char str_sl_77[] PROGMEM = "Ni shranjenih sporocil";
static const char str_sl_78[] PROGMEM = "Beri";
static const char str_sl_79[] PROGMEM = "Izbrisi";
static const char str_sl_80[] PROGMEM = "Od: %s";
static const char str_sl_81[] PROGMEM = "Od: %s (h%d)";
static const char str_sl_82[] PROGMEM = "Radio statistike";
static const char str_sl_83[] PROGMEM = "Porocila";
static const char str_sl_84[] PROGMEM = "  Prikazi GPS info";
static const char str_sl_85[] PROGMEM = "  Prikazi radio stat.";
static const char str_sl_86[] PROGMEM = "  Prikazi telemetrijo";
static const char str_sl_87[] PROGMEM = "  Prikazi debug tipke";
static const char str_sl_88[] PROGMEM = "  Prikazi sist. stat.";
static const char str_sl_89[] PROGMEM = "[ GPS IZKLOP ]";
static const char str_sl_90[] PROGMEM = "Vklopi v nastavitvah";
static const char str_sl_91[] PROGMEM = "GPS: NAPAKA";
static const char str_sl_92[] PROGMEM = "Ni dostopa do GPS";
static const char str_sl_93[] PROGMEM = "GPS [SIGNAL]";
static const char str_sl_94[] PROGMEM = "GPS [ISKANJE]";
static const char str_sl_95[] PROGMEM = "lat";
static const char str_sl_96[] PROGMEM = "lon";
static const char str_sl_97[] PROGMEM = "visina";
static const char str_sl_98[] PROGMEM = "nat";
static const char str_sl_99[] PROGMEM = "Telemetrija";
static const char str_sl_100[] PROGMEM = "Baterija";
static const char str_sl_101[] PROGMEM = "Temperatura";
static const char str_sl_102[] PROGMEM = "Vlaznost";
static const char str_sl_103[] PROGMEM = "Tlak";
static const char str_sl_104[] PROGMEM = "Zemljepisna sirina";
static const char str_sl_105[] PROGMEM = "Zemljepisna dolzina";
static const char str_sl_106[] PROGMEM = "Ni dodatnih senzorjev";
static const char str_sl_107[] PROGMEM = "zaznano";
static const char str_sl_108[] PROGMEM = "C";
static const char str_sl_109[] PROGMEM = "hPa";
static const char str_sl_110[] PROGMEM = "< Povezano >";
static const char str_sl_111[] PROGMEM = "Pin: %d";
static const char str_sl_112[] PROGMEM = "%d v blizini";
static const char str_sl_113[] PROGMEM = "Ni stikov";
static const char str_sl_114[] PROGMEM = "Pritisni za ogled";
static const char str_sl_115[] PROGMEM = "Ni sporocil";
static const char str_sl_116[] PROGMEM = "%d neprebranih";
static const char str_sl_117[] PROGMEM = "%d skupaj";
static const char str_sl_118[] PROGMEM = "id";
static const char str_sl_119[] PROGMEM = "deli";
static const char str_sl_120[] PROGMEM = "natancnost";
static const char str_sl_121[] PROGMEM = "sateliti";
static const char str_sl_122[] PROGMEM = "Poslji lokacijo";
static const char str_sl_123[] PROGMEM = "pritisni Enter";
static const char str_sl_124[] PROGMEM = "dolgi pritisk";
static const char str_sl_125[] PROGMEM = " za poslji";
static const char str_sl_126[] PROGMEM = "v mirovanju...";
static const char str_sl_127[] PROGMEM = "Mirovanje";
static const char str_sl_128[] PROGMEM = "Neprebranih: %d";
static const char str_sl_129[] PROGMEM = "Oglas poslan!";
static const char str_sl_130[] PROGMEM = "Oglas ni uspel..";
static const char str_sl_131[] PROGMEM = "Vnesi pri povezovanju";

// Croatian strings (Hrvatski)
static const char str_hr_0[] PROGMEM = "DA";
static const char str_hr_1[] PROGMEM = "NE";
static const char str_hr_2[] PROGMEM = "N/A";
static const char str_hr_3[] PROGMEM = "Nije podrzano";
static const char str_hr_4[] PROGMEM = "Postavke";
static const char str_hr_5[] PROGMEM = "* Povezivost";
static const char str_hr_6[] PROGMEM = "* Zvuk";
static const char str_hr_7[] PROGMEM = "* GPS";
static const char str_hr_8[] PROGMEM = "* Oglas lokacije";
static const char str_hr_9[] PROGMEM = "* Privatnost";
static const char str_hr_10[] PROGMEM = "* Odrzavanje";
static const char str_hr_11[] PROGMEM = "  Bluetooth";
static const char str_hr_12[] PROGMEM = "  Zvucnik";
static const char str_hr_13[] PROGMEM = "  Zvuk pritiska";
static const char str_hr_14[] PROGMEM = "  GPS";
static const char str_hr_15[] PROGMEM = "  Emituj lokaciju";
static const char str_hr_16[] PROGMEM = "  Prag pokreta";
static const char str_hr_17[] PROGMEM = "  Frekv. azuriranja";
static const char str_hr_18[] PROGMEM = "  Zagarant. interval";
static const char str_hr_19[] PROGMEM = "  Potrebna tocnost";
static const char str_hr_20[] PROGMEM = "  Dijel. telemetrija";
static const char str_hr_21[] PROGMEM = "  Oglasi lokaciju";
static const char str_hr_22[] PROGMEM = "  Obrisi shrambo";
static const char str_hr_23[] PROGMEM = "  Jezik";
static const char str_hr_24[] PROGMEM = "ODBIJ";
static const char str_hr_25[] PROGMEM = "ZASTAVICE";
static const char str_hr_26[] PROGMEM = "SVE";
static const char str_hr_27[] PROGMEM = "1m";
static const char str_hr_28[] PROGMEM = "2m";
static const char str_hr_29[] PROGMEM = "5m";
static const char str_hr_30[] PROGMEM = "10m";
static const char str_hr_31[] PROGMEM = "15m";
static const char str_hr_32[] PROGMEM = "BLE: NE";
static const char str_hr_33[] PROGMEM = "BLE: DA";
static const char str_hr_34[] PROGMEM = "Zvuk: DA";
static const char str_hr_35[] PROGMEM = "Zvuk: NE";
static const char str_hr_36[] PROGMEM = "OglasLok: DA";
static const char str_hr_37[] PROGMEM = "OglasLok: NE";
static const char str_hr_38[] PROGMEM = "Udaljenost: %dm";
static const char str_hr_39[] PROGMEM = "Frekvencija: %ds";
static const char str_hr_40[] PROGMEM = "Interval: %s";
static const char str_hr_41[] PROGMEM = "Tocnost: %dm";
static const char str_hr_42[] PROGMEM = "Telemetrija: %s";
static const char str_hr_43[] PROGMEM = "Oglas lok: DA";
static const char str_hr_44[] PROGMEM = "Oglas lok: NE";
static const char str_hr_45[] PROGMEM = "Brisanje datoteka...";
static const char str_hr_46[] PROGMEM = "Datoteke obrisane! Ponovno pokretanje...";
static const char str_hr_47[] PROGMEM = "Brisanje nije uspjelo!";
static const char str_hr_48[] PROGMEM = "Jezik: %s";
static const char str_hr_49[] PROGMEM = "Engleski";
static const char str_hr_50[] PROGMEM = "Slovenski";
static const char str_hr_51[] PROGMEM = "Hrvatski";
static const char str_hr_52[] PROGMEM = "Sistemske statistike";
static const char str_hr_53[] PROGMEM = "=== Vrijeme rada ===";
static const char str_hr_54[] PROGMEM = "=== Heap ===";
static const char str_hr_55[] PROGMEM = "=== Stack ===";
static const char str_hr_56[] PROGMEM = "Vrijeme rada:";
static const char str_hr_57[] PROGMEM = "Slobodno:";
static const char str_hr_58[] PROGMEM = "Min slobodno:";
static const char str_hr_59[] PROGMEM = "Ukupno:";
static const char str_hr_60[] PROGMEM = "Koristenje:";
static const char str_hr_61[] PROGMEM = "Slobodno (rijeci):";
static const char str_hr_62[] PROGMEM = "Slobodno (bajtovi):";
static const char str_hr_63[] PROGMEM = "h";
static const char str_hr_64[] PROGMEM = "m";
static const char str_hr_65[] PROGMEM = "s";
static const char str_hr_66[] PROGMEM = "B";
static const char str_hr_67[] PROGMEM = "%";
static const char str_hr_68[] PROGMEM = "Nedavni kontakti";
static const char str_hr_69[] PROGMEM = "Nema kontakata u blizini";
static const char str_hr_70[] PROGMEM = "Nema GPS signala";
static const char str_hr_71[] PROGMEM = "Potreban GPS za kartu";
static const char str_hr_72[] PROGMEM = "d";
static const char str_hr_73[] PROGMEM = "km";
static const char str_hr_74[] PROGMEM = "Poruke";
static const char str_hr_75[] PROGMEM = "Poruke %d/%d";
static const char str_hr_76[] PROGMEM = "Opcije";
static const char str_hr_77[] PROGMEM = "Nema spremljenih poruka";
static const char str_hr_78[] PROGMEM = "Citaj";
static const char str_hr_79[] PROGMEM = "Obrisi";
static const char str_hr_80[] PROGMEM = "Od: %s";
static const char str_hr_81[] PROGMEM = "Od: %s (h%d)";
static const char str_hr_82[] PROGMEM = "Radio statistike";
static const char str_hr_83[] PROGMEM = "Izvjestaji";
static const char str_hr_84[] PROGMEM = "  Prikazi GPS info";
static const char str_hr_85[] PROGMEM = "  Prikazi radio stat.";
static const char str_hr_86[] PROGMEM = "  Prikazi telemetriju";
static const char str_hr_87[] PROGMEM = "  Prikazi debug tipke";
static const char str_hr_88[] PROGMEM = "  Prikazi sist. stat.";
static const char str_hr_89[] PROGMEM = "[ GPS ISKLJUC ]";
static const char str_hr_90[] PROGMEM = "Ukljuci u postavkama";
static const char str_hr_91[] PROGMEM = "GPS: GRESKA";
static const char str_hr_92[] PROGMEM = "Nema pristupa GPS-u";
static const char str_hr_93[] PROGMEM = "GPS [SIGNAL]";
static const char str_hr_94[] PROGMEM = "GPS [PRETRAZIVANJE]";
static const char str_hr_95[] PROGMEM = "lat";
static const char str_hr_96[] PROGMEM = "lon";
static const char str_hr_97[] PROGMEM = "visina";
static const char str_hr_98[] PROGMEM = "tocnost";
static const char str_hr_99[] PROGMEM = "Telemetrija";
static const char str_hr_100[] PROGMEM = "Baterija";
static const char str_hr_101[] PROGMEM = "Temperatura";
static const char str_hr_102[] PROGMEM = "Vlaznost";
static const char str_hr_103[] PROGMEM = "Tlak";
static const char str_hr_104[] PROGMEM = "Zemljopisna sirina";
static const char str_hr_105[] PROGMEM = "Zemljopisna duzina";
static const char str_hr_106[] PROGMEM = "Nema dodatnih senzora";
static const char str_hr_107[] PROGMEM = "otkriveno";
static const char str_hr_108[] PROGMEM = "C";
static const char str_hr_109[] PROGMEM = "hPa";
static const char str_hr_110[] PROGMEM = "< Povezano >";
static const char str_hr_111[] PROGMEM = "Pin: %d";
static const char str_hr_112[] PROGMEM = "%d u blizini";
static const char str_hr_113[] PROGMEM = "Nema kontakata";
static const char str_hr_114[] PROGMEM = "Pritisni za pregled";
static const char str_hr_115[] PROGMEM = "Nema poruka";
static const char str_hr_116[] PROGMEM = "%d neprocitanih";
static const char str_hr_117[] PROGMEM = "%d ukupno";
static const char str_hr_118[] PROGMEM = "id";
static const char str_hr_119[] PROGMEM = "dijeli";
static const char str_hr_120[] PROGMEM = "tocnost";
static const char str_hr_121[] PROGMEM = "sateliti";
static const char str_hr_122[] PROGMEM = "Posalji lokaciju";
static const char str_hr_123[] PROGMEM = "pritisni Enter";
static const char str_hr_124[] PROGMEM = "dugi pritisak";
static const char str_hr_125[] PROGMEM = " za slanje";
static const char str_hr_126[] PROGMEM = "u hibernaciji...";
static const char str_hr_127[] PROGMEM = "V Spanje";
static const char str_hr_128[] PROGMEM = "Neprocitanih: %d";
static const char str_hr_129[] PROGMEM = "Oglas poslan!";
static const char str_hr_130[] PROGMEM = "Oglas nije uspio..";
static const char str_hr_131[] PROGMEM = "Unesi pri povezivanju";

// Translation tables - array of pointers to strings in PROGMEM
static const char* const string_table_en[] PROGMEM = {
  str_en_0, str_en_1, str_en_2, str_en_3, str_en_4, str_en_5, str_en_6, str_en_7, str_en_8, str_en_9,
  str_en_10, str_en_11, str_en_12, str_en_13, str_en_14, str_en_15, str_en_16, str_en_17, str_en_18, str_en_19,
  str_en_20, str_en_21, str_en_22, str_en_23, str_en_24, str_en_25, str_en_26, str_en_27, str_en_28, str_en_29,
  str_en_30, str_en_31, str_en_32, str_en_33, str_en_34, str_en_35, str_en_36, str_en_37, str_en_38, str_en_39,
  str_en_40, str_en_41, str_en_42, str_en_43, str_en_44, str_en_45, str_en_46, str_en_47, str_en_48, str_en_49,
  str_en_50, str_en_51, str_en_52, str_en_53, str_en_54, str_en_55, str_en_56, str_en_57, str_en_58, str_en_59,
  str_en_60, str_en_61, str_en_62, str_en_63, str_en_64, str_en_65, str_en_66, str_en_67, str_en_68, str_en_69,
  str_en_70, str_en_71, str_en_72, str_en_73, str_en_74, str_en_75, str_en_76, str_en_77, str_en_78, str_en_79,
  str_en_80, str_en_81, str_en_82, str_en_83, str_en_84, str_en_85, str_en_86, str_en_87, str_en_88, str_en_89,
  str_en_90, str_en_91, str_en_92, str_en_93, str_en_94, str_en_95, str_en_96, str_en_97, str_en_98, str_en_99,
  str_en_100, str_en_101, str_en_102, str_en_103, str_en_104, str_en_105, str_en_106, str_en_107, str_en_108, str_en_109,
  str_en_110, str_en_111, str_en_112, str_en_113, str_en_114, str_en_115, str_en_116, str_en_117, str_en_118, str_en_119,
  str_en_120, str_en_121, str_en_122, str_en_123, str_en_124, str_en_125, str_en_126, str_en_127, str_en_128, str_en_129,
  str_en_130, str_en_131
};

static const char* const string_table_sl[] PROGMEM = {
  str_sl_0, str_sl_1, str_sl_2, str_sl_3, str_sl_4, str_sl_5, str_sl_6, str_sl_7, str_sl_8, str_sl_9,
  str_sl_10, str_sl_11, str_sl_12, str_sl_13, str_sl_14, str_sl_15, str_sl_16, str_sl_17, str_sl_18, str_sl_19,
  str_sl_20, str_sl_21, str_sl_22, str_sl_23, str_sl_24, str_sl_25, str_sl_26, str_sl_27, str_sl_28, str_sl_29,
  str_sl_30, str_sl_31, str_sl_32, str_sl_33, str_sl_34, str_sl_35, str_sl_36, str_sl_37, str_sl_38, str_sl_39,
  str_sl_40, str_sl_41, str_sl_42, str_sl_43, str_sl_44, str_sl_45, str_sl_46, str_sl_47, str_sl_48, str_sl_49,
  str_sl_50, str_sl_51, str_sl_52, str_sl_53, str_sl_54, str_sl_55, str_sl_56, str_sl_57, str_sl_58, str_sl_59,
  str_sl_60, str_sl_61, str_sl_62, str_sl_63, str_sl_64, str_sl_65, str_sl_66, str_sl_67, str_sl_68, str_sl_69,
  str_sl_70, str_sl_71, str_sl_72, str_sl_73, str_sl_74, str_sl_75, str_sl_76, str_sl_77, str_sl_78, str_sl_79,
  str_sl_80, str_sl_81, str_sl_82, str_sl_83, str_sl_84, str_sl_85, str_sl_86, str_sl_87, str_sl_88, str_sl_89,
  str_sl_90, str_sl_91, str_sl_92, str_sl_93, str_sl_94, str_sl_95, str_sl_96, str_sl_97, str_sl_98, str_sl_99,
  str_sl_100, str_sl_101, str_sl_102, str_sl_103, str_sl_104, str_sl_105, str_sl_106, str_sl_107, str_sl_108, str_sl_109,
  str_sl_110, str_sl_111, str_sl_112, str_sl_113, str_sl_114, str_sl_115, str_sl_116, str_sl_117, str_sl_118, str_sl_119,
  str_sl_120, str_sl_121, str_sl_122, str_sl_123, str_sl_124, str_sl_125, str_sl_126, str_sl_127, str_sl_128, str_sl_129,
  str_sl_130, str_sl_131
};

static const char* const string_table_hr[] PROGMEM = {
  str_hr_0, str_hr_1, str_hr_2, str_hr_3, str_hr_4, str_hr_5, str_hr_6, str_hr_7, str_hr_8, str_hr_9,
  str_hr_10, str_hr_11, str_hr_12, str_hr_13, str_hr_14, str_hr_15, str_hr_16, str_hr_17, str_hr_18, str_hr_19,
  str_hr_20, str_hr_21, str_hr_22, str_hr_23, str_hr_24, str_hr_25, str_hr_26, str_hr_27, str_hr_28, str_hr_29,
  str_hr_30, str_hr_31, str_hr_32, str_hr_33, str_hr_34, str_hr_35, str_hr_36, str_hr_37, str_hr_38, str_hr_39,
  str_hr_40, str_hr_41, str_hr_42, str_hr_43, str_hr_44, str_hr_45, str_hr_46, str_hr_47, str_hr_48, str_hr_49,
  str_hr_50, str_hr_51, str_hr_52, str_hr_53, str_hr_54, str_hr_55, str_hr_56, str_hr_57, str_hr_58, str_hr_59,
  str_hr_60, str_hr_61, str_hr_62, str_hr_63, str_hr_64, str_hr_65, str_hr_66, str_hr_67, str_hr_68, str_hr_69,
  str_hr_70, str_hr_71, str_hr_72, str_hr_73, str_hr_74, str_hr_75, str_hr_76, str_hr_77, str_hr_78, str_hr_79,
  str_hr_80, str_hr_81, str_hr_82, str_hr_83, str_hr_84, str_hr_85, str_hr_86, str_hr_87, str_hr_88, str_hr_89,
  str_hr_90, str_hr_91, str_hr_92, str_hr_93, str_hr_94, str_hr_95, str_hr_96, str_hr_97, str_hr_98, str_hr_99,
  str_hr_100, str_hr_101, str_hr_102, str_hr_103, str_hr_104, str_hr_105, str_hr_106, str_hr_107, str_hr_108, str_hr_109,
  str_hr_110, str_hr_111, str_hr_112, str_hr_113, str_hr_114, str_hr_115, str_hr_116, str_hr_117, str_hr_118, str_hr_119,
  str_hr_120, str_hr_121, str_hr_122, str_hr_123, str_hr_124, str_hr_125, str_hr_126, str_hr_127, str_hr_128, str_hr_129,
  str_hr_130, str_hr_131
};

// Master table of all language tables
static const char* const* const all_languages[] PROGMEM = {
  string_table_en,
  string_table_sl,
  string_table_hr
};

// Buffer for reading strings from PROGMEM
static char string_buffer[64];

// Get current language from NodePrefs
UILanguage uiGetCurrentLanguage(NodePrefs* prefs) {
  if (prefs == NULL) {
    return LANG_EN; // Default to English if no preferences available
  }

  uint8_t lang = prefs->ui_language;
  if (lang >= LANG_COUNT) {
    return LANG_EN; // Default to English for invalid values
  }

  return (UILanguage)lang;
}

// Set UI language
void uiSetLanguage(NodePrefs* prefs, UILanguage lang) {
  if (prefs != NULL && lang < LANG_COUNT) {
    prefs->ui_language = lang;
  }
}

// Get string by ID for current language
const char* uiGetString(NodePrefs* prefs, UIStringID id) {
  if (id >= STR_COUNT) {
    return "???"; // Invalid string ID
  }

  UILanguage lang = uiGetCurrentLanguage(prefs);

  // Get pointer to the language table from PROGMEM
  const char* const* string_table = (const char* const*)pgm_read_ptr(&all_languages[lang]);

  // Get pointer to the specific string from PROGMEM
  const char* str_ptr = (const char*)pgm_read_ptr(&string_table[id]);

  // Copy string from PROGMEM to RAM buffer
  strcpy_P(string_buffer, str_ptr);

  return string_buffer;
}

// Get language name for display
const char* uiGetLanguageName(NodePrefs* prefs, UILanguage lang) {
  // Temporarily change language to get the name
  UILanguage old_lang = uiGetCurrentLanguage(prefs);
  if (prefs) {
    prefs->ui_language = lang;
  }

  const char* name;
  switch (lang) {
    case LANG_EN: name = uiGetString(prefs, STR_LANG_ENGLISH); break;
    case LANG_SL: name = uiGetString(prefs, STR_LANG_SLOVENIAN); break;
    case LANG_HR: name = uiGetString(prefs, STR_LANG_CROATIAN); break;
    default: name = "???"; break;
  }

  // Restore original language
  if (prefs) {
    prefs->ui_language = old_lang;
  }

  return name;
}
