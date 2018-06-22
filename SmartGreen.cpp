#include <Arduino.h>
#include <Wire.h>
#include "ds3231-master/ds3231.h"

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
///////////////////

//Pompe
int pinPompe=7;
///////

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
int limitHumidité=50;//en pourcent
int frequenceMesure=20*60;//en sec
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

void getHumidite(){
	//Capteurs Humidités
		hsol1=analogRead(pinAnaH1);
		hAlarm1=digitalRead(pinDigiH1);
		Serial.print("Capteur Humidité num1 : ");
		Serial.print(hsol1);
		Serial.print("\t");
		Serial.print (hAlarm1);
		Serial.print("\n");

		hsol2=analogRead(pinAnaH2);
		hAlarm2=digitalRead(pinDigiH2);
		Serial.print("Capteur Humidité num2 : ");
		Serial.print(hsol2);
		Serial.print("\t");
		Serial.print (hAlarm2);
		Serial.print("\n");

		hsol3=analogRead(pinAnaH3);
		hAlarm3=digitalRead(pinDigiH3);
		Serial.print("Capteur Humidité num3 : ");
		Serial.print(hsol3);
		Serial.print("\t");
		Serial.print (hAlarm3);
		Serial.print("\n");
}

void checkHumidity(){
	int moy = (hsol1+hsol2+hsol3)/3;
	Serial.print("Moyenne Capteurs Humidités : ");
	Serial.print(hsol3);
	Serial.print("\n");

	if(moy<limitHumidité){
		launchPump(tempsArrosage);
	}
}

void getDate(){
	//RTC
		DS3231_get(&t);
		Serial.print("date : ");
		Serial.print(dateToDigit(t.mday));
		Serial.print("/");
		Serial.print(dateToDigit(t.mon));
		Serial.print("/");
		Serial.print(dateToDigit(t.year));
		Serial.print("\t Heure : ");
		Serial.print(dateToDigit(t.hour));
		Serial.print(":");
		Serial.print(dateToDigit(t.min));
		Serial.print(".");
		Serial.print(dateToDigit(t.sec));
		Serial.print("\n");
//		year=t.year;
//		month=t.mon;
//		day=t.mday;
//		hour=t.hour;
//		minutes=t.min;
//		secondes=t.sec;
}

bool compDate(){
	bool res =false;
	int hoursOld=tOld.hour;
	int minutesOld=tOld.min;
	int secondesOld=tOld.sec;
	getDate();
	if((t.hour*3600+t.min*60+t.sec-hoursOld*3600-minutesOld*60-secondesOld)>=frequenceMesure){
		res=true;
	}
	return res;
}
void writeDataToSD() {
	//todo ecrire les données des capteurs sur la SD
}

void readParamSD (){
	//todo lire les params sur SD si le fichier existe
	//sinon creer le fichier avec valeurs par défaut
}

void refreshParamToSD(){
	//todo ecrire les nouvelles valeurs des params demandés par le tel
}

void setup() {
 Serial.begin(115200);

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

 //obtenir les premières données
 getDate();
 tOld=t;
 getHumidite();
 checkHumidity();
 writeDataToSD();

 //todo init les params en lisant fichier de param
 readParamSD();

 //LastMAJ = temps actuelle;
 Serial.print("Welcome in SmartGreen \n");
}


void loop() {

if(demandMAJ || compDate())
{
	getDate();
	getHumidite();
	writeDataToSD();

	if(demandMAJ){
		//todo envoi de données via Bluetooth
		demandMAJ=false;
	}
	else checkHumidity();

	tOld=t;

 }

	if(demandLaunchPump){
		launchPump(tempsArrosage);
		demandLaunchPump=false;
	}

	if(refreshParam){
		//todo lire données du tel et enregistrer valeurs params
		refreshParamToSD();
		refreshParam=false;
	}

	delay(1000);
}

