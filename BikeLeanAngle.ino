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



void setup(){
  Serial.begin(115200);

  displaySetup();
  display.clearDisplay();
  display.setCursor(0,15);
  display.printf("   Bike Lean ");
  display.setCursor(0,30);
  display.printf("      %s",appVersion);
  display.display();
  delay(2000);
  display.clearDisplay();


  displayMaxLeanAngle(0,0); // leff, right max angles
  displayAngle(0);
  displayAcceleration(0);  
  
  /*
  displayAngleGauge(40,30,30); // size = 30 => 30 pixels high, 60 wide
  uint8_t centerX = 40+30, centerY=30;
      for(int i = 0 ; i < 40; i++){
        static int8_t lastAngle = 0;
        DEBUG_MSG("lastAngle:%d\tangle:%d\n",lastAngle, i);
        displayLineAngle(centerX, centerY, lastAngle,BLACK,GAUGESIZE,0.5f);
        displayLineAngle(centerX, centerY,i,WHITE,GAUGESIZE,0.5f);
        lastAngle = i;
        display.display();
        delay(200);
      }
  */
    
  

  // initialize WiFi
  Esp.initialize(false);

  ReadAppConfig(); // load appConfig structure with stored app parameters

  server.on("/appCFG",send_app_html);
  server.on("/appCFGvalues",send_app_values_html);
  server.onNotFound(handleNotFound);
  server.begin();


  // intitiate MPU
  mpu_setup();
  mpu_Offsets(mpu, offsets[4]);
  //delay(5000); // settling time for MPU
  
	  
  // Neopixel setup
  pixels.Begin();
  pixels.Show();
  pixels.PIX_flash(0,NEOWHITE,3);


}


void loop(){

  // OTA request handling
  ArduinoOTA.handle();

  //  WebServer requests handling
  server.handleClient();

   //  feed de DOG :)
  customWatchdog = millis();

  // if new reading , display
  
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

  static float leanAngle, minAngle = 0, maxAngle = 0 ;
  uint8_t maxAngleChanged = false;
  uint32_t readtime = millis();
  char strMSG[50]= "";
  uint8_t newValues = false;
  
  while(mpuInterrupt){
    //Serial.print("#");
    newValues = true;
    if(!mpu_loop()){
      continue;
    };
    /*
    if(overflow){
      overflow = false;
      fifoIdx = (fifoIdx -1) % 8;
      DEBUG_MSG("[1st after overflow Accel:%d\tyaw:%d\n\n",aaReal.x,ypr[2]*180/PI)+180;
      return;
    }

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
  }
  
  if(newValues)
  { 
    newValues = false;
    RgbColor color = 0; 
    uint8_t level;
    uint8_t emergency = false;
    int16_t acceleration;

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
         
    if(emergency){  // flash LED according to emergency and level
      mpu.setFIFOEnabled(false);
      //mpu.resetFIFO();
      mpuResetFIFO();
    
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
      mpu.setFIFOEnabled(true);

    }
    // else light as many LED as Level
    else{
      for(int i = 0;i < NUMPIXELS; i++){ 
        if(i < level) pixels.PIX_on(i,color);
        else pixels.PIX_off(i); // turn off the remaing ones
      }
    }

    if( maxAngleChanged) {
      displayMaxLeanAngle(-minAngle,maxAngle);
      maxAngleChanged= false;
      playTone(BUZ,4000,500);
    }
    
    displayAngle((int8_t)leanAngle);
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

}


