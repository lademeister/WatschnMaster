#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h> 
#include <WiFiManager.h> 
#include <ArduinoOTA.h>
#include <WebSocketsClient.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Servo.h>
#include <Stepper.h>
#include "PCF8575.h"
#include "SSD1306Wire.h"
//#include "Adafruit_SSD1306.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <time.h>
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

//comment Vince for github-auto-update:
//you need to do some prerequisites on the ESP (setting up spiffs filesystem on ESP8266) and setting up arduino ide to be able to upload files to ESP8266 spiffs filesystem,
//and you need to create a certificate that you will upload to ESP8266.

//download only the file 'certs-from-mozilla.py' from  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi/examples/BearSSL_CertStore (put it to an raspi, run './certs-from-mozilla.py' and copy
//the created certs.ca file to your Mac - in this example the Mac is at 192.168.178.34 - for example by using screencopy (SCP)) to a folder named "data" within the sketch directory like this: 
//on the Raspi put this command in the shell to copy the file from raspi to MacBook into a folder named data that YOU need to create within the sketch folder if it doesnt exist:  
// scp certs.ar vince@192.168.178.34:"/Volumes/Macintosh\ HD/Users/vince/Documents/Arduino/Günter_Watschn/WatschnMaster_productive_code/data/"

//then you need the zip file from https://github.com/esp8266/arduino-esp8266fs-plugin/releases/tag/0.5.0.
//unpack and (if on MacOS) put it so that the path is like this:
//   /Users/vince/Documents/Arduino/tools/ESP8266FS/tool/esp8266fs.jar
//notice 'tools' and 'tool' in the path.
//then restart arduino IDE and you have a new entry under 'Werkzeuge' (or under tools if english): ESP8266 sketch data upload
//this will upload the files that are in the sketch folder of the currently openend sketch under /data - but only if that folder exists and contains a file.
//as we just copied the certificate to that folder (we created that folder manually in the sketch folder), we can now upload it, using that uploader tool.
//If it fails, it may be that spiffs is not yet correctly configured.
//To solve that, you need to upload any sketch with the setting: 'Flash size: 4MB (FS: 1MB OTA:–1019KB)', so that SPIFFS filesystem is being set up:

//For a board with 4MB flash, the memory should be set to allow 1MB program space, 1MB SPIFFS and 1MB for OTA. In Arduino IDE 1.8.10 with ESP8266 Core v2.6.3 the option is "4MB (FS:1MB, OTA:~1019KB)".

//TAKE CARE that you dont change those settings when creating new sketches for this device, otherwise the certificate may be lost and needs to be reuploaded (currently only possible with local access).

//make sure that you set the upload routine so that after uploading a file, you DO NOT (!!!!) upload a new sketch with the setting "erase flash: all flash contents", 
// but only with the setting "erase flash: only sketch" or "sketch + wifi settings". otherwise you will erase the certificate file.
//If you dont see those settings, choose 'generic ESP8266', then you will see it under 'Werkzeuge' (tools if english).
//see: https://github.com/yknivag/ESP_OTA_GitHub
//see: https://github.com/esp8266/arduino-esp8266fs-plugin/


//#################  PIN DEFINITIONS  #################
// Define I2C pins
#define SCL_PIN 5    // GPIO5/D1
#define SDA_PIN 4    // GPIO4/D2


// Define pins for LEDs with PWM
#define LED_Kopf 12  // GPIO12/D6
#define LED_BLUE 2   // GPIO2/D4

// Define pin for limit switch
#define LIMIT_SWITCH_PIN 14 // GPIO14/D5

// Define the stepper motor interface pins
#define IN4_PIN 0  // GPIO0/D3   //IN4-D4(2)
#define IN3_PIN 16 // GPIO16/D0  //IN3-D3(0)
#define IN2_PIN 13 // GPIO13/D7  //IN2-D2(4)
#define IN1_PIN 15 // GPIO15/D8  //IN1-D1(5)

#define SERVO_PIN 2 // GPIO2/D4 (blue LED)
//#########  END OF PIN DEFINITIONS  #################

//define servo positions
#define zurück 0 
#define vorwärts 143

// Define the number of steps per revolution for the stepper motor
#define STEPS_PER_REV 200

// Create a stepper motor object
Stepper stepper(STEPS_PER_REV, IN4_PIN, IN3_PIN, IN2_PIN, IN1_PIN);

