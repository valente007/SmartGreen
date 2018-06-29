#include <Arduino.h>
#include <Wire.h>
#include "ds3231-master/ds3231.h"
#include <SPI.h>
#include "SdFat.h"
#include <SimpleDHT.h>
#include <SoftwareSerial.h>


#define SD_CS_PIN SS
SdFat SD;

#define PIN_POMPE 7

#define PIN_ANA_H1 0
#define PIN_ANA_H2 1
#define PIN_ANA_H3 2
#define PIN_DIGIT_H1 2
#define PIN_DIGIT_H2 3
#define PIN_DIGIT_H3 4

#define PIN_DHT11 8


#define VALEUR_ETALO_B 350.0 //valeur capteur qd dans l'eau à 100%
#define VALEUR_ETALO_H 1000.0 //valeur qd est dans l'air soit 0%

//Valeur par défaut parametres de l'appli
#define LIMITE_HUMIDITE 50  //en pourcent
#define FREQ_MESURE 10.0 //en sec
#define TEMPS_ARROSAGE 3000 //en ms

int limiteHumidite;
int frequenceMesure;
int temspArrosage;

//Bluetooth AT09
SoftwareSerial BLESerial(5, 6); // RX, TX

struct HumidityGND{
	int hsol1;
	int hAlarm1;
	int hsol2;
	int hAlarm2;
	int hsol3;
	int hAlarm3;
};

struct TempDHT11{
	int temp;
	int humidity;
};

//actions via l'appli
bool demandMAJ;
bool demandLaunchPump;
bool refreshParam;

struct ts tG;
struct ts tOld;

String dateToDigit(int date){
	String sec;
	String zero = "0";
	if(String(date).length()<=1)
			sec = zero + date;
	else
			sec = date;
	return sec;
}

float convertDataH(float mesure){
	float pourcentage=100-((mesure-VALEUR_ETALO_B)/((VALEUR_ETALO_H-VALEUR_ETALO_B)/100));
	if(mesure>VALEUR_ETALO_H) pourcentage =0;
	if(mesure<VALEUR_ETALO_B) pourcentage =100;
	return pourcentage;
}

HumidityGND getHumidityGnd(){
	//Capteurs Humidités
	HumidityGND humidityGND;
	int hsol1=0;
	int hAlarm1=0;
	int hsol2=0;
	int hAlarm2=0;
	int hsol3=0;
	int hAlarm3=0;
	hsol1=analogRead(PIN_ANA_H1);
	hAlarm1=digitalRead(PIN_DIGIT_H1);
	hsol2=analogRead(PIN_ANA_H2);
	hAlarm2=digitalRead(PIN_DIGIT_H2);
	hsol3=analogRead(PIN_ANA_H3);
	hAlarm3=digitalRead(PIN_DIGIT_H3);
	humidityGND.hsol1=convertDataH(hsol1);
	humidityGND.hsol2=convertDataH(hsol2);
	humidityGND.hsol3=convertDataH(hsol3);
	humidityGND.hAlarm1=hAlarm1;
	humidityGND.hAlarm2=hAlarm2;
	humidityGND.hAlarm3=hAlarm3;
	return humidityGND;
}

bool checkHumidityGnd(HumidityGND humidityGND){
	bool res;
	float moy = (humidityGND.hsol1+humidityGND.hsol2+humidityGND.hsol3)/3;

	if(moy<limiteHumidite){
		res=true;
	}
	else{
		res=false;
	}
	return res;
}

void launchPump(int time){
	digitalWrite(PIN_POMPE,true);
	delay(time);
	digitalWrite(PIN_POMPE,false);
}

ts getDate(){
	//RTC
	struct ts t;
	DS3231_get(&t);
	return t;
}

