#include<Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include <string_asukiaaa.h>
#include <ESP8266mDNS.h>

const long utcOffsetInSeconds = 7200; // 2 hour
const int relayPin = 14;
const byte DNS_PORT = 53;
//const char*  mqtt_server = "192.168.1.202";
// Timer1
int dayTrigger;
int hourTrigger;
int minuteTrigger;
int secondTrigger;
bool days[7] = {0, 0, 0, 0, 0, 0, 0};
bool days2[7] = {0, 0, 0, 0, 0, 0, 0};

String dayNames[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" , "Sunday"};
String startTimeStr = ((String(hourTrigger)) + ":"+ String(minuteTrigger));

// Timer2
int dayTrigger2;
int hourTrigger2;
int minuteTrigger2;
int secondTrigger2;

String startTimeStr2 = ((String(hourTrigger2)) + ":"+ String(minuteTrigger2));

//timelib variables

tmElements_t tinfo;
time_t  initialt;
uint32_t last = 0;
uint32_t lastmqtt = 0;
RTC_DS1307 rtc;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
DNSServer dnsServer; // Create the DNS server
// Pump1 Timer

int dTime1 = 7200000; // irrigation gap
int pumpactivated = 0;
int iTime1 = 6000; //irrigation time
int i = 0; //timer for remain countdown of Pump 1
unsigned int  ctr1 = 3;
unsigned long previousMillis = 0;  
unsigned long currentMillis = 0;
unsigned long lastMeasure = 0;    

// Pump2 Timer

int dTime2 = 7200000; // irrigation gap
int pump2activated = 0;
int iTime2 = 6000; //irrigation time
int i2 = 0; //timer for remain countdown of Pump 1
unsigned int  ctr2 = 3;
unsigned long previousMillis2 = 0;  
unsigned long currentMillis2 = 0;
unsigned long lastMeasure2 = 0;

// PowerMate001 auxiliar variables

int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
const char* mqtt = "*************";
String st;
String content;

WiFiClient PowerMate;
PubSubClient client(PowerMate);

//pubsub parameters
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

//Function Decalration
bool testWifi(void);
void remain(void);

void launchWeb(void);
void setupAP(void);
void initWiFi(void);
void pump_on(void);
void createWebServer(void);



//Establishing Local server at port 80 whenever required

ESP8266WebServer server(80); //set webserver port 80

 void writeArrayToEEPROM(int address) {
  for (int i = 0; i < 7; i++) {
    int value = days[i];
    byte lowByte = value & 0xFF;
    byte highByte = (value >> 8) & 0xFF;
    EEPROM.write(address + i*2, lowByte);
    EEPROM.write(address + i*2 + 1, highByte);
   }  
  }


void writeArray2ToEEPROM(int address) {
  for (int i = 0; i < 7; i++) {
    int value = days2[i];
    byte lowByte = value & 0xFF;
    byte highByte = (value >> 8) & 0xFF;
    EEPROM.write(address + i*2, lowByte);
    EEPROM.write(address + i*2 + 1, highByte);
   }  
  }

void readArrayFromEEPROM(int address) {
  for (int i = 0; i < 7; i++) {
    byte lowByte = EEPROM.read(address + i*2);
    byte highByte = EEPROM.read(address + i*2 + 1);
    days[i] = (highByte << 8) | lowByte;
  }
}

void readArray2FromEEPROM(int address) {
  for (int i = 0; i < 7; i++) {
    byte lowByte = EEPROM.read(address + i*2);
    byte highByte = EEPROM.read(address + i*2 + 1);
    days2[i] = (highByte << 8) | lowByte;
  }
}
void writeUnsignedIntIntoEEPROM(int address, unsigned int number)
{
  Serial.print("Writing reps to EEPROM");
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}
unsigned int readUnsignedIntFromEEPROM(int address)
{
  Serial.print("Reading reps from EEPROM");
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeLongIntoEEPROM(int address, long number)
{
  Serial.println("Write to EEPROM");
  delay(1000);
  EEPROM.write(address, (number >> 24) & 0xFF);
  EEPROM.write(address + 1, (number >> 16) & 0xFF);
  EEPROM.write(address + 2, (number >> 8) & 0xFF);
  EEPROM.write(address + 3, number & 0xFF);
  EEPROM.commit();
}
long readLongFromEEPROM(int address)
{
  Serial.println("Read from EEPROM");
  delay(1000);

  return ((long)EEPROM.read(address) << 24) +
         ((long)EEPROM.read(address + 1) << 16) +
         ((long)EEPROM.read(address + 2) << 8) +
         (long)EEPROM.read(address + 3);
}

void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}

String read_String(char add)
{
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}

void setUpOverTheAirProgramming() {

  // Change OTA port.
  // Default: 8266
  // ArduinoOTA.setPort(8266);

  // Change the name of how it is going to
  // show up in Arduino IDE.
  // Default: esp8266-[ChipID]
  ArduinoOTA.setHostname("PowerMate001");

  // Re-programming passowrd.
  // No password by default.
  // ArduinoOTA.setPassword("123");

  ArduinoOTA.begin();
}

void getSettings() {
  
  dTime1 = readLongFromEEPROM(96);
  Serial.print("dTime: ");
  Serial.println(String(dTime1));
  
  iTime1 = readLongFromEEPROM(100);
  Serial.print("iTime: ");
  Serial.println(String(iTime1));
  
  Serial.print("P1 Reps: ");
  ctr1 = readUnsignedIntFromEEPROM(104);
  Serial.println(String(ctr1));
  
  Serial.print("Hour Trigger: ");
  hourTrigger = EEPROM.read(106);
  Serial.println(String(hourTrigger));
  
  //Serial.print("Min Trigger: ");
  int minuteTrigger = EEPROM.read(108);
  Serial.println(String(minuteTrigger));
  startTimeStr = ((String(hourTrigger)) + ":"+ (String(minuteTrigger)));

  //pump 2 

  dTime2 = readLongFromEEPROM(110);
  Serial.print("dTime: ");
  Serial.println(String(dTime1));
  
  iTime2 = readLongFromEEPROM(114);
  Serial.print("iTime2: ");
  Serial.println(String(iTime2));
  
  Serial.print("P2 Reps: ");
  ctr2 = readUnsignedIntFromEEPROM(118);
  Serial.println(String(ctr2));
  
  Serial.print("Hour Trigger2: ");
  hourTrigger2 = EEPROM.read(120);
  Serial.println(String(hourTrigger2));
  
  Serial.print("MinTrigger2: ");
  minuteTrigger2 = EEPROM.read(122);
  Serial.println(String(minuteTrigger2));
  startTimeStr2 = ((String(hourTrigger2)) + ":"+ (String(minuteTrigger2)));
  
 for (i=0;i<7;i++){ //initialize arrays
     days[i] = 0;
     days2[i] = 0;
  }
  
  readArrayFromEEPROM(10); //READ ACTIVE Timer 1
  delay(300) ;  
  for (int n = 0;n<7;n++){
    Serial.print(String(days[n]));
  }
  Serial.println("");
  readArray2FromEEPROM(20); //READ ACTIVE DAYS P2
  delay(300);
  for (int o = 0;o<7;o++){
    Serial.print(String(days2[o]));
  }
}
void pump_on() {
  Serial.println("Timer turned on");
  client.publish("/PowerMate/T1", "Turned on");
  digitalWrite(relayPin, LOW);
  delay(iTime1);
  pumpactivated++;
  Serial.println("Timer 1 stop");
  digitalWrite(relayPin, HIGH);
  client.publish("/PowerMate/T1", "Turned off");
  lastMeasure = millis();
}
void turn_on(){ 
  client.publish("/PowerMate/T1", "Turned on");
  digitalWrite(relayPin, LOW);
} 
void turn_off(){ 
  client.publish("/PowerMate/T1", "Turned off");
  digitalWrite(relayPin, HIGH);
  }

void pump2_on() {
  Serial.println("Timer 2 turned on");
  client.publish("/PowerMate/T2", "Turned on");
  digitalWrite(relayPin, LOW);
  delay(iTime2);
  pump2activated++;
  Serial.println("Timer 2 stop");
  digitalWrite(relayPin, HIGH);
  client.publish("/PowerMate/T2", "Turned off");
  lastMeasure2 = millis();
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

time_t time_provider() {
  // Prototype if the function providing time information
  return initialt;
}

void launchWeb() {
  Serial.println("");
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
}
void onConnect(const WiFiEventStationModeConnected& event) { // connection event handler
  server.handleClient();
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);
  Serial.print("Status code:  ");
  Serial.println(WiFi.status());
  // Redirect to the captive portal webpage
  server.sendHeader("Location", "http://" + ip.toString() + "/info", true);
  server.send(302, "text/plain", ""); // Empty content, but with HTTP 302 status code
}
void setupAP(void) {
  
  WiFi.mode(WIFI_AP);
  delay(100);
  IPAddress ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gateway, subnet);
  delay(100);
  WiFi.softAP("PowerMate001", "");
  Serial.println("AP started");
  delay(100);
  WiFi.onStationModeConnected(onConnect); // register connection event 
}

