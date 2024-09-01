//to do:
////local MQTT:
//char* mqttBockfotznTopicLocal = "MyDevice/bo123/"; //MQTT Message received in topic 'MyDevice/bo123/'. Message: 'watschn' --> does not trigger from local

//WATSCHNMASTER  GLOBALE WATSCHN-ALLIANZ im Bockfotzn-Netzwerk



/* This code is intended to drive Günter Watschn Master Hardware.
 * This code will try to get a password protected MQTT connection to a MQTT broker.
 * ESP8266 does not support Certificates, but the code also can be used for ESP32 only pin assignments need to be adapted)
 * Example MacOS compatible shell commands to connect to a MQTT broker and subscribe/publish:
 * mosquitto_sub -h 123456789999999999987654321a1.s1.eu.hivemq.cloud -p 8883 -t "WatschnMaster/bockfotzn/" -u MyUserNameIswatschnmaster -P MyPassword1 --insecure
 * mosquitto_pub -h 123456789999999999987654321a1.s1.eu.hivemq.cloud -p 8883 -t WatschnMaster/bockfotzn/ -m "watschn" -u MyUserNameIswatschnmaster -P MyPassword1 --insecure
 */
#define USE_LOCAL_MQTT
#define USE_GLOBAL_MQTT

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
#include "PCF8575.h" //https://github.com/RobTillaart/PCF8575
#include "SSD1306Wire.h"
//#include "Adafruit_SSD1306.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <time.h>
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
//#include "MeshManager.h"

//#include <ESP8266WiFi.h>
#include <FS.h>
//#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <DoubleResetDetector.h>
#include <ArduinoOTA.h>
//#include "ESP_OTA_GitHub.h"

#include <Wire.h>

//general settings:
////#################  PIN DEFINITIONS  #################
//// Define I2C pins
//#define SCL_PIN 4    // GPIO4/D2
//#define SDA_PIN 5    // GPIO5/D1

//SSD1306Wire display(0x3c, SDA_PIN, SCL_PIN);
//PCF8575 PCF_20(0x20);


long lastReconnectAttempt = 0;


//WiFiClient wifiClient;
//PubSubClient mqttClient(wifiClient);
//WiFiClientSecure secureClient;  // Create a secure WiFi client
//PubSubClient mqttClient(secureClient);  // Initialize the PubSubClient with the secure client
//PubSubClient mqttClientLocal(secureClient);  // Initialize the PubSubClient with the secure client

////old:
//WiFiClientSecure secureClientGlobal;  // Secure WiFi client for global MQTT
//WiFiClientSecure secureClientLocal;   // Secure WiFi client for local MQTT
//
//PubSubClient mqttClient(secureClientGlobal);    // Global MQTT client
//PubSubClient mqttClientLocal(secureClientLocal); // Local MQTT client

//NEW:
WiFiClientSecure secureClientGlobal;  // Secure WiFi client for global MQTT
PubSubClient mqttClient(secureClientGlobal);    // Global MQTT client

//secure local client:
//WiFiClientSecure secureClientLocal;   // Secure WiFi client for local MQTT
//PubSubClient mqttClientLocal(secureClientLocal); // Local MQTT client

//normal local client:
WiFiClient ClientLocal;
PubSubClient mqttClientLocal(ClientLocal);





//WiFiManager wifiManager;

////variables to read a wifi SSID and wifi password from a text file (named known_wifis.txt) which is stored locally on your ESP8266, so that the publicly visible code on the GitHub repository doesn't contain your wifi credentials
//String SSID_txt = "";
//String PASS_txt = "";


// Define variables to hold the credentials
const int MAX_WIFI_NETWORKS = 10;
char* ssid[MAX_WIFI_NETWORKS];
char* pass[MAX_WIFI_NETWORKS];
int numWifiNetworks = 0;

//unsigned long now = 0;
unsigned long boottime = millis();

// A single, global CertStore which can be used by all
// connections.  Needs to stay live the entire time any of
// the WiFiClientBearSSLs are present.
#include <CertStoreBearSSL.h>
BearSSL::CertStore certStore;
bool check_OTA_on_boot = false; // may be changed by user before compiling. if set to true, the device will check for an update on each boot.
/* Set up values for your repository and binary names */
//assuming you use this example, www.github.com/lademeister/ESP_OTA_GitHub is the repository we are using to fetch our OTA updates from:
#define GHOTA_USER "lademeister" //the name of the GitHub user that owns the repository you want to update from
#define GHOTA_REPO "ESP_OTA_GitHub" //the name of the repository you want to update from
#define GHOTA_CURRENT_TAG "0.0.0" //This resembles the current version of THIS sketch.
//If that version number matches the latest release on GitHub, the device will not refleash with the binary (additionally uploaded as an ASSET of the release) from Github.
// in case of mismatch, the device will upgrade/downgrade, as soon as the check for new firmware is running.

//Explanation: you need to CHANGE THAT in case you create new software. Lets say latest release on GitHub was 0.2.3 and you modify something that you want to be the latest shit.
//you then set the above line as #define GHOTA_CURRENT_TAG "0.2.4" (or 0.3.0 for major changes) for example.
//to get that latest software you just created onto your device, you could just flash it locally via USB and you are fine.
//but to make it available as OTA binary on github, you should:

//1. use Sketch/export compiled binary to create an already compiled .bin file. (-->You should keep the name the same all times unless you understand the comments below)
//2. upload the latest .ino to your github repository (thats only for reference and for others to work/improve it)
//3. draft a new release of that github repository (using web interface on github.com) and set a tag for that release that matches your new version number (0.2.4 as in the example above)
//4. AND, most important, you NEED to upload the binary for that version as an ASSET to that release (you will find a square field where you can drag&drop the .bin when using the GitHub web interface).

//COMMENTS:
//it is important to understand that you do NOT need to upload the .bin to the actual repository next to the other files in the repository (like the .ino or so).
//You could do that without harm, but while you are free to upload ten or hundred binaries to your repository, none of them will be used for the OTA update.
//Only the one binary that was uploaded separately as an asset of the latest release of the repository will be used.
//You need to create releases manually, they are NOT created by uploading something.
//The release that you create must have a correct tag (=version numbering) - you can set that manually while drafting a release.
//The release that you create must have at least the .bin file for that current software version uploaded as an asset.
//The GHOTA_CURRENT_TAG while compiling that binary must be the SAME as the tag that you set for that release.
//Note that you also need to set the release as "latest" and that the binary that you compiled fromn your latest code must have the same name that 
//the firmware which is running on the OTA devices out in the field expects. This name is stored in #define GHOTA_BIN_FILE "GitHub_Upgrade.ino.generic.bin", so the 
//binary you upload as an assed for a new release must be named GitHub_Upgrade.ino.generic.bin to successfully update.
//Of yourse, you can change that at the beginning of your project to whatever the generated bin file is named as a standard.
//Lets assume you clone this repository and do your own work and name it 'MyProject'.
//When you go to Sketch/export compiled binary, depending if you compile for Generic ESP8266 or for Wemos D1 mini or something else, the output name will be different.
//It could -for example - be named MyProject.D1mini.bin or so.
//Then you would do yourself a favour in just changing the variable in the sketch to #define GHOTA_BIN_FILE "MyProject.D1mini.bin".
//When cecking for an OTA update, the code will 
//-check the version tag: it compares GHOTA_CURRENT_TAG of the currently running software with the tag that you set for the latest release on github.
//-check the binary name: it compares GHOTA_BIN_FILE with the name of the binary that was uploaded as an asset on github
//-check the MIME type: nothing you need to care about as long as it works (if you need to adapt look at ESP_OTA_GitHub.h and ESP_OTA_GitHub.cpp file in /src folder of this repository. If you need to edit for other MIME types, then you need to edit the files in the forlder where you imported this library (most likely Arduino/libraries/ESP_OTA_GitHub/src)
//-check if the release was set as prerelease and compare if accepting a prerelease is allowed in the sketch currently running on the device. #define GHOTA_ACCEPT_PRERELEASE 0 would not allow to update from a release marked as prerelease.
//only if all requirements are met, an OTA update will be started.
//So, if you have devices out in the field that expect a binary named aaa.bin and in a later version that you create you change GHOTA_BIN_FILE to bbb.bin, you are fine if you update all devices first.
//But if you upload a file named bbb.bin as an asset to a new release while the code on the devies still expects aaa.bin, the update will fail.
//As there is no issue in keeping the same name unchanged forever, you should only change it at the beginning or if needed for some special reason.
//As the version control is done with the tag number of the GitHub release and the GHOTA_CURRENT_TAG in the code, there is no need for versioning file names.

// ### ATTENTION ###: Understand how it works before you use a different name for the .bin that you upload to GitHub or before you change #define GHOTA_BIN_FILE "GitHub_Upgrade.ino.generic.bin".

//additional remark: for testing you may want to flash a device locally that will definitely be OTA updated right away. To do so, just use #define GHOTA_CURRENT_TAG "0.0.0" for the 
//sketch that you upload to your test device while the GitHub repository at least has a release of 0.0.1. 
//In this way your test device will update even if you were flashing it with the most recent code (because it thinks it is 0.0.0).
//That can be handy for testing because you do not need to create new releases each time.

#define GHOTA_BIN_FILE "GitHub_Upgrade.ino.generic.bin" //only change it if you understand the above comments
#define GHOTA_ACCEPT_PRERELEASE 0 //if set to 1 we will also update if a release of the github repository is set as 'prereelease'. Ignore prereleases if set to 0.


#define DRD_TIMEOUT 10 // Number of seconds after reset during which a subseqent reset will be considered a double reset.

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

bool mqtt_config_hasbeenread = false; //do not change. used to know if we need to read the config data
// Publish device ID and firmware version to MQTT topic
char device_id[9]; // 8 character hex device ID + null terminator

//Static WiFi SSID and password definitions
#ifndef STASSID
#define STASSID "dont_place_it_here"
#define STAPSK  "put_it_in_textfile_on_SPIFFS_instead"
#endif
#define wm_accesspointname "WatschnFM" //SSID when device opens up an access point for configuration
const int wm_accesspointtimeout = 60;
const int wifitimeout = 6; //in SECONDS.

//initial MQTT setup: if a file named mqtt_config.txt is uploaded to spiffs that contains the information in the form
/*
  mqttClientId:MyDeviceName
  mqttDevicenameTopic:MyDeviceName/devicename/
  mqttupdateTopic:MyDeviceName/update/
  mqttOnlineTopic:MyDeviceName/status/
  mqttFirmwareTopic:MyDeviceName/firmwareversion/
  mqttWillTopic:MyDeviceName/online/


  The "mqtt_config.txt" file contains MQTT topics and variables that are used in the Arduino sketch to connect to the MQTT broker. The contents of the file specify the MQTT client ID, topics for device name, firmware version, online/offline status, and updates. These topics are used by the sketch to publish messages and subscribe to topics on the MQTT broker.
  The code reads each line of the file, trims the line, and checks if it is a comment or empty. If the line contains a variable name and topic separated by an equals sign, the sketch finds the matching MQTT variable and sets the topic to the corresponding value.
  For example, the line "mqttDevicenameTopic=MyDevice/devicename/" sets the "mqttDevicenameTopic" variable to "MyDevice/devicename/". You should change 'MyDevice' to something unique.
  The sketch uses this variable to subscribe to the "devicename" topic on the public MQTT broker broker.hivemq.com.
  in summary, the "mqtt_config.txt" file allows you to customize the MQTT topics and variables used by the sketch without having to modify the code, and to keep those adaptions privately on the device.
*/
//the following variables will be automatically changed, if you have uploaded a configuration file named mqtt_config.txt to your ESP8266's SPIFFS memory.
//You do NOT need to change them here, so that your actual topics can be kept in the mqtt_config.txt on the device, instead of uploading it to a public GitHub repository.
char* mqttClientId = "MyDevice";

//local MQTT:
char* mqttBockfotznTopicLocal = "MyDevice/bo123/"; //MQTT Message received in topic 'MyDevice/bo123/'. Message: 'watschn' --> does not trigger from local
char* mqttGetAggressiveTopicLocal = "MyDevice/getA123/";
char* mqttMoveStepperTopicLocal = "MyDevice/moveS123/";
char* mqttMultiwatschnTopicLocal = "MyDevice/multiw123/";
char* mqttonlineTopicLocal = "MyDevice/status/";
char* mqttfirmwareTopicLocal = "MyDevice/firmwareversion/";
char* mqttWillTopicLocal = "MyDevice/online/";

char *mqttServerLocal = "192.168.82.14";
char *mqttUserLocal = "MyUserName";
char *mqttPassLocal = "MyPassword";
char *mqttPortLocal_txt = "1883"; //char variable to read from configuration text file mqtt_config.txt

//global MQTT:
char* mqttDevicenameTopic = "MyDevice/devicename/";
char* mqttupdateTopic = "MyDevice/update/";
char* mqttBockfotznTopic = "MyDevice/bockfotzn/";
char* mqttGetAggressiveTopic = "MyDevice/getAggressive/";
char* mqttMoveStepperTopic = "MyDevice/moveStepper/";
char* mqttMultiwatschnTopic = "MyDevice/multiwatschn/";
char* mqttonlineTopic = "MyDevice/status/";
char* mqttfirmwareTopic = "MyDevice/firmwareversion/";
char* mqttWillTopic = "MyDevice/online/";
//char *mqttServer = "broker.hivemq.com";
char *mqttServer = "123456789999999999987654321a1.s1.eu.hivemq.cloud";
char *mqttUser = "MyUserName";
char *mqttPass = "MyPassword";
char *mqttPort_txt = "1883"; //char variable to read from configuration text file mqtt_config.txt


