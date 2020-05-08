#include <Wire.h>                           //Knižnjica za I2C povezavo arduina s senzorjem
#include <LiquidCrystal.h>                  //Knjižnjica za upravljanje LCD zaslona
#define ADDRESS_SENSOR 0x77                 //Naslov senzorja
#define tipka A0                            //Nastavitev za tipki levo in desno
int zasloni=1;                              //Nastavitev začetnega zaslona LCD zaslona
int tipka_desno;                            //Stanje tipke desno
int tipka_levo;                             //Stanje tipke levo
//Če ni nobena tipka pritisnjena je vrednost 1023
//Če je pritisnjena tipka levo vrednost niha med 480 in 500
//Če je pritisnjena tipka desno je vrednost 0
byte stopinja[8] = {
  B00010,
  B00101,
  B00010,
  B00000,
  B00000,
  B00000,
  B00000,
};

const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //Spremenjlivke za nastavitev LCD zaslona
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  //Nastavitev LCD zaslona
int16_t  ac1, ac2, ac3, b1, b2, mb, mc, md; //16 bitna spremenljivka tipa intiger (od -32,768 do 32,767)
uint16_t ac4, ac5, ac6;                     //16 bitna spremenljivka tipa intiger (od 0 do 65,535)
// Ultra Low Power       OSS = 0, OSD =  5 ms
// Standard              OSS = 1, OSD =  8 ms
// High                  OSS = 2, OSD = 14 ms
// Ultra High Resolution OSS = 3, OSD = 26 ms
const uint8_t oss = 3;                      //Nastavitev oversampling_setting 
const uint8_t osd = 26;                     //Nastavitev corresponding oversampling delay 
float T;                                    //Spremenljivka za temperaturo
float P;                                    //Spremenljivka za zračni tlak
float h;                                    //Spremenljivka za nadmorsko višino 
void setup() {
 Serial.begin(9600);                        //Aktivira serial port
 Wire.begin();                              //Aktivira I2C povezavo  
 lcd.begin(16, 2);                          //Nastavitev stolpcev in vrstic LCD zaslona
 inicializacija_senzorja();                 //Inicializira senzor                                

}

void loop() {
  int32_t b5;
  int32_t p;

  if(analogRead(tipka) == 0 && tipka_desno == 0){
    tipka_desno = 1;
    zasloni++;
    if(zasloni == 4){
      zasloni = 1;
      }
    }
  if(analogRead(tipka) <= 500 && analogRead(tipka) >= 480 && tipka_levo == 0){
    tipka_levo = 1;
    zasloni--;
    if(zasloni == 0){
      zasloni = 3;
      }
    }
  if(analogRead(tipka) == 1023){
    tipka_desno = 0;
    tipka_levo = 0;
    }
  
  b5 = temperatura();                       //Prebere vrednosti in izračuna temperaturo
  Serial.print("Temperatura = ");
  Serial.print(T);
  Serial.println(" °C");

  P = zracni_tlak(b5);                      //Prebere vrednosti in izračuna zračni tlak
  Serial.print("Zračni tlak = ");
  Serial.print(P);
  Serial.println(" Pa");

  h = nadmorska_visina(P);                  //Prebere vrednosti in izračuna nadmorsko višino
  Serial.print("Nadmorska višina = ");
  Serial.print(h);
  Serial.println(" m");
  Serial.println("");

  if(zasloni == 1){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Zracni tlak = ");
    lcd.setCursor(0, 1); 
    lcd.print((P / 100), 0);  
    lcd.print(" kPa"); 
  }
  if(zasloni == 2){
    lcd.clear();
    lcd.createChar(0, stopinja);
    lcd.setCursor(0, 0);
    lcd.print("Temperatura = ");
    lcd.setCursor(0, 1); 
    lcd.print(T, 1);
    lcd.print(" ");
    lcd.write(byte(0));  
    lcd.print("C"); 
  }
  if(zasloni == 3){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nadm. visina = ");
    lcd.setCursor(0, 1); 
    lcd.print(h, 0);  
    lcd.print(" m"); 
  }
  delay(250);                               //Počakamo četrt sekunde
}
//////////////////////////////////////////
//Inicializacija senzorskih spremenljivk//
//////////////////////////////////////////
void inicializacija_senzorja()
{
  ac1=read_2_bytes(0xAA);                   //Branje registrov
  ac2=read_2_bytes(0xAC);                   //
  ac3=read_2_bytes(0xAE);                   //
  ac4=read_2_bytes(0xB0);                   //
  ac5=read_2_bytes(0xB2);                   //
  ac6=read_2_bytes(0xB4);                   //
  b1=read_2_bytes(0xB6);                    //
  b2=read_2_bytes(0xB8);                    //
  mb=read_2_bytes(0xBA);                    //
  mc=read_2_bytes(0xBC);                    //
  md=read_2_bytes(0xBE);                    //
  
  Serial.println("Inicializacija senzorskih spremenljivk :");
  Serial.print(F("AC1 = ")); Serial.println(ac1);
  Serial.print(F("AC2 = ")); Serial.println(ac2);
  Serial.print(F("AC3 = ")); Serial.println(ac3);
  Serial.print(F("AC4 = ")); Serial.println(ac4);
  Serial.print(F("AC5 = ")); Serial.println(ac5);
  Serial.print(F("AC6 = ")); Serial.println(ac6);
  Serial.print(F("B1 = "));  Serial.println(b1);
  Serial.print(F("B2 = "));  Serial.println(b2);
  Serial.print(F("MB = "));  Serial.println(mb);
  Serial.print(F("MC = "));  Serial.println(mc);
  Serial.print(F("MD = "));  Serial.println(md);
  Serial.println("");
}