void handleRoot() {
  // Function to handle the root page request
  
  // Get current time in milliseconds
  currentMillis = millis();
  currentMillis2 = millis();

  // Check if WiFi is connected and update time variables accordingly
  if (WiFi.status() == WL_CONNECTED) {
    tinfo.Hour = timeClient.getHours();
    tinfo.Minute = timeClient.getMinutes();
    tinfo.Second = timeClient.getSeconds();
  } else {
    rtc.now();
  }
  // Create a time object from the extracted time information
  initialt = makeTime(tinfo);
  
  // Convert minutes to two-digit format
  String padmin = String(tinfo.Minute);
  if (tinfo.Minute < 10) {
    padmin = string_asukiaaa::padStart(padmin, 2, '0');
  }
  String minfix = String(minuteTrigger); 
  if (minuteTrigger<10){
   minfix = string_asukiaaa::padStart(minfix, 2, '0'); 
  }
  String minfix2 = String(minuteTrigger2);
  if (minuteTrigger2 < 10){
    minfix2 = string_asukiaaa::padStart(minfix2, 2, '0');  
  }
  // Create strings for start times in Timer 1 and Timer 2
  String startTimeStr = ((String(hourTrigger)) + ":"+ minfix);
  String startTimeStr2 = ((String(hourTrigger2)) + ":"+ minfix2);
  // Create a string for the current time
  String timenow = (String(tinfo.Hour) + ":" + padmin);
  
  // Calculate remaining time for pump 1 and pump 2
  int a = (((previousMillis + dTime1) - currentMillis) / 60000);
  int b = (((previousMillis2 + dTime2) - currentMillis2) / 60000);

  // Calculate full cycle time for pump 1 and pump 2
  unsigned long fullcycle = ((dTime1 + iTime1) * ctr1);
  unsigned long fullcycle2 = ((dTime2 + iTime2) * ctr2);

  // Create strings for displaying remaining time and full cycle time for pump 1 and pump 2
  String remValue; 
  String remValue2;
  if (pumpactivated == 0) {
    remValue = String(dTime1/60000);
  } else {
    remValue = String(a);
  } 
  if (pump2activated == 0) { 
    remValue2 = String(dTime2/60000);
  } else { 
    remValue2 = String(b);
  }

  String fc = String(fullcycle / 3600000);
  String fc2 = String(fullcycle2 / 3600000);
  String ctr1tmp = String(ctr1);
  String ctr2tmp = String(ctr2);

  // Start building the HTML content
  content = "<!DOCTYPE html><html>";
  content += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  content += "<style>";
  content += "html, body { font-size: 18px; margin: 0; padding: 0; font-family: Arial, sans-serif; }";
  content += ".container { background-color: #f5f5f5; padding: 20px; line-height: 1; align-items:center; }";
  content += ".button-10 { display: inline-block; padding: 5px; font-size: 16px; font-weight: bold; text-align: center; border-radius: 6px; background-color: #4CAF50; color: white; text-decoration: none;  width: 80%;}";
  content += ".tbl {padding-left:5px; text-align:left;}";
  content += ".form {input[type=textinput[type=text]}, select, textarea{ width: 100%; padding: 12px; border: 1px solid #ccc; border-radius: 4px; }";
  content += ".button-column {display: flex;flex-direction: row;justify-content: center;gap: 10px;}";
  content += ".square-button { display: inline-block; width: 100px; height: 100px; line-height: 100px;text-align: center;border: 2px solid #000; text-decoration: none; font-size: 16px; font-weight: bold; color: #000; background-color: #fff; cursor: pointer; border-radius: 5px;}";
  content += "</style>";
  content += "</head>";
  content += "<body><div class=container>";
  content += "<center><table class=tbl><tr><td><strong>PowerMate Version: </strong></td><td>Beta 0.6</td></tr>";
  content += "<tr><td><strong>Time Now: </strong></td><td>" + timenow + "</td></tr>";

  // Add IP address information to the table
  String lip = WiFi.softAPIP().toString().c_str();
  content += "<tr><td><strong>IP ADD: </strong></td><td>" + lip + "</td></tr>";
  content += "</table></center><br>";

  // Add Refresh button to the center
  content += "<center><button id='button2' class='button-10' onclick=\"window.location.href='/'\">Refresh info</button></center>";
  content += "<div class=container><center><table class=tbl>";

  // Add Timer 1 information to the table
  content += "<tr><td><strong>Timer 1</strong></td><td><strong>Info</strong></td></tr>";
  content += "<tr><td>Interval Remaining  :</td><td>" + remValue + " mins</td></tr>";
  content += "<tr><td>Full Cycle time : </td><td> " + fc + " hours</td></tr>";
  content += "<tr><td>Active time:</td><td>" + String(iTime1/1000) + " sec</td></tr>";
  content += "<tr><td>Activations per cycle :</td><td>" + String(ctr1) + " times</td></tr>";
  content += "<tr><td>Times activated :</td><td>" + String(pumpactivated) + " times</td></tr>";
  content += "<tr><td>Active Days</td><td>"; 

  // Add the active days for Timer 1 to the table
  for (int i = 0; i < 7; i++) {
    if (days[i] == 1) {
      content += " " + dayNames[i] + ", ";
    }
  }
  content += "</td></tr>";
  content += "<tr><td>Start Time : </td><td>" + startTimeStr + "</td></tr>";
  
  // Timer 2 
  content += "</table></center></br>";
  content += "<center><table class=tbl>";
  content += "<tr><td><strong>Timer 2</strong></td><td><strong>Info</strong></td></tr>";
  content += "<tr><td>Interval Remaining  :</td><td>" + remValue2 + " mins</td></tr>";
  content += "<tr><td>Full Cycle time : </td><td> " + fc2 + " hours</td></tr>";
  content += "<tr><td>Active time:</td><td>" + String(iTime2/1000) + " sec</td></tr>";
  content += "<tr><td>Activations per cycle :</td><td>" + String(ctr2) + " times</td></tr>";
  content += "<tr><td>Times activated :</td><td>" + String(pump2activated) + " times</td></tr>";
  content += "<tr><td>Active Days</td><td>"; 

  // Add the active days for Timer 2 to the table
  for (int j = 0; j < 7; j++) {
    if (days2[j] == 1) {
      content += " " + dayNames[j] + ", ";
    }
  }
  content += "</td></tr>";
  content += "<tr><td>Start Time : </td><td>" + startTimeStr2 + "</td></tr>";
  content += "</table></div>";

  // Add buttons to the bottom
  content += "</BR><div class='button-column'>";
  content += "<center><button id='button2' class='button-10' onclick=\"window.location.href='/setclock'\">Set Clock</button></center>";
  content +="</BR><center><button id ='infobutton' class='button-10' onclick=\"window.location='/info'\">Set Start DAY / TIME</a></center></div>";
  content += "</BR><center><button id='toggleButton' class='button-10' onclick='toggleButton()'>Turn On</button></center></div>";

  // Add JavaScript for button functionality
  content += "<script>";
  content += "function toggleButton() {";
  content += "    var button = document.getElementById('toggleButton');";
  content += "    if (button.innerHTML === 'Turn On') {";
  content += "        button.innerHTML = 'Turn Off';";
  content += "        button.classList.add('button-pressed');"; // Add pressed state class
  content += "        turn_on();"; // Call turn_on function
  content += "    } else {";
  content += "        button.innerHTML = 'Turn On';";
  content += "        button.classList.remove('button-pressed');"; // Add pressed state class
  content += "        turn_off();"; // Call turn_off function
  content += "    }";
  content += "}";
  content += "function turn_on() {";
  content += "    var xhr = new XMLHttpRequest();";
  content += "    xhr.open('GET', '/turn_on', true);"; // URL for the turn_on function
  content += "    xhr.send();";
  content += "}";
  content += "function turn_off() {";
  content += "    var xhr = new XMLHttpRequest();";
  content += "    xhr.open('GET', '/turn_off', true);"; // URL for the turn_off function
  content += "    xhr.send();";
  content += "}";
  content += "</script>";

  // Closing tags
  content += "</div></body></html>";

  // Set headers and send the response
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  // Here begins chunked transfer
  server.send(200, "text/html", "");
  server.sendContent(content); // send content
}
void checkClient() { // check if a client has connected
  if (WiFi.softAPgetStationNum() > 0) {
    delay(100);
    Serial.print("Number of Clients connected: ");
    Serial.println(WiFi.softAPgetStationNum());
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/info", false);
    server.send(302, "text/plain", "");    
    server.client().stop();
    delay(100);
  }
}
void boot(void) {
  server.send(200, "text/html", content);
  ESP.restart();
}
  void remain() {
    String html;
    html = "<!DOCTYPE html><html>";
    html += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "html, body { font-size: 18px; margin: 0; padding: 0; font-family: Arial, sans-serif; }";
    html += ".container { background-color: #f5f5f5; padding: 20px; }";
    html += ".button-10 { display: inline-block; padding: 12px 20px; font-size: 16px; font-weight: bold; text-align: center; border-radius: 6px; background-color: #4CAF50; color: white; text-decoration: none; margin: 10px; }";
    html += ".tbl { width: 100%; margin: 20px 0; }";
    html += ".tbl td { padding: 8px; text-align: left; }";
    html += ".tbl strong { color: #333; }";
    html += ".header { background-color: #4CAF50; padding: 20px; text-align: center; color: white; font-size: 24px; }";
    html += ".content { padding: 20px; width:80%;}";
    html += ".form-field { width: 70px; }";
    html += ".large-checkbox { width: 25px; height: 25px; }";
    html += "</style></head>";
    html += "<body>";
    html += "<div class=\"header\">Timer Setup</div>";
    html += "<div class=\"container\">";
    html += "<div class=\"content\">";
    html += "<form action='/config' method='post' class='form'>";
    html += "<table class=\"tbl\">";
    html += "<tr><td>Select Activation Days :</td></tr><td></td></tr>";
    html += "<tr><td>Monday</td><td><input type='checkbox' class='large-checkbox' name='day' value='0'></td></tr>";
    html += "<tr><td>Tuesday</td><td><input type='checkbox' class='large-checkbox' name='day' value='1'></td></tr>";
    html += "<tr><td>Wednesday</td><td><input type='checkbox' class='large-checkbox' name='day' value='2'></td></tr>";
    html += "<tr><td>Thursday</td><td><input type='checkbox' class='large-checkbox' name='day' value='3'></td></tr>";
    html += "<tr><td>Friday</td><td><input type='checkbox' class='large-checkbox' name='day' value='4'></td></tr>";
    html += "<tr><td>Saturday</td><td><input type='checkbox' class='large-checkbox' name='day' value='5'></td></tr>";
    html += "<tr><td>Sunday</td><td><input type='checkbox' class='large-checkbox'  name='day' value='6'></td></tr>";
    html += "<tr><td>Start Time:</td><td> <input type='time' name='start' class='form-field'></td></tr>";
    html += "<tr><td><label>Active duration: </label></td><td><input type='number' name='iTime1' min='0' size='2' class='form-field'></td></tr>";
    html += "<tr><td><label>Enter time between activations in mins: </label></td><td><input name='dTime1' type='number' min='0' class='form-field'>";
    html += "<tr><td><label>Enter number of activations : </label></td><td><input name='ctr1' type='number' min='0' class='form-field'></table>";  
    // Pump 2  
    html += "<div class=\"header\">Timer 2 Setup</div>";
    html += "<table class='tbl'>"; 
    html += "<tr><td>Select Days to turn on :</td></tr>";
    html += "<tr><td>Monday</td><td><input type='checkbox' name='day2' value='0' class='large-checkbox'></td></tr>";
    html += "<tr><td>Tuesday</td><td><input type='checkbox' name='day2' value='1' class='large-checkbox'></td></tr>";
    html += "<tr><td>Wednesday</td><td><input type='checkbox' name='day2' value='2' class='large-checkbox'></td></tr>";
    html += "<tr><td>Thursday</td><td><input type='checkbox' name='day2' value='3' class='large-checkbox'></td></tr>";
    html += "<tr><td>Friday</td><td><input type='checkbox' name='day2' value='4' class='large-checkbox'></td></tr>";
    html += "<tr><td>Saturday</td><td><input type='checkbox' name='day2' value='5' class='large-checkbox'></td></tr>";
    html += "<tr><td>Sunday</td><td><input type='checkbox' name='day2' value='6' class='large-checkbox'></td></tr>";
    html += "<tr><td>" "</td><td>" "</td></tr>";
    html += "<tr><td>Start Time: </td><td><input type='time' name='start2' class='form-field'></td></tr>";
    html += "<tr><td><label>Enter activation time in sec: </label></td><td><input type='number' name='iTime2' min='0' size='2' class='form-field'></td></tr>";
    html += "<tr><td><label>Enter time between activations in mins: </label></td><td><input name='dTime2' type='number' min='0' class='form-field'>";
    html += "<tr><td><label>Enter number of activations : </label></td><td><input name='ctr2' type='number' min='0' class='form-field'>";
    html += "</table>";      
    html += "<center><input type='submit' value='Submit' class='button-10'></center>";
    html += "</form></div>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  }

void handleRequest(){
  Serial.println("handle Request");
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString());
  server.send(302, "text/plain", "");
}
void handleConfig() {
//Timer1 
int INThour=0;
int INTminute=0;
//Timer2 
int INThour2=0;
int INTminute2=0;

for (int l = 0; l < 7; l++) { //initiallise both Day and Day2 Arrays to 0
  days[l] = 0;
  days2[l] = 0;
}

for (int i = 0; i < server.args(); i++) {
 if (String(server.argName(i)) == "day") {
  int day = server.arg(i).toInt();
  days[day] = 1;
 }
if (String(server.argName(i)) == "day2") {
  int day2 = server.arg(i).toInt();
  days2[day2] = 1;
 }
 if (server.argName(i) == "start") { 
  INThour = server.arg(i).substring(0, 2).toInt();
  INTminute = server.arg(i).substring(3, 5).toInt();
 }
 if (server.argName(i) == "start2") { 
  INThour2 = server.arg(i).substring(0, 2).toInt();
  INTminute2 = server.arg(i).substring(3, 5).toInt();
 } 
}
  String iTime1tmp = server.arg("iTime1");
  String dTime1tmp = server.arg("dTime1");
  String ctr1tmp = server.arg("ctr1"); 
  dTime1 = (dTime1tmp.toInt() * 60000);
  iTime1 = (iTime1tmp.toInt() * 1000);
  ctr1 = ctr1tmp.toInt();
//pump2
  String iTime2tmp = server.arg("iTime2");
  String dTime2tmp = server.arg("dTime2");
  String ctr2tmp = server.arg("ctr2"); 
  dTime2 = (dTime2tmp.toInt() * 60000);
  iTime2 = (iTime2tmp.toInt() * 1000);
  ctr2 = ctr2tmp.toInt();

//pump1 
writeArrayToEEPROM(10);  // save the days of Pump1 to EEPROM
writeLongIntoEEPROM(96, dTime1); // save delay time to EEPROM
writeLongIntoEEPROM(100, iTime1); // save Irrigation time to EEPROM
writeUnsignedIntIntoEEPROM(104, ctr1); // save reps pump1  to EEPROM
EEPROM.write(106, INThour); // save hour portion of trigger time  to EEPROM
EEPROM.write(108, INTminute);  // save minute portion of trigger time  to EEPROM
//pump2 
writeArray2ToEEPROM(20);  // save the days of Pump2 to EEPROM 
writeLongIntoEEPROM(110, dTime2);// save delay time of Pump2 to EEPROM
writeLongIntoEEPROM(114, iTime2); // save Irrigation of Pump2 time to EEPROM
writeUnsignedIntIntoEEPROM(118, ctr2); // save reps pump1  to EEPROM
EEPROM.write(120, INThour2); // save hour portion of trigger time  to EEPROM
EEPROM.write(122, INTminute2);
EEPROM.commit();
String content;
content = "<!DOCTYPE html><html>";
content += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
content += "<meta http-equiv=\"refresh\" content=\"1;url=/\" />";
content += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; text-decoration: none; font-size: 14px; margin: 5px; cursor: pointer;}";
content += ".container {border-radius: 5px; background-color: #53917e; padding: 20px; height: 100%;}";
content += ".message { font-size: 18px; color: white; }";
content += "</style></head><body>";  
content += "<div class=\"container\">";
content += "<center><p class=\"message\">Redirecting...</p></center>";
content += "</div></body></html>";
server.send(200, "text/html", content);
pumpactivated = 0;//reset activation flag
pump2activated = 0;//reset activation flag

