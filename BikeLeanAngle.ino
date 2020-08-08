/*
*   This software was developed as a research base to produce several autonomous projects
*   this code is free to be copied as long as it respects the licence agreement and keep
*   
*   History:
*   
*   version 1.0
*   first working version developed on ESP32, but tested partially on ESP8266 and Arduino Nano
*   features:
*     continuous reading of MPU, interrupt driven
*     instant, min and max lean angle display
*     buz when min or max angle updated
*     instant acceleration display on progressbars and on a strip of NEO Pixels
*     Neo pixels blink on emergency, and light proportionaly to the acceleration strengh

*/

#define appVersion "V1.0" 

#include <ESPBASE.h>
#include <I2Cdev.h>
#include <OSCMessage.h>

#include <MPU6050_6Axis_MotionApps20.h>
#include <DebugTools.h>
#include <PABlink.h>
#include <PANeoPix.h>
#include <ESP32PWM.h>
#include "display.h"
#include "appParameters.h"

TaskHandle_t core0taskHandler;

//********
// GLOBAL VARS AND OBJECTS FOR WFI
//***********

ESPBASE Esp;

#define LED 16 //2

// Buzzer parameters
#define BUZ 18 
#define Hightone 8000
#define Lowtone 4000

void playTone(uint8_t pin, int16_t freq, uint32_t dur){
  PWM_initialize(pin,0,255,freq);
  analogWrite(pin, 128);
  delay(dur);
  analogWrite(pin,0);

}

// Include the app HTML, STYLE and Script "Pages"
#include "Page_app.h"


/*
*   Defines for Brake sensor
*/

#define MPU_INT 23 //5 //for ESP32  //14 for ESP8266
#define SCLPIN  4 //5 ESP8266 // 22 for ESP32
#define SDAPIN  5 //4 ESP8266 // 21 for ESP32

uint32_t valueSetTimestamp;
bool valuelock = false;

#include <PAMpu.h>


//#define BRAKE_THRS -500
//#define ACCEL_THRS 500
//#define BRAKE_EMG -2000 // emergency brake
//#define ACCEL_EMG 2000  // Accelelration emergency

//uint8_t accAxis = 1;  // 1= X; 2 = Y; 3= Z
//uint8_t leanAxis = 1; // 1= X; 2 = Y; 3 = Z

/*
*   defines and object for NeoPixel
*/

#define NUMPIXELS 5

#ifdef AVR
  #define NEOPIX_PIN 6
#else
  #define NEOPIX_PIN 13 //15
#endif

PA_NeoPixel <NeoGrbFeature, Neo800KbpsMethod> pixels(NUMPIXELS, NEOPIX_PIN);
//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(NUMPIXELS, NEOPIX_PIN);

RgbColor NEORED(20, 0, 0);
RgbColor NEOGREEN(0, 20, 0);
RgbColor NEOBLUE(0, 0, 20);
RgbColor NEOWHITE(7);
RgbColor NEOBLACK(0);
RgbColor NEOYELLOW(12,8,0);
RgbColor NEOVIOLET(10,0,10);


// in case an HTTP rquest received not foreseen
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void core0task( void *pvPameters){

  // task specific setup()

  setup0();


  // task loop()
  while(true){
    loop0();
  }



}

void setup0(){

  Serial.print("Running at core:");
  Serial.println(xPortGetCoreID());

    // Neopixel setup
  pixels.Begin();
  pixels.Show();
  pixels.PIX_flash(0,NEOWHITE,3);

}

