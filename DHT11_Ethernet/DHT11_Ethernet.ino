// Ethernet monitoring node Temperature and Humidity.
// <me@sergeykrasnov.ru>

#include "EtherCard.h"
#include "DHT.h"
//#include "OneWire.h"
#include <avr/wdt.h>

// change these settings to match your own setup
#define FEED    "1780818140"
#define APIKEY  ""


DHT dht(9, DHT11);  // pin 9, type dht11

//OneWire  ds(10);

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };   // my mac, look lie a cisco ;)
uint8_t myip[] = { 192, 168, 0, 91 };          // The fallback board address.
uint8_t mydns[] = { 8, 8, 8, 8 };        // The DNS server address.
uint8_t mygateway[] = { 192, 168, 0, 1 };    // The gateway router address.
uint8_t mysubnet[] = { 255, 255, 255, 0 };    // The subnet mask.
char website[] PROGMEM = "api.xively.com";

byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");

/*  wdt_disable();
  delay(5000); 
  wdt_enable (WDTO_8S); 
*/
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup()) {
   Serial.println("DHCP failed");
      ether.staticSetup(myip, mygateway, mydns);
  }
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);

  dht.begin();
}

void loop () {
/*  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
    ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  
   // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  Serial.println(celsius);
*/ 
  ether.packetLoop(ether.packetReceive());
  
  if (millis() > timer) {
    timer = millis() + 60000;
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
  
    if (!isnan(t) || !isnan(h)) {
      Serial.print("Humidity: "); 
      Serial.print(h);
      Serial.print(" %\t");
      Serial.print("Temperature: "); 
      Serial.print(t);
      Serial.println(" *C");
    
      byte sd = stash.create();
      stash.print("humidity,");
      stash.println(h);
      stash.print("temperature,");
      stash.println(t);
      stash.save();
    
      // generate the header with payload - note that the stash size is used,
      // and that a "stash descriptor" is passed in as argument using "$H"
      Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
                          "Host: $F" "\r\n"
                          "X-PachubeApiKey: $F" "\r\n"
                          "Content-Length: $D" "\r\n"
                          "\r\n"
                          "$H"),
              website, PSTR(FEED), website, PSTR(APIKEY), stash.size(), sd);
  
      // send the packet - this also releases all stash buffers once done
      ether.tcpSend();
    }  
    

  }
}
