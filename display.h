//Display OLED declares

#define DSP_SDAPIN  5 //21 ESP32 MPU //4 ESP8266 // 21 for ESP32
#define DSP_SCLPIN  4 //22 ESP32 MPU //5 ESP8266 // 22 for ESP32

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
//#include <Fonts/Tiny3x3a2pt7b.h>


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//#define PORTRAIT // uncomment this line for Portrait

#define GAUGESIZE  30 //radius of gauge in pixe 
#define AngleX 55
#define AngleY 25
#define AccBarX 110
#define BrkBarX 120


// screen size
// 128x32 pixels monocrome

void displaySetup(uint8_t reset = true){

  // OLED Setup
  /*
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  */

  // Initialising the UI will init the display too.
  Wire.begin(DSP_SDAPIN, DSP_SCLPIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.display();
  //delay(2000);
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  
  DEBUG_MSG("font H:%d\tW:%d\n",FreeSans9pt7b.glyph->height, FreeSans9pt7b.glyph->width);


  #ifdef PORTRAIT
    display.setRotation(1);
  #endif

  //display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_16); // ArialMT_Plain_10/ArialMT_Plain_16/ArialMT_Plain_24
  //display.setTextAlignment(TEXT_ALIGN_LEFT);
  //display.setTextSize(16);

  //display.clear();
  //display.drawString(0,0,"Start");
  //display.display();
}
void drawHProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress) {
  uint16_t radius = height / 2;
  uint16_t xRadius = x + radius;
  uint16_t yRadius = y + radius;
  uint16_t doubleRadius = 2 * radius;
  uint16_t innerRadius = radius - 2;

  display.drawCircleHelper(xRadius, yRadius, radius, 0b00001001,WHITE);
  display.drawFastHLine(xRadius, y, width - doubleRadius,WHITE);
  display.drawFastHLine(xRadius, y + height-1, width - doubleRadius ,WHITE);
  display.drawCircleHelper(x + width - radius-1, yRadius, radius, 0b00000110,WHITE);
  // clean previous bar
  display.fillCircle(xRadius, yRadius, innerRadius,BLACK);
  display.fillRect(xRadius , y + 1, width-doubleRadius, height - 2,BLACK);
  display.fillCircle(x+width-radius-1 + width, yRadius, innerRadius,BLACK);

  uint16_t maxProgressWidth = (width - doubleRadius - 1) * progress / 100;

  display.fillCircle(xRadius, yRadius, innerRadius,WHITE);
  display.fillRect(xRadius , y + 1, maxProgressWidth, height - 2,WHITE);
  display.fillCircle(xRadius + maxProgressWidth, yRadius, innerRadius,WHITE);
  display.display();
}
void drawVProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress) {
  uint16_t radius = width / 2 ;
  uint16_t xRadius = x + radius;
  uint16_t yRadius = y + radius;
  uint16_t doubleRadius = 2 * radius;
  uint16_t innerRadius = radius - 1;

  display.drawCircleHelper(xRadius, yRadius, radius, 0b00000011,WHITE);
  display.drawFastVLine(x, yRadius, height - doubleRadius ,WHITE);
  display.drawFastVLine(x + width-1, yRadius, height - doubleRadius ,WHITE);
  display.drawCircleHelper(xRadius, y+height-radius-1, radius, 0b00001100,WHITE);
  
  // clean previous bar
  display.fillCircle(xRadius, yRadius, innerRadius,BLACK);
  display.fillRect(x + 1, yRadius, width -2, height-doubleRadius,BLACK);
  display.fillCircle(xRadius , y+height-radius-1, innerRadius,BLACK);
  
  uint16_t maxProgressHeight = (height - doubleRadius-1) * progress / 100;
  
  display.fillCircle(xRadius, y+height-radius-1, innerRadius,WHITE);
  display.fillRect(x + 1, y+height-radius-1-maxProgressHeight, width -2, maxProgressHeight,WHITE);
  display.fillCircle(xRadius , y+height-radius-1-maxProgressHeight, innerRadius,WHITE);
  
  //display.display();
  
  //DEBUG_MSG("Bar Start:%d/%d\tprog:%d\tprogHeight:%d\n",x + 1,y+height-radius-1-maxProgressHeight,progress,maxProgressHeight);
}

/* 
*  size = circle radius of gauge
*  ratio if a float value of ratio of width/heigth due to phiscal display pixel size ratio
*/
void displayLineAngle(uint16_t x, uint16_t y, int8_t angle, uint8_t color = WHITE,  uint8_t size = GAUGESIZE,float ratio=1){

  int16_t endX, endY;
  float rad = (angle) * PI / 180;
  endY = -(size-2) * cos(angle) * ratio;
  endX = sqrt(pow(size-2,2)-pow(endY,2));
  DEBUG_MSG("cos*ratio:%f\tsize-2:%d\tendY:%d\tendX:%d\n",cos(rad)*ratio,(size-2), endY,endX);
  //endX = (size-2) * sin(rad);
  endY += y;
  endX += x;
  display.drawLine(x,y,endX,endY,color);
  //display.display();

}
void displayAngleGauge(uint8_t x, uint8_t y, uint8_t size){


  uint8_t centerX=x+size, centerY = y;

  DEBUG_MSG("G center:%d,%d\n",centerX,centerY);
  DEBUG_MSG("L Start:%d\n",centerX-size);
  // assuming LANDSCAPE
  display.drawFastHLine(centerX-size, centerY,size*2,WHITE);
  display.drawFastVLine(centerX, 2,size-2,WHITE );
  display.drawCircleHelper(centerX, centerY,size,3,WHITE);
  
  //display.display();


}
void displayAngle(uint8_t x, uint8_t y, int8_t angle){

    //DEBUG_MSG("[displayAngle] angle:%d\n",angle);

    //display.setTextColor(BLACK,WHITE);
    static int16_t prevAngle = 0;
    int16_t x1,y1;
    uint16_t w,h;
    char tmpStr[5];
    sprintf(tmpStr,"%+03d",prevAngle);
    prevAngle = angle;
    display.getTextBounds(tmpStr,(int16_t)x,(int16_t)y,&x1,&y1,&w,&h);
    display.fillRect(x1,y1,w,h,BLACK);
    display.setCursor(x,y);
    display.printf("%+03d",angle);
    //display.display();
    //display.setTextColor(WHITE,BLACK);
}

void displayAngle(int8_t angle){
    displayAngle(AngleX,AngleY,angle);
}

void displayMaxLeanAngle(int8_t left, int8_t right){

display.setCursor(0,13);
display.fillRect(0,0,40,15,BLACK);
display.printf("L%02d",left);
display.setCursor(0,13+15);
display.fillRect(0,15,40,15,BLACK);
display.printf("R%02d",right);
//display.display();
}

void displayAcceleration(int16_t acc){

  if(acc >= 0){
    drawVProgressBar(BrkBarX,0,7,30,acc);
    drawVProgressBar(AccBarX,0,7,30,0);
  }
  else {
    drawVProgressBar(BrkBarX,0,7,30,0);
    drawVProgressBar(AccBarX,0,7,30,-acc);
  }
}
