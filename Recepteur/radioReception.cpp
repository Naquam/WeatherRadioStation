/*
 * Code pour reception d'un signal de sonde de temperature "maison", récupère une temperature et l'envois sur un fichier php passé en paramêtre
 * Fréquence : 433.92 mhz
 * Protocole : Ridle (Radio Idle, protocole customisé basé sur home easy)
 * Licence : CC -by -sa
 * Matériel associé : récepteur  RF AM 433.92 mhz 
 * Matériel associé : récepteur  RF AM 433.92 mhz 
 * Auteur : Valentin CARRUESCO  aka idleman (idleman@idleman.fr - http://blog.idleman.fr)
 */
 
#include <wiringPi.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <sstream>

using namespace std;


int pin = 7;

void log(string a){
	//Décommenter pour avoir les logs
	cout << a << endl;
}

void scheduler_realtime() {

struct sched_param p;
p.__sched_priority = sched_get_priority_max(SCHED_RR);
if( sched_setscheduler( 0, SCHED_RR, &p ) == -1 ) {
perror("Failed to switch to realtime scheduler.");
}
}

void scheduler_standard() {

struct sched_param p;
p.__sched_priority = 0;
if( sched_setscheduler( 0, SCHED_OTHER, &p ) == -1 ) {
perror("Failed to switch to normal scheduler.");
}
}

string longToString(long mylong){
    string mystring;
    stringstream mystream;
    mystream << mylong;
    return mystream.str();
}


int pulseIn(int pin, int level, int timeout)
{
   struct timeval tn, t0, t1;
   long micros;
   gettimeofday(&t0, NULL);
   micros = 0;
   while (digitalRead(pin) != level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros += (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   gettimeofday(&t1, NULL);
   while (digitalRead(pin) == level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros = micros + (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0;
   micros = micros + (tn.tv_usec - t1.tv_usec);
  
   return micros;
}

float convertFloat(unsigned long intPart, unsigned long nbFloat, unsigned long floatPart, unsigned long positive = 1){
	float result = intPart;
	if((nbFloat == 1 && floatPart > 99) || (nbFloat == 0 && floatPart > 9)){
		nbFloat = 1;
		floatPart = 99;
	}
	if(nbFloat==1){
		result += (float)floatPart/100;
	} else {
		result += (float)floatPart/10;
	}
	if(positive==0){
		result *= -1;
	}
	return result;
}

int main (int argc, char** argv)
{
//scheduler_realtime();
	string command;
	string path = "python ";
	path.append(argv[1]);
	log("Demarrage du programme");
	pin = atoi(argv[2]);
    if(wiringPiSetup() == -1)
    {
        log("Librairie Wiring PI introuvable, veuillez lier cette librairie...");
        return -1;
    }
    pinMode(pin, INPUT);
	log("Pin GPIO configure en entree");
    log("Attente d'un signal du transmetteur ...");
	for(;;)
    {
    	int i = 0;
		unsigned long t = 0;
		int prevBit = 0;
		int bit = 0;
	    unsigned long intTemp = 0;
		unsigned long floatTemp = 0;
		unsigned long numberFloatTemp = 0;
		unsigned long positiveTemp = 0;
		
		unsigned long intHum = 0;
		unsigned long floatHum = 0;
		unsigned long numberFloatHum = 0;
		
		unsigned long emiter = 0;
		
		float temperature;
		float humidity;
		
	    bool group=false;
	    bool on =false;
	    unsigned long recipient = 0;
		command = path+" ";
		t = pulseIn(pin, LOW, 1000000);

		while((t < 2550  || t > 2900)){
			t = pulseIn(pin, LOW,1000000);
		}

		while(i < 70)
		{
			t = pulseIn(pin, LOW, 1000000);
	        if(t > 500  && t < 1500)
			{
				bit = 0;
			}
			
	        else if(t > 2000  && t < 3000)
			{
				bit = 1;
			}
			else
			{
				i = 0;
				cout << "FAIL? = " << t << endl;
				break;
			}

			if(i % 2 == 1)
			{
				if((prevBit ^ bit) == 0)
				{
					i = 0;
					break;
				}

				if(i < 15)
				{
					intTemp <<= 1;
					intTemp |= prevBit;
				}else if(i == 15){
					numberFloatTemp = prevBit;
				}else if(i < 30){
					floatTemp <<= 1;
					floatTemp |= prevBit;
				}else if(i == 30){
					positiveTemp = prevBit;
				}else if(i < 45){
					intHum <<= 1;
					intHum |= prevBit;
				}else if(i == 45){
					numberFloatHum = prevBit;
				}else if(i < 60){
					floatHum <<= 1;
					floatHum |= prevBit;
				}else{
					emiter <<= 1;
					emiter |= prevBit;
				}
			}

			prevBit = bit;
			++i;
		}
		if(i>0){
			if(((numberFloatTemp == 1 && floatTemp > 99) || (numberFloatTemp == 0 && floatTemp > 9)) || ((numberFloatHum == 1 && floatHum > 99) || (numberFloatHum == 0 && floatHum > 9))){
				cout << "FAIL? = Wrong Datas" << endl;
				break;
			}
			temperature=convertFloat(intTemp, numberFloatTemp, floatTemp, positiveTemp);
			humidity=convertFloat(intHum, numberFloatHum, floatHum);
			
			log("------------------------------");
			log("Donnees detectees");
			
			cout << "temperature = " << temperature << " C" << endl;
			cout << "humidité = " << humidity << endl;
			cout << "code sonde = " << emiter << endl;
		
			command.append("UPDATE_ENGINE_STATE ");
			command.append(" "+longToString(emiter));
			command.append(" "+longToString(temperature));
			command.append(" "+longToString(positive));
			
			log("Execution de la commande.");
			log(command.c_str());
			system(command.c_str());
		}else{
			log("Aucune donnee...");
		}
	
    	delay(3000);
    }
//scheduler_standard();
}