int mqttPort = 1883;//integer variable that will be overwritten with the content of *mqttPort_txt, if it can be read from mqtt config file.
int mqttPortLocal = 1883;//integer variable that will be overwritten with the content of *mqttPort_txt, if it can be read from mqtt config file.

//otherMQTT definitions (they are set in sketch as they dont contain private data. Therefore its not critical to upload this code publicly on github).

int maximum_mqtt_connection_tries = 5;
const char* mqttWillMessage = "Offline";
byte mqttwillQoS = 0;
boolean mqttwillRetain = false;
boolean mqttcleanSession = false;



// Valid variable names for MQTT topics in mqtt_config.txt config file in SPIFFS memory of ESP8266
const char* validVarNames[] = { //attention: adding something (or changing arrangement) means you have to keep the same position in definitions everywhere in the code and in the file as well
  "mqttClientId",
  "mqttDevicenameTopic",
  "mqttupdateTopic",
  "mqttonlineTopic",
  "mqttfirmwareTopic",
  "mqttWillTopic",
  "mqttServerLocal",
  "mqttPortLocal_txt",
  "mqttUserLocal",
  "mqttPassLocal",
  "mqttServer",
  "mqttPort_txt",
  "mqttUser",
  "mqttPass",
  "mqttBockfotznTopic",
  "mqttGetAggressiveTopic",
  "mqttMoveStepperTopic", 
  "mqttMultiwatschnTopic"
};





// MQTT topic variables definition. Essentially, mqttVars is an array of pointers to the MQTT topics that will be read from the mqtt_config.txt file and assigned to the respective MQTT variables.
const size_t numValidVarNames = sizeof(validVarNames) / sizeof(validVarNames[0]);
char** mqttVars[numValidVarNames] = { //reihenfolge (und kommas nach jedem Element ausser dem letzten) hier unbedingt beachten (-->identisch wie die MQTT topics im Configfile im SPIFFS)
  &mqttClientId,
  &mqttDevicenameTopic,
  &mqttupdateTopic,
  &mqttonlineTopic,
  &mqttfirmwareTopic,
  &mqttWillTopic,
  &mqttServerLocal,
  &mqttPortLocal_txt,
  &mqttUserLocal,
  &mqttPassLocal,
  &mqttServer,
  &mqttPort_txt,
  &mqttUser,
  &mqttPass,
  &mqttBockfotznTopic,
  &mqttGetAggressiveTopic,
  &mqttMoveStepperTopic,
  &mqttMultiwatschnTopic
};


#include <ESP_OTA_GitHub.h>
// Initialise Update Code
ESPOTAGitHub ESPOTAGitHub(&certStore, GHOTA_USER, GHOTA_REPO, GHOTA_CURRENT_TAG, GHOTA_BIN_FILE, GHOTA_ACCEPT_PRERELEASE);


//MeshManager meshManager;

//comment Vince for github-auto-update:
//currently ESP8266 has memory issues with that much code. it hangs when trying to update via github.
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
//th is will upload the files that are in the sketch folder of the currently openend sketch under /data - but only if that folder exists and contains a file.
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
WiFiManagerParameter custom_mqtt_server_local("MQTT broker IP", "local mqtt server IP", "broker.hivemq.com", 40);
WiFiManagerParameter custom_mqtt_user_local("MQTT broker user", "local mqtt user", "", 40);
WiFiManagerParameter custom_mqtt_password_local("MQTT broker password", "local mqtt password", "", 40);
WiFiManagerParameter custom_mqtt_port_local("MQTT server port", "local mqtt broker port", "1883", 5);
WiFiManagerParameter custom_mqtt_server("MQTT broker IP", "mqtt server IP", "broker.hivemq.com", 40);
WiFiManagerParameter custom_mqtt_user("MQTT broker user", "mqtt user", "", 40);
WiFiManagerParameter custom_mqtt_password("MQTT broker password", "mqtt password", "", 40);
WiFiManagerParameter custom_mqtt_port("MQTT server port", "mqtt broker port", "1883", 5);
WiFiManagerParameter calibrate_rotationspeed; // global param ( for non blocking w params )
WiFiManagerParameter calibrate_limitswitch; // global param ( for non blocking w params )
WiFiManagerParameter forbid_global_watsch_alliance; // global param ( for non blocking w params )
WiFiManagerParameter forbid_mesh_remote; // global param ( for non blocking w params )
WiFiManagerParameter sofort_loswatschen; // global param ( for non blocking w params )

bool verbiete_globale_watschn_allianz=false; //used to determine if global watsch network shall be joined
bool verbiete_mesh_remote=false;
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


//global variables to track loop time statistics for debugging purposes
unsigned long loopStartTime = 0;
unsigned long totalLoopTime = 0;
unsigned long maxLoopTime = 0;
unsigned long loopCount = 0;
unsigned long lastReportTime = 0;
const unsigned long reportInterval = 5000; // Report every 5000 ms (5 seconds)
unsigned long checkpointTime = 0;  // Store the time of the last checkpoint

/* Short description: This sketch for ESP8266 updates itself if a newer version is available in a GitHub repository.
    preset wifi credentials can be put to a textfile in SPIFFS memory of ESP8266. If none of them works, WiFiManager will be started and open an acces point for setup.
    Attention: to upload files to SPIFFS, serial monitor must be closed first.
    To start the OTA update from github:
    - it's possible to enter a command like mosquitto_pub -h broker.hivemq.com -p 1883 -t 'MyDevice/update/' -m 'update' to start the update manually
    - it's possible to check for an update on device start
    - it's possible to start an update based on a time schedule, e.g. once a day (to be implemented)

    WiFi SSID and Password - if you want to upload the sketch to a public GitHub repository, you might not want to put personal data in that sketch
   --> Therefore you can use the 'known_wifis.txt'-file and put your credentials there instead (even more than one wifi).
   You then upload the file as a configuration file for your ESP8266 to its SPIFFS memory (using for example an Arduino IDE SPIFFS uploader plugin like https://github.com/esp8266/arduino-esp8266fs-plugin).
   The sketch will read the file, extract Wifi(s) and password(s) and try to connect to them. If it fails, the sketch starts WiFiManager.
   You then can select one of the available netorks and enter the password for it by connecting to the wifi acces point that the wifimanager opens on your ESP8266.
   The Wifi credentials will be saved on the device as well.
   
   A double reset of ESP8266 will reset wifi credentials. You may need to triple-reset, depending on the speed of pressing reset to detect it correctly.
*/






/*The function read_mqtt_config_from_configfile() reads MQTT topics from a file named "mqtt_config.txt" stored in the SPIFFS memory of a device.
   The function opens the file and reads each line of the file, skipping comments and empty lines. The function then splits each line into a variable name and a topic using the equal sign (=)
   as a delimiter. The function searches for a matching variable name in an array of valid variable names and sets the corresponding MQTT variable to the topic. If the variable name is invalid,
   an error message is printed. After all topics are read and set, the function prints the set MQTT topics and information about the MQTT broker to which the device will connect.
   If the file cannot be opened, an error message is printed, instructing the user to create the file with the correct format.

*/
void read_mqtt_config_from_configfile() {
  // Open mqtt_config.txt file for reading
  File MQTTconfigFile = SPIFFS.open("/mqtt_config.txt", "r");

  if (MQTTconfigFile) {
    Serial.println();
    Serial.println(F("reading mqtt_config.txt from SPIFFS memory."));
    // Read each line from the file
    while (MQTTconfigFile.available()) {
      String line = MQTTconfigFile.readStringUntil('\n');
      line.trim();

      // Skip comments and empty lines
      if (line.startsWith("#") || line.isEmpty()) {
        continue;
      }

      // Split the line into variable name and topic
      int equalsIndex = line.indexOf("=");
      if (equalsIndex == -1) {
        Serial.println("Error: Invalid format on line '" + line + "'. Skipping.");
        continue;
      }
      String varName = line.substring(0, equalsIndex);
      String topic = line.substring(equalsIndex + 1);

      // Find the matching MQTT variable and set the topic
      bool foundVar = false;
      for (int i = 0; i < numValidVarNames; i++) {
        if (varName == validVarNames[i]) {
          *mqttVars[i] = strdup(topic.c_str());
          foundVar = true;
          break;
        }
      }

      // Print an error message if the variable name is invalid
      if (!foundVar) {
        Serial.println();
        Serial.println("Error: Invalid variable name '" + varName + "' in config file. Skipping.");
        Serial.println();
      }
    }
    MQTTconfigFile.close();

    mqttPort = atoi(&*mqttPort_txt); //set the content of the char variable that was read from config file into the int variable for MQTT configuration
    mqttPortLocal = atoi(&*mqttPortLocal_txt); //set the content of the char variable that was read from config file into the int variable for MQTT configuration




    Serial.println(F("The MQTT settings are now set to:"));
    Serial.print(F("mqttClientId: "));
    Serial.println(mqttClientId);
    Serial.println(F("adding ESP ChipID to mqttClientId to generate device-individual identifier:"));
    add_ESPchip_ID_to_mqttClientId();
    Serial.print(F("mqttDevicenameTopic: "));
    Serial.println(mqttDevicenameTopic);
    Serial.print(F("mqttupdateTopic: "));
    Serial.println(mqttupdateTopic);
    Serial.print(F("mqttonlineTopic: "));
    Serial.println(mqttonlineTopic);
    Serial.print(F("mqttfirmwareTopic: "));
    Serial.println(mqttfirmwareTopic);
    Serial.print(F("mqttWillTopic: "));
    Serial.println(mqttWillTopic);
    Serial.println();
    Serial.print(F("Client name: '"));
    Serial.print(mqttClientId);
    Serial.println(F("'"));
    Serial.println();
    Serial.print(F("local MQTT broker '"));
    Serial.print(mqttServerLocal);
    Serial.print(F("' at port "));
    Serial.print(mqttPortLocal);
    Serial.println(F("."));
    Serial.print(F("local MQTT Username: "));
    Serial.println(mqttUserLocal);
    Serial.print(F("local MQTT Password: "));
    Serial.println(mqttPassLocal);
    Serial.println();
    
    Serial.print(F("global MQTT broker: '"));
    Serial.print(mqttServer);
    Serial.print(F("' at port "));
    Serial.print(mqttPort);
    Serial.println(F("."));
    Serial.print(F("global MQTT Username: "));
    Serial.println(mqttUser);
    Serial.print(F("global MQTT Password: "));
    Serial.println(mqttPass);
    Serial.println();
    Serial.print(F("globales Topics der Watschn-allianz: "));
    Serial.print(F("mqttBockfotznTopic: "));
    Serial.println(mqttBockfotznTopic);
    Serial.print(F("mqttGetAggressiveTopic: "));
    Serial.println(mqttGetAggressiveTopic);
    Serial.print(F("mqttMoveStepperTopic: "));
    Serial.println(mqttMoveStepperTopic);
    Serial.print(F("mqttMultiwatschnTopic: "));
    Serial.println(mqttMultiwatschnTopic);
    Serial.println();
    mqtt_config_hasbeenread = true;

  } else {
    Serial.println(F("Failed to read mqtt_config.txt file from SPIFFS memory. To store individual MQTT topics for the device to connect to, create a text file named 'mqtt_config.txt', and add the MQTT topics in a form like this:"));
    Serial.println(F("topic1variable_in_sketch=MyTopic/subtopic1"));
    Serial.println(F("topic2variable_in_sketch=MyTopic/subtopic2"));
    Serial.println(F("topic3variable_in_sketch=MyTopic/subtopic3"));
    Serial.println(F("you must not change the variable names before '=' - if you do, the serial output will display an according error message, as it checks for validity."));
    Serial.println();
  }
}




////char auth[] = "your_token";
//char* ssid[] = {""}; //list a necessary wifi networks
//char* pass[] = {""}; //list a passwords
//void readWifiCredentials(char* ssid[], char* pass[], int maxNetworks) {
//  // open the "known_wifis.txt" file from SPIFFS
//  File file = SPIFFS.open("/known_wifis.txt", "r");
//  if (!file) {
//    Serial.println(F("Failed to open file"));
//    return;
//  }
//
//  // read each line from the file and extract SSID and password
//  int networkCount = 0;
//  while (file.available() && networkCount < maxNetworks) {
//    String line = file.readStringUntil('\n');
//    int separatorIndex = line.indexOf(':');
//    if (separatorIndex == -1) continue; // skip lines without separator
//    ssid[networkCount] = strdup(line.substring(0, separatorIndex).c_str());
//    pass[networkCount] = strdup(line.substring(separatorIndex + 1).c_str());
//    networkCount++;
//  }
//
//  // close the file
//  file.close();
//
//  // print the extracted SSIDs and passwords
//  Serial.println(F("Extracted Wi-Fi credentials:"));
//  for (int i = 0; i < networkCount; i++) {
//    Serial.print(ssid[i]);
//    Serial.print(F(": "));
//    Serial.println(pass[i]);
//  }
//}