//clreate a servo object
Servo ausfallschritt_servo;

// Initialize the OLED display using Wire library
SSD1306Wire display(0x3c, SDA_PIN, SCL_PIN);


//create an PCF8575 I2C LED port expander instance
PCF8575 PCF_20(0x20);

//initiate NTP time handling including Sommer/Winterzeit (daylight saving time) update for Germany/Berlin time
static tm getDateTimeByParams(long time){
    struct tm *newtime;
    const time_t tim = time;
    newtime = localtime(&tim);
    return *newtime;
}

static String getDateTimeStringByParams(tm *newtime, char* pattern = (char *)"%d/%m/%Y %H:%M:%S"){
    char buffer[30];
    strftime(buffer, 30, pattern, newtime);
    return buffer;
}
 

static String getEpochStringByParams(long time, char* pattern = (char *)"%d.%m.%Y %H:%M:%S"){ //set Serial output format
//    struct tm *newtime;
    tm newtime;
    newtime = getDateTimeByParams(time);
    return getDateTimeStringByParams(&newtime, pattern);
}

static String getTimeStringByParams(long time, char* pattern = (char *)"%H:%M"){
    tm newtime = getDateTimeByParams(time);
    char buffer[6];
    strftime(buffer, 6, pattern, &newtime);
    return buffer;
}
 
WiFiUDP ntpUDP;
 
// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
// NTPClient timeClient(ntpUDP);
 
// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
int GTMOffset = 0; // SET TO UTC TIME
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GTMOffset*60*60, 60*60*1000);
 
// Central European Time (Berlin)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);


//set up wifimanager configuration
WiFiManager wm;
WiFiManagerParameter custom_mqtt_server("MQTT broker IP", "mqtt server IP", "", 40);
WiFiManagerParameter custom_mqtt_password("MQTT broker password", "mqtt password", "", 40);
WiFiManagerParameter calibrate_rotationspeed; // global param ( for non blocking w params )
WiFiManagerParameter calibrate_limitswitch; // global param ( for non blocking w params )

//define WiFimanager variables
bool configportal=false;

bool doppelpunkt=true;
//define PCF8575 ON/OFF state for LED switching
const int LED_on = 1;
const int LED_off = 0;

//define stepper variables
int stepper_maxspeed = 80;
int current_steps = 0;
long target_position = 0;

boolean motorRunning = false;         // flag to indicate whether the motor is currently running or not
unsigned long lastStepTime = 0;       // the time when the last step was taken
unsigned long stepInterval = 1000;    // the interval between steps in milliseconds
unsigned int starttime=0;
unsigned int starttime2=0;

//define variables for Günter getting angry
int provocation_counter = 0;
unsigned int last_time_guenter_was_provocated=0;
int calm_down_time = 5000; //Guenter looses one accumulated aggression point every this amount of milliseconds
int wutlevel=0; //will change variably
const int mindestwut=2; //Guenters aggression starting value
const int watschweite = 30; //Guenter distributes a Fotzn this far from initial position (has to be adapted to Watschopfer-distance)

//define global sketch variables
unsigned int nowtime=0;
unsigned int inittime=0;
unsigned int NTPlastDisplayed = 0;
bool blink_blue_LED = true;
// Variables will change:
int ledState = LOW;             // ledState used to set the LED
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 500;           // interval at which to blink (milliseconds)
unsigned int initstarttime = 0;
const int init_position_timeout = 5000; //do not try longer than this amount of ms to move günter to initial body position to prevent being stuck in while loop and preventing OTA

int ota_target_position =0;
int old_ota_target_position = 0;


