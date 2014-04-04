/*
 Скетч для термометра с двумя термодатчиками, запоминанием минимальной утренней температуры и отображением динамики изменений показаний

 */

// подключаем нужные нам библиотеки
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <EEPROM.h>

// инициализирует переменные
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
int buttonState = 0;
float prevInTemp;
float prevOutTemp;
int tempIndex = 0;
float minOutTemp;

// Кодируем значек градуса
uint8_t temp_cel[8] =
{
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000
}; 

// кодируем значек "стрелка вверх"
uint8_t arrow_up[8] =
{
  B00100,
  B01110,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000
};

 // кодируем значек "стрелка вверх"
uint8_t arrow_down[8] =
{
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100
};

void setup() {
  pinMode(13, OUTPUT);  
  pinMode(8, INPUT);
  digitalWrite(13, LOW); 
  
  minOutTemp = EEPROM_float_read(1);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16,2);
  lcd.createChar(0, arrow_up);
  lcd.createChar(1, temp_cel);
  lcd.createChar(2, arrow_down);
  lcd.setCursor(3,0);
  lcd.print("Termometer");
  lcd.setCursor(1,1);
  lcd.print("on two DS18B20");
  delay(1500); 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("by skrasnov.ru");
  lcd.setCursor(10,1);
  lcd.print("v0.1.7");
  delay(1500); 
}

void loop() {
  
  buttonState = digitalRead(8);

  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  while (buttonState==HIGH) {    
    // turn LED on:    
    digitalWrite(13, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Min out temp:");
    lcd.setCursor(0, 1);
    lcd.print(EEPROM_float_read(1));
    lcd.write(byte(1));
    lcd.print("C");
    buttonState = digitalRead(8);
    delay(500);
    
  }
  
  // Измеряем температуру
  float outTemp = getTemp(12);
  float inTemp = getTemp(11);

  // Сохраняем минимальное значение температуры в энергонезависимую память 
  if (outTemp < minOutTemp) {
    minOutTemp = outTemp;
    EEPROM_float_write(1,minOutTemp);
  }

  // Запоминаем значение температуры каждые 10 измерений
  if (tempIndex==0) {
    prevInTemp = inTemp;
    prevOutTemp = outTemp;
  }
 
  // Рисует стартовый экран термометра 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T(out):");
  lcd.setCursor(0,1);
  lcd.print("T(in):");
  
  // Отображаем температуру внутри
  lcd.setCursor(8, 0);
  lcd.print(outTemp);
  lcd.write(byte(1));
  lcd.print("C");
  (prevOutTemp < outTemp) ? lcd.write(byte(0)) : lcd.write(byte(2));
  
  // Отображаем температуры снаружи
  lcd.setCursor(8,1);
  lcd.print(inTemp);
  lcd.write(byte(1));
  lcd.print("C");
  (prevInTemp < inTemp) ? lcd.write(byte(0)) : lcd.write(byte(2));
  
  // Считаем каждое измерение температуры
  if (tempIndex==200) {
    tempIndex=0;
  } else {
    tempIndex++;
  }
  
  //
  delay(250); 
}

// Получаем температуру с датчика висящего на указанном пине
float getTemp(int pin) {
  OneWire ds(pin);
  
  byte i;
  byte data[12];
  byte addr[8];
  float celsius;


  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
   // return;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
      lcd.print("CRC Err");
      //return;
  }
 
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read ram of ds18b20

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time
  celsius = (float)raw / 16.0;
  return celsius;
}

void EEPROM_float_write(int addr, float val) // запись в ЕЕПРОМ
{ 
	  byte *x = (byte *)&val;
	  for(byte i = 0; i < 4; i++) EEPROM.write(i+addr, x[i]);
}
	 
float EEPROM_float_read(int addr) // чтение из ЕЕПРОМ
{   
	  byte x[4];
	  for(byte i = 0; i < 4; i++) x[i] = EEPROM.read(i+addr);
	  float *y = (float *)&x;
	  return y[0];
}