/*This function reads known Wi-Fi credentials from a text file named "known_wifis.txt" stored in the SPIFFS memory. If the file cannot be opened,
   the function prints out instructions on how to create the file and add Wi-Fi credentials. The function reads the file line by line, splits each line into an SSID and password and stores them
   in the 'ssid' and 'pass' arrays respectively. Each SSID and password string is dynamically allocated using 'new', and their pointers are stored in the arrays. The function also prints out the
   number of Wi-Fi networks found in the file and the SSID and password credentials for debugging purposes. The maximum number of Wi-Fi networks that can be stored is defined as 'MAX_WIFI_NETWORKS'.
*/

void read_known_wifi_credentials_from_configfile() {
  // Open the file 'known_wifis.txt' in SPIFFS memory for reading
  File file = SPIFFS.open("/known_wifis.txt", "r");
  if (!file) {
    Serial.println(F("Failed to open known_wifis.txt file. To store preset wifi credentials, create a text file named 'known_wifis.txt', and add the preset wifi credentials like this:"));
    Serial.println(F("ssid1:password1"));
    Serial.println(F("ssid2:password2"));
    Serial.println(F("ssid3:password3"));
    Serial.println("...up to MAX_WIFI_NETWORKS, which is currently set to a maximum number of " + MAX_WIFI_NETWORKS);
    Serial.println();
    Serial.println(F("then put this text file in the 'data' folder in the sketch directory, next to the 'certs.ar' file."));
    Serial.println(F("upload both files to ESP8266's SPIFFS memory (using some SPIFFS upload plugin for Arduino IDE or an SPIFFS upload sketch, before flashing this sketch again."));
    Serial.println(F("You then have both files in SPIFFS memory, which will be accessed by this sketch."));
    Serial.println(F("You can omit uploading the known_wifis.txt if you dont want the ESP8266 to initially try to connect to some preset wifi networks, and only use WiFiManager"));
    Serial.println(F("WiFiManager will be started if either no connection to a preset WiFi can be established, or if known_wifis.txt is not preset at all."));
    Serial.println();
    return;
  }

  // Read the file line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');

    // Split the line into ssid and password
    int splitIndex = line.indexOf(':');
    if (splitIndex == -1) {
      continue;
    }
    String ssidStr = line.substring(0, splitIndex);
    String passStr = line.substring(splitIndex + 1);
    ssid[numWifiNetworks] = new char[ssidStr.length() + 1];
    strcpy(ssid[numWifiNetworks], ssidStr.c_str());
    pass[numWifiNetworks] = new char[passStr.length() + 1];
    strcpy(pass[numWifiNetworks], passStr.c_str());
    numWifiNetworks++;

    if (numWifiNetworks >= MAX_WIFI_NETWORKS) {
      break;
    }
  }

  // Close the file
  file.close();

  if (numWifiNetworks > 0) {
    Serial.print(F("Found "));
    Serial.print(numWifiNetworks);
    Serial.println(F(" preset WiFi networks in 'known_wifis.txt' on ESP8266 SPIFFS memory:"));
    Serial.println();
  }
  // Print the credentials for debugging purposes
  for (int i = 0; i < numWifiNetworks; i++) {
    Serial.printf("Wifi %d: %s, %s\n", i, ssid[i], pass[i]);
  }
}


/*MultiWiFiCheck that attempts to connect to multiple WiFi networks in sequence until it successfully connects to one.
   It first calculates the number of WiFi networks stored in the ssid array and then tries to connect to each network in turn.
*/
//void MultiWiFiCheck() {
//  int ssid_count=0;
//  int ssid_mas_size = sizeof(ssid) / sizeof(ssid[0]);
//  do {
//    Serial.println();
//    Serial.print("Trying to connect to WiFi '" + String(ssid[ssid_count]));
//    Serial.print(F("' with a timeout of "));
//    Serial.print(wifitimeout);
//    Serial.print(F(" seconds.  "));
//    WiFi.begin(ssid[ssid_count], pass[ssid_count]);
//    int WiFi_timeout_count=0;
//    while (WiFi.status() != WL_CONNECTED && WiFi_timeout_count<(wifitimeout*10)) { //waiting wifitimeout seconds (*10 is used to get more dots "." printed while trying)
//      delay(100);
//      Serial.print(F("."));
//      ++WiFi_timeout_count;
//    }
//    if (WiFi.status() == WL_CONNECTED) {
//      Serial.println();
//      Serial.println(F("Connected to WiFi!"));
//      //connected_to_preset_wifi=true;
//      Serial.println();
//      return;
//    }
//    Serial.println();
//    ++ssid_count;
//  }
//  //while (ssid_count<ssid_mas_size);
//  while (ssid_count<numWifiNetworks);
//
//
//}

void MultiWiFiCheck() {

  int numFoundNetworks = WiFi.scanNetworks();
  int bestSignalStrength = -1000;  // Start with a very weak signal strength
  int bestNetworkIndex = -1;  // Index of the network with the best signal strength

  // Find the network with the strongest signal strength
  for (int i = 0; i < numWifiNetworks; i++) {
    for (int j = 0; j < numFoundNetworks; j++) {
      if (WiFi.SSID(j) == String(ssid[i])) {
        int32_t rssi = WiFi.RSSI(j);  // Get the signal strength of the current network
        if (rssi > bestSignalStrength) {
          bestSignalStrength = rssi;
          bestNetworkIndex = j;
        }
        break;
      }
    }
    // Output the strongest network's SSID and signal strength to the serial monitor
  }

  if (bestNetworkIndex == -1) {
    // None of the preset networks were found
    Serial.println(F("None of the preset WiFi networks were found."));
    return;
  }
  else {
    if (bestNetworkIndex >= 0) {
      Serial.println();
      Serial.print(F("Strongest (known) network from known_wifis.txt file in SPIFFS memory: "));
      Serial.print(WiFi.SSID(bestNetworkIndex));
      Serial.print(F(", signal strength: "));
      Serial.println(bestSignalStrength);
    }
  }

  // Connect to the network with the strongest signal strength
  String selectedSSID = WiFi.SSID(bestNetworkIndex);
  String selectedPassword = pass[0];

  for (int i = 0; i < numWifiNetworks; i++) {
    if (selectedSSID == String(ssid[i])) {
      selectedPassword = pass[i];
      break;
    }
  }

  Serial.print(F("Connecting to Wifi '"));
  Serial.print(selectedSSID);
  Serial.println(F("' ..."));
  WiFi.begin(selectedSSID.c_str(), selectedPassword.c_str());

  int WiFi_timeout_count = 0;
  while (WiFi.status() != WL_CONNECTED && WiFi_timeout_count < (wifitimeout * 10)) {
    delay(100);
    Serial.print(F("."));
    ++WiFi_timeout_count;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("successfully connected to WiFi with IP address "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println(F("Failed to connect to WiFi."));
  }
}



void setup_GitHUB_OTA_upgrade() {
    if (drd.detectDoubleReset()) {
        handleDoubleReset();
        return; // Restart the ESP after handling double reset
    }

    // Print startup information
    printStartupInfo();

    // Initialize SPIFFS and check for certificates
    SPIFFS.begin();
    if (!initCertStore()) {
        return; // Exit if no certificates are found
    }

    // WiFi connection setup
    if (WiFi.SSID() == "") {
        handleWiFiConnection();
    }

    // Initialize WiFiManager if necessary
    if (WiFi.status() != WL_CONNECTED) {
        initializeWiFiManager();
    }

    // Once WiFi is connected
    Serial.println(F("Should now call setup_mqtt_config() and depending on #defines also setupLocalMQTT() and setupGlobalMQTT()"));
    if (WiFi.status() == WL_CONNECTED) {
      
        setup_mqtt_config();
        #ifdef USE_LOCAL_MQTT
        setupLocalMQTT();
        #endif
        #ifdef USE_GLOBAL_MQTT
        setupGlobalMQTT();
        #endif
        
    } else {
        handleWiFiConnectionFailure();
    }

    Serial.println(F("Setup finished. Going to loop() now"));
    Serial.flush();
}

void handleDoubleReset() {
    Serial.println(F("======================== Double Reset Detected ========================"));
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F("Erasing WiFi credentials saved in WiFiManager - but will still connect to WiFi from known_wifis.txt in SPIFFS memory"));
    delay(1000);
    wm.resetSettings();
    delay(100);
    ESP.restart();
}

void printStartupInfo() {
    Serial.println();
    Serial.println(F("================================================================================"));
    Serial.println(F("|                                                                              |"));
    Serial.println(F("|                   WatschnMaster is part of the global                        |"));
    Serial.println(F("|                   Watschn-Allianz in the Bockfotzn network.                   |"));
    Serial.println(F("|                =============================================                 |"));
    Serial.print(F("|    Version:    "));
    Serial.print(GHOTA_CURRENT_TAG);
    Serial.println(F("                                                         |"));
    Serial.println(F("|                                                                              |"));
    Serial.println(F("|  - Double reset (power-OFF) will reset WiFi credentials from WiFi Manager -  |"));
    Serial.println(F("|                     - You may need to triple reset -                         |"));
    Serial.println(F("|                                                                              |"));
    Serial.println(F("|    CODE:     https://github.com/lademeister/WatschnMaster                    |"));
    Serial.println(F("================================================================================"));
    Serial.println();
}

bool initCertStore() {
    int numCerts = certStore.initCertStore(SPIFFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.print(F("Reading CA certificates: "));
    Serial.print(numCerts);
    Serial.println(F(" certificates read."));
    if (numCerts == 0) {
        Serial.println(F("No certificates found. Make sure you have uploaded the 'certs.ar' file to SPIFFS."));
        return false;
    }
    return true;
}

void handleWiFiConnection() {
    Serial.println(F("It seems that WiFiManager was not previously connected to a network."));
    Serial.println(F("Performing setup for first-time use."));
    read_known_wifi_credentials_from_configfile();
    MultiWiFiCheck();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println();
        Serial.println(F("Could not connect to any known WiFi networks."));
        Serial.print(F("Starting WiFiManager. Please connect to the access point '"));
        Serial.print(wm_accesspointname);
        Serial.print(F("' within "));
        Serial.print(wm_accesspointtimeout);
        Serial.println(F(" seconds timeout."));
        Serial.println();
    }
}

void initializeWiFiManager() {
    WiFiManager wifiManager;
    wifiManager.setConfigPortalBlocking(true);
    wifiManager.setSaveParamsCallback(saveParamsCallback);
    wifiManager.setTimeout(wifitimeout);
    wifiManager.autoConnect(wm_accesspointname);
}

void setupGlobalMQTT() {
//    display.clear();
//    delay(10);
//    display.setFont(ArialMT_Plain_16);
//    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//    display.drawString(display.getWidth() / 2, 31, "trying to connect\nto global MQTT\nWatschn-Allianz\n(Bockfotzn-Netzwerk)");
//    display.display();
    
    Serial.println(F("Setting up global MQTT client (Globale Watschn-Allianz):"));
    //setup_mqtt_config();
    connectToMQTT(mqttClient, mqttServer, mqttPort, mqttUser, mqttPass);

    if (mqttClient.connected()) {
        mqttClient.publish(mqttWillTopic, "Online", true);
        publish_device_info();
        mqttClient.publish(mqttfirmwareTopic, GHOTA_CURRENT_TAG, true);

        // Subscribe to MQTT topics
        mqttClient.subscribe(mqttupdateTopic);
        mqttClient.subscribe(mqttBockfotznTopic);
        mqttClient.subscribe(mqttGetAggressiveTopic);
        mqttClient.subscribe(mqttMoveStepperTopic);
        mqttClient.subscribe(mqttMultiwatschnTopic);

        Serial.println(F("Device is now participating in the 'Globale Watschn-Allianz'."));
        printMQTTCommandExamples();
    } else {
        Serial.println(F("ERROR: Global MQTT connection to Watschn-Allianz not successful."));
    }
}

void setupLocalMQTT() {
//    display.clear();
//    delay(10);
//    display.setFont(ArialMT_Plain_16);
//    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//    display.drawString(display.getWidth() / 2, 31, "trying to connect\nto local MQTT");
//    display.display();
    Serial.println();
    Serial.println(F("Setting up local MQTT client:"));
    //setup_mqtt_config(); // Ensure this function sets up local MQTT parameters
    connectToMQTT(mqttClientLocal, mqttServerLocal, mqttPortLocal, mqttUserLocal, mqttPassLocal);

    if (mqttClientLocal.connected()) {
        mqttClientLocal.publish(mqttWillTopicLocal, "Online", true);
        publish_device_info(); // Adapt this if needed for local MQTT

        // Subscribe to local MQTT topics (interims-Topics, später: nutze untere kommentierte) -  also at another place in code!! around line 1445
        mqttClientLocal.subscribe(mqttupdateTopic);
        mqttClientLocal.subscribe(mqttBockfotznTopicLocal);
        mqttClientLocal.subscribe(mqttGetAggressiveTopicLocal);
        mqttClientLocal.subscribe(mqttMoveStepperTopicLocal);
        mqttClientLocal.subscribe(mqttMultiwatschnTopicLocal);
        
        // Subscribe to local MQTT topics
//        mqttClient.subscribe(mqttupdateTopic);
//        mqttClient.subscribe(mqttBockfotznTopic);
//        mqttClient.subscribe(mqttGetAggressiveTopic);
//        mqttClient.subscribe(mqttMoveStepperTopic);
//        mqttClient.subscribe(mqttMultiwatschnTopic);

        Serial.println(F("Device is now participating in the local MQTT network."));
    } else {
        Serial.println(F("ERROR: Local MQTT connection not successful."));
    }
}