void setup() {
    Wire.begin(); // initialize I2C interface
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    pinMode(LED_BLUE, OUTPUT); // initialize onboard LED as output
    pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);
    free_stepper();

    i2cscan();
    PCF_20.begin();
    target_leds(0); //all LEDs off
    ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
    ausfallschritt_servo.write(zurück);
    delay(100); //time for servo to move before detaching
    //ausfallschritt_servo.detach();
    display.init();
    display.flipScreenVertically();
    display.setContrast(255);
    display.clear();
    delay(10);
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    inittime=millis();
    
    //schwungholen();
    //schwungholen_V2();
    //delay(3000);

    goto_init_position(1);
    
    display.drawString(display.getWidth() / 2, 31, "trying to connect\nto known WiFi");
    display.display();


    //----------------------
    

   
    
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    // put your setup code here, to run once:
    
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
//------------------------------------------------------------------------
  // add a custom input field
  int customFieldLength = 40;


  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
  
  // test custom html input type(checkbox)
  new (&calibrate_rotationspeed) WiFiManagerParameter("customfieldid", "maximalen Rotationsspeed Günter kalibrieren", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
  new (&calibrate_limitswitch) WiFiManagerParameter("customfieldid", "Limitswitch kalibrieren", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

  // test custom html(radio)
//  const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three<br>";
//  new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
//  
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_password);
  wm.addParameter(&calibrate_rotationspeed);
  wm.addParameter(&calibrate_limitswitch);

//------------------------------------------------------------------------
    
    //wm.addParameter(&custom_mqtt_server);
    wm.setConfigPortalBlocking(false);
    wm.setSaveParamsCallback(saveParamsCallback);

    //------------------

  // set dark theme
  wm.setClass("invert");
//------------------------------------------------------------------------
    //------------------

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("WatschnFM")){
        Serial.println("connected to WiFi...");
        display.clear();
        display.drawString(display.getWidth() / 2, 31, "connected to\nto known WiFi");
        display.display();
        display.display();
        nowtime=millis();
        if(nowtime-inittime<2000){
          delay(2000-(nowtime-inittime));
        }
    }
    else {
        Serial.println("Configportal running");
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(display.getWidth() / 2,8, "! CONFIG NEEDED !");
        display.drawString(display.getWidth() / 2,20, "Access point opened.");
        display.drawString(display.getWidth() / 2,32, "Please connect to");
        display.drawString(display.getWidth() / 2,44, "WiFi ' WatschnFM '");
        display.drawString(display.getWidth() / 2,56, "to configure WiFi.");
        display.display();
        configportal=true;
        //delay(1000);
        //display.clear();
    }
  display.setFont(ArialMT_Plain_10);
    // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("WatschnMaster");

  ArduinoOTA.begin();
    ArduinoOTA.onStart([]() {
    //ota_target_position =0;
    //old_ota_target_position = 0;
//    ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
//    ausfallschritt_servo.write(zurück);
//    delay(100);
//    ausfallschritt_servo.detach();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 36, 120, 10, progress / (total / 100) );
    Serial.println("Progress: "+ progress);
    Serial.println("Progress/(total/100): "+ progress / (total / 100));
    display.display();
    //use 'stufe der weisheit' as LED progress bar for OTA update:
    int ota_percentage_done=progress / (total / 100);
    target_leds(ota_percentage_done);

//    
//    init_stepper();
//    //int target_position = map(ota_percentage_done, 0, 100, 0, 200);
//    int target_position = map(progress, 0, 100, 0, 200);
//    // Convert position to a number of steps based on the steps per revolution
//    
//    ota_target_position=ota_target_position-old_ota_target_position;
//    
//    stepper.step(ota_target_position);
//
//    old_ota_target_position=ota_target_position;
//  
//    free_stepper();
  });

  ArduinoOTA.onEnd([]() {
    //ota_target_position =0;
    //old_ota_target_position = 0;
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "rebooting...");
    display.display();
    for(int i=100; i>=0; i--){ //sweep down LEDs in one second
    target_leds(i);
    delay(10);
    }
  });

  // Align text vertical/horizontal center
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_10);
  //display.drawString(1,8, "Günter WatschnFM");
  if(!configportal){
    //("");
  display.setFont(ArialMT_Plain_10);
  display.drawString(display.getWidth() / 2, (display.getHeight() / 2)+6, "Ready for OTA update...\n Device name:\n' WatschnMaster '\nIP: " + WiFi.localIP().toString());
  display.display();
  
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

        Serial.print("WiFiManager-Checkbox 'Günter kalibrieren': ");
        Serial.println(calibrate_rotationspeed.getValue());
        Serial.print("WiFiManager-Checkbox 'Limitswitch kalibrieren': ");
        Serial.println(calibrate_limitswitch.getValue());


timeClient.begin();
  delay ( 1000 );
  if (timeClient.update()){
     Serial.println( "Lokale Uhrzeit Einstellen inkl. Sommer/Winterzeit" );
     unsigned long epoch = timeClient.getEpochTime();
     setTime(epoch);
  }else{
     Serial.print ( "NTP Update ist fehlgeschlagen" );
  }
  

}


