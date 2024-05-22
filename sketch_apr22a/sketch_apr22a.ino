#include "sbus.h"
#include "iBus.h"

#define MAX_CHANNELS 18 // Max iBus Channels

/* SBUS object, writing SBUS */
bfs::SbusTx sbus_tx(&Serial1);
/* SBUS data */
bfs::SbusData data;
bfs::SbusData sbusInput;

iBus receiver(Serial1, MAX_CHANNELS);

// int channelValues[] = {1500, 1500, 1500, 885, 1500, 1500, 1500};
long telemetry[18] = {};
int pitch; // pitch is broken?
int roll;
int yaw;
int throttle;
int arming;
bool enabled = false;
long t = 0;
long blinkTime = 0;
int state = -1;  
int killswitch = 1000;
int failsafe = 1000;
int LEDpin = 13;
int compare = 1000;
bool lightOn = false;
int rampUpTime = 0;
/*
-1: startup
0: arming state
1: sending signals
2: test state
*/
void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  t = millis();  
  blinkTime = millis();
  /* Begin the SBUS communication */
  receiver.begin();
  sbus_rx.Begin();
  sbus_tx.Begin();

  roll = 1500;
  pitch = 1500;
  yaw = 1500;
  throttle = 880;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("setup has finished");
}

void loop() 
{
  receiver.process();
  if (state == -1){
    compare = 1000;       
    throttle = 885;
    killswitch = 1000;
    Arm(false);     
    if(millis() - t > 10000) //sending init data for 10 seconds
    {
      t = millis();
      state = 0; //it's time to arm
    }
  }
  else if (state == 0){
    compare = 100;
    //set arming signals
    Arm(true);
    if (millis() - t > 10000){
      rampUpTime = millis();
      throttle = 900;
      state = 2; // 1 is intstant, 2 is slow incraments of 50
      t = millis();
    }
  }
  else if (state == 1)
  {
    //Roll Ch 0, Pitch Ch 1, Yaw Ch 3, Throttle Ch 2, Arm Ch 4
    /*
    
    
    CONNOR PUT CONTROL HERE
    
    
    */
  } // end state 1
  else if(state == 2)
  {
    if((millis() - rampUpTime >= 1000) && (killswitch < 1500) && (throttle < 1150))
    {
      throttle += 50;
      rampUpTime = millis();
    }
    if((throttle >= 1150) && (millis() - rampUpTime >= 2000))
    {
      killswitch = 1700;
      failsafe = 2000;
    }
  }
  DataSetSend();
  PrintChannels();


  if(state == -1 || state == 0){ //Light Blinker
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
}


void Arm(bool armmed) // turns arm into a bool
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

double ChannelMath(int setpoint) // Sets Sbus data to proper point for conversion
{
  return((setpoint - 879.7)/.6251);
}

void LightSRLatch() // Blinker
{
  if(lightOn)
  {
    digitalWrite(LED_BUILTIN, LOW);
    lightOn = false;    
  }
  else if(!lightOn)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    lightOn = true;    
  }
}

void DataSetSend() // Sends Relivent SBUS channels
{
  data.ch[0] = (ChannelMath(roll));
  data.ch[1] = (ChannelMath(pitch));
  data.ch[2] = (ChannelMath(throttle));
  data.ch[3] = (ChannelMath(yaw));
  data.ch[4] = (ChannelMath(arming));
  data.ch[7] = (ChannelMath(failsafe)); //mutes beeper NO KONNER ITS FAILSAFE, BEEPERMUTE IS THE SAME AS ARM
  data.ch[10] = (ChannelMath(2000)); //Signal
  data.ch[12] = (ChannelMath(killswitch)); //Killswitch
  sbus_tx.data(data);
  sbus_tx.Write();
}

void PrintChannels()
{
  Serial.println("lost frame data: " +data.lost_frame +"/n");
  Serial.println("failsafe data: " +data.failsafe +"/n");
  Serial.println("Input");
  for(byte i = 1; i <= MAX_CHANNELS; i++){  // get channel values starting from 1
    Serial.println(i +": " +receiver.get(i) +"/n");
  }
  Serial.println("Output");
  for (int8_t i = 0; i < data.NUM_CH; i++){ //print data we're sending.
      Serial.print(i);
      Serial.print(": ");
      if(data.ch[i] != 0)
      {
        Serial.println((.6251*(data.ch[i]))+879.7);
      }
      else
      {
        Serial.println(data.ch[i]);
      }
  } // close printing for
  Serial.println("-----");
  Serial.print("State is: ");
  Serial.println(state +"/n" +"-----" +"/n");
}
