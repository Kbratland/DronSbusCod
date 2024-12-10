#include <WiFiNINA.h>                    //https://github.com/arduino-libraries/WiFiNINA/tree/master
#include <WiFiUDP.h>                     
#include <MSP.h>                         //https://github.com/yajo10/MSP-Arduino

MSP msp;
char ssid[] = "XV_Basestation";          //  network SSID (name)
int status = WL_IDLE_STATUS;             // the Wi-Fi radio's status
int ledState = LOW;                      //ledState used to set the LED
unsigned long previousMillisInfo = 0;    //will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;     // will store the last time LED was updated
const int intervalInfo = 5000;           // interval at which to update the board information
char packetBuffer[256];                  //buffer to hold incoming packet
WiFiUDP Udp;
unsigned int localPort = 2390;
char  ReplyBuffer[] = "Drone 1";
int wifiState = 0;                       //Wifi connection state
bool firstConnectFrame = false;          //First Loop while connected to wifi
double pytime = 0;                        //time of python code
IPAddress bsip;

double pitch; // pitch is broken?
double roll;
double yaw;
double throttle;
int arming;
int updateTime = 0;
int connectTime = 0;
long t = 0;
bool isArmed = false;
bool isFailsafed = false;
bool isKilled = false;
long blinkTime = 0;
int droneState = -1;  
int killswitch = 1000;
int failsafe = 1000;
int LEDpin = 13;
int blinkSpeed = 10;
bool lightOn = false;
bool bootComplete = false;               //Finished Drone Booting sequence
bool enabled = false;

struct ManualControlMessage{
  IPAddress sourceIP;
  String cmd;
  double yaw;
  double pitch;
  double roll;
  double throttle;
  double killswitch;
  double pytime;
};

struct BSIPMessage{
  String cmd;
  IPAddress BSIP;
};

void setup()
{
  Serial.begin(115200);
  msp.begin(Serial);
  if(Serial)
  {
    serialUSB = true;
    Serial.println("Setup");
  }
  delay(1000);
  msp_set_raw_rc_t rcT;

  delay(1000);
  // set the LED as output
  pinMode(LED_BUILTIN, OUTPUT);
  sbus_tx.Begin();
  roll = 1500;
  pitch = 1500;
  yaw = 1500;
  throttle = 885;
  WifiConnection();
}//end setup

void loop()
{
  msp_rc_t rcR;
  msp_set_raw_rc_t rcT;
  if (msp.request(MSP_RC, &rcR, sizeof(rcR))) {
  	
    uint16_t inputRoll     = rcR.channelValue[0];
    uint16_t inputPitch    = rcR.channelValue[1];
    uint16_t inputYaw      = rcR.channelValue[2];
    uint16_t inputThrottle = rcR.channelValue[3]; 
  }
  status = WiFi.status();
//   if(status != WL_CONNECTED)
//   {  
//     blinkSpeed = 50;
//     failsafe = 2000;
//   }
//   else
//   {
//     failsafe = 1000;
//   }
  //MillisStuff();
  WifiConnection();//checks the wi-fi status
  Listen();
  DroneSystems();
  rcT.channelValue[0] = roll;
  rcT.channelValue[1] = pitch; 
  rcT.channelValue[2] = throttle;
  rcT.channelValue[3] = yaw;
  rcT.channelValue[4] = arming;
  rcT.channelValue[5] = 1500;
  rcT.channelValue[6] = 1500;
  rcT.channelValue[7] = failsafe;
  rcT.channelValue[8] = 1500;
  rcT.channelValue[9] = 1500;
  rcT.channelValue[10] = 2000;
  rcT.channelValue[11] = 1500; //RSSI (Signal Strength)
  rcT.channelValue[12] = killswitch; //Killswitch
  rcT.channelValue[13] = 1500;
  rcT.channelValue[14] = 1500;
  rcT.channelValue[15] = 1500;
  msp.send(MSP_SET_RAW_RC, &rcT, sizeof(rcT));
  if (wifiState == 1 && droneState == 1 && status == WL_CONNECTED){
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write("State: 1 -> 2");
    Udp.endPacket();
    wifiState = 2;
  }
  else if (wifiState == 2){ 
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write("AmDrone");
    Udp.endPacket();
    wifiState = 3;
  }
  else if (wifiState == 4) {
    Udp.beginPacket(bsip, 5005);
    Udp.write("HND|-1|NEWDRONE");
    Udp.endPacket();
    wifiState = 5; 
  }
  if(millis() - updateTime > 5000 && serialUSB)
  {
    Serial.println("<--------------------------->\nDrone Data");
    Serial.print("Throttle: "); Serial.println(throttle);
    Serial.print("Pitch: "); Serial.println(pitch);
    Serial.print("Roll: "); Serial.println(roll);
    Serial.print("Yaw: "); Serial.println(yaw);
    Serial.print("Armed: "); Serial.println(isArmed);
    Serial.print("Failsafe: "); Serial.println(isFailsafed);
    Serial.print("Killswitch: "); Serial.println(isKilled);
    Serial.println("<--------------------------->");
    updateTime = millis();
  }
  // else if (state == 5) {
  //   //call parsemanualcontrolmessage and process the results
  //   //but do it in listen
  //   // Serial.println(roll);
  //   // Serial.println(yaw);
  //   // Serial.println(pitch);
  //   // Serial.println(throttle);
  // }
}//End Loop