// Define a function to move the stepper motor forward or backward by given steps
void move_stepper(int steps) {
  Serial.print("got steps: ");
  Serial.println(steps);

//  display.clear();
//  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//  display.setFont(ArialMT_Plain_10);
//  display.drawString(0,6, "Watschweite: " + steps);
//  display.setFont(ArialMT_Plain_10);
//  //display.drawString(0, 50, "Watschweite2\n Device name:\n' WatschnMaster '\nIP: " + WiFi.localIP().toString());
//  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//  display.setFont(ArialMT_Plain_10);
//  display.drawString(0, 0, "Stepcount: ");
//  display.display();

  
  int direction = steps > 0 ? 1 : -1;
  Serial.print("DBG 1: direction=");
  Serial.println(direction);
  Serial.print("current steps: ");
  Serial.println(current_steps);
//  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//  display.setFont(ArialMT_Plain_10);
//  display.drawString(0, 0, "Stepcount: " + current_steps);
//  display.display();
  //delay(1000);
  init_stepper();
  // Calculate the target position in steps as relative movement:
  target_position = current_steps + steps;
  Serial.print("Target position initial: ");
  Serial.println(target_position);
  
//  while (target_position > STEPS_PER_REV) {
//    target_position -= STEPS_PER_REV;
//  }
//  while (target_position < -STEPS_PER_REV) {
//    target_position += STEPS_PER_REV;
//  }
//  if (target_position==200){
//    target_position=0;
//  }

  
//  if (target_position<0){
//    //target_position=0;
//    target_position=abs(target_position);
//  }
  Serial.print("Target position corrected: ");
  Serial.println(target_position);
//  //target_position *= STEPS_PER_REV / STEPS_PER_REV;
  Serial.print("Target position: ");
  Serial.println(target_position);
  // Determine the direction to move the stepper motor
//  int direction = steps > 0 ? 1 : -1;
//  Serial.print("DBG 1: direction=");
//  Serial.println(direction);
  // Move the stepper motor one step at a time until the target position is reached
  while (current_steps != target_position) {
    // Check if the limit switch is triggered
//    if (digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
//      // Reset the current steps to zero
//      current_steps = 0;
//    }
    if (target_position == 0&&abs(current_steps)<(STEPS_PER_REV)){
      while (digitalRead(LIMIT_SWITCH_PIN) == LOW){ //step one step until reaching limit switch
        stepper.step(direction);
      }
      current_steps=0;
      target_position=0;
      Serial.print("DBG 3: current steps=");
      Serial.println(current_steps);
//      display.drawString(0, 50, "Stepcount: " + current_steps);
//      display.display();
    }
    else{
    // Move the stepper motor one step in the desired direction
    stepper.step(direction);
    // Update the current steps of the stepper motor
    current_steps += direction;
    Serial.print("DBG 3: current steps=");
    Serial.println(current_steps);
    }
//    // If the stepper motor has completed one full turn, adjust the current steps accordingly
//    if (current_steps >= STEPS_PER_REV) {
//      current_steps -= STEPS_PER_REV;
//    }
//    //if (current_steps < 0) {
//    if (current_steps < -STEPS_PER_REV) {
//      current_steps += STEPS_PER_REV;
//    }

    
  }
  //free_stepper();
}



/*Other font options that may be available include:
FreeSansBold9pt7b or FreeSansBold12pt7b: Uses the "FreeSans" font with a bold style at a size of 9 or 12 pixels, respectively.
TimesNewRomanBold12pt7b: Uses the "Times New Roman" font with a bold style at a size of 12 pixels.
Roboto_Thin_24 or Roboto_Regular_24: Uses the "Roboto" font at a size of 24 pixels with a thin or regular style, respectively.
Verdana_bold_14 or Verdana_plain_10: Uses the "Verdana" font with a bold or plain style at a size of 14 or 10 pixels, respectively.

SSD1306wire.h has those options for Arial Plain:
ArialMT_Plain_10: This option sets the ArialMT_Plain font at a size of 10 pixels.
ArialMT_Plain_16: This option sets the ArialMT_Plain font at a size of 16 pixels.
ArialMT_Plain_24: This option sets the ArialMT_Plain font at a size of 24 pixels.
 */