//void connectToMQTT(PubSubClient &client, const char* server, int port, const char* user, const char* pass) {
//    int mqtt_connection_tries = 0;
//    while (!(client.connect(mqttClientId, user, pass)) && mqtt_connection_tries < maximum_mqtt_connection_tries) {
//        Serial.print(F("MQTT: Trying to connect as clientID '"));
//        Serial.print(mqttClientId);
//        Serial.print(F("' to MQTT server "));
//        Serial.print(server);
//        Serial.print(F(" at Port "));
//        Serial.print(port);
//        Serial.print(F(" with user '"));
//        Serial.print(user);
//        Serial.print(F("'"));
//        Serial.print(F(" and passwort '"));
//        Serial.print(pass);
//        Serial.println(F("'"));
//        
//        mqtt_connection_tries++;
//        delay(1000);
//    }
//}

void connectToMQTT(PubSubClient &client, const char* server, int port, const char* user, const char* pass) {
    int mqtt_connection_tries = 0;

    // Print out connection parameters before attempting to connect
    Serial.println(F("### MQTT Connection Parameters ###"));
    Serial.print(F("Client ID: "));
    Serial.println(mqttClientId);
    Serial.print(F("Server: "));
    Serial.println(server);
    Serial.print(F("Port: "));
    Serial.println(port);
    Serial.print(F("User: "));
    Serial.println(user);
    Serial.print(F("Password: "));
    Serial.println(pass);
    Serial.println(F("################################"));
    

    // Attempt to connect to the MQTT broker
    while (!(client.connect(mqttClientId, user, pass)) && mqtt_connection_tries < maximum_mqtt_connection_tries) {
        Serial.print(F("MQTT: Trying to connect as clientID '"));
        Serial.print(mqttClientId);
        Serial.print(F("' to MQTT server "));
        Serial.print(server);
        Serial.print(F(" at Port "));
        Serial.print(port);
        Serial.print(F(" with user '"));
        Serial.print(user);
        Serial.print(F("'"));
        Serial.print(F(" and password '"));
        Serial.print(pass);
        Serial.println(F("'"));
        
        mqtt_connection_tries++;
        delay(1000);
    }

    if (client.connected()) {
        Serial.println(F("MQTT: Connected successfully."));
    } else {
        Serial.println(F("MQTT: Failed to connect after maximum tries."));
    }
}


void printMQTTCommandExamples() {
    Serial.println(F("Update syntax for MacOS / Linux shell for this device:"));
    Serial.print(F("mosquitto_pub -h "));
    Serial.print(mqttServer);
    Serial.print(F(" -p "));
    Serial.print(mqttPort);
    Serial.print(F(" -t "));
    Serial.print(mqttupdateTopic);
    Serial.print(F(" -m update -u "));
    Serial.print(mqttUser);
    Serial.print(F(" -P "));
    Serial.print(mqttPass);
    Serial.println(F(" --insecure"));
    // Additional examples as needed
}

void handleWiFiConnectionFailure() {
    Serial.println(F("WiFi could not be connected:"));
    Serial.println(F("- No success to connect to preset WiFi's."));
    Serial.println(F("- WiFiManager timed out, or"));
    Serial.println(F("- WiFi credentials were not entered correctly."));
    Serial.println(F("Skipping check for Firmware update due to missing internet connection."));
    Serial.println(F("Reboot to try again."));
}

/****** root certificate *********/

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";


//void add_ESPchip_ID_to_mqttClientId(){
//    // Get the ESP8266 Chip ID
//  uint32_t chipID = ESP.getChipId();
//
//  // Convert the chip ID to a string and append it to mqttClientId
//  snprintf(mqttClientId + strlen(mqttClientId), sizeof(mqttClientId) - strlen(mqttClientId), "-%08X", chipID);
//
//  // Now mqttClientId contains the final mqttClientId with the chip ID
//  Serial.print("Final MQTT Client ID: ");
//  Serial.println(mqttClientId);
//}

//void add_ESPchip_ID_to_mqttClientId(){
//    // Get the ESP8266 Chip ID
//    uint32_t chipID = ESP.getChipId();
//
//    // Determine the length of the original mqttClientId
//    size_t originalLength = strlen(mqttClientId);
//
//    // Estimate the length needed to append the chip ID (8 characters + 1 for '-')
//    size_t additionalLength = 9; // "-%08X" is 9 characters long
//
//    // Allocate sufficient memory for the final mqttClientId
//    char* finalMqttClientId = (char*)malloc(originalLength + additionalLength + 1);
//
//    if (finalMqttClientId != nullptr) {
//        // Copy the original mqttClientId to the new memory location
//        strcpy(finalMqttClientId, mqttClientId);
//
//        // Append the chip ID to the new mqttClientId
//        snprintf(finalMqttClientId + originalLength, additionalLength + 1, "-%08X", chipID);
//
//        // Replace the original mqttClientId with the new one
//        mqttClientId = finalMqttClientId;
//
//        // Output the final result
//        Serial.print(F("mqttClientId is now: "));
//        Serial.println(mqttClientId);
//
//        // Note: No need to free the memory if you're using mqttClientId afterward.
//    } else {
//        Serial.println("Failed to allocate memory for the final MQTT Client ID.");
//    }
//}

void add_ESPchip_ID_to_mqttClientId() {
    // Get the ESP8266 Chip ID
    uint32_t chipID = ESP.getChipId();
    char chipIDStr[9]; // Buffer to hold the chip ID string in hex format

    // Convert the chip ID to a string
    snprintf(chipIDStr, sizeof(chipIDStr), "%08X", chipID);

    // Determine the length of the original mqttClientId
    size_t originalLength = strlen(mqttClientId);
    
    // Length of chipIDStr including the hyphen
    size_t suffixLength = strlen(chipIDStr) + 1; // "+1" for the hyphen
    
    // Check if mqttClientId already ends with "-chipID"
    if (originalLength >= suffixLength && 
        strcmp(mqttClientId + originalLength - suffixLength + 1, chipIDStr) == 0) {
        Serial.println(F("mqttClientId already ends with the chip ID."));
        return; // Chip ID is already appended; no need to change mqttClientId
    }

    // Allocate memory for the new mqttClientId
    size_t newLength = originalLength + suffixLength + 1; // +1 for null terminator
    char* finalMqttClientId = (char*)malloc(newLength);

    if (finalMqttClientId != nullptr) {
        // Copy the original mqttClientId to the new memory location
        strcpy(finalMqttClientId, mqttClientId);

        // Append the chip ID to the new mqttClientId
        snprintf(finalMqttClientId + originalLength, suffixLength + 1, "-%s", chipIDStr);

        // Replace the original mqttClientId with the new one
        free(mqttClientId); // Free the old mqttClientId if previously allocated
        mqttClientId = finalMqttClientId;

        // Output the final result
        Serial.print(F("mqttClientId is now: "));
        Serial.println(mqttClientId);
    } else {
        Serial.println(F("Failed to allocate memory for the final MQTT Client ID."));
    }
}



//old 
//void setup_mqtt_config() {
//  read_mqtt_config_from_configfile();
//  mqttClient.setServer(mqttServer, mqttPort);
//  mqttClient.setCallback(mqttCallback);
//  Serial.println(F("MQTT broker settings and MQTT callback setting: done."));
//}

//new:
#ifdef ESP8266
// ESP8266-specific SSL/TLS configuration
void setup_mqtt_config() {
  read_mqtt_config_from_configfile();
  Serial.println(F("### can NOT use certificates for MQTT connection because ESP8266 does not support it ###"));
  secureClientGlobal.setInsecure();  // Disable certificate verification for ESP8266
  //secureClientLocal.setInsecure();  // Disable certificate verification for ESP8266 //commented out when using normal client instead of secure client
  #ifdef USE_GLOBAL_MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  #endif
  #ifdef USE_LOCAL_MQTT
  mqttClientLocal.setServer(mqttServerLocal, mqttPortLocal);
  mqttClientLocal.setCallback(mqttCallback);
  #endif
  Serial.println(F("MQTT broker settings and MQTT callback setting: done."));
}
#else
// For ESP32 or other platforms where certificate is used
void setup_mqtt_config() {
  read_mqtt_config_from_configfile();
  if (numCerts > 0) {
    // Set CertStore for secureClient
  #ifdef USE_GLOBAL_MQTT
    secureClientGlobal.setCertStore(&certStore);
    #endif
    #ifdef USE_LOCAL_MQTT
    secureClientLocal.setCertStore(&certStore);
    #endif
    Serial.println(F("Using certificate certs.ar/certs.idx from SPIFFS for MQTT connection."));
  } else {
    Serial.println(F("No certificate found in SPIFFS, MQTT connection may fail. Trying to use hardcoded certificate instead."));
    #ifdef USE_LOCAL_MQTT
    secureClientLocal.setCACert(root_ca);  // Provide the CA certificate for verification
    #endif
    #ifdef USE_GLOBAL_MQTT
    secureClientGlobal.setCACert(root_ca);  // Provide the CA certificate for verification
    #endif
  }
  #ifdef USE_GLOBAL_MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  #endif
  #ifdef USE_LOCAL_MQTT
  mqttClientLocal.setServer(mqttServerLocal, mqttPortLocal);
  mqttClientLocal.setCallback(mqttCallback);
  #endif
  Serial.println(F("MQTT broker settings and MQTT callback setting: done."));
  }
#endif





void setup_local_OTA(){
     //ArduinoOTA.setHostname("MyDevice");
     ArduinoOTA.setHostname(mqttClientId); //using the MQTT name also for the name the device will appear as device for *local* OTA updates over WiFi (using Arduino IDE)
     

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
}

void check_ota_github() {
  // code to be executed when there is a successful WiFi connection

  /* This is the actual code to check and upgrade */
  Serial.println(F("Checking if an OTA update is available on GitHub..."));
  Serial.println();
  Serial.println(F("IF YOU GET AN ERROR THAT LÒOKS LIKE THIS:"));
  Serial.println(F("'User exception (panic/abort/assert)"));
  Serial.println(F("--------------- CUT HERE FOR EXCEPTION DECODER ---------------"));
  Serial.println();
  Serial.println(F("Unhandled C++ exception: OOM"));
  Serial.println(F("..."));
  Serial.println(F("...that 'OOM' means 'Out Of Memory' which indicates that too much RAM is used and the code therefore crashes."));
  Serial.println(F("That happens e.g. if using too much memory with variables, especially Strings that are not put to the Flash memory but also an other variables."));

  Serial.println();
  if (ESPOTAGitHub.checkUpgrade()) {
    Serial.print(F("Upgrade found at: "));
    Serial.println(ESPOTAGitHub.getUpgradeURL());
    if (ESPOTAGitHub.doUpgrade()) {
      Serial.println(F("Upgrade complete.")); //This should never be seen as the device should restart on successful upgrade.
    } else {
      Serial.print(F("Unable to upgrade: "));
      Serial.println(ESPOTAGitHub.getLastError());
    }
  } else {
    Serial.print(F("Not proceeding to upgrade: "));
    //Serial.println(ESPOTAGitHub.getLastError());


    String errorCode = ESPOTAGitHub.getLastError();
    if (errorCode == "Failed to parse JSON.") { //DEBUG: reboot here, as JSON parsing fails after freshly setting an AP with wifimanager.
      Serial.println(ESPOTAGitHub.getLastError());
      Serial.println(F("rebooting..."));
      ESP.restart();
      delay(1000); //we do not arrive here because of reboot.
    } else {
      Serial.println(ESPOTAGitHub.getLastError());
    }
  }
}

//void mqttCallback(char* topic, byte* payload, unsigned int length) {
//  // Handle MQTT messages received
//}


