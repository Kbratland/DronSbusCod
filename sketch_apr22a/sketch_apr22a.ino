#include <WiFiNINA.h>
#include <WiFiUDP.h>
#include "sbus.h"
//please enter your sensitive data in the Secret tab
char ssid[] = "XV_Basestation";                //  network SSID (name)
int status = WL_IDLE_STATUS;             // the Wi-Fi radio's status
int ledState = LOW;                       //ledState used to set the LED
unsigned long previousMillisInfo = 0;     //will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;      // will store the last time LED was updated
const int intervalInfo = 5000;            // interval at which to update the board information
char packetBuffer[256]; //buffer to hold incoming packet
WiFiUDP Udp;
unsigned int localPort = 2390;
char  ReplyBuffer[] = "Drone 1";
int wifiState = 0;
bool bootComplete = false;
bool firstConnectFrame = false;
bool wifiLostConnection = false;
 
//KONERRRRRR
/* SBUS object, writing SBUS */
bfs::SbusTx sbus_tx(&Serial1);
/* SBUS data */
bfs::SbusData data;

double pitch; // pitch is broken?
double roll;
double yaw;
double throttle;
int arming;
int connectTime = 0;
bool enabled = false;
long t = 0;
long blinkTime = 0;
int droneState = -1;  
int killswitch = 1000;
int failsafe = 1000;
int LEDpin = 13;
int compare = 1000;
bool lightOn = false;
int rampUpTime = 0;
//KOOONNNEEER
IPAddress bsip; //holds the base station ip address

struct ManualControlMessage{
  IPAddress sourceIP;
  String cmd;
  double yaw;
  double pitch;
  double roll;
  double throttle;
  double killswitch;
};

struct BSIPMessage{
  String cmd;
  IPAddress BSIP;
  
};

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  Serial.println("Testing.");
  // set the LED as output
  pinMode(LED_BUILTIN, OUTPUT);
  sbus_tx.Begin();
  roll = 1500;
  pitch = 1500;
  yaw = 1500;
  throttle = 880;
  WifiConnection();
}//end setup

void loop() {
  if(status != WL_CONNECTED && bootComplete)
  {
    wifiLostConnection = true;
  }
  if(wifiLostConnection)
  {
    failsafe = 2000;
  }
  else
  {
    failsafe = 1000;
  }
  //MillisStuff();
  DroneSystems();
  Listen();
  DataSetSend();
  WifiConnection();
  if (wifiState == 1 && droneState == 1){
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
      //Serial.println("fewuifuweeee");
      Udp.beginPacket(bsip, 5005);
      Udp.write("HND|-1|NEWDRONE");
      Udp.endPacket();
      wifiState = 5; 
    }
//   else if (state == 5) {
//     //call parsemanualcontrolmessage and process the results
//     //but do it in listen
//     // Serial.println(roll);
//     // Serial.println(yaw);
//     // Serial.println(pitch);
//     // Serial.println(throttle);
//     }
}//End Loop

void DroneSystems(){
  if (droneState == -1){
    compare = 1000;       
    throttle = 885;
    Arm(false);     
    DataSetSend();
    if(millis() - t > 1000){ //sending init data for 10 seconds
      t = millis();
      droneState = 0; //it's time to arm
    }
  }
  else if (droneState == 0){
    compare = 100;
    //set arming signals
    Arm(true);
    DataSetSend();
    if (millis() - t > 500){
      rampUpTime = millis();
      //throttle = 900;
      droneState = 1; // 1 is intstant, 2 is slow incraments of 50
      t = millis();
    }
  }
  else if (droneState == 1)
  {
    //Roll Ch 0, pitch Ch 1, Yaw Ch 3, Throttle Ch 2, Arm Ch 4
    
  } 
  if(droneState == -1 || droneState == 0){ //Light Blinker
    if(millis()- blinkTime >= compare){
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
  delay(10);  
}//End Drone Systems

void Arm(bool armmed)
{
  if(armmed)
  {
    arming = 1800;
  }
  else if(!armmed)
  {
    arming = 1000;
  }
}

int ChannelMath(double setpoint)
{
  //channel limits assume setpoint between 1000 - 2000
  //throttle can go as low as 885
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
    Serial.print("Attempting to connect to network: ");
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
    printBoardInfo();
    firstConnectFrame = false;
    wifiLostConnection = false;
    bootComplete = true;
  }
}

void DataSetSend()
{
  data.ch[0] = (ChannelMath(roll));
  data.ch[1] = (ChannelMath(pitch)); 
  data.ch[2] = (ChannelMath(throttle));
  data.ch[3] = (ChannelMath(yaw));
  data.ch[4] = (ChannelMath(arming));
  data.ch[7] = (ChannelMath(failsafe));
  data.ch[10] = (ChannelMath(2000)); //RSSI (Signal Strenght)
  data.ch[12] = (ChannelMath(killswitch)); //Killswitch
  sbus_tx.data(data);
  sbus_tx.Write();
}

void sendMessage(char msg[]){
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
    while (token != 0){
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
        }
        roll = msg.roll;
        pitch = msg.pitch;
        throttle = msg.throttle;
        yaw = msg.yaw;
        killswitch = msg.killswitch;
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
  while (token != 0){
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

  if (packetSize) {

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
        if (msg.cmd == "MAN"){

          roll = msg.roll;
          pitch = msg.pitch;
          throttle = msg.throttle;
          yaw = msg.yaw;
          

        }

      }
    }    
  }
}