void DroneSystems(){
  if (droneState == -1){//starting up
    blinkSpeed = 1000;       
    throttle = 885;
    Arm(false);     
    if(millis() - t > 1000){
      t = millis();
      droneState = 0;
    }
  }
  else if (droneState == 0){//arm drone
    blinkSpeed = 1000;
    //set arming signals
    Arm(true);
    if (millis() - t > 500){
      droneState = 1;
      t = millis();
    }
  }
  else if (droneState == 1) //nothing
  {
    //Roll Ch 0, pitch Ch 1, Yaw Ch 3, Throttle Ch 2, Arm Ch 4
    
  }
  else if(droneState == 2)
  {
    
    
  }
  if(status != WL_CONNECTED) 
  {
    if(millis()- blinkTime >= blinkSpeed){
      LightSRLatch();
      blinkTime = millis();
    }
  }
  else if(droneState == -1 || droneState == 0){ //Light Blinker
    if(millis()- blinkTime >= blinkSpeed){
      LightSRLatch();
      blinkTime = millis();
    }
  }
  else if(killswitch == 1700){
    digitalWrite(LEDpin,LOW);
  }
  else{
    digitalWrite(LEDpin,HIGH);
  }
  CheckModeStates();
  // delay(10);  
}//End Drone Systems

void Arm(bool armed)
{
  if(armed)
  {
    arming = 2000;
  }
  else if(!armed)
  {
    arming = 1000;
  }
}

void CheckModeStates(){ //sets booleans of the modes for enabled/disabled
  if(arming > 1500)
  {
    isArmed = true;
  }
  else
  {
    isArmed = false;
  }
  if(failsafe > 1500)
  {
    isFailsafed = true;
  }
  else
  {
    isFailsafed = false;
  }
  if(killswitch > 1500)
  {
    isKilled = true;
  }
  else
  {
    isKilled = false;
  }
}

int ChannelMath(double setpoint) //Math to set the same values as used in inav veiwer, < 0.0001% stray
{
  //channel limits assume setpoint between 900 - 2100
  setpoint = (setpoint - 879.7)/.6251;
  return round(setpoint);  
}

void LightSRLatch()
{
  if(lightOn)
  {
    digitalWrite(LEDpin, LOW);
    lightOn = false;    
  }
  else if(!lightOn)
  {
    digitalWrite(LEDpin, HIGH);
    lightOn = true;    
  }
}

void WifiConnection()
{
  // attempt to connect to Wi-Fi network:
  if(status != WL_CONNECTED && ((millis() - connectTime) > 5000)) {
    if(serialUSB)
    {
      Serial.print("Attempting to connect to network: ");
    }
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid);
    //Retry every 5 seconds
    firstConnectFrame = true;
    connectTime = millis();
  }
  else if(status == WL_CONNECTED && firstConnectFrame)
  {
    Udp.begin(localPort);
      // Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.beginPacket("192.168.4.22", 80);
    Udp.write("State: 0 -> 1 |Connected|");
    Udp.endPacket();
    wifiState = 1;
    
    Udp.beginPacket("192.168.4.22", 80);
    Udp.write(ReplyBuffer);
    Udp.endPacket();
    // you're connected now, so print out the data:
    if(serialUSB)
    {
      printBoardInfo();
    }
    firstConnectFrame = false;
    bootComplete = true;
  }
}

