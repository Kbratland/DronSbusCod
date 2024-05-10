#include "sbus.h"

bfs::SbusRx sbus_rx(&Serial1);
/* SBUS object, writing SBUS */
bfs::SbusTx sbus_tx(&Serial1);
/* SBUS data */
bfs::SbusData data;
bfs::SbusData sbusInput;
// int channelValues[] = {1500, 1500, 1500, 885, 1500, 1500, 1500};
int tman; // pitch is broken?
int roll;
int yaw;
int throttle;
int arming;
bool failsafe = false;
bool enabled = false;
long t = 0;
int state = -1;  
int kill = 1000;
/*
0: arming state
1: sending signals
*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) {}
  t = millis();  
  /* Begin the SBUS communication */
  sbus_rx.Begin();
  sbus_tx.Begin();
  roll = 1500;
  tman = 1500;
  yaw = 1500;
  throttle = 880;
}

void loop() {
    // roll = channelValues[1];
    // pitch = channelValues[2];
    // yaw = channelValues[3];
    // throttle = channelValues[4];
    if (state == -1){
      throttle = 885;
      Arm(false);     
      data.ch[0] = (ChannelMath(roll));
      data.ch[1] = (ChannelMath(tman));
      data.ch[2] = (ChannelMath(throttle));
      data.ch[3] = (ChannelMath(yaw));
      data.ch[4] = (ChannelMath(arming));
      data.ch[7] = (ChannelMath(1881)); //mutes beeper
      data.ch[10] = (ChannelMath(2000));
      data.ch[12] = (ChannelMath(1000));
      if(millis() - t > 10000) //sending init data for 10 seconds
      {
        t = millis();
        state = 0; //it's time to arm
      }
    }
    else if (state == 0){
      //set arming signals
      Arm(true);
      data.ch[2] = (ChannelMath(throttle));
      data.ch[4] = (ChannelMath(arming));
      data.ch[10] = (ChannelMath(1700));
      if (millis() - t > 10000){
        state = 1;
        t = millis();
      }
    }
    else if (state == 1){
      //Roll Ch 0, Pitch Ch 1, Yaw Ch 3, Throttle Ch 2, Arm Ch 4
      if(millis() - t < 5000)
      {
        roll = 1500;
        tman = 1500;
        yaw = 1500;
        throttle = 885;
        kill = 1000;
      }  
      else if(millis() - t < 15000)
      {    
        roll = 1500;
        tman = 1500;
        yaw = 1500;
        throttle = 1400;
        kill = 1000;
      }
      else{
        roll = 1500;
        tman = 1500;
        yaw = 1500;
        throttle = 885;  
        Arm(false);      
        kill = 1700;
      }
      
      data.ch[0] = (ChannelMath(roll));
      data.ch[1] = (ChannelMath(tman));
      data.ch[2] = (ChannelMath(throttle));
      data.ch[3] = (ChannelMath(yaw));
      data.ch[4] = (ChannelMath(arming));       
      data.ch[17] = ((988-879.7)/.6251);
      data.ch[16] = ((988-879.7)/.6251);
      data.ch[10] = (ChannelMath(2000));
      data.ch[12] = (ChannelMath(kill));
    } // end state 1
    // --- back to the base loop code ---  
    /* Grab the received data */
    sbusInput = sbus_rx.data();
    // data = sbus_rx.data(); THIS STUPID LINE OF CODE MADE ME SPEND LIKE 2.5 WEEKS CHASING GEESE WILDLY
    /* Display the received data */
    // Serial.print("\t Output \t");
    // for (int8_t i = 0; i < data.NUM_CH; i++) {
    //     data.ch[i] = ((1500-879.7)/.6251);
    // }
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
        delay(100);
    } // close printing for
    Serial.println("-----");
    Serial.print("State is: ");
    Serial.println(state);
    // ChangeThrottle();
    Serial.println("Input");
   for(int i = 0; i < sbusInput.NUM_CH; i++)
    {
      Serial.print(i);
      Serial.print(": ");
      if(/*data.ch[i] != 0*/ false)
      {
        Serial.println((.6251*(sbusInput.ch[i]))+879.7);
      }
      else
      {
        Serial.println(sbusInput.ch[i]);
      }
      delay(100);
    }
    // data.ch[4] = ((500-879.7)/.6251);
    // data.ch[5] = 100.5555;
    // Serial.println(data);

    /* Display lost frames and failsafe data */
    // Serial.print(data.lost_frame);
    // Serial.print("\t");
    // Serial.println(data.failsafe);
    /* Set the SBUS TX data to the received data */
    sbus_tx.data(data);
    // Serial.println(data);
    /* Write the data to the servos */
    sbus_tx.Write();
    delay(100);
}
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
double ChannelMath(int setpoint)
{
  return((setpoint - 879.7)/.6251);
}
// void ChangeThrottle()
// { 
//   if (Serial.available() > 0) {
//   throttle = Serial.read();
//   }
// }