///////////////////////////////////
//Odčitavanje 2 bytov iz senzorja//
///////////////////////////////////
uint16_t read_2_bytes(uint8_t code)        //Funkcija za branje 16 bitnih registrov
{
  uint16_t value;
  Wire.beginTransmission(ADDRESS_SENSOR);         
  Wire.write(code);                               
  Wire.endTransmission();                         
  Wire.requestFrom(ADDRESS_SENSOR, 2);            
  if(Wire.available() >= 2)
  {
    value = (Wire.read() << 8) | Wire.read();     
  }
  return value;                                   
}
///////////////////////////
//Kalkulacija temperature//
///////////////////////////
int32_t temperatura()
{
  int32_t x1, x2, b5, UT;

  Wire.beginTransmission(ADDRESS_SENSOR); //Vzpostavimo I2C povezavo s senzorjem
  Wire.write(0xf4);                       //Pošljemo naslov registra
  Wire.write(0x2e);                       //Zapišemo podatke
  Wire.endTransmission();                 //Končamo I2C povezavo
  delay(5);                               //Počakamo
  
  UT = read_2_bytes(0xf6);                // Preberemo vrednost registra

  x1 = (UT - (int32_t)ac6) * (int32_t)ac5 >> 15;  //Izračun temperature v stopinjah celzijah
  x2 = ((int32_t)mc << 11) / (x1 + (int32_t)md);  //
  b5 = x1 + x2;                           //
  T  = (b5 + 8) >> 4;                     //
  T = T / 10.0;                           //
  return b5;                              //Vrnemo spremenljivko b5, ker jo bomo potrebovali pri merjenju tlaka
}
//////////////////////////////
//Kalkulacija zračnega tlaka// 
//////////////////////////////
int32_t zracni_tlak(int32_t b5){
  int32_t x1, x2, x3, b3, b6, p, UP;
  uint32_t b4, b7; 

  Wire.beginTransmission(ADDRESS_SENSOR);   //Vzpostavimo I2C povezavo s senzorjem 
  Wire.write(0xf4);                         //Pošljemo naslov registra
  Wire.write(0x34 + (oss << 6));            //Zapišemo podatke
  Wire.endTransmission();                   //Končamo I2C povezavo
  delay(osd);                               
  Wire.beginTransmission(ADDRESS_SENSOR);
  Wire.write(0xf6);                         
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS_SENSOR, 3);      
  if(Wire.available() >= 3)
  {
    UP = (((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | ((int32_t)Wire.read())) >> (8 - oss);
  }
  b6 = b5 - 4000;                           //Izračun zračnega tlaka v kilo paskalih
  x1 = (b2 * (b6 * b6 >> 12)) >> 11;        //
  x2 = ac2 * b6 >> 11;                      //
  x3 = x1 + x2;                             //
  b3 = (((ac1 * 4 + x3) << oss) + 2) >> 2;  //
  x1 = ac3 * b6 >> 13;                      //
  x2 = (b1 * (b6 * b6 >> 12)) >> 16;        //
  x3 = ((x1 + x2) + 2) >> 2;                //
  b4 = (ac4 * (uint32_t)(x3 + 32768)) >> 15;//
  b7 = ((uint32_t)UP - b3) * (50000 >> oss);//
  if(b7 < 0x80000000) {                     //
    p = (b7 * 2) / b4;                      //
    }                                       //
  else {                                    //
    p = (b7 / b4) * 2;                      //
    }                                       //
  x1 = (p >> 8) * (p >> 8);                 //
  x1 = (x1 * 3038) >> 16;                   //
  x2 = (-7357 * p) >> 16;                   //
  p = p + ((x1 + x2 + 3791) >> 4);          //                                  
  
  return p;                        
}
////////////////////////////////
//Kalkulacija nadmorske višine// 
////////////////////////////////
float nadmorska_visina(float P){
  float h = 44330 * (1 - pow((P / 101325),(1 / 5.2558797)));  //Formula za izračun nadmorske višine s pomočjo zračnega tlaka in konstante atm (zračni tlak morske gladine) 
  return h; 
}
