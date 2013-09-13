//helper macros for constants
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// ########################################### //
// ++++++++++++++ CONFIGURATION ++++++++++++++ //
// ########################################### //

#define DATABASE "nedm%2Ftemperature"
#define COOKIE "c29sYXI6NTIwNEU4OTA66nT2KHyr9n5RrouelBtLnBru00c"
#define INTERVAL 1000

#define SERVERNAME "db.nedm1"
#define MACADDRESS 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
#define PIN 7
#define SERVERADDRESS 10,155,59,172
#define COUCHDBPORT 5984

// ########################################### //
// -------------- CONFIGURATION -------------- //
// ########################################### //


// create beginning of document sent to CouchDB
#define DOCUMENT "POST http://10.155.59.172:"STR(COUCHDBPORT)"/"DATABASE"/_design/nedm_default/_update/insert_with_timestamp HTTP/1.0\nContent-Length: ###\nCookie: AuthSession="COOKIE"\nX-CouchDB-WWW-Authenticate: Cookie\nContent-Type: application/json\n\n{\"type\":\"data\",\"value\":{"

// include all the libraries that we need
#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>

// Enter a MAC address for your controller below.
byte mac[] = {MACADDRESS};

// OneWire Sensors connected on digatal pin 7
OneWire  ds(PIN);

// Enter the IP address of the server you're connecting to:  138.246.58.26
IPAddress server(SERVERADDRESS);
// ... or enter the hostname of the server.
//char server[] = SERVERNAME;

EthernetClient client;

boolean conv = true; // for paralell output
boolean network = false; // to enable network related stuff
boolean couchdb = false; //  to enable couchdb related stuff

long previousMillis = 0;
long interval = 250;

// arduino setup
void setup() {
 // Open serial communications and wait for port to open:
  //Ethernet.begin(mac, ip);
  Serial.begin(9600);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    network = false;
  } else {
    network = true;
    Serial.println("connecting to CouchDB...");
    // give the Ethernet shield a second to initialize:
    delay(1000);
    // if you get a connection, report back via serial:
    if (client.connect(server, COUCHDBPORT)) {
      Serial.println("connected to CouchDB");
      couchdb = true;
    } 
    else {
      // if you didn't get a connection to the server:
      Serial.println("connection failed. Maybe CouchDB isn't running?");
      couchdb = false;
      
    }
  }
}

void loop() {
  
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  
  
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;
    
    // tell ALL DS18B20 s to start data acquisition
    if (conv==true) {
      while (ds.search(addr)) {
        ds.reset();
        ds.select(addr);
        ds.write(0x44,0);         // start conversion, with parasite power off at the end
      } 
      ds.reset_search();
      conv = false; // don't need to convert next time
      interval = INTERVAL; // interval until data will be read

      if (network == true && couchdb == false && client.connect(server, 5984)) {
        Serial.println("connected to CouchDB");
        couchdb = true;
      } 
      return;
    }

  // get date, read data and write everything to CouchDB
   if (conv == false) {
   
    String doc = DOCUMENT;
    
    int len;
    int l0 = doc.length()-26;
    
    boolean first = true;
    int numbsen = 0;
    
    while (ds.search(addr)) {
 
      numbsen += 1;
 
      ds.reset();
      ds.select(addr);    
      ds.write(0xBE);         // Read Scratchpad
      for ( i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
      }
      // convert the data to actual temperature
      unsigned int raw = (data[1] << 8) | data[0];
      byte cfg = (data[4] & 0x60);
      if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
   
      celsius = (float)raw / 16.0;
      int front = (int)celsius;
      int back = (int)(100*celsius)-100*front;
   
      if (first == false) {
        doc += ",";
      }
      else {first = false;}
      
      doc += "\"";

      for( i = 0; i < 8; i++) {
        char buffer [33];
        doc += itoa (addr[i],buffer,16);
      }
      Serial.print(numbsen);
      Serial.print(": ");
      Serial.print(front);
      Serial.print(".");
      Serial.println(back);
 
      doc += "\":";
      doc += front;
      doc += ".";
      doc += back;

      len = doc.length()-l0;
     
    }
    Serial.print(numbsen);
    Serial.println(" sensors");
    doc += "}}";
    
    {String le = ""; le+=len;
    doc.replace("###",le);
    }

    if (couchdb == true) {

      if (client.connected()) {
         Serial.println(doc);
         client.print(doc);
      }
       client.stop();
       delay(250);
       client.connect(server, 5984);
    
    }
    
    ds.reset_search();
    conv = true;
    interval = 250;
    
    Serial.println();
    
    return;
    }
    
    //you can put other code here:
    
  }

}