void SendMessage(char msg[]){
  Udp.begin(localPort);
  // Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.beginPacket("192.168.4.22", 80);
  Udp.write(msg);
  Udp.endPacket();
  Serial.println("Sent");
  Serial.println(msg);
  Serial.println("**********");
}

void printBoardInfo(){
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your network's SSID:
  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
  Serial.println("---------------------------------------");
  Serial.println("You're connected to the network");
  Serial.println("---------------------------------------");

}

void MillisStuff() { //specifies whatever this stuff is for use in the loop, to make the looop easier to read
  unsigned long currentMillisInfo = millis();

  // check if the time after the last update is bigger the interval
  if (currentMillisInfo - previousMillisInfo >= intervalInfo) {
    previousMillisInfo = currentMillisInfo;

  }
  
  unsigned long currentMillisLED = millis();
  
  // measure the signal strength and convert it into a time interval
  int intervalLED = WiFi.RSSI() * -10;
 
  // check if the time after the last blink is bigger the interval 
  if (currentMillisLED - previousMillisLED >= intervalLED) {
    previousMillisLED = currentMillisLED;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
  }
}

ManualControlMessage parseMessage(char buffer[]){
  ManualControlMessage msg;
  char *token;
  token = strtok(buffer, "|");
  int i = 0;
  while(token != 0){
    //Serial.println(token);
    switch(i){
      case 0:
        msg.cmd = token;
        break;
      case 1:
        msg.sourceIP = token;
        break;
      case 2:
        msg.yaw = atoi(token);       
        break;
      case 3:
        msg.pitch = atoi(token);  //pitch
        break;
      case 4: 
        msg.roll = atoi(token);
        break;
      case 5:
        msg.throttle = atoi(token);
        break;
      case 6:
        msg.killswitch = atoi(token);
        break;
      case 7:
        msg.pytime = atoi(token);
        break;
      }
      roll = msg.roll;
      pitch = msg.pitch;
      throttle = msg.throttle;
      yaw = msg.yaw;
      killswitch = msg.killswitch;
      pytime = msg.pytime;
    i++;
    token = strtok(NULL, "|"); 
  }
return msg;  
//HAS REQUIRED PACKETS FROM LISTEN, CODE FOR MANUAL MODE HERE --------------
//Currently does not include a break, repeats loop forever
}  

BSIPMessage parseBSIP(char buffer[]){
  BSIPMessage msg;
  char *token;
  token = strtok(buffer, "|");
  int i = 0;
  Serial.println("In parseBSIP");
  while(token != 0){
    Serial.println(token);
    switch(i){
      case 0:
        msg.cmd = token;
        break;
      case 1:
        msg.BSIP = token;
        break;        
      }
    i++;
    token = strtok(NULL, "|");
    
  }
  return msg;  
}  

void Listen(){
  int packetSize = Udp.parsePacket();
  if(packetSize){
    //Serial.print("Received packet of size ");
    //Serial.println(packetSize);
    //Serial.print("From ");
    IPAddress remoteIp = Udp.remoteIP();
    //Serial.print(remoteIp);
    //Serial.print(", port ");
    //Serial.println(Udp.remotePort());
    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, 255);
    //Serial.println(packetBuffer);
    if (len > 0) {
      packetBuffer[len] = 0;
      if (wifiState == 3){
        Serial.println("Parsing BSIP Message");
        //read BSID response from AP      
        BSIPMessage msg = parseBSIP(packetBuffer);
        if (msg.cmd == "BSIP"){
          bsip = msg.BSIP;
          Serial.print("Base Station IP: ");
          Serial.println(bsip);
          wifiState = 4;
        }
      }
      else if (wifiState == 5){
        //Serial.println("listening for manual message");
        ManualControlMessage msg = parseMessage(packetBuffer);
        if(serialUSB)
        {
          Serial.print("packet: ");
          Serial.print(msg.throttle);
          Serial.print(" recived at: ");
          Serial.print(millis());
        }
        if (msg.cmd == "MAN"){
          roll = msg.roll;
          pitch = msg.pitch;
          throttle = msg.throttle;
          yaw = msg.yaw;
          ();
        }
      }
    }    
  }
}
