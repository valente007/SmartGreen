#include <Arduino.h>
#include <Wire.h>
#include "ds3231-master/ds3231.h"
#include <SPI.h>
#include "SdFat.h"
#include <SimpleDHT.h>

SdFat SD;
#define SD_CS_PIN SS
//File paramFile;
File dataFile;
//capteurs humidités
int pinAnaH1=0;
int pinDigiH1=2;
int pinAnaH2=1;
int pinDigiH2=3;
int pinAnaH3=2;
int pinDigiH3=4;

int hsol1=0;
int hAlarm1=0;
int hsol2=0;
int hAlarm2=0;
int hsol3=0;
int hAlarm3=0;
float valeurEtalonnageB=350;//valeur capteur qd dans l'eau à 100%
float valeurEtalonnageH=1000;//valeur qd est dans l'air soit 0%
///////////////////

//Pompe
int pinPompe=7;
///////

int pinDHT11 = 8;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;

//RTC
//int year=0;
//int month=0;
//int day=0;
//int hour=0;
//int minutes=0;
//int secondes=0;

//actions via l'appli
bool demandMAJ;
bool demandLaunchPump;
bool refreshParam;

//parametres de l'appli
int limitHumidite=50;//en pourcent
int frequenceMesure=10*60;//en sec
int tempsArrosage=3*1000;//en ms


struct ts t;
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

void launchPump(int time){
	digitalWrite(pinPompe,true);
	delay(time);
	digitalWrite(pinPompe,false);
}

float convertDataH(float mesure){
	float pourcentage=100-((mesure-valeurEtalonnageB)/((valeurEtalonnageH-valeurEtalonnageB)/100));
	if(mesure>valeurEtalonnageH) pourcentage =0;
	if(mesure<valeurEtalonnageB) pourcentage =100;
	return pourcentage;
}

void getHumidityGnd(){
	float temp =0;
	//Capteurs Humidités
		hsol1=analogRead(pinAnaH1);
		hAlarm1=digitalRead(pinDigiH1);
//		Serial.print("Capteur Humidité num1 : ");
//		Serial.print("  ");
//		Serial.print(hsol1);
//		Serial.print("  ");
//		temp=convertDataH(hsol1);
//		Serial.println(temp,1);
//		Serial.print(" %");
//		Serial.print("\t");
//		Serial.print (hAlarm1);
//		Serial.print("\n");

		hsol2=analogRead(pinAnaH2);
		hAlarm2=digitalRead(pinDigiH2);
//		Serial.print("Capteur Humidité num2 : ");
//		Serial.print("  ");
//		Serial.print(hsol2);
//		Serial.print("  ");
//		temp=convertDataH(hsol2);
//		Serial.println(temp,1);
//		Serial.print(" %");
//		Serial.print("\t");
//		Serial.print (hAlarm2);
//		Serial.print("\n");

		hsol3=analogRead(pinAnaH3);
		hAlarm3=digitalRead(pinDigiH3);
//		Serial.print("Capteur Humidité num3 : ");
//		Serial.print("  ");
//		Serial.print(hsol3);
//		Serial.print("  ");
//		temp=convertDataH(hsol3);
//		Serial.println(temp,1);
//		Serial.print(" %");
//		Serial.print("\t");
//		Serial.print (hAlarm3);
//		Serial.print("\n");
}

void checkHumidityGnd(){
	float moy = (convertDataH(hsol1)+convertDataH(hsol2)+convertDataH(hsol3))/3;
//	Serial.print("Moyenne Capteurs Humidités : ");
//	Serial.println(moy,1);
//	Serial.print(" %\n");

	if(moy<limitHumidite){
		Serial.print("Pompe lancée \n");
		launchPump(tempsArrosage);
	}
}