for (int k=0;k<7;k++){
  Serial.println(String(days[k]));
  Serial.println(String(days2[k]));
}
}
void handleNotFound() {
  // Redirect all requests to the captive portal
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString());
  server.send(302, "text/plain", "");
}
void createWebServer() {
  
    server.onNotFound(handleNotFound);
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on ("/info", remain );
    server.on("/turn_on", turn_on);
    server.on("/turn_off", turn_off);
    server.on("/boot", boot);
    server.on("/setclock", []() {
      String html;
      html = "<!DOCTYPE html><html>";
      html += "<html><head><style>.form-row {display: flex;justify-content: start;gap: 20px;}.form-row .form-group {display: flex;flex-direction: column;}.header { background-color: #4CAF50; padding: 20px; text-align: center; color: white; font-size: 24px; }.button-10 { display: inline-block; padding: 5px; font-size: 16px; font-weight: bold; text-align: center; border-radius: 6px; background-color: #4CAF50; color: white; text-decoration: none;  width: 80%;}</style></head><body><div class=\"header\">Clock Setup</div><div>";
          

      html += "<form method='post' action='/setRTC'>";
      html += "<fieldset><legend>Time</legend>";
      html += "<div class='form-row'>";
      html += "<div class='form-group'><label for='hour'>Hour:</label> <input id='hour' type='number' name='hour' min='0' max='23'></div>";
      html += "<div class='form-group'><label for='minute'>Minute:</label> <input id='minute' type='number' name='minute' min='0' max='59'></div>";
      html += "<div class='form-group'><label for='second'>Second:</label> <input id='second' type='number' name='second' min='0' max='59'></div>";
      html += "</div></fieldset>";
      html += "<fieldset><legend>Date</legend>";
      html += "<div class='form-row'>";
      html += "<div class='form-group'><label for='month'>Month:</label> <input id='month' type='number' name='month' min='1' max='12'></div>";
      html += "<div class='form-group'><label for='day'>Day:</label> <input id='day' type='number' name='day' min='1' max='31'></div>";
      html += "<div class='form-group'><label for='year'>Year:</label> <input id='year' type='number' name='year' min='2023' max='2100'></div>";
      html += "</div></fieldset>";
      html += "<div class='form-group'><center><input type='submit' class='button-10' value='Set Time'></center></div>";
      html += "</form></div></body></html>";
      server.send(200, "text/html", html);
    });

    server.on("/setRTC", []() {
       if (server.hasArg("hour") && server.hasArg("minute") && server.hasArg("second")&& server.hasArg("month")&& server.hasArg("day")&& server.hasArg("year")) {
          int hour = server.arg("hour").toInt();
          int minute = server.arg("minute").toInt();
          int second = server.arg("second").toInt();
          int month = server.arg("month").toInt();
          int day = server.arg("day").toInt();
          int year = server.arg("year").toInt();
          if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
            server.send(400, "text/plain", "Invalid time values. Hour should be 0-23, minute and second should be 0-59.");
            return;
          }
        
         // Validate date values
         if (month < 1 || month > 12 || day < 1 || day > 31 || year < 2023 || year > 2100) {
            server.send(400, "text/plain", "Invalid date values. Month should be 1-12, day should be 1-31, year should be 2023-2100.");
            return;
          }
          rtc.adjust(DateTime(year, month, day, hour, minute, second));
          String html = "<html><body><h1>Clock Time Set</h1>";
          html += "The clock time has been set to: " + String(hour) + ":" + String(minute) + ":" + String(second);
          html += "<br><br><a href='/'>Return to Home</a>";
          html += "</body></html>";
          server.send(200, "text/html", html);
          } 
          else {
              server.send(400, "text/plain", "Missing required arguments. Please provide hour, minute, second, month, day, and year.");
              }
        });
    
  };