//function prototype to let the compiler know that functions exists later in code, including default parameter values:
void massier_den_kürbis_bis_er_mürb_is(bool retransmit_message = true); 
void massier_den_kürbis_soft(bool retransmit_message = true);
void schwungholen_und_watschen(bool retransmit_message = true);
void ausfallschritt_und_schelln(bool retransmit_message = true); 
void multiwatschn(int haudrauf,bool retransmit_message = true); 
void ausfallschritt_und_schelln_andeuten(bool retransmit_message = true);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print(F("MQTT Message received in topic '"));
  Serial.print(topic);
  Serial.print(F("'. Message: '"));
  Serial.print(message);
  Serial.println(F("'"));
  Serial.println();

  if (strcmp(topic, mqttupdateTopic) == 0) {
    if (message == "update") {
      check_ota_github();
    }
  }
  else if ((strcmp(topic, mqttBockfotznTopic) == 0)||(strcmp(topic, mqttBockfotznTopicLocal) == 0)) {
    if (message == "watschn") {
      Serial.println(F("Aufruf 'schwungholen_und_watschen(bool retransmit_message)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk oder über lokal."));
      schwungholen_und_watschen(0);
    }
    else if (message == "schelln") {
      Serial.println(F("Aufruf 'ausfallschritt_und_schelln(bool retransmit_message)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
      ausfallschritt_und_schelln(0); 
    }
    else if (message == "schelln andeuten") {
      Serial.println(F("Aufruf 'ausfallschritt_und_schelln_andeuten(bool retransmit_message)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
      ausfallschritt_und_schelln_andeuten(0); 
    }
    else if (message == "kuerbis_hard"){
        Serial.println(F("Aufruf 'massier_den_kürbis_bis_er_mürb_is(bool retransmit_message)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
        massier_den_kürbis_bis_er_mürb_is(0); 
    } else if (message == "kuerbis_soft"){
        Serial.println(F("Aufruf 'massier_den_kürbis_soft(bool retransmit_message)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
        massier_den_kürbis_soft(0); 
    }
  }
  else if (strcmp(topic, mqttMultiwatschnTopic) == 0) {
    if (isNumber(message)) {
        Serial.println(F("Aufruf 'multiwatschn(int haudrauf, bool retransmit)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
        int number = message.toInt(); // Convert the String to an integer
        multiwatschn(number, 0); //multiwatschn(int haudrauf);
    } else {
        Serial.println(F("The message received in mqttMultiwatschnTopic is not a valid number."));
    }
  }
  else if (strcmp(topic, mqttMoveStepperTopic) == 0) {
    if (isNumber(message)) {
        Serial.println(F("Aufruf 'move_stepper(steps)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
        int number = message.toInt(); // Convert the String to an integer
        move_stepper(number); //move_stepper(steps)
    } else {
        Serial.println(F("The message received in mqttMultiwatschnTopic is not a valid number."));
    }
  }
  else if (strcmp(topic, mqttGetAggressiveTopic) == 0) {
    if (isNumber(message)) {
        Serial.println(F("Aufruf 'get_aggressive(int how_agressive)' aus der globalen Watsch-Allianz über das Bockfotzn-Netzwerk."));
        int number = message.toInt(); // Convert the String to an integer
        get_aggressive(number); //get_aggressive(int how_agressive)
    } else {
        Serial.println(F("The message received in mqttGetAggressiveTopic is not a valid number."));
    }
  }
}



bool isNumber(String message) {
    for (unsigned int i = 0; i < message.length(); i++) {
        if (!isDigit(message[i])) {
            return false;
        }
    }
    return true;
}

void setupMQTT() {
  mqttClient.setServer(mqttServer, mqttPort);
  // set the callback function
  mqttClient.setCallback(mqttCallback);
}

//boolean reconnect_global(){
//   setupGlobalMQTT();
//
//  // Attempt to reconnect global MQTT client
//  if (!mqttClient.connected()) {
//      Serial.print(F("Global MQTT: Attempting to reconnect using client name '"));
//      Serial.print(mqttClientId);
//      Serial.print(F("' as user '"));
//      Serial.print(mqttUser);
//      Serial.print(F("' with password '"));
//      Serial.print(mqttPass);
//      Serial.print(F("' to server "));
//      Serial.print(mqttServer);
//      Serial.print(F(" at Port "));
//      Serial.println(mqttPort);
//
//      if (mqttClient.connect(mqttClientId, mqttUser, mqttPass)) {
//          Serial.println(F("(Global MQTT) Reconnected."));
//          mqttClient.publish(mqttDevicenameTopic, mqttClientId);
//          mqttClient.publish(mqttonlineTopic, "online"); // Will be set up with will message
//          publish_device_info();
//          mqttClient.publish(mqttfirmwareTopic, GHOTA_CURRENT_TAG, true); // Retained message
//          mqttClient.subscribe(mqttupdateTopic);
//          mqttClient.subscribe(mqttBockfotznTopic);
//          mqttClient.subscribe(mqttGetAggressiveTopic);
//          mqttClient.subscribe(mqttMoveStepperTopic);
//          mqttClient.subscribe(mqttMultiwatschnTopic);
//          return true;
//      } else {
//          Serial.print(F("Global MQTT connection failed. Error: "));
//          Serial.println(mqttClient.state());
//          return false;
//      }
//  }
//}
//
//boolean reconnect_local(){
//    setupLocalMQTT();
//
// 
//
//  // Attempt to reconnect local MQTT client
//  if (!mqttClientLocal.connected()) {
//      Serial.print(F("Local MQTT: Attempting to reconnect using client name '"));
//      Serial.print(mqttClientId);
//      Serial.print(F("' as user '"));
//      Serial.print(mqttUserLocal);
//      Serial.print(F("' with password '"));
//      Serial.print(mqttPassLocal);
//      Serial.print(F("' to server "));
//      Serial.print(mqttServerLocal);
//      Serial.print(F(" at Port "));
//      Serial.println(mqttPortLocal);
//
//      if (mqttClientLocal.connect(mqttClientId, mqttUserLocal, mqttPassLocal)) {
//          Serial.println(F("(Local MQTT) Reconnected."));
//          mqttClientLocal.publish(mqttDevicenameTopic, mqttClientId);
//          mqttClientLocal.publish(mqttonlineTopicLocal, "online"); // Will be set up with will message
////          publish_device_info(); // Adapt if necessary for local MQTT
//
//
////interims-topics - use commented ones below later. also at another place in code!!  (around line 963)
//          mqttClientLocal.subscribe(mqttupdateTopic);
//          mqttClientLocal.subscribe(mqttBockfotznTopicLocal);
//          mqttClientLocal.subscribe(mqttGetAggressiveTopicLocal);
//          mqttClientLocal.subscribe(mqttMoveStepperTopicLocal);
//          mqttClientLocal.subscribe(mqttMultiwatschnTopicLocal);
//
////          mqttClient.subscribe(mqttupdateTopic);
////          mqttClient.subscribe(mqttBockfotznTopic);
////          mqttClient.subscribe(mqttGetAggressiveTopic);
////          mqttClient.subscribe(mqttMoveStepperTopic);
////          mqttClient.subscribe(mqttMultiwatschnTopic);
//       return true;
//      } else {
//          Serial.print(F("Local MQTT connection failed. Error: "));
//          Serial.println(mqttClientLocal.state());
//          return false;
//      }
//  }
//}

boolean reconnect_global() {
    setupGlobalMQTT();

    // Attempt to reconnect global MQTT client
    if (!mqttClient.connected()) {
        Serial.print(F("Global MQTT: Attempting to reconnect using client name '"));
        Serial.print(mqttClientId);
        Serial.print(F("' as user '"));
        Serial.print(mqttUser);
        Serial.print(F("' with password '"));
        Serial.print(mqttPass);
        Serial.print(F("' to server "));
        Serial.print(mqttServer);
        Serial.print(F(" at Port "));
        Serial.println(mqttPort);

        if (mqttClient.connect(mqttClientId, mqttUser, mqttPass)) {
            Serial.println(F("(Global MQTT) Reconnected."));
            mqttClient.publish(mqttDevicenameTopic, mqttClientId);
            mqttClient.publish(mqttonlineTopic, "online"); // Will be set up with will message
            publish_device_info();
            mqttClient.publish(mqttfirmwareTopic, GHOTA_CURRENT_TAG, true); // Retained message
            mqttClient.subscribe(mqttupdateTopic);
            mqttClient.subscribe(mqttBockfotznTopic);
            mqttClient.subscribe(mqttGetAggressiveTopic);
            mqttClient.subscribe(mqttMoveStepperTopic);
            mqttClient.subscribe(mqttMultiwatschnTopic);
            return true;
        } else {
            Serial.print(F("Global MQTT connection failed. Error: "));
            Serial.println(mqttClient.state());
            return false;
        }
    }

    // Return true if already connected
    return true;
}

boolean reconnect_local() {
    setupLocalMQTT();

    // Attempt to reconnect local MQTT client
    if (!mqttClientLocal.connected()) {
        Serial.print(F("Local MQTT: Attempting to reconnect using client name '"));
        Serial.print(mqttClientId);
        Serial.print(F("' as user '"));
        Serial.print(mqttUserLocal);
        Serial.print(F("' with password '"));
        Serial.print(mqttPassLocal);
        Serial.print(F("' to server "));
        Serial.print(mqttServerLocal);
        Serial.print(F(" at Port "));
        Serial.println(mqttPortLocal);

        if (mqttClientLocal.connect(mqttClientId, mqttUserLocal, mqttPassLocal)) {
            Serial.println(F("(Local MQTT) Reconnected."));
            mqttClientLocal.publish(mqttDevicenameTopic, mqttClientId);
            mqttClientLocal.publish(mqttonlineTopicLocal, "online"); // Will be set up with will message
//            publish_device_info(); // Adapt if necessary for local MQTT

//interims-topics - use commented ones below later. also at another place in code!!  (around line 963)
            mqttClientLocal.subscribe(mqttupdateTopic);
            mqttClientLocal.subscribe(mqttBockfotznTopicLocal);
            mqttClientLocal.subscribe(mqttGetAggressiveTopicLocal);
            mqttClientLocal.subscribe(mqttMoveStepperTopicLocal);
            mqttClientLocal.subscribe(mqttMultiwatschnTopicLocal);

//            mqttClient.subscribe(mqttupdateTopic);
//            mqttClient.subscribe(mqttBockfotznTopic);
//            mqttClient.subscribe(mqttGetAggressiveTopic);
//            mqttClient.subscribe(mqttMoveStepperTopic);
//            mqttClient.subscribe(mqttMultiwatschnTopic);
            return true;
        } else {
            Serial.print(F("Local MQTT connection failed. Error: "));
            Serial.println(mqttClientLocal.state());
            return false;
        }
    }

    // Return true if already connected
    return true;
}


//boolean reconnect() {
//  // Setup MQTT configurations
//  setup_mqtt_config();
//  #ifdef USE_LOCAL_MQTT
//  reconnect_local();
//  #endif
//  #ifdef USE_GLOBAL_MQTT
//  reconnect_global();
//  #endif
//  
////  #ifdef USE_LOCAL_MQTT
////   #ifdef USE_GLOBAL_MQTT
////       // Return true if both clients are connected
////      return mqttClient.connected() && mqttClientLocal.connected();
////    #endif
////  #endif
////
////  #ifdef USE_LOCAL_MQTT
////    return mqttClientLocal.connected()
////  #endif
////
////  #ifdef USE_GLOBAL_MQTT
////    return mqttClient.connected()
////  #endif
//bool checkMQTTConnections() {
//    #ifdef USE_LOCAL_MQTT
//        #ifdef USE_GLOBAL_MQTT
//            // Both USE_LOCAL_MQTT and USE_GLOBAL_MQTT are defined
//            return mqttClient.connected() && mqttClientLocal.connected();
//        #else
//            // Only USE_LOCAL_MQTT is defined
//            return mqttClientLocal.connected();
//        #endif
//    #else
//        #ifdef USE_GLOBAL_MQTT
//            // Only USE_GLOBAL_MQTT is defined
//            return mqttClient.connected();
//        #else
//            // Neither USE_LOCAL_MQTT nor USE_GLOBAL_MQTT is defined
//            return false;
//        #endif
//    #endif
//}
//}

bool checkMQTTConnections() {
    #ifdef USE_LOCAL_MQTT
        #ifdef USE_GLOBAL_MQTT
            return mqttClient.connected() && mqttClientLocal.connected();
        #else
            return mqttClientLocal.connected();
        #endif
    #else
        #ifdef USE_GLOBAL_MQTT
            return mqttClient.connected();
        #else
            return false;
        #endif
    #endif
}

boolean reconnect() {
  // Setup MQTT configurations
  setup_mqtt_config();
  
  #ifdef USE_LOCAL_MQTT
  reconnect_local();
  #endif
  
  #ifdef USE_GLOBAL_MQTT
  reconnect_global();
  #endif

  // Check MQTT connections
  return checkMQTTConnections();
}


void publishToMQTT(String message, char* topic) {
  // Publish the user-defined message to the specified topic
  mqttClient.publish(topic, message.c_str());
}
/*examples:
 * publishToMQTT("watschn", mqttBockfotznTopic);
 * publishToMQTT("68", mqttMoveStepperTopic);
 */

void publish_device_info() {
  String message = "DeviceID ";
  message += device_id;
  message += " (Identifying as ";
  message += mqttClientId;
  message += ") running firmware ";
  message += GHOTA_CURRENT_TAG;
  #ifdef USE_GLOBAL_MQTT
  mqttClient.publish(mqttDevicenameTopic, message.c_str());
  Serial.print(F("Device info published to global MQTT server in topic '"));
  Serial.print(mqttDevicenameTopic);
  Serial.println(F("'."));
  #endif
  #ifdef USE_LOCAL_MQTT
  mqttClientLocal.publish(mqttDevicenameTopic, message.c_str());
  Serial.print(F("Device info published to local MQTT server in topic '"));
  Serial.print(mqttDevicenameTopic);
  Serial.println(F("'."));
  #endif
}





void setup() {
    Wire.begin(); // initialize I2C interface
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    setup_GitHUB_OTA_upgrade();
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
    
    //schwungholen_und_watschen();
    //schwungholen_V2();
    //delay(3000);

    goto_init_position(1);
    
    display.drawString(display.getWidth() / 2, 31, "trying to connect\nto known WiFi");
    display.display();
    setup_mqtt_config();

    //----------------------
    

   
    
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    // put your setup code here, to run once:
    
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
//------------------------------------------------------------------------
  // add a custom input field
  int customFieldLength = 40;


  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
  
//  // test custom html input type(checkbox)
//  new (&calibrate_rotationspeed) WiFiManagerParameter("customfieldid", "maximalen Rotationsspeed Günter kalibrieren", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
//  new (&calibrate_limitswitch) WiFiManagerParameter("customfieldid", "Limitswitch kalibrieren", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

//corrected:


//// Create the WiFiManagerParameter for limit switch calibration
//new (&calibrate_rotationspeed) WiFiManagerParameter(
//    "calibrate_rot_speed", // Unique ID with only alphanumeric characters
//    "max. Rotationsgeschwindigkeit kalibrieren", // Display label
//    "0",                      // Default value (0 for unchecked, 1 for checked)
//    10,                       // Field length (not strictly needed for checkboxes)
////    "type=\"checkbox\""       // HTML attribute to render as a checkbox
//    "type=\"checkbox\" style=\"display: block; margin-bottom: 15px;\""
//);

// Create the WiFiManagerParameter for limit switch calibration
// Create the WiFiManagerParameter for limit switch calibration
//new (&calibrate_limitswitch) WiFiManagerParameter(
//    "calibrate_limit_switch", // Unique ID with only alphanumeric characters
//    "Limitswitch kalibrieren", // Display label
//    "0",                      // Default value (0 for unchecked, 1 for checked)
//    10,                       // Field length (not strictly needed for checkboxes)
////    "type=\"checkbox\""       // HTML attribute to render as a checkbox
//    "type=\"checkbox\" style=\"display: block; margin-bottom: 15px;\""
//);

//new (&forbid_global_watsch_alliance) WiFiManagerParameter(
//    "global_watsch_allianz", // Unique ID with only alphanumeric characters
//    "globale Watschn-Allianz deaktivieren", // Display label
//    "0",                      // Default value (0 for unchecked, 1 for checked)
//    10,                       // Field length (not strictly needed for checkboxes)
////    "type=\"checkbox\""       // HTML attribute to render as a checkbox
//    "type=\"checkbox\" style=\"display: block; margin-bottom: 15px;\""
//);

//new (&forbid_mesh_remote) WiFiManagerParameter(
//    "allow_mesh_remote", // Unique ID with only alphanumeric characters
//    "Mesh-Fernbedienung verbieten", // Display label
//    "0",                      // Default value (0 for unchecked, 1 for checked)
//    10,                       // Field length (not strictly needed for checkboxes)
////    "type=\"checkbox\""       // HTML attribute to render as a checkbox
//    "type=\"checkbox\" style=\"display: block; margin-bottom: 15px;\""
//);

new (&sofort_loswatschen) WiFiManagerParameter(
    "sofortloswatschen", // Unique ID with only alphanumeric characters
    "sofort nach Speichern loswatschen", // Display label
    "0",                      // Default value (0 for unchecked, 1 for checked)
    10,                       // Field length (not strictly needed for checkboxes)
//    "type=\"checkbox\""       // HTML attribute to render as a checkbox
    "type=\"checkbox\" style=\"display: block; margin-bottom: 15px;\""
);



//// MQTT parameters
//new (&custom_mqtt_server) WiFiManagerParameter(
//    "mqtt_server",                // Unique ID
//    "MQTT Server für globale Watschn-Allianz",                // Display label
//    "broker.hivemq.com",          // Default value
//    40                            // Length of the input field
//);


// local MQTT parameters
new (&custom_mqtt_server_local) WiFiManagerParameter(
    "mqtt_server_local",               // Unique ID
    "lokaler MQTT Server", // Display label
    mqttServerLocal,                  // Default value from the mqttServer variable
    60                           // Length of the input field
);

new (&custom_mqtt_user_local) WiFiManagerParameter(
    "mqtt_user_name_local",              // Unique ID
    "local MQTT username",              // Display label
    mqttUserLocal,                           // Default value (empty for no default password)
    40                            // Length of the input field
);

new (&custom_mqtt_password_local) WiFiManagerParameter(
    "mqtt_password_local",              // Unique ID
    "lokales MQTT Password",              // Display label
    mqttPassLocal,                           // Default value (empty for no default password)
    40,                            // Length of the input field
    "type=\"password\""           // HTML attribute to hide password input
);

// Define custom MQTT server port parameter
new (&custom_mqtt_port_local) WiFiManagerParameter(
    "mqtt_port_local",                 // Unique ID
    "lokaler MQTT Port",                 // Display label
    mqttPortLocal_txt,                      // Default value (standard MQTT port)
    5                            // Length of the input field (5 is enough for "1883")
);


// global MQTT parameters Watschnallianz:
new (&custom_mqtt_server) WiFiManagerParameter(
    "mqtt_server",               // Unique ID
    "MQTT Server für globale Watschn-Allianz", // Display label
    mqttServer,                  // Default value from the mqttServer variable
    60                           // Length of the input field
);

new (&custom_mqtt_user) WiFiManagerParameter(
    "mqtt_user_name",              // Unique ID
    "MQTT user für globale Watschn-Allianz",              // Display label
    mqttUser,                           // Default value (empty for no default password)
    40                            // Length of the input field
);

new (&custom_mqtt_password) WiFiManagerParameter(
    "mqtt_password",              // Unique ID
    "MQTT Password für globale Watschn-Allianz",              // Display label
    mqttPass,                           // Default value (empty for no default password)
    40,                            // Length of the input field
    "type=\"password\""           // HTML attribute to hide password input
);

// Define custom MQTT server port parameter
new (&custom_mqtt_port) WiFiManagerParameter(
    "mqtt_port",                 // Unique ID
    "MQTT Port für globale Watschn-Allianz",                 // Display label
    mqttPort_txt,                      // Default value (standard MQTT port)
    5                            // Length of the input field (5 is enough for "1883")
);

  // test custom html(radio)
//  const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three<br>";
//  new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
//  
  wm.addParameter(&custom_mqtt_server_local);
  wm.addParameter(&custom_mqtt_user_local);
  wm.addParameter(&custom_mqtt_password_local);
  wm.addParameter(&custom_mqtt_port_local); 
  wm.addParameter(&forbid_global_watsch_alliance);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_password);
  wm.addParameter(&custom_mqtt_port); 
  wm.addParameter(&calibrate_rotationspeed);
  wm.addParameter(&calibrate_limitswitch);
  wm.addParameter(&forbid_mesh_remote);
  wm.addParameter(&sofort_loswatschen);
  

//------------------------------------------------------------------------
    
    //wm.addParameter(&custom_mqtt_server);
    wm.setConfigPortalBlocking(true);
    wm.setSaveParamsCallback(saveParamsCallback);

    //------------------

  // set dark theme
  wm.setClass("invert");
//------------------------------------------------------------------------
    //------------------

        // Get the ESP8266 Chip ID
          uint32_t chipID = ESP.getChipId();
        
          // Create a String to hold the portal name
          String portalName = "WatschnFM (ChipID " + String(chipID) + ")";
        
          // Convert String to char array for startConfigPortal
          char portalNameChar[portalName.length() + 1];
          portalName.toCharArray(portalNameChar, portalName.length() + 1);

          
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    //if(wm.autoConnect("WatschnFM")){
    
    
    
    //if (wm.autoConnect("WatschnFM", "")) {
    if (wm.autoConnect(portalNameChar, "")) {
      
        // Print the SSID of the connected WiFi network to Serial Monitor
        Serial.print(F("Connected to WiFi Network: "));
        Serial.println(WiFi.SSID());  // Print the SSID of the currently connected WiFi network
        Serial.print(F("...using static wifi settings from known_wifis.txt or manually set wifi using WifiManager. Double-reset/double-power-OFF to reset wifi settings from WifManager."));
        // Display status and SSID on OLED
        display.clear();
        display.setFont(ArialMT_Plain_10);  // Set a smaller font for better fit
        display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        
        // Display a message indicating connection
        display.drawString(display.getWidth() / 2, 10, "Connected to:");
        
        // Display the SSID
        String ssid = WiFi.SSID();
        if (ssid.length() > 16) {
            ssid = ssid.substring(0, 16) + "...";  // Truncate SSID if it's too long
        }
        display.drawString(display.getWidth() / 2, 30, ssid);  // Position adjusted for clarity
    
        // Refresh display
        display.display();
        
        // Delay to keep the message visible
        nowtime = millis();
        if (nowtime - inittime < 2000) {
            delay(2000 - (nowtime - inittime));
        }
    }
    else {
        wm.setTimeout(0);  // Disable timeout, AP stays up indefinitely
        
//        wm.startConfigPortal("WatschnFM","");

       // Start the WiFiManager configuration portal with the dynamic name
          wm.startConfigPortal(portalNameChar, "");

  
        Serial.println(F("Configportal running"));
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(display.getWidth() / 2,8, "! CONFIG NEEDED !");
        display.drawString(display.getWidth() / 2,20, "Access point opened.");
        display.drawString(display.getWidth() / 2,32, "Please connect to");
        display.drawString(display.getWidth() / 2,44, "WiFi 'WatschnFM (x)'");
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
  Serial.println(F("Ready"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

//        Serial.print(F("WiFiManager-Checkbox 'Günter kalibrieren': "));
//        Serial.println(calibrate_rotationspeed.getValue());
//        Serial.print(F("WiFiManager-Checkbox 'Limitswitch kalibrieren': "));
//        Serial.println(calibrate_limitswitch.getValue());


timeClient.begin();
  delay ( 1000 );
  if (timeClient.update()){
     //Serial.println( "Lokale Uhrzeit Einstellen inkl. Sommer/Winterzeit" );
     unsigned long epoch = timeClient.getEpochTime();
     setTime(epoch);
     Serial.println(F("timeserver: setup done."));
  }else{
     Serial.print ( "NTP Update (Zeitserver) ist fehlgeschlagen - ist evtl. noch kein Wifi verbunden/konfiguriert?" );
     delay(100);
  }
  

//setup_ESPMesh();
  
Serial.println(F("Setup finished, entering loop"));
loopStartTime = micros();  // Initialize loop start time
}

void looptime_calculation(){
  // Calculate the time taken by the previous loop
  unsigned long currentTime = micros();
  unsigned long loopTime = currentTime - loopStartTime;
  
  // Update the loop start time for the next iteration
  loopStartTime = currentTime;

  // Accumulate total loop time and count the loops
  totalLoopTime += loopTime;
  loopCount++;

  // Check if this loop time is the maximum encountered so far
  if (loopTime > maxLoopTime) {
    maxLoopTime = loopTime;
  }

  // Check if it's time to report the loop statistics
  if (currentTime - lastReportTime >= reportInterval * 1000) { // Convert reportInterval to microseconds
    reportLoopTimeStats();
    // Reset for the next reporting period
    totalLoopTime = 0;
    loopCount = 0;
    maxLoopTime = 0;
    lastReportTime = currentTime;
  }
}

void reportLoopTimeStats() {
  if (loopCount == 0) return; // Avoid division by zero

  // Calculate mean loop time in milliseconds
  unsigned long meanLoopTime = totalLoopTime / loopCount / 1000; // Convert to milliseconds

  // Output the statistics to the serial monitor
  Serial.print(F("Mean Loop Time: "));
  Serial.print(meanLoopTime);
  Serial.println(F(" ms"));

  // Convert max loop time to milliseconds and output
  unsigned long maxLoopTimeMs = maxLoopTime / 1000;
  Serial.print(F("Max Loop Time: "));
  Serial.print(maxLoopTimeMs);
  Serial.println(F(" ms"));
}

void printElapsedTime(const char* label) {
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - checkpointTime;
  float elapsedTimeMs = elapsedTime / 1000.0; // Convert to milliseconds

  Serial.print(label);
  Serial.print(F(": "));
  Serial.print(elapsedTime);
  Serial.print(F(" microseconds (= "));
  Serial.print(elapsedTimeMs, 3);  // Print milliseconds with 3 decimal places
  Serial.println(F(" ms)"));
  
  checkpointTime = currentTime;  // Update the checkpoint time
}


//void setup_ESPMesh(){
//    //functions for mesh remote:
//  Serial.println(F("setting up ESP mesh..."));
//    meshManager.setup();
//    
//    // Define callback functions
//    auto watschnFunction = []() {
//        Serial.println(F("Watschn function called via mesh remote!"));
//        // Add your code for watschn here
//    };
//    
//    auto schellnFunction = []() {
//        Serial.println(F("Schelln function called via mesh remote!"));
//        // Add your code for schelln here
//    };
//    
//    // Set callback functions
//    meshManager.setWatschnCallback(watschnFunction);
//    meshManager.setSchellnCallback(schellnFunction);
//    Serial.println(F(" done!"));
//}

// Define a function to move the stepper motor forward or backward by given steps
void move_stepper(int steps) {
  Serial.print(F("function move_stepper(int steps) was called. got steps: "));
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
  Serial.print(F("DBG 1: direction="));
  Serial.println(direction);
  Serial.print(F("current steps: "));
  Serial.println(current_steps);
//  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//  display.setFont(ArialMT_Plain_10);
//  display.drawString(0, 0, "Stepcount: " + current_steps);
//  display.display();
  //delay(1000);
  init_stepper();
  // Calculate the target position in steps as relative movement:
  target_position = current_steps + steps;
  Serial.print(F("Target position initial: "));
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
  Serial.print(F("Target position corrected: "));
  Serial.println(target_position);
//  //target_position *= STEPS_PER_REV / STEPS_PER_REV;
  Serial.print(F("Target position: "));
  Serial.println(target_position);
  // Determine the direction to move the stepper motor
//  int direction = steps > 0 ? 1 : -1;
//  Serial.print("DBG 1: direction=");
//  Serial.println(direction);

stepper.setSpeed(120);


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
      Serial.print(F("DBG 3: current steps="));
      Serial.println(current_steps);
//      display.drawString(0, 50, "Stepcount: " + current_steps);
//      display.display();
    }
    else{
    // Move the stepper motor one step in the desired direction
    stepper.step(direction);
    // Update the current steps of the stepper motor
    current_steps += direction;
    Serial.print(F("DBG 3: current steps="));
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
  Serial.println(F("funktion jetzt gibts auf die 12 - noch nicht implementiert"));
}

void jetzt_schlägts_13() {
  // do something at 13:00
  Serial.println(F("funktion jetzt schlägts 13 - noch nicht implementiert"));
  //z.B.:
  //multiwatschn(13);
}

void free_stepper(){
  Serial.println(F("freeing Stepper"));
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);

}

void free_stepper_high(){
  Serial.println(F("freeing Stepper"));
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, HIGH);

}

void init_stepper(){
  //Serial.println(F("Initializing Stepper"));
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
    Serial.println(F("searching for init position..."));
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



void schwungholen_und_watschen(bool retransmit_message){ //default value of bool is true, given by separate function declaration , means calling without parameter schwungholen_und_watschen(); will retransmit the message to the Bockfotzn-Network
  if(retransmit_message){
  publishToMQTT("watschn", mqttBockfotznTopic);
  }
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
  ausfallschritt_servo.write(zurück);
  delay(500);
  init_stepper();
  int umdrehungen=1;
  while(stepper_maxspeed<250){
    int leds_schwung = map(stepper_maxspeed, 80, 250, 0, 100);
    target_leds(leds_schwung);
  for(int i=0; i<umdrehungen;i++){
    ESP.wdtFeed();
    stepper.step(-15);
    stepper_maxspeed=stepper_maxspeed+1;
    if(stepper_maxspeed>245){
      ausfallschritt_servo.write(vorwärts);
    }
    if(stepper_maxspeed>250){
      stepper_maxspeed=250;
      //free_stepper();
      //free_stepper_high();
      //delay(4000);
      stepper_maxspeed=80;
      ausfallschritt_servo.write(vorwärts);
      delay(1500);
      return;
       
    }
    stepper.setSpeed(stepper_maxspeed);
  }
  }
  stepper.step(-80);
  
  ausfallschritt_servo.write(vorwärts);
  delay(150);
  //free_stepper();
  stepper_maxspeed=120;
  stepper.setSpeed(stepper_maxspeed);
  //multiwatschn(3);

  
//  delay(2000);
//  stepper.step(watschweite);
//  delay(100);
//  stepper.step(-watschweite);
//  delay(100);
//
//  //free_stepper();
//  stepper.step(watschweite);
//  delay(100);
//  stepper.step(-watschweite);
//  delay(100);
//
//  stepper.step(watschweite);
//  delay(100);
//  stepper.step(-watschweite);
//  delay(100);
//
//  stepper.step(watschweite);
//  delay(100);
//  stepper.step(-watschweite);
//  delay(100);

  free_stepper();


  
  stepper_maxspeed=80;
  stepper.setSpeed(30);

  //free_stepper();
  ausfallschritt_servo.write(zurück);
  stepper.step(-10);
  stepper_maxspeed=80;
  stepper.setSpeed(stepper_maxspeed);
  //Statt line 668:
  delay(100);

  stepper.step(-5);
  free_stepper();
  for(int i=100; i>=0; i--){
   target_leds(i);
   delay(2);
  }
  
  
  //goto_init_position_idle();

  
  //ausfallschritt_servo.write(vorwärts);
//    delay(500);
//    ausfallschritt_servo.write(zurück);
//    delay(500);
//    goto_init_position_idle();
//    delay(300);
    //multiwatschn(5);
  
provocation_counter=0;
  
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
  provocation_counter=0;
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
    //also count if that happens more than once. If so, Günter will get angry and start hitting.
      
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
          Serial.println(F("searching for init position..."));
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

void multiwatschn(int haudrauf, bool retransmit_message){ //default value bool retransmit_message = true is given in separate function definition
  if(retransmit_message){
    String message = String(haudrauf); // Convert the int to a String
    //Serial.println(F("DBG:  post amount of multiwatschn to mqttMultiwatschnTopic if the message did not come in through MQTT already"));
    publishToMQTT(message, mqttMultiwatschnTopic);
  }
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
  //stepper.setSpeed(40); //debug: go slow
  delay(300); //DEBUG: nötig? reduzieren?
  init_stepper();
  //stepper.setSpeed(140);
  int ausholen_ausfallschritt=14; // nach vollem Ausfallschritt (braucht ca 200ms) braucht es 14 steps um "Hand anzulegen" aber gerade noch nicht zuzuwatschen
  int ausholen_watschn=20; //wie viel "Vorspannung" durch Körperdrehung soll die Watschn bekommen
  int watschüberschneidung=4; //wie weit die Watschn den Scheitel umlegt
  ausfallschritt_servo.write(vorwärts);
  move_stepper(-ausholen_ausfallschritt);
  
  delay(200); //zeit bis der Servo Ausfallschrittposition erreicht (kann ggfs. noch optimiert werden bzw. muss im idealfall ohnehin als nonblocking delay geändert werden)
  int test=ausholen_ausfallschritt+watschüberschneidung+ausholen_watschn; //combine for smooth movement
//  move_stepper(ausholen_ausfallschritt);
//  move_stepper(watschüberschneidung);

  delay(80);
  //move_stepper(ausholen_watschn);
    move_stepper(test);
  for(int i=0;i<haudrauf;i++){
    //move_stepper(ausholen_watschn);
    delay(50);//debug: go slow
    int vollwatschweite=ausholen_watschn+watschweite;
    move_stepper(-vollwatschweite);
    delay(100);//debug: go slow
    move_stepper(vollwatschweite);
    //find_positon_backward_null();
  }
//  move_stepper(-ausholen_watschn);
//  delay(300);
//  move_stepper(-watschüberschneidung);
  int test2=watschüberschneidung+ausholen_watschn;
  //delay(300);
  ausfallschritt_servo.write(zurück);
  //delay(300);
  stepper.setSpeed(20);
  move_stepper(-test2);
  //ausfallschritt_servo.write(zurück);
  delay(200);
  ausfallschritt_servo.detach();
  stepper.setSpeed(stepper_maxspeed);
  free_stepper();
  provocation_counter=0;
}



void multiwatschn2(int haudrauf, int sollwatschweite){
//  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
//  ausfallschritt_servo.write(vorwärts);
  init_stepper();
  for(int i=0;i<haudrauf;i++){
  Serial.println(F("start -sollwatschweite"));
  move_stepper(-sollwatschweite);
  Serial.println(F("finish -sollwatschweite"));
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

void multiwatschn3(int haudrauf){
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
  //delay(300); //DEBUG: nötig? reduzieren?
  init_stepper();
  int ausholen_ausfallschritt=14; // nach vollem Ausfallschritt (braucht ca 200ms) braucht es 14 steps um "Hand anzulegen" aber gerade noch nicht zuzuwatschen
  int ausholen_watschn=20; //wie viel "Vorspannung" durch Körperdrehung soll die Watschn bekommen
  int watschüberschneidung=8; //wie weit die Watschn den Scheitel umlegt
  ausfallschritt_servo.write(vorwärts);
  delay(200);
  move_stepper(ausholen_watschn);
  delay(1000);
  move_stepper(-ausholen_watschn);
  delay(1000);
  move_stepper(ausholen_watschn);
  
//  delay(250); //zeit bis der Servo Ausfallschrittposition erreicht (kann ggfs. noch optimiert werden bzw. muss im idealfall ohnehin als nonblocking delay geändert werden)
//  move_stepper(test);
//  delay(400);
//  move_stepper(ausholen_watschn);
//  delay(500);
//  move_stepper(-ausholen_watschn);
//  delay(400);
//  move_stepper(ausholen_watschn);
//  delay(500);
//  move_stepper(-ausholen_watschn);
//  delay(400);
//  move_stepper(ausholen_watschn);
//  delay(500);
//  move_stepper(-ausholen_watschn);
  
//  int test=ausholen_ausfallschritt-watschüberschneidung+watschweite; //combine for smooth movement
////  move_stepper(ausholen_ausfallschritt);
////  move_stepper(watschüberschneidung);
//
//  //move_stepper(ausholen_watschn);
//    move_stepper(watschweite);
//    delay(200);
//    move_stepper(-watschweite);
//    delay(1000);
//    int test3=watschweite+watschüberschneidung;
//    move_stepper(test3);
//    
////  for(int i=0;i<haudrauf;i++){
////    //move_stepper(ausholen_watschn);
////    delay(500);//debug: go slow
////    int vollwatschweite=ausholen_watschn+watschweite;
////    move_stepper(-vollwatschweite);
////    delay(200);//debug: go slow
////    move_stepper(vollwatschweite);
////    //find_positon_backward_null();
////  }
////  move_stepper(-ausholen_watschn);
////  delay(300);
////  move_stepper(-watschüberschneidung);
//    move_stepper(test);
   // delay(1000);
  ausfallschritt_servo.write(zurück);
  delay(200);
  //ausfallschritt_servo.detach();
  free_stepper();
  provocation_counter=0;
}

void ausfallschritt_und_schelln_andeuten(bool retransmit_message){ //ready: Günter macht einen schritt vorwärts und haut gerade noch nicht zu . //default value bool retransmit_message = true is defined in separate function definition
  if(retransmit_message){
    publishToMQTT("schelln andeuten", mqttBockfotznTopic);
  }
  Serial.println(F("DBG: Anfang funktion ausfallschritt_und_schelln_andeuten()"));
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
 // delay(300); //DEBUG: nötig? reduzieren?
  init_stepper();
  int ausholen_ausfallschritt=14;
  ausfallschritt_servo.write(vorwärts);
  move_stepper(-ausholen_ausfallschritt);
  delay(200);
  move_stepper(ausholen_ausfallschritt);
  ausfallschritt_servo.write(zurück);
  delay(200);
  ausfallschritt_servo.detach();
  free_stepper();
  Serial.println(F("DBG: Ende funktion ausfallschritt_und_schelln_andeuten()"));
  provocation_counter=0;
}

void ausfallschritt_und_schelln(bool retransmit_message){ //ready: Günter macht einen schritt vorwärts und haut zu  //default value bool retransmit_message = true given in separate function definition
  if(retransmit_message){
    publishToMQTT("schelln", mqttBockfotznTopic);
  }
  Serial.println(F("DBG: Anfang funktion ausfallschritt_und_schelln()"));
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
 // delay(300); //DEBUG: nötig? reduzieren?
  init_stepper();
  int ausholen_ausfallschritt=40;
  ausfallschritt_servo.write(vorwärts);
  move_stepper(-ausholen_ausfallschritt);
  delay(200);
  move_stepper(ausholen_ausfallschritt);
  ausfallschritt_servo.write(zurück);
  delay(200);
  ausfallschritt_servo.detach();
  free_stepper();
  Serial.println(F("DBG: Ende funktion ausfallschritt_und_schelln()"));
  provocation_counter=0;
}

void massier_den_kürbis_soft(bool retransmit_message){ //ready: Günter macht einen schritt vorwärts und Rüttelwatscht sanft die Rübe . //default value bool retransmit_message = true given in separate function definition
  if(retransmit_message){
    publishToMQTT("kuerbis_soft", mqttBockfotznTopic);
  }
  Serial.println(F("DBG: Anfang funktion massier_den_kürbis_soft()"));
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
 // delay(300); //DEBUG: nötig? reduzieren?
  init_stepper();
  stepper.setSpeed(80);
  int ausholen_ausfallschritt=65;
  ausfallschritt_servo.write(vorwärts);
  delay(250);
  move_stepper(-ausholen_ausfallschritt);
  
  stepper.setSpeed(30);
  int schädelmassierweite=8;
  for(int i=0; i<20; i++){
    move_stepper(-schädelmassierweite);
    //stepper.step(-schädelmassierweite);
    delay(32);
    move_stepper(schädelmassierweite);
    //stepper.step(schädelmassierweite);
    delay(32);
  }
  move_stepper(ausholen_ausfallschritt);
  ausfallschritt_servo.write(zurück);
  delay(200);
  ausfallschritt_servo.detach();
  stepper.setSpeed(stepper_maxspeed);
  free_stepper();
  Serial.println(F("DBG: Ende funktion massier_den_kürbis_soft()"));
  provocation_counter=0;
}

void massier_den_kürbis_bis_er_mürb_is(bool retransmit_message){ //ready: Günter macht einen schritt vorwärts und Rüttelwatscht intensiv die Rübe . //default value bool retransmit_message = true given in separate function definition
  if(retransmit_message){
    publishToMQTT("kuerbis_hard", mqttBockfotznTopic);
  }
  Serial.println(F("DBG: Anfang funktion massier_den_kürbis_bis_er_mürb_is()"));
  ausfallschritt_servo.attach(SERVO_PIN);  // attaches the servo on GIO2 to the servo object
 // delay(300); //DEBUG: nötig? reduzieren?
  init_stepper();
  stepper.setSpeed(80);
  int ausholen_ausfallschritt=25;
  ausfallschritt_servo.write(vorwärts);
  delay(250);
  move_stepper(-ausholen_ausfallschritt);
  delay(100);
  stepper.setSpeed(40);
  int schädelmassierweite=8;
  for(int i=0; i<20; i++){
    //move_stepper(-schädelmassierweite);
    stepper.step(-schädelmassierweite);
    delay(20);
    //move_stepper(schädelmassierweite);
    stepper.step(schädelmassierweite);
    delay(20);
  }
  move_stepper(ausholen_ausfallschritt);
  ausfallschritt_servo.write(zurück);
  delay(200);
  ausfallschritt_servo.detach();
  stepper.setSpeed(stepper_maxspeed);
  free_stepper();
  Serial.println(F("DBG: Ende funktion massier_den_kürbis_bis_er_mürb_is()"));
  provocation_counter=0;
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
  Serial.println(F("starte Wutaufbau..."));
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
  Serial.println(F("Wutaufbau beendet, starte jetzt dreifachen Watschberger mittels multiwatschn()"));
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
  Serial.println(F("Scanning for I2C devices..."));
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
      Serial.print(F("I2C device found at address 0x"));
      if (address<16)
         Serial.print(F("0"));
      Serial.print(address,HEX);
      Serial.println(F("  !"));
      nDevices++;
     }
     else if (error==4)
     {
      Serial.println(F("If you get a lot of devices next to the following error, there might be an error in hardware wiring for I2C..."));
      Serial.print(F("Unknown error at address 0x"));
      if (address<16){
         Serial.print(F("0"));
      }
      Serial.println(address,HEX);
     }
    }
    if (nDevices == 0){
       Serial.println(F("No I2C devices found\n"));
    }
    else
    {
       Serial.println(F("done\n"));
    }
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
  if ( digitalRead(LIMIT_SWITCH_PIN) == LOW) { //using that pin just for debugging, not in productive version!

      wm.resetSettings();
//      delay(100);
//      ESP.restart();
//      delay(5000);

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    // put your setup code here, to run once:
   // Serial.begin(115200);
    
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
        Serial.println(F("Configportal running"));
    }
    else {
        Serial.println(F("connected to WiFi..."));
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
    if(target_steps==1){
      Serial.println(F("schwungholen_und_watschen() requested via serial"));
      schwungholen_und_watschen();
      
    }
    else if(target_steps==2){
      Serial.println(F("get_aggressive(3) requested via serial"));
      get_aggressive(3);
    }
    else if(target_steps==3){
      Serial.println(F("massier_den_kürbis_bis_er_mürb_is() requested via serial"));
      //ausfallschritt_und_schelln_andeuten();
      //delay(200);
      massier_den_kürbis_bis_er_mürb_is();
      
      
    }
    else if(target_steps==4){
      Serial.println(F("ausfallschritt_und_schelln() requested via serial"));
      ausfallschritt_und_schelln();
      
      
    }
    else if(target_steps<=6&&target_steps>0){
      Serial.println(F("multiwatschn(target_steps) requested via serial"));
      multiwatschn(target_steps);
      
    }
    
    
    else{
    // Move the stepper motor to the target steps
    Serial.print(F("manual request to move stepper via Serial..."));
    move_stepper(target_steps);
    
    // Output the current steps of the stepper motor
    Serial.print(F("Current steps: "));
    Serial.println(current_steps * STEPS_PER_REV / STEPS_PER_REV);
    }

  }
}


void show_time_to_oled(){
      nowtime=millis();
    if(nowtime-NTPlastDisplayed>1000){

        String timestamp = getEpochStringByParams(CE.toLocal(now()));    //convert to local time and format timestamp string
  int hour = timestamp.substring(11, 13).toInt();    //extract hour from timestamp string
  int minute = timestamp.substring(14, 16).toInt();  //extract minute from timestamp string
  int seconds = timestamp.substring(17, 19).toInt(); //extract seconds from timestamp string
  
//  Serial.print("Hour: ");
//  Serial.println(hour);
//  Serial.print("Minute: ");
//  Serial.println(minute);
//  Serial.print("Seconds: ");
//  Serial.println(seconds);



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

void check_for_clock_triggered_actions(){
      //ACHTUNG: das ist noch ohne Zeitoffset sommer/winter!! passt also noch nicht:
    if (timeClient.getHours() == 12 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0) {
      auf_die_12();
    }

    if (timeClient.getHours() == 13 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0) {
      jetzt_schlägts_13();
    }
//    if (timeClient.getHours() == 00 && timeClient.getMinutes() == 55 && timeClient.getSeconds() == 0) {
//      Serial.println(F("################ test 00:43h . #####################"));
//    }
}

void loop() {
    //meshManager.loop(); //ESP mesh
    wm.process(); // Keep the config portal alive
    //handle_blue_LED();  //commented because servo uses same pin as LED - only uncomment it for debug purposes
    //check_for_wifimanager_request(); //ATTENTION: function is currently triggered by limit switch - only use for debugging, if you dont find another trigger for it.
    listen_to_serial();
    show_time_to_oled();
    goto_init_position_idle2();
    get_aggressive_when_provocated(3);
    ausfallschritt_servo.detach();
    



//functions for GitHub OTA-update:
  //wifiManager.process();
  drd.loop(); //double reset detector
  //  OFF FOR DEBUG
//  if (WiFi.status() == WL_CONNECTED) { //if connected to wifi
//    ArduinoOTA.handle();
//    check_for_clock_triggered_actions();
//
//    //to do: decide between local and global here
//    if (!mqttClient.connected()) {
//      if (!mqtt_config_hasbeenread) { //if we have not yet read mqtt config, try to read it from config file
//        setup_mqtt_config();
//      }
//      nowtime = millis();
//      if (nowtime - lastReconnectAttempt > 5000) {
//        lastReconnectAttempt = millis();
//        // Attempt to reconnect
//        //Serial.println(F("DEBUG: reconnecting MQTT, retrying every 5 seconds"));
//        if (reconnect()) {
//          lastReconnectAttempt = 0;
//        }
//      }
//    } else {
//      // Client connected
//      mqttClient.loop();
//      //Serial.println(F("DEBUG: looping MQTT"));
//    }
//  }
    if (WiFi.status() == WL_CONNECTED) { // If connected to WiFi
      ArduinoOTA.handle(); // Handle OTA updates
      check_for_clock_triggered_actions(); // Perform any scheduled actions
  
      // Handle MQTT connections
      #ifdef USE_LOCAL_MQTT
      if (!mqttClientLocal.connected()) { // If local MQTT client is not connected
        if (!mqtt_config_hasbeenread) { // If MQTT config hasn't been read
          setup_mqtt_config(); // Read MQTT configuration
        }
        unsigned long nowtime = millis(); // Get the current time
        if (nowtime - lastReconnectAttempt > 5000) { // Check if 5 seconds have passed
          lastReconnectAttempt = nowtime; // Update the last reconnect attempt time
          if (reconnect_local()) { // Attempt to reconnect to local MQTT
            lastReconnectAttempt = 0; // Reset the reconnect attempt timer if successful
          }
        }
      } else { // Local MQTT client is connected
        mqttClientLocal.loop(); // Process incoming messages for local MQTT client
        //Serial.println(F("DEBUG:looping mqttClient for local MQTT"));
      }
      #endif
  
      #ifdef USE_GLOBAL_MQTT
      if (!mqttClient.connected()) { // If global MQTT client is not connected
        if (!mqtt_config_hasbeenread) { // If MQTT config hasn't been read
          setup_mqtt_config(); // Read MQTT configuration
        }
        unsigned long nowtime = millis(); // Get the current time
        if (nowtime - lastReconnectAttempt > 5000) { // Check if 5 seconds have passed
          lastReconnectAttempt = nowtime; // Update the last reconnect attempt time
          if (reconnect_global()) { // Attempt to reconnect to global MQTT
            lastReconnectAttempt = 0; // Reset the reconnect attempt timer if successful
          }
        }
      } else { // Global MQTT client is connected
        mqttClient.loop(); // Process incoming messages for global MQTT client
        //Serial.println(F("DEBUG:looping mqttClient for global MQTT"));
      }
      #endif
    } else {
      // Optionally handle WiFi disconnection here if needed
    }

}


void saveParamsCallback() {
    Serial.println(F("Get Parameters from WiFiManager configuration:"));

    // Retrieve new values from WiFiManager
    const char* tempMqttServerLocal = custom_mqtt_server_local.getValue();
    const char* tempMqttUserLocal = custom_mqtt_user_local.getValue();
    const char* tempMqttPassLocal = custom_mqtt_password_local.getValue();
    const char* tempMqttPortLocal = custom_mqtt_port_local.getValue();

    // Update the global variables
    mqttServerLocal = strdup(tempMqttServerLocal);
    mqttUserLocal = strdup(tempMqttUserLocal);
    mqttPassLocal = strdup(tempMqttPassLocal);
    mqttPortLocal_txt = strdup(tempMqttPortLocal);

    // Print values
    Serial.print(F("MQTT Server lokal: "));
    Serial.println(mqttServerLocal);
    Serial.print(F("MQTT user lokal: "));
    Serial.println(mqttUserLocal);
    Serial.print(F("MQTT passwort lokal: "));
    Serial.println(mqttPassLocal);
    Serial.print(F("MQTT port lokal: "));
    Serial.println(mqttPortLocal_txt);

    // Convert mqttPortLocal_txt to integer
    mqttPortLocal = atoi(mqttPortLocal_txt);

          // Print mesh remote parameter
  Serial.print(F("Checkbox sofort loswatschen: "));
  String str_loswatschen = sofort_loswatschen.getValue();
  //Serial.println(limitSwitchValue);  // Print raw value
if (str_loswatschen == "0") {
    Serial.println(F("Checked"));
    Serial.println(F("Watsche jetzt drauf los"));
    schwungholen_und_watschen(0);
  } else {
    Serial.println(F("Unchecked"));
  }

    // Save updated configuration to file
    writeConfigFile();
}

void writeConfigFile() {
    Serial.println(F("trying to write new config from WiFiManager to mqtt_config.txt in SPIFFS..."));

    // Prepare the updated values in a map
    std::map<String, String> configMap;
    configMap["mqttClientId"] = mqttClientId;
    configMap["mqttDevicenameTopic"] = mqttDevicenameTopic;
    configMap["mqttupdateTopic"] = mqttupdateTopic;
    configMap["mqttBockfotznTopic"] = mqttBockfotznTopic;
    configMap["mqttGetAggressiveTopic"] = mqttGetAggressiveTopic;
    configMap["mqttMoveStepperTopic"] = mqttMoveStepperTopic;
    configMap["mqttMultiwatschnTopic"] = mqttMultiwatschnTopic;
    configMap["mqttonlineTopic"] = mqttonlineTopic;
    configMap["mqttfirmwareTopic"] = mqttfirmwareTopic;
    configMap["mqttWillTopic"] = mqttWillTopic;
    configMap["mqttServerLocal"] = mqttServerLocal;
    configMap["mqttPortLocal_txt"] = mqttPortLocal_txt;
    configMap["mqttUserLocal"] = mqttUserLocal;
    configMap["mqttPassLocal"] = mqttPassLocal;
    configMap["mqttServer"] = mqttServer;
    configMap["mqttPort_txt"] = mqttPort_txt;
    configMap["mqttUser"] = mqttUser;
    configMap["mqttPass"] = mqttPass;
    configMap["mqttBockfotznTopic"] = mqttBockfotznTopic;
    configMap["mqttGetAggressiveTopic"] = mqttGetAggressiveTopic;
    configMap["mqttMoveStepperTopic"] = mqttMoveStepperTopic;
    configMap["mqttMultiwatschnTopic"] = mqttMultiwatschnTopic;

    // Open the config file for writing
    File configFile = SPIFFS.open("/mqtt_config.txt", "w");
    if (!configFile) {
        Serial.println(F("Failed to open config file for writing"));
        return;
    }

    // Write the updated values to the file
    for (const auto& pair : configMap) {
        Serial.println("Writing: " + pair.first + "=" + pair.second); // Debug output
        configFile.println(pair.first + "=" + pair.second);
    }
    configFile.flush();
    configFile.close();

    Serial.println(F("Configuration saved."));
}





//// Call this function to send a mesh message on demand
//void sendMessageToMesh(const String &message) {
//    meshManager.sendMeshMessage(message);
//}