void setup(){
  Serial.begin(115200);


  //#### setup task to run on Core 0 ####

  xTaskCreatePinnedToCore(core0task, "core0task", 20000, NULL, 0, &core0taskHandler, 0); /* Core where the task should run */

  //delay(500); 


  // initiate display, show startup splash and draw dashboard
  displaySetup();
  display.clearDisplay();
  display.setCursor(0,15);
  display.printf("   Bike Lean ");
  display.setCursor(0,30);
  display.printf("      %s",appVersion);
  display.display();
  delay(2000);
  display.clearDisplay();

  displayAngle(0);
  display.display();

  for(float i = 1;i <10; i++){
      
      displayAngle((int8_t)(i*10));

  }
  display.display();

  displayMaxLeanAngle(0,0); // leff, right max angles
  displayAngle(0);
  displayAcceleration(0);  
      
  
  // initialize WiFi
  Esp.initialize(false);

  server.on("/appCFG",send_app_html);
  server.on("/appCFGvalues",send_app_values_html);
  server.onNotFound(handleNotFound);
  server.begin();

  // load app configurations
  ReadAppConfig(); // load appConfig structure with stored app parameters

  // intitiate MPU
  mpu_setup();
  mpu_Offsets(mpu, offsets[4]);
  //delay(5000); // settling time for MPU
  
	/*
  // Neopixel setup
  pixels.Begin();
  pixels.Show();
  pixels.PIX_flash(0,NEOWHITE,3);
  */


}


// variables common to core1 and core0

float leanAngle, minAngle = 0, maxAngle = 0 ;
uint8_t maxAngleChanged = false;
uint8_t newValues = false;
char strMSG[50]= "";
RgbColor color = 0; 
uint8_t level;
uint8_t emergency = false;
int16_t acceleration;


void loop0(){
    
  //DEBUG_MSG("core:%d\t",xPortGetCoreID());
  uint32_t loop0timestamp = millis();

  if(newValues)
  { 
    DEBUG_MSG("new values\n\n");
    newValues = false;
    // collect accelaratio form configured axis
    switch (appConfig.accAxis){
      case 1:// X axis   
        acceleration = aaReal.x;
        break;
      case 2: // Y axis
        acceleration = aaReal.y;
      case 3: // Z axis
        acceleration = aaReal.z;  
    } 

    // according to acc set color and level and emergency
    if(acceleration < appConfig.brakeEmg){
      color = NEOBLUE;
      emergency = true;
      level = 0 ;
    }
    else if(acceleration < appConfig.brakeThrs){ // brake has negative acceleration
      level = map(acceleration,appConfig.brakeEmg,appConfig.brakeThrs,5,1);
      color = NEORED;
      strcpy(strMSG,"\n ***  BRAKE! \n");
    }
    else if(acceleration > appConfig.accelThrs ){ // ACCEL has postive acceleration
      level = map(acceleration,appConfig.accelThrs,appConfig.accelEmg,1,5);
      color = NEOYELLOW;
      strcpy(strMSG,"\n --->  GAS!\n");

    }
    else {
      level = 0;
    }

    
    int16_t acc;
    acc = (acceleration * 100 / appConfig.brakeEmg);
    displayAcceleration( acc );
    
    /*
    String prtStr = "";
    char tmpStr[10] ="";
    prtStr = (String)"Lean:" + dtostrf(leanAngle,5,1,tmpStr) ;
    prtStr+= (String)"\tminAngle:"+ dtostrf(minAngle,5,1,tmpStr); 
    prtStr+= (String)"\tmaxAngle:"+ dtostrf(maxAngle,5,1,tmpStr) ;
    

    DEBUG_MSG("level:%d\tacc:%d\tacc%%:%d\t%s\ttaskTime:%d\n%s",level,acceleration, acc, prtStr.c_str(),millis()-readtime,strMSG);
    */
  }
  
  if(emergency ){  // flash LED according to emergency and level
  
    DEBUG_MSG("\n\n <<<<<<<    EMERGENCY >>>>>>>   %d\n\n",acceleration);
    // flash LEDs 3 times
    
    for(int j = 0; j<3; j++){ 
      // Switch ALL pixel ON
      for (int i = 0 ; i < NUMPIXELS; i++){
        pixels.SetPixelColor(i,color);
      }
      pixels.Show();
      delay(200);
      // Switch ALL pixel OFF
      for (int i = 0 ; i < NUMPIXELS; i++){
        pixels.SetPixelColor(i,NEOBLACK);
      }
      pixels.Show();
      delay(200);

    }
    
    emergency = false;
  }
  else{  // else light as many LED as Level
    DEBUG_MSG("\n INT - led start :%d\n",millis()-valueSetTimestamp);
    valueSetTimestamp = millis();

    for(int i = 0;i < NUMPIXELS; i++){ 
      if(i < level) pixels.PIX_on(i,color);
      else pixels.PIX_off(i); // turn off the remaing ones
    }
    DEBUG_MSG("\n INT - led END # :%d\n",millis()-valueSetTimestamp);
    valuelock = false;

  }
  
  DEBUG_MSG("\nloop0 flash:%d\n",millis()-loop0timestamp);
  loop0timestamp = millis();



}