void DNSSetup() {
  IPAddress apIP(192, 168, 4, 1);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
}
void setup() {
  Serial.flush();
  Serial.begin(9600);
  Serial.print("Starting up .. ");
  rtc.begin();
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  EEPROM.begin(512); //Initialasing EEPROM
  Serial.println("Reading EEPROM ssid");  
  getSettings(); // Get settings from EEPROM
  setupAP();// Setup HotSpot
  DNSSetup();
  launchWeb();
  setUpOverTheAirProgramming(); 
}

void loop() {
dnsServer.processNextRequest();
hourTrigger = EEPROM.read(106);
minuteTrigger = EEPROM.read(108);
hourTrigger2 = EEPROM.read(120);
minuteTrigger2 = EEPROM.read(122);

DateTime now = rtc.now(); // Get the current time
time_t current_time = now.unixtime(); // Convert to unix time
struct tm* local_time = localtime(&current_time); 
unsigned long currentTime = now.unixtime();
int currentDay = ((currentTime / 86400L + 3) % 7); //get the current day of the week

if (millis() - last > 1000) //Update time every second
   {
    // Keep track of the time we last sent data to serial monitor
    last = millis();
    // Get the current time
    tinfo.Hour = local_time->tm_hour;
    tinfo.Minute = local_time->tm_min;
    tinfo.Second = local_time->tm_sec;    
    tinfo.Day = local_time->tm_mday;
    tinfo.Month = local_time->tm_mon + 1;
    initialt = time_t(&tinfo);
    time_t setTime(initialt);
   }
Serial.print("Current Date: ");
Serial.print(tinfo.Day);
Serial.print("/");
Serial.println(tinfo.Month);
Serial.print("Current Time: ");
Serial.print(tinfo.Hour);
Serial.print(":");
Serial.print(tinfo.Minute);
Serial.print(":");
Serial.println(tinfo.Second);
Serial.print("Current Day: ");
Serial.println(currentDay);
Serial.print("T1 Trigger Day: ");
Serial.println(days[currentDay]);
Serial.print("T2 Trigger Day: "	);
Serial.println(days2[currentDay]);
Serial.print("Today is :");
Serial.println(dayNames[currentDay]);
  
  unsigned long currentMillis = millis(); // get the current time
  unsigned long currentMillis2 = millis(); // get the current time
  
  if ((days[currentDay] == 1)  && (tinfo.Hour == hourTrigger) && (tinfo.Minute == minuteTrigger) && (tinfo.Second < 2)){
       pump_on();          
       previousMillis = currentMillis;      // flag the trigger time   
      }
  
  if ((pumpactivated > 0) && (currentMillis - previousMillis >= dTime1) && (pumpactivated < ctr1)) {
    // if the current time is greater than or equal to the start time of the first interval and the desired interval time has passed and the counter is greater than 0
    // execute the first interval code
    pump_on();   
    previousMillis = currentMillis; 
    if (pumpactivated == ctr1  ){
      pumpactivated = 0;
    } 
     }
//pump 2 

  if ((days2[currentDay] == 1)  && (tinfo.Hour == hourTrigger2) && (tinfo.Minute == minuteTrigger2) && (tinfo.Second < 2)){
       pump2_on();          
       previousMillis2 = currentMillis2;      // set the previoustime   
      }
  
 if ((pump2activated > 0) && (currentMillis2 - previousMillis2 >= dTime2) && (pump2activated < ctr2)) {
    // if the current time is greater than or equal to the start time of the first interval and the desired interval time has passed and the counter is greater than 0
    // execute the first interval code
    pump2_on();   
    previousMillis2 = currentMillis2; 
    if (pump2activated == ctr2  ){
      pump2activated = 0;
    } 
    }
  

  Serial.print("Timer 1 Days: ");
  for (int k=0;k<7;k++){
  Serial.print(String(days[k]));
  }
  Serial.println("");
  Serial.print("Timer 2 Days: ");
  for (int l=0;l<7;l++){
  Serial.print(String(days2[l]));
  }
  Serial.println("");
  server.handleClient();
  ArduinoOTA.handle();
  checkClient();
  delay(1000);
} // end of loop