void getDate(){
	//RTC
		DS3231_get(&t);
//		Serial.print("date : ");
//		Serial.print(dateToDigit(t.mday));
//		Serial.print("/");
//		Serial.print(dateToDigit(t.mon));
//		Serial.print("/");
//		Serial.print(dateToDigit(t.year));
//		Serial.print("\t Heure : ");
//		Serial.print(dateToDigit(t.hour));
//		Serial.print(":");
//		Serial.print(dateToDigit(t.min));
//		Serial.print(".");
//		Serial.print(dateToDigit(t.sec));
//		Serial.print("\n");
//		year=t.year;
//		month=t.mon;
//		day=t.mday;
//		hour=t.hour;
//		minutes=t.min;
//		secondes=t.sec;
}

void getHumidityAir(){
	int err = SimpleDHTErrSuccess;
	if ((err = dht11.read(pinDHT11, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
	Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
	return;
	}

//	  Serial.print((int)temperature); Serial.print(" *C, ");
//	  Serial.print((int)humidity); Serial.println(" H");
}

bool compDate(){
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
	if(lastCalcul>=frequenceMesure){
		res=true;
	}
//	Serial.print("La dernière mesure il y a : ");
	Serial.print((lastCalcul));
	Serial.print("sec\n");
	return res;
}
void writeDataToSD() {
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
				+convertDataH(hsol1)
				+","
				+convertDataH(hsol2)
				+","
				+convertDataH(hsol3)
				+","
				+(int)temperature
				+","
				+(int)humidity;
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
	if(!SD.exists("Param.csv"))
		{
		dataFile = SD.open("Param.csv", FILE_WRITE);
		if (dataFile) {
//				Serial.print("Init and Writing to Param.csv...");
				dataFile.println("frequenceMesure,timerPump,limitHumidity");
				String param=(String)frequenceMesure
						+","
						+(String)tempsArrosage
						+","
						+(String)limitHumidite;
				dataFile.println(param);
				// close the file:
				dataFile.close();
				Serial.println("Param.csv is create.");
		  } else {
				// if the file didn't open, print an error:
				Serial.println("error opening Param.csv");
			  }
		}else {
			Serial.print("Param.csv already exist\n");
		}
}

void readParamSD (){
	//todo lire les params sur SD si le fichier existe
	//sinon creer le fichier avec valeurs par défaut
	//myFile=SD.open("Param.csv",FILE_READ);
}

void refreshParamToSD(){
	//todo ecrire les nouvelles valeurs des params demandés par le tel
}

void setup() {
 Serial.begin(9600);

 Wire.begin();
 DS3231_init(DS3231_INTCN);

 pinMode(pinAnaH1,INPUT);
 pinMode(pinDigiH1,INPUT);
 pinMode(pinAnaH2,INPUT);
 pinMode(pinDigiH2,INPUT);
 pinMode(pinAnaH3,INPUT);
 pinMode(pinDigiH3,INPUT);

 pinMode(pinPompe,OUTPUT);

 demandMAJ=false;
 demandLaunchPump=false;
 refreshParam=false;

 Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

 //obtenir les premières données
 getDate();
 tOld=t;
 getHumidityGnd();
 getHumidityAir();
 //checkHumidity();
 initParamFile();
 initDataFile();
 writeDataToSD();

 //todo init les params en lisant fichier de param
// readParamSD();
 //LastMAJ = temps actuelle;
 Serial.print("Welcome in SmartGreen \n");

}


void loop() {

//	getDate();
//	getHumidite();
//	checkHumidity();
	if(demandMAJ || compDate())
	{
		getDate();
		getHumidityGnd();
		getHumidityAir();
		writeDataToSD();

		if(demandMAJ){
			//todo envoi de données via Bluetooth
		}
		else checkHumidityGnd();

		tOld=t;
		demandMAJ=false;
	 }

	if(demandLaunchPump){
		launchPump(tempsArrosage);
		demandLaunchPump=false;
	}

	if(refreshParam){
		//todo lire données du tel et enregistrer valeurs params
		//refreshParamToSD();
		refreshParam=false;
	}

	delay(1000);
}