void displayTime() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  String currentTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
  //display.println(currentTime);
  display.drawString(54,10, "WatschnFM");
  display.setFont(ArialMT_Plain_24);
  display.drawString((display.getWidth() / 2), (display.getHeight() / 2), currentTime);
  //display.drawString(display.getWidth() / 2, (display.getHeight() / 2), "Ready for OTA update...\n Device name:\n' WatschnMaster '\nIP: " + WiFi.localIP().toString());
  display.display();
  display.setFont(ArialMT_Plain_10);

//  for adafruitssd1306:
//  display.clear();
//  display.setTextSize(2);
//  display.setTextColor(SSD1306_WHITE);
//  display.setCursor(0,0);
//  String currentTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
//  display.println(currentTime);
//  display.display();
}

void auf_die_12() {
  // do something at 12:00
  Serial.println("funktion jetzt gibts auf die 12 - noch nicht implementiert");
}

void jetzt_schlägts_13() {
  // do something at 13:00
  Serial.println("funktion jetzt schlägts 13 - noch nicht implementiert");
  //z.B.:
  //multiwatschn(13);
}

void free_stepper(){
  Serial.println("freeing Stepper");
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);

}

void init_stepper(){
  //Serial.println("Initializing Stepper");
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  stepper.setSpeed(stepper_maxspeed);
}

void goto_init_position(int direction){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_10);
  display.drawString(display.getWidth() / 2, 29, "waiting for init position\nif stepper is not powered,\nmanually move Günter\nto normal position.");
  display.display();
    init_stepper();
    stepper.setSpeed(40);
  // Move the stepper motor to the initial position if necessary
//  if (digitalRead(LIMIT_SWITCH_PIN) == LOW) {
//    move_stepper(-STEPS_PER_REV);
//  }
  while (digitalRead(LIMIT_SWITCH_PIN) == LOW && nowtime-initstarttime<init_position_timeout) {
    nowtime=millis();
    stepper.step(direction);
    Serial.println("searching for init position...");
    delay(1);
  }
  
  //if target position is reached or timeout is being hit, continue.
  free_stepper();
  stepper.setSpeed(stepper_maxspeed);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.display();
   
}

void schwungholen(){
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
  ausfallschritt_servo.write(zurück);
  delay(500);
  init_stepper();
  int umdrehungen=1;
  while(stepper_maxspeed<250){
  for(int i=0; i<umdrehungen;i++){
    ESP.wdtFeed();
    stepper.step(-15);
    stepper_maxspeed=stepper_maxspeed+1;
    if(stepper_maxspeed>250){
      stepper_maxspeed=250;
      free_stepper();
      delay(4000);
      stepper_maxspeed=80;
      //ausfallschritt_servo.write(vorwärts);
      delay(100);
      return;
       
    }
    stepper.setSpeed(stepper_maxspeed);
  }
  }
  stepper_maxspeed=80;
  stepper.setSpeed(stepper_maxspeed);

  free_stepper();
  //ausfallschritt_servo.write(vorwärts);
    delay(500);
    ausfallschritt_servo.write(zurück);
    delay(500);
    goto_init_position_idle();
    delay(300);
    multiwatschn(5);
  

  
}

void schwungholen_V2(){ //führt zu reboots!
  init_stepper();
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
  ausfallschritt_servo.write(zurück);
  delay(900);
  runMotor();
  nowtime=millis();
  starttime=millis();
  starttime2=millis();
  while(nowtime-starttime<7000){
    nowtime=millis();
    if(nowtime-starttime2>100){
    starttime2=millis();
    stepper_maxspeed++;
    stepper.setSpeed(stepper_maxspeed);
    }
    updateMotor();
  }

  stopMotor();
}


void runMotor() {
  motorRunning = true;
  lastStepTime = millis();  // set the last step time to the current time
  stepper.setSpeed(60);   // set the speed of the motor in RPM
}

void stopMotor() {
  motorRunning = false;
  stepper.step(0);  // stop the motor
}

void updateMotor() {
  if (motorRunning) {
    unsigned long currentTime = millis();
    if (currentTime - lastStepTime >= stepInterval) {
      stepper.step(1);         // take one step
      lastStepTime = currentTime; // set the last step time to the current time
    }
  }
}

