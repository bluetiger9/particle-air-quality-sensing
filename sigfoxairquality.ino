// This #include statement was automatically added by the Particle IDE.
#include <BMP180.h>

// This #include statement was automatically added by the Particle IDE.
#include <PMS7003-Particle-Sensor-Serial.h>

#define LED_BUILTIN D7

/*** SigFox ***/
#define Sigfox Serial1
#define SIGFOX_RESET_PIN D4
#define SIGFOX_SEND_INTERVAL (10 * 60000)

//Set to 0 if you don't need to see the messages in the console
#define DEBUG 1

void sigfoxBegin();
void sigfoxLoop();

long sigfoxNextSend = millis() + SIGFOX_SEND_INTERVAL;

typedef struct SigFoxMessage {
    uint16_t pm_1_0;
    uint16_t pm_2_5;
    uint16_t pm_10_0;
    uint16_t cnt_0_3;
    uint16_t cnt_0_5;
    uint16_t cnt_1_0;
} SigFoxMessage __attribute__((packed));

SigFoxMessage msg;

/*** PMS 7003 ***/

#define PMS_7003_SET_PIN D3

PMS7003Serial<USARTSerial> pms7003(Serial1, PMS_7003_SET_PIN);

unsigned long last = 0;
unsigned long last_pm_reading = 0;

void pmsBegin();
void pmsLoop();

/*** Particle ***/
#define PARTICLE_SEND_INTERVAL 10000

long particleNextSend = millis() + PARTICLE_SEND_INTERVAL;

void particleBegin();
void particleLoop();

/*** BMP180 ***/
BMP180 bmp180;

double temperature = 0;
double pressure = 0;
double pressureSL = 0;

void bmpBegin();
void bmpLoop();

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // wait for serial
  while (!Serial);
  
  if(DEBUG){
    Serial.begin(9600);
    Serial.println("Setup");
  }

  sigfoxBegin();
  pmsBegin();
  bmpBegin();
  particleBegin();
}

// the loop function runs over and over again forever
void loop() {
  pmsLoop();
  bmpLoop();
  sigfoxLoop();
  particleLoop();
}

/*** SigFox ***/
void sigfoxBegin() {
  // reset
  pinMode(SIGFOX_RESET_PIN, OUTPUT);
  digitalWrite(SIGFOX_RESET_PIN, LOW);
  
  // open Wisol communication
  Serial.println("SigFox begin");
  Sigfox.begin(9600);
  
}

bool sigfoxEnable() {
  Serial.println("Sigfox module enable.");
  digitalWrite(SIGFOX_RESET_PIN, HIGH);
  delay(100);
  
  Serial.println("SigFox getID");
  getID();
  delay(100);
  
  Serial.println("SigFox getPAC");  
  getPAC();
  
  return true;
}

bool sigfoxDisable() {
  Serial.println("Sigfox module disable.");
  digitalWrite(SIGFOX_RESET_PIN, LOW);
  delay(100);
}

uint16_t fixEndianness(uint16_t val) {
    return val; // not needed if we specify :little-endian in SigFox backend
    //return ((val & 0x00FF) << 8) + ((val & 0xFF00) >> 8);
}

void sigfoxLoop() {
  if (millis() >= sigfoxNextSend) {
    msg.pm_1_0 = fixEndianness(pms7003.GetData(pms7003.pm1_0));
    msg.pm_2_5 = fixEndianness(pms7003.GetData(pms7003.pm2_5));
    msg.pm_10_0 = fixEndianness(pms7003.GetData(pms7003.pm10));
    msg.cnt_0_3 = fixEndianness(pms7003.GetData(pms7003.count0_3um));
    msg.cnt_0_5 = fixEndianness(pms7003.GetData(pms7003.count0_5um));
    msg.cnt_1_0 = fixEndianness(pms7003.GetData(pms7003.count1um));
    
    pmsDisable();
    sigfoxEnable();
    
    sendMessage((uint8_t*) &msg, sizeof(msg));
    
    sigfoxDisable();
    pmsEnable();
    
    sigfoxNextSend = millis() + SIGFOX_SEND_INTERVAL;
  }
}

void blink(){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(200);    
}

//Get Sigfox ID
String getID(){
  String id = "";
  char output;

  Sigfox.print("AT$I=10\r");
  while (!Sigfox.available()){
     blink();
     Sigfox.print("AT$I=10\r");
  }

  while(Sigfox.available()){
    output = Sigfox.read();
    id += output;
    delay(10);
  }

  if(DEBUG){
    Serial.println("Sigfox Device ID: ");
    Serial.println(id);
  }

  return id;
}


