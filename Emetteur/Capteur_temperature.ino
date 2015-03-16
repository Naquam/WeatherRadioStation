/*
 * Code pour construction d'une sonde de temperature "maison", récupère une temperature et l'envois sur la fréquence de 433 mhz
 * Fréquence : 433.92 mhz
 * Protocole : Ridle (Radio Idle, protocole customisé basé sur home easy)
 * Licence : CC -by -sa
 * Matériel associé : Atmega 328 (+résonateur associé) + emetteur RF AM 433.92 mhz + capteur DS18B20 + led d'etat + résistance 4.7 kohms
 * Auteur : Valentin CARRUESCO  aka idleman (idleman@idleman.fr - http://blog.idleman.fr)
 */
 
#include <OneWire.h>
#include "DHT.h"

//La sonde de température et d'humidité DHT22 est branchée au pin 2 de l'atmega
#define DHTPIN 2
//L'émetteur radio 433 mhz est branché au pin 9 de l'atmega
#define TRANSMITTER_PIN 9

// La sonde est de type DHT22 (peut être remplacé par DHT11 ou DHT21)
#define DHTTYPE DHT22

// Initialise la sonde DHT Pour un Arduino normal de 16mhz
DHT dht(DHTPIN, DHTTYPE);

//Tableaud de stockage du signal binaire à envoyer
bool bit2[31]={};

//Fonction lancée à l'initialisation du programme
void setup(void)
{
  //On definis les logs à 9600 bauds
  Serial.begin(9600);
  //On initialise le capteur de temperature
  sensors.begin();
  //On définis le pin relié à l'emetteur en tant que sortie
  pinMode(TRANSMITTER_PIN, OUTPUT);
  //On démarre la réception de la sonde DHT
  dht.begin();
}

//Fonction qui boucle à l'infinis
void loop(void)
{ 
  // Récupère l'humidité
  float h = dht.readHumidity();
  // Récupère la température
  float t = dht.readTemperature();
  
  //Affichage de la température dans les logs
  Serial.print(h);  
  Serial.print(" - ");  
  Serial.println(t);  
  
  unsigned long intTemp = t;
  unsigned long floatTemp = 100.0*(t-(float)intTemp);
  bool *binIntTemp = itob(intTemp, 7);
  bool nbFloatTemp = floatTemp>9;
  bool *binfloatTemp = itob(floatTemp, 7);
  bool positiveTemp = t>=0;
  
  unsigned long intHum = h;
  unsigned long floatHum = 100.0*(t-(float)intHum);
  bool *binIntTemp = itob(intHum, 7);
  bool nbFloatHum = floatHum>9;
  bool *binfloatTemp = itob(floatHum, 7);
  
  int i;
  for(i=0; i<7; i++)
  {
	bit2[i] = binIntTemp[i];
  }
  bit2[7] = nbFloatTemp;
  for(i=0; i<7; i++)
  {
	bit2[8+i] = binFloatTemp[i];
  }
  bit2[15] = positiveTemp;
  
  for(i=0; i<7; i++)
  {
	bit2[16+i] = binIntHum[i];
  }
  bit2[23] = nbFloatHum;
  for(i=0; i<7; i++)
  {
	bit2[24+i] = binFloatHum[i];
  }
  
  //Envois du signal radio comprenant la temperature (on l'envois 5 fois parce qu'on est pas des trompettes :p, et on veux être sûr que ça recoit bien)
  i=0;
  for(i=0; i<5;i++)
  {
    transmit();
    delayMicroseconds(666);   
  }
  //delais de 5sc avant le prochain envois
  delay(5000);
}

//Fonction de tansmission radio
void transmit()
{
 
  // On envois 2 impulsions 1->0 avec un certain delais afin de signaler le départ du siganl( verrou de départ)
  //Initialisation radio à 1
  digitalWrite(TRANSMITTER_PIN, HIGH);
  delayMicroseconds(275);     
  //Verrou 1
  digitalWrite(TRANSMITTER_PIN, LOW);
  delayMicroseconds(9900);     
  digitalWrite(TRANSMITTER_PIN, HIGH); 
  //Pause entre les verrous  
  delayMicroseconds(275);     
  //Verrou 2
  digitalWrite(TRANSMITTER_PIN, LOW);    
  delayMicroseconds(2675);
  // End on a high
  digitalWrite(TRANSMITTER_PIN, HIGH);
 
 //On envois les 31 bits stockés dans le tableau bit2
 int i;
 for(i=0; i<31;i++)
 {
    sendPair(bit2[i]);
 }
 
 //On envois le code de la sonde (1010 = code 10)
 sendPair(true);
 sendPair(false);
 sendPair(true);
 sendPair(false);
 
 //On envois un verrou de fin pour signaler la fin du signal :)
  digitalWrite(TRANSMITTER_PIN, HIGH);   
  delayMicroseconds(275);     
  digitalWrite(TRANSMITTER_PIN, LOW);  
 
}
 
//Fonction d'envois d'un bit pure (0 ou 1) 
void sendBit(boolean b) {
  if (b) {
    digitalWrite(TRANSMITTER_PIN, HIGH);
    delayMicroseconds(310);   //275 orinally, but tweaked.
    digitalWrite(TRANSMITTER_PIN, LOW);
    delayMicroseconds(2500);  //1225 orinally, but tweaked.
  }
  else {
    digitalWrite(TRANSMITTER_PIN, HIGH);
    delayMicroseconds(310);   //275 orinally, but tweaked.
    digitalWrite(TRANSMITTER_PIN, LOW);
    delayMicroseconds(1000);   //275 orinally, but tweaked.
  }
}
 
//Fonction d'envois d'un bit codé en manchester (0 = 01 et 1 = 10) 
void sendPair(boolean b) {
  if(b)
  {
    sendBit(true);
    sendBit(false);
  }
  else
  {
  sendBit(false);
  sendBit(true);
  }
}
 
//fonction de conversion d'un nombre décimal "integer" en binaire sur "length" bits et stockage dans le tableau bit2
bool *itob(unsigned long integer, int length)
{
  bool *result = new bool[length];
  for (int i=0; i<length; i++){
    if ((integer / power2(length-1-i))==1){
      integer-=power2(length-1-i);
      result[i]=1;
    }
    else result[i]=0;
  }
  return result;
}

//Calcule 2^"power"
unsigned long power2(int power){    
 unsigned long integer=1;          
 for (int i=0; i<power; i++){      
   integer*=2;
 }
 return integer;
}