void goto_init_position_idle(){
    init_stepper();
    stepper.setSpeed(40);
    bool null_erreicht=0;
for(int i=30; i>0; i--){ //30 steps zurückdrehen
  if (digitalRead(LIMIT_SWITCH_PIN) == LOW) { //falls null erreicht, vorerst abbrechen
    null_erreicht=true;
  }
  stepper.step(-1);
  
  if(null_erreicht){
    return;
  }
}
if(null_erreicht){ //... und dann 30 steps weiter zurückdrehen
    for(int i=30; i>0; i--){ //30 steps zurückdrehen
      stepper.step(-1);
    }
  }
while (digitalRead(LIMIT_SWITCH_PIN) == LOW) { //wieder vordrehen, bis limitswitcvh erreicht ist
  for(int i=0; i<201; i++){
    stepper.step(1);
  }
}
  free_stepper();
  stepper.setSpeed(stepper_maxspeed);
  //reset variables - we are at zero position
  current_steps=0;
  target_position=0;
}

void goto_init_position_idle2(){
  if(current_steps == target_position && target_position == 0){ //if Günter *should* be at zero position, monitor that this is the case - if not, correct that.
    //also count if that happens mor than once. If so, Günter will get angry and start hitting.
      
    if (digitalRead(LIMIT_SWITCH_PIN) == LOW) {
      delay(100); //wait before rotating back
        init_stepper();
        stepper.setSpeed(40);
        last_time_guenter_was_provocated=millis();
        provocation_counter++;
        
        for(int i=30; i>0; i--){
          if (digitalRead(LIMIT_SWITCH_PIN) == HIGH) { //set remaining steps to turn back  while limit switch is triggered
            i=0;
            //return;
          }
          stepper.step(1);
        }
        while (digitalRead(LIMIT_SWITCH_PIN) == LOW) {
          //move_stepper(xxx);
          stepper.step(-1);
          Serial.println("searching for init position...");
          delay(1);
        }
        for(int i=5; i>0; i--){
          if (digitalRead(LIMIT_SWITCH_PIN) == LOW) {
            stepper.step(1);//return overshoot (about 5 when günters Body is attached)
          }
        }
        stepper.setSpeed(stepper_maxspeed);
        free_stepper();
    }
  }
}

void multiwatschn(int haudrauf){
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
  ausfallschritt_servo.write(vorwärts);
  init_stepper();
  for(int i=0;i<haudrauf;i++){
  move_stepper(-watschweite);
  delay(20);
  move_stepper(watschweite);
  //find_positon_backward_null();
  delay(400);
  }
  ausfallschritt_servo.write(zurück);
  delay(300);
  //ausfallschritt_servo.detach();
  free_stepper();
}

void multiwatschn2(int haudrauf, int sollwatschweite){
//  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
//  ausfallschritt_servo.write(vorwärts);
  init_stepper();
  for(int i=0;i<haudrauf;i++){
  Serial.println("start -sollwatschweite");
  move_stepper(-sollwatschweite);
  Serial.println("finish -sollwatschweite");
  delay(100);
  move_stepper(sollwatschweite);
  //find_positon_backward_null();
  delay(300);
  }
//  ausfallschritt_servo.write(zurück);
//  delay(300);
  //ausfallschritt_servo.detach();
  free_stepper();
}

void get_aggressive_when_provocated(int how_often){
  nowtime=millis();
  //handle calmdown first
  if(nowtime-last_time_guenter_was_provocated>calm_down_time){
    provocation_counter--;
    if(provocation_counter < 0){ //prevent excessive calmness (there may not be any negative threshold)
      provocation_counter = 0;
    }
  }
  //last_time_guenter_was_provocated=millis();
  if(provocation_counter>=3){
    get_aggressive(provocation_counter); //get aggressive with this amount of aggression
  }
}