TempDHT11 getHumidityAir()
{
	SimpleDHT11 dht11;
	TempDHT11 tempTab;
	byte temperature = 0;
	byte humidity = 0;
	int err = SimpleDHTErrSuccess;
	if ((err = dht11.read(PIN_DHT11, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
	{
	Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
	}
	tempTab.humidity=humidity;
	tempTab.temp=temperature;
	return tempTab;
}

bool compDate(ts t,ts tOld){
	bool res =false;
	getDate();
	long calcul1=t.hour*3600+t.min*60+t.sec;
	long calcul2=tOld.hour*3600+tOld.min*60+tOld.sec;
	long lastCalcul=0;
	if(calcul2>calcul1){
		lastCalcul=23*60*60+59*60+59-calcul2+calcul1;
	}else{
		lastCalcul=calcul1-calcul2;
	}
	if(lastCalcul>=frequenceMesure)
	{
		res=true;
	}
	Serial.println(lastCalcul);
	return res;
}
void writeDataToSD(HumidityGND humidityGND,ts t,TempDHT11 temp) {
	File dataFile;
	  dataFile = SD.open("Data.csv",FILE_WRITE);

	  // if the file opened okay, write to it:
	  if (dataFile) {
	    //Serial.print("Writing to Data.csv...");
	    String data=dateToDigit(t.year)
	    		+"-"
	    		+dateToDigit(t.mon)
				+"-"
				+dateToDigit(t.mday)
				+","
				+dateToDigit(t.hour)
				+":"
				+dateToDigit(t.min)
				+":"
				+dateToDigit(t.sec)
				+","
				+humidityGND.hsol1
				+","
				+humidityGND.hsol2
				+","
				+humidityGND.hsol3
				+","
				+temp.temp
				+","
				+temp.humidity;
	    dataFile.println(data);
	    Serial.println(data);
	    // close the file:
	    dataFile.close();
	    //Serial.println("done.");
	  } else {
	    // if the file didn't open, print an error:
	    Serial.println("error opening Data.csv");
	  }
}

void initDataFile(){
	File dataFile;
	if(!SD.exists("Data.csv"))
		{
		dataFile = SD.open("Data.csv", FILE_WRITE);
		if (dataFile) {
				//Serial.print("Init and Writing to Data.csv...");
				dataFile.println("year-month-day,hour:minutes:secondes,H1,H2,H3,tempAir,HAir");
				// close the file:
				dataFile.close();
				Serial.println("Data.csv is create.");
			  } else {
				// if the file didn't open, print an error:
				Serial.println("error opening Data.csv");
			  }
		}else Serial.print("Data.csv already exist\n");
}

void initParamFile(){
	File paramFile;
	if(!SD.exists("Param.csv"))
		{
		paramFile = SD.open("Param.csv", FILE_WRITE);
		if (paramFile) {
//				Serial.print("Init and Writing to Param.csv...");
			paramFile.println("frequenceMesure,timerPump,limitHumidity");
				String param=(String)FREQ_MESURE
						+","
						+(String)TEMPS_ARROSAGE
						+","
						+(String)LIMITE_HUMIDITE;
				paramFile.println(param);
				// close the file:
				paramFile.close();
				Serial.println("Param.csv is create.");
		  } else {
				// if the file didn't open, print an error:
				Serial.println("error opening Param.csv");
			  }
		}else {
			Serial.print("Param.csv already exist\n");
		}
}

void writeDataToBLE(HumidityGND humidityGND,ts t,TempDHT11 temp) {
  Serial.println("Writing to BLE");
  /*String data=dateToDigit(t.year)
  	    		+"-"
  	    		+dateToDigit(t.mon)
  				+"-"
  				+dateToDigit(t.mday)
  				+","
  				+dateToDigit(t.hour)
  				+":"
  				+dateToDigit(t.min)
  				+":"
  				+dateToDigit(t.sec)
  				+","
  				+humidityGND.hsol1
  				+","
  				+humidityGND.hsol2
  				+","
  				+humidityGND.hsol3
  				+","
  				+temp.temp
  				+","
  				+temp.humidity;
  char copyData[data.length()];
  data.toCharArray(copyData,data.length()+1);
  BLESerial.write(copyData, data.length()+1);*/
  BLESerial.write("yo");
}

void readParamSD (){
	//todo lire les params sur SD si le fichier existe
	//sinon creer le fichier avec valeurs par défaut
	//myFile=SD.open("Param.csv",FILE_READ);
	frequenceMesure=FREQ_MESURE;
	limiteHumidite=LIMITE_HUMIDITE;
	temspArrosage=TEMPS_ARROSAGE;
}

void refreshParamToSD(){
	//todo ecrire les nouvelles valeurs des params demandés par le tel
}

void setup() {
 Serial.begin(115200);
 while (!Serial) {
     ; // wait for serial port to connect. Needed for native USB port only
   }
 BLESerial.begin(9600);
 Wire.begin();
 DS3231_init(DS3231_INTCN);

 pinMode(PIN_ANA_H1,INPUT);
 pinMode(PIN_DIGIT_H1,INPUT);
 pinMode(PIN_ANA_H2,INPUT);
 pinMode(PIN_DIGIT_H2,INPUT);
 pinMode(PIN_ANA_H3,INPUT);
 pinMode(PIN_DIGIT_H3,INPUT);

 pinMode(PIN_POMPE,OUTPUT);

 demandMAJ=false;
 demandLaunchPump=false;
 refreshParam=false;

 Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

 initParamFile();
 initDataFile();

 //todo init les params en lisant fichier de param
 readParamSD();

 tG =getDate();
 tOld=tG;
 Serial.print("Welcome in SmartGreen \n");

}


void loop() {
	tG =getDate();

	if(demandMAJ || compDate(tG,tOld))
	{
		//tG =getDate();
		HumidityGND humi=getHumidityGnd();
		TempDHT11 temp=getHumidityAir();
		writeDataToSD(humi,tG,temp);

		if(demandMAJ){
			writeDataToBLE(humi,tG,temp);
		}
		else {
			if(checkHumidityGnd(humi)){
				launchPump(temspArrosage);
			}
		}

		writeDataToBLE(humi,tG,temp);
		tOld=tG;
		demandMAJ=false;
	 }

	if(demandLaunchPump){
		launchPump(TEMPS_ARROSAGE);
		demandLaunchPump=false;
	}

	if(refreshParam){
		//todo lire données du tel et enregistrer valeurs params
		//refreshParamToSD();
		refreshParam=false;
	}

	delay(1000);
}

