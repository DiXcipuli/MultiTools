// multi_tool.ino
// Displays the temperature and humidity, compute distances
// (ultrasonic sensor + laser to show direction) and timer function.
// Project mainly made to use the pin extension module

#include <Wire.h>
#include <TFT.h>
#include <SPI.h>
#include <Keypad.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


//TEMP sensor -------------------------------------------------------------------
//-------------------------------------------------------------------------------
#define DHTTYPE    DHT11     // DHT 11
#define DHTPIN 2     // Digital pin connected to the DHT sensor 
float temp = 0;
float humidity = 0;
float previous_temp = 0;
float previous_humidity = 0;
DHT_Unified dht(DHTPIN, DHTTYPE);
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------


//LCD SCREEN---------------------------------------------------------------------
//-------------------------------------------------------------------------------
#define cs   10
#define dc   A2
#define rst  A3
TFT TFTscreen = TFT(cs, dc, rst);
char character[15] = {'_', '_', '_', '_'};
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------



//TIMER--------------------------------------------------------------------------
//-------------------------------------------------------------------------------
bool timer_on = false;
float current_timer = 0;
float timer_saved = 0;
float timer_offset = 0;
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------


//BUTTONS------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int previous_green_button_state = 0;
int previous_red_button_state = 1;
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

//ULTRQSONIC SENSOR--------------------------------------------------------------
//-------------------------------------------------------------------------------
#define trigPin A0
#define echoPin A1
long duration;
float distance;
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------


//CJMCU I2C----------------------------------------------------------------------
//-------------------------------------------------------------------------------
//byte received by the I2C expander pins.
byte input_byte = 0;
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------


//MENU---------------------------------------------------------------------------
//-------------------------------------------------------------------------------
const int menu_size = 3;
int index_menu = 0;
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------


 
void setup() {
  //Serial and Wire setup
  Serial.begin(9600);
  Wire.begin(); // wake up I2C bus

  //LCD screen ----------------------------------------------------------------------
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.setTextSize(3);
  TFTscreen.setRotation(2);
  TFTscreen.stroke(255,255,255);

  //ULTRASONIC SENSOR ---------------------------------------------------------------
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //CJMCU I2C -----------------------------------------------------------------------
  Wire.beginTransmission(0x20);
  Wire.write(0x00); // IODIRA register
  Wire.write(0x07); // Set A0, A1 and A2 as input.
  Wire.endTransmission();

  Wire.beginTransmission(0x20);
  Wire.write(0x12); // address GPIO A
  Wire.write(0x08);  // value to send (just to turn on the LED)
  Wire.endTransmission();


  Wire.requestFrom(0x20, 1); // request one byte of data from MCP20317
  input_byte = Wire.read(); 

  // Temperature
  //------------------------------------------------------------
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  //------------------------------------------------------------

}
 
void loop() {
  Wire.beginTransmission(0x20);
  Wire.write(0x12); // GPIOA register
  Wire.endTransmission();

  Wire.requestFrom(0x20, 1); // request one byte of data from MCP20317, to get the state of the red button
  input_byte = Wire.read();
  Serial.println(input_byte, BIN);
  
  if(bitRead(input_byte, 1) == 0 && previous_red_button_state == 1){
    index_menu ++;
    delay(200);

    if(index_menu >= menu_size)
      index_menu = 0;
      
    updateMenu();
  }
  previous_red_button_state == input_byte;


  
  // Depending on the menu state, pressing the green button will trigger different stuff
  switch(index_menu){
    case 0: // temperature sensor
      sensors_event_t event;
      dht.temperature().getEvent(&event);
      if (isnan(event.temperature)) {
        Serial.println(F("Reading Error!"));
      }
      else {
        temp = event.temperature;
        dht.humidity().getEvent(&event);
        if(isnan(event.relative_humidity)){
          Serial.println(F("Reading Error!"));
        }

        else{
          humidity = event.relative_humidity;
          if(previous_temp != temp || previous_humidity != humidity){
            TFTscreen.background(0, 0, 0);
            (String(temp) + " T").toCharArray(character, 15);
            TFTscreen.text(character,1,40);

            (String(humidity) + "%H").toCharArray(character, 15);
            TFTscreen.text(character,1,90);

            previous_temp = temp;
            previous_humidity = humidity;
          }
        } 
      }
    break;

    case 1: // Ultrasonic sensor
      if(bitRead(input_byte, 2) == 1){
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        duration = pulseIn(echoPin, HIGH);
        distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (back and forth)

        TFTscreen.background(0, 0, 0);
        String dist;
        if(distance < 100){
          dist = String(distance);
          dist.toCharArray(character, 15);
          TFTscreen.text(character,20,60);
          TFTscreen.text("cm",55,90);
        }
        else if(distance < 400){
          dist = String(float(distance/ 100));
          dist.toCharArray(character, 15);
          TFTscreen.text(character,30,60);
          TFTscreen.text("m",65,90);
        }
        else{
          TFTscreen.text("ndef",33,70);
        }
        
      }

    break;

    case 2: // Timer
      if(bitRead(input_byte, 2) == 1 && previous_green_button_state == 0){
        timer_on = !timer_on;
        timer_offset = millis();
      }
      if(timer_on){
        current_timer = timer_saved+ (millis() - timer_offset)/1000;
          TFTscreen.background(0, 0, 0);

        String(current_timer).toCharArray(character, 15);
        TFTscreen.text(character,20,65);
      }
      else{
        timer_saved = current_timer;
      }

      previous_green_button_state = bitRead(input_byte, 2);

    break;
  }
}

void updateMenu(){
  //Reset
  TFTscreen.background(0, 0, 0);
  Wire.beginTransmission(0x20);
  Wire.write(0x12); // address port A
  Wire.write(0x08);  // value to send
  Wire.endTransmission();
  current_timer = 0;
  timer_saved = 0;

  switch(index_menu){
    case 0:
      (String(temp) + " T").toCharArray(character, 15);
      TFTscreen.text(character,1,40);

      (String(humidity) + "%H").toCharArray(character, 15);
      TFTscreen.text(character,1,90);
    break;

    case 1:
      TFTscreen.text("Press",20,40);
      TFTscreen.text("Green",20,70);
      TFTscreen.text("Button",14,100);
      Wire.beginTransmission(0x20);
      Wire.write(0x12); // address port A
      Wire.write(0x18);  // value to send
      Wire.endTransmission();

    break;

    case 2:
      TFTscreen.text("Press",20,40);
      TFTscreen.text("Green",20,70);
      TFTscreen.text("Button",14,100);
    break;
    
  }
}