void get_aggressive(int agression_level){
    ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
     ausfallschritt_servo.write(zurück);
    int max_aggression_led=agression_level*50;
    int max_aggression_stepper=mindestwut+agression_level;
    int led_agression_increase_coefficient=1;
    int stepper_agression_increase_coefficient=5;
    int min_provocations_to_vibrate=1;
    wutlevel=mindestwut;

    init_stepper();
    
  // fade Günters face from min to max_aggression_led in increments of n points:
  for (int fadeValue = 0 ; fadeValue <= max_aggression_led; fadeValue += led_agression_increase_coefficient) {
    // sets the value (range from 0 to 255):
    analogWrite(LED_Kopf, fadeValue);
    if(agression_level>min_provocations_to_vibrate){
    wutlevel=((fadeValue/50)*stepper_agression_increase_coefficient)+mindestwut;
    if(wutlevel>max_aggression_stepper){ //limit the amount of anger-induced vibration of Günter
      wutlevel=max_aggression_stepper;
    }
      stepper.step(wutlevel);
      delay(30);
      stepper.step(-wutlevel);
      //wutlevel=wutlevel*stepper_agression_increase_coefficient;
    }
    else
      delay(30);
    }
  wutlevel=0;
  //delay(500);
  Serial.println("Debug: Wutaufbau beendet, starte jetzt multiwatschn()");
  multiwatschn(3);
  delay(100);
  ausfallschritt_servo.write(zurück);
  free_stepper();
  calm_down();
  
}

void calm_down(){
   provocation_counter=0;
   // fade out from max to min in increments of 5 points:
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
    // sets the value (range from 0 to 255):
    analogWrite(LED_Kopf, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }
}

void i2cscan(){
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
         Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      nDevices++;
     }
     else if (error==4)
     {
      Serial.print("Unknown error at address 0x");
      if (address<16)
         Serial.print("0");
      Serial.println(address,HEX);
     }    
    }
    if (nDevices == 0)
       Serial.println("No I2C devices found\n");
    else
       Serial.println("done\n");
}

void sweepup_leds(){
  // Turn on LEDs one after another according to the percentage
  for (int i = 0; i <= 100; i++) {
    int num_leds = map(i, 0, 100, 0, 16); // map percentage to number of LEDs to turn on
    //setLeds(num_leds); // update state of the LEDs
    delay(50); // wait for 50ms
  }
}

void target_leds(int percentage){
  // Turn on LEDs one after another according to the percentage
  if(percentage>100){
    percentage=100;
  }
  if(percentage<=0){
    percentage=0;
    for (int i = 0; i < 16; i++)
    {
    PCF_20.write(i, LED_off);
    //Serial.print("all LEDs off");
    }
  }
  if(percentage>1){
    int num_leds = map(percentage, 0, 100, 0, 16); // map percentage to number of LEDs to turn on
    //Serial.print("DBG:num_leds="+num_leds);
    for (int i = 0; i < num_leds; i++)
    {
    PCF_20.write(i, LED_on);
    }
    for (int i = 15; i >= num_leds; i--)
    {
    PCF_20.write(i, LED_off);
    }
  }
  else{
    //setLeds(0);
    for (int i = 0; i < 16; i++)
    {
    PCF_20.write(i, LED_off);
    //Serial.print("all LEDs off");
    }
  }
}


void handle_blue_LED(){
 if(blink_blue_LED){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LED_BLUE, ledState);
  }
      // Ping www.google.com once and set blink_blue_LED to false if successful
  if (WiFi.status() == WL_CONNECTED && blink_blue_LED) {
    IPAddress googleIP;
    if (WiFi.hostByName("www.google.com", googleIP) == 1) {
      blink_blue_LED = false;
      digitalWrite(LED_BLUE, LOW); //on
    }
  }
 }
}

void check_for_wifimanager_request(){
    // is configuration portal requested?
  if ( digitalRead(LIMIT_SWITCH_PIN) == LOW) { //using that pin just for debugging, not in prodiúctive version!

      wm.resetSettings();
//      delay(100);
//      ESP.restart();
//      delay(5000);

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    // put your setup code here, to run once:
    Serial.begin(115200);
    
    //reset settings - wipe credentials for testing
    wm.resetSettings();
//------------------------------------------------------------------------
  // add a custom input field
  int customFieldLength = 40;


  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
  
  // test custom html input type(checkbox)
  //new (&calibrate_rotationspeed) WiFiManagerParameter("customfieldid", "maximalen Rotationsspeed Günter kalibrieren", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
  //new (&calibrate_limitswitch) WiFiManagerParameter("customfieldid", "Limitswitch kalibrieren", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

  // test custom html(radio)
//  const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three<br>";
//  new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
//  
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_password);
  //wm.addParameter(&calibrate_rotationspeed);
  //wm.addParameter(&calibrate_limitswitch);

//------------------------------------------------------------------------
    
    //wm.addParameter(&custom_mqtt_server);
    wm.setConfigPortalBlocking(false);
    wm.setSaveParamsCallback(saveParamsCallback);

    //------------------

  // set dark theme
  wm.setClass("invert");
//------------------------------------------------------------------------
    //------------------

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(!wm.startConfigPortal("WatschnFM")){
        Serial.println("Configportal running");
    }
    else {
        Serial.println("connected to WiFi...");
    }
    }

}