//Get PAC number
String getPAC(){
  String pac = "";
  char output;

  Sigfox.print("AT$I=11\r");
  while (!Sigfox.available()){
     blink();
  }

  while(Sigfox.available()){
    output = Sigfox.read();
    pac += output;
    delay(10);
  }

  if(DEBUG){
    Serial.println("PAC number: ");
    Serial.println(pac);
  }

  return pac;
}


//Send Sigfox Message
void sendMessage(uint8_t msg[], int size){

  String status = "";
  char output;

  Sigfox.print("AT$SF=");
  for(int i= 0;i<size;i++){
    Sigfox.print(String((msg[i] >> 4) & 0x0F, HEX) + String(msg[i] & 0x0F, HEX));
    if(DEBUG){
      Serial.print("Byte:");
      Serial.print(msg[i], HEX);
      Serial.print(" ");
      Serial.print(String((msg[i] >> 4) & 0x0F, HEX));
      Serial.println(String(msg[i] & 0x0F, HEX));
    }
  }

  Sigfox.print("\r");

  while (!Sigfox.available()){
     blink();
  }
  while(Sigfox.available()){
    output = (char)Sigfox.read();
    status += output;
    delay(10);
  }
  if(DEBUG){
    Serial.println();
    Serial.print("Status \t");
    Serial.println(status);
  }
}

/** PMS 7003 ***/
void pmsBegin() {

}

void pmsEnable() {
    Serial.println("PMS7003 enable");
    pms7003.SetSleep(false);
}

void pmsDisable() {
    Serial.println("PMS7003 enable");
    pms7003.SetSleep(true);
}

void pmsLoop() {
  unsigned long now = millis();

  // check every time to see if there is data
  if (pms7003.Read()) {
    last_pm_reading = now;
  }

  if ((now - last) > 10000) {
    // Let us be generous. Active state the device
    // reports at least every 2.3 seconds.
    if ((now - last_pm_reading) > 10000) {
      Serial.println("No reading for at least 10 seconds!");
    } else {
      Serial.println("pm1:" + String(pms7003.GetData(pms7003.pm1_0)));
      Serial.println("pm2_5:" + String(pms7003.GetData(pms7003.pm2_5)));
      Serial.println("pm10:" + String(pms7003.GetData(pms7003.pm10)));
      Serial.println("um_3:" + String(pms7003.GetData(pms7003.count0_3um)));
      Serial.println("um_5:" + String(pms7003.GetData(pms7003.count0_5um)));
      Serial.println("um1:" + String(pms7003.GetData(pms7003.count1um)));
      Serial.println("um2_5:" + String(pms7003.GetData(pms7003.count2_5um)));
      Serial.println("um5:" + String(pms7003.GetData(pms7003.count5um)));
      Serial.println("um10:" + String(pms7003.GetData(pms7003.count10um)));
    }
    last = now;
  }
}

/*** Particle ***/
void particleBegin() {
}

void particleLoop() {
    if (millis() >= particleNextSend) {
        char msg[256];
        sprintf(
            msg, 
            "{ \"pm_1_0\": %d, \"pm_2_5\": %d, \"pm_10_0\": %d, \"cnt_0_3\": %d, \"cnt_0_5\": %d, \"cnt_1_0\": %d, \"temperature\": %.2f, \"pressure\": %.2f, \"pressureSL\": %.2f}",
            pms7003.GetData(pms7003.pm1_0),
            pms7003.GetData(pms7003.pm2_5),
            pms7003.GetData(pms7003.pm10),
            pms7003.GetData(pms7003.count0_3um),
            pms7003.GetData(pms7003.count0_5um),
            pms7003.GetData(pms7003.count1um),
            temperature,
            pressure,
            pressureSL
        );
        
        Serial.print("Particle publish: ");
        Serial.println(msg);
        
        Particle.publish("air-quality", msg, PRIVATE);
        
        particleNextSend = millis() + PARTICLE_SEND_INTERVAL;
    }
}

/*** BMP 180 ***/
void bmpBegin() {
    if (!bmp180.begin()) {
        Serial.println("BMP initialization failed");
    }
}

void bmpLoop() {
    long sleepMs = bmp180.startTemperature();
    if (sleepMs == 0) {
       Serial.println("BMP start temperature measurement failed");
    }
    
    delay(sleepMs);
    bmp180.getTemperature(temperature);
    
    sleepMs = bmp180.startPressure(2);
    if (sleepMs == 0) {
       Serial.println("BMP start pressure measurement failed");
    }
    
    delay(sleepMs);
    bmp180.getTemperature(pressure);
    
    pressureSL = bmp180.sealevel(pressure, 330.0);
}