void loop(){
  uint32_t loop1timestamp = millis() ;
  static uint32_t displayTimer = millis();
  uint32_t displayInterval_ms = 100;

  // OTA request handling
  ArduinoOTA.handle();

  //  WebServer requests handling
  server.handleClient();

   //  feed de DOG :)
  customWatchdog = millis();

  // blink LED every second
  blinkEvery(LED,1000,200);

  /*
  *   if the is an interrupt or sucessive interrupt, read all avaliable value
  *     and calculate Roll Pitch, Yaw and acceleration
  *     chech min or max lean have been achieved
  *   if angle < MinAngle update min angle
  *   if angle > Max angle update max angle
  *   if new reading show angle , min angle, max angle reading on display
  *   in case new lean value have been read display it by
  *     check min or max have been achieved and display it
  *     check if acceleration alarm levels have been reach and display NOEPixels
  *         if Y accel >  Break threshold 
  *             show neopixel NEORED
  *          else
  *             shutdown NeoPixel
  *   
  *         if Y accel < Accel threashold
  *             show neopixel NEOGREEN
  *         else
  *             shutdown NeoPixel
  *     display acceleration level on vertical prograssBars
  */
  DEBUG_MSG("\nloop1 - Wifi:%d\n",millis()-loop1timestamp);
  loop1timestamp = millis();
  while(mpuInterrupt){
    delay(1);
    DEBUG_MSG("#");
    newValues = true;
    DEBUG_MSG("\n INT - detect :%d\n",millis()-valueSetTimestamp);
    valueSetTimestamp = millis();

    if(!mpu_loop()){
      continue;
    }    
    DEBUG_MSG("\n INT - read :%d\n",millis()-valueSetTimestamp);
    valueSetTimestamp = millis();

  DEBUG_MSG("\nloop1 - MPU loop:%d\n",millis()-loop1timestamp);
  loop1timestamp = millis();

    /*
    * update Lean angle on display
    * */
    switch (appConfig.leanAxis){
      case 1: // X axis
          leanAngle = ypr[2];
          break;
      case 2: // ? Axis
          leanAngle = ypr[0];
          break;
      case 3: // ? Axis
          leanAngle = ypr[1];
          break;
    }
    leanAngle = leanAngle*180/M_PI; // roll angle to do : make angle config considering sensor position on bike
    if(leanAngle <= -90) leanAngle += 180;
    else if(leanAngle >= 90) leanAngle -= 180;
    
    if((int16_t)leanAngle < (int16_t)minAngle) {
      if(leanAngle >=-90){
        minAngle = leanAngle;
        maxAngleChanged = true;
      }
    }
    if((int16_t) leanAngle > (int16_t) maxAngle) {
      if(leanAngle <=90){
        maxAngle = leanAngle;
        maxAngleChanged = true;
      }
    } 
  }// while
  
  DEBUG_MSG("\nloop1 -  max angle calc:%d\n",millis()-loop1timestamp);
  loop1timestamp = millis();

  if( maxAngleChanged) {
    displayMaxLeanAngle(-minAngle,maxAngle);
    maxAngleChanged= false;
    playTone(BUZ,4000,500);
  }
  DEBUG_MSG("\nloop1 -  max angle disp :%d\n",millis()-loop1timestamp);
  loop1timestamp = millis();

  displayAngle((int8_t)leanAngle);
  DEBUG_MSG("\nloop1 -  angle disp :%d\n",millis()-loop1timestamp);
  loop1timestamp = millis();
  
  
  if(millis()-displayTimer > displayInterval_ms){
    display.display();
    displayTimer = millis();
  }
  
  
  DEBUG_MSG("\nloop1 -  disp update :%d\n",millis()-loop1timestamp);
  loop1timestamp = millis();

}