void flash_all_white_leds(){
    for (int i = 0; i < 16; i++)
  {
    PCF_20.write(i, 1);
  }
  delay(100);
    for (int i = 0; i < 16; i++)
  {
    PCF_20.write(i, 0);
  }
}

void sweep5x(){
   for (int i = 0; i < 5; i++){
   for (int i = 0; i < 16; i++)
    {
    PCF_20.write(i, LED_on);
    delay(60);
    }
  delay(500);
    for (int i = 0; i < 16; i++)
    {
    PCF_20.write(i, LED_off);
    delay(60);
    }
 }
}

void listen_to_serial(){
  // Read a target step count from the serial port
  if (Serial.available() > 0) {
    int target_steps = Serial.parseInt();

    // Move the stepper motor to the target steps
    move_stepper(target_steps);

    // Output the current steps of the stepper motor
    Serial.print("Current steps: ");
    Serial.println(current_steps * STEPS_PER_REV / STEPS_PER_REV);
  }
}


void show_time_to_oled(){
      nowtime=millis();
    if(nowtime-NTPlastDisplayed>1000){

        String timestamp = getEpochStringByParams(CE.toLocal(now()));    //convert to local time and format timestamp string
  int hour = timestamp.substring(11, 13).toInt();    //extract hour from timestamp string
  int minute = timestamp.substring(14, 16).toInt();  //extract minute from timestamp string
  int seconds = timestamp.substring(17, 19).toInt(); //extract seconds from timestamp string
  
  Serial.print("Hour: ");
  Serial.println(hour);
  Serial.print("Minute: ");
  Serial.println(minute);
  Serial.print("Seconds: ");
  Serial.println(seconds);



//      Serial.println("Datum/Uhrzeit: ");
//      Serial.println(getEpochStringByParams(CE.toLocal(now())));
//      Serial.print("Es ist ");
//      Serial.print(getTimeStringByParams(CE.toLocal(now())));
//      Serial.println(" Uhr");
      time_t now = time(nullptr);
      struct tm* local = localtime(&now);
      char timeStr[10];
      //sprintf(timeStr, "%02d:%02d:%02d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
      //sprintf(timeStr, "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);
      doppelpunkt=!doppelpunkt; //toggle every second
      if(doppelpunkt){
        sprintf(timeStr, "%02d:%02d", hour, minute);
      }
      else{
        sprintf(timeStr, "%02d %02d", hour, minute);
      }
      //incl sekunden:
      //sprintf(timeStr, "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);
      //Serial.println(timeStr);
      display.clear();
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 25, timeStr);
      display.display();
      display.setFont(ArialMT_Plain_10);
      NTPlastDisplayed=millis();
    }
}

void loop() {
    wm.process();
    ArduinoOTA.handle();
    handle_blue_LED();
    //check_for_wifimanager_request();
    listen_to_serial();
    
    show_time_to_oled();
    goto_init_position_idle2();
    get_aggressive_when_provocated(3);

 

//    nowtime=millis();
//    if(nowtime-starttime>3000){
//    multiwatschn2(3,30);
//    starttime=millis();
//    }

    //schwungholen();    //delay(5000);
    



//ACHTUNG: das ist ohne Zeitoffset!! passt also noch nicht:
    if (timeClient.getHours() == 00 && timeClient.getMinutes() == 19 && timeClient.getSeconds() == 0) {
      auf_die_12();
    }
//    if (timeClient.getHours() == 13 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0) {
//      jetzt_schlägts_13();
//    }
//    if (timeClient.getHours() == 00 && timeClient.getMinutes() == 43 && timeClient.getSeconds() == 0) {
//      Serial.println("################ test 00:43h . #####################");
//      Serial.println("################ test 00:43h . #####################");
//      Serial.println("################ test 00:43h . #####################");
//      Serial.println("################ test 00:43h . #####################");
//    }

}

void saveParamsCallback () {
  Serial.println("Get Params:");
  Serial.print(custom_mqtt_server.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_server.getValue());
}
