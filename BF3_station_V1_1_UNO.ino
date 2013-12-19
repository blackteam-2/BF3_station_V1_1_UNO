/*
Test program for 2 station 

BF3_station_V1.1_UNO

*/
#include <SPI.h>
#include <LiquidCrystal.h>
#include "nRF24L01.h"
#include "RF24.h"


//=================================================================================
//=============================Global variables and pins===========================
//=================================================================================

//-------------------Library definitions----------------------- 
RF24 radio(0,1);
union radpack
{
  byte package[32];
  int myints[16];
  float myfloats[8];
};

radpack radioA_in;
radpack radioA;
//
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
typedef enum { role_ping_out = 1, role_pong_back } role_e;
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};
role_e role;

//
LiquidCrystal lcd(9, 8, A3, A2, A1, A0);


//----------------Pin Definitions-------------------

const int role_pin = 4;
const int button_pin = 2;
const int spawnButt = 30; 
const int buzzer = 3;//assigned to timer 5, compare A
const int LEDG = 5;
const int LEDO = 7;
const int LEDR = 6;


//-------------Global variables--------------------

boolean lck = false;
boolean tempbool = false;
int transFail = 0;
int i = 0;
int resendID = 0;
unsigned long timerOld = 0;
unsigned long timerTemp = 0;

int state = 0;
boolean startReady = false;


//-----------Adjustment variables------------------

int ticketStart = 80; //The number of tickets to start with
int duty = 250; // Duty cycle for the buzzer (0-255)
unsigned long timerMax = 30; //in min
unsigned long timerStep = 10; //in Sec
unsigned long buttTimer = 0.6; //in Sec
unsigned long startTime = 0;


//=================================================================================
//=====================================Setup=======================================
//=================================================================================

void setup(void)
{
  //------Setup pin direction and levels------
  pinMode(buzzer, OUTPUT);
  pinMode(role_pin, INPUT);
  pinMode(13,OUTPUT);
  pinMode(spawnButt, INPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDO, OUTPUT);
  pinMode(LEDR, OUTPUT);
  digitalWrite(buzzer,LOW);
  digitalWrite(role_pin,HIGH);
  digitalWrite(13,LOW);
  digitalWrite(spawnButt, HIGH);
  digitalWrite(LEDG, LOW);
  digitalWrite(LEDO, LOW);
  digitalWrite(LEDR, LOW);
  delay(20); 
  
  // read the address pin, establish our role
  if (digitalRead(role_pin))
    role = role_ping_out;
  else
    role = role_pong_back;
  
  //Initilise data array  
  for(i = 0 ; i < 15 ; i++)
  {
    radioA_in.myints[i] = 0;
    radioA.myints[i] = 0;
  }
  
  //Set starting tickets
  radioA.myints[2] = ticketStart;
  radioA.myints[3] = ticketStart;
  
  //Set up variables for use with timer
  timerMax = timerMax * 60 * 1000;
  timerStep = timerStep * 1000;
  buttTimer = buttTimer * 1000;
  
  //Set up the radio and role of the station
  radio.begin();
  radio.setRetries(20,15);
  radio.setPayloadSize(16);
  
  //Set radio up for high power long distance transmition 
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  
  if ( role == role_ping_out )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
    digitalWrite(LEDG, HIGH);
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
    digitalWrite(LEDO, HIGH);
  }
  
  radio.startListening();
  radio.printDetails();
  
  //Initilise the LCD
  lcd.begin(16, 2);
  
  //Display initial start up screen and tests
  lcd.clear();
  lcd.print("BF3 station test");
  lcd.setCursor(0,1);
  lcd.print("J McKenna [MMS]");
  delay(6000);
  
//  digitalWrite(buzzer, HIGH);
  
  lcd.clear();
//  updateTic();
//  lcd.setCursor(10,1);
//  lcd.print(timerMax);
}


//=================================================================================
//=====================================Loop========================================
//=================================================================================

void loop(void)
{
  switch(state)
  {
    
    //====================================================Starting screen================================================
    case 0:
    
      //============Master station=======================
      if(role == role_ping_out)
      {
          //-----------------Butt-----------------
          if(((digitalRead(spawnButt) == HIGH) && (lck == false)) && (startReady == true))
          { 
            timerTemp = millis();
            while(digitalRead(spawnButt) == HIGH){//Do nothing
            }
            
            if((millis() - timerTemp) > buttTimer)
            {
              //start game
              state = 1;
              lcd.clear();
            }

          }
          else if(digitalRead(spawnButt) == LOW){
            lck = false;
          }
          
          //-------------------radio-----------------------
          if ( radio.available() )
          {
            bool done = false;
            while (!done)
            {
              done = radio.read( &radioA_in, 32);
            }
            delay(20);
            
            if(radioA_in.myints[4] == 1)
            {
              startReady = true; 
            }
          }
          
          //-----------------LCD----------------------
          if(startReady)
          {
            lcd.setCursor(0,0);
            lcd.print("Hold to start game");
            lcd.setCursor(0,1);
            lcd.print("Station ready");
            lcd.print(radioA_in.myints[4]);
          }
          else
          {
            lcd.setCursor(0,0);
            lcd.print("Waiting for");
            lcd.setCursor(0,1);
            lcd.print("opposition");
          }
        
      }
      else// ========Slave station========
      {
        
        //------------button-----------
        if((digitalRead(spawnButt) == HIGH) && (lck == false))
        { 
          timerTemp = millis();
          while(digitalRead(spawnButt) == HIGH){//Do nothing
          }
          
          if((millis() - timerTemp) > buttTimer)
          {
            //station ready
            sendReady(1);
            startReady = true;
          }

        }
        else if(digitalRead(spawnButt) == LOW){
          lck = false;
        }
        
        //----------LCD----------
        if(startReady)
        {
          lcd.setCursor(0,0);
          lcd.print("Waiting for start");
          lcd.setCursor(0,1);
          lcd.print("Station ready");
        }
        else
        {
          lcd.setCursor(0,0);
          lcd.print("To Signal ready");
          lcd.setCursor(0,1);
          lcd.print("press Button");
        }
        
        //-------------------radio-----------------------
        if ( radio.available() )
        {
          bool done = false;
          while (!done)
          {
            done = radio.read( &radioA_in, 32);
          }
          delay(2);
          
          if(radioA_in.myints[4] != 0)
          {
            state = 1;
            lcd.clear();
          }
        }
        
      }
    break;
    
    //===============================================Countdown==================================================
    case 1:
      
      if(role == role_ping_out)
      {
        lcd.clear();
        for(int j = 11 ; j >= 0 ; j--)
        {
           sendReady(j);
           lcd.setCursor(0,0);
           lcd.print("Game starting in");
           lcd.setCursor(5,1);
           lcd.print(j);
           lcd.print(" sec");
           delay(1000);
        }
        state = 2;
        lcd.clear();
        startTime = millis();
        timerMax = timerMax + startTime;
      }
      else
      {
        
        if (radio.available())
        {
          bool done = false;
          while (!done)
          {
            done = radio.read( &radioA_in, 32);
          }
          delay(2);
          
          lcd.setCursor(0,0);
          lcd.print("Game starting in");
          lcd.setCursor(5,1);
          lcd.print(radioA_in.myints[4]);
          lcd.print(" sec");
          
          if(radioA_in.myints[4] == 0)
          {
            state = 2;
            lcd.clear();
            
            digitalWrite(buzzer,HIGH);
            delay(500);
            digitalWrite(buzzer,LOW);
          }
        }
        
      }
      
    break;
    
    //=============================================Main rinning program=============================================
    case 2:
    
      //-------------------radio-----------------------
      if ( radio.available() )
      {
        bool done = false;
        while (!done)
        {
          done = radio.read( &radioA_in, 32);
        }
        delay(20);
        tempbool = checkData();
          
        if(tempbool == false)
        {
          transFail++;
          if(transFail > 50)
          {
            //To many transmition errors, do something here 
          }
        }
      }
      //------------------Inputs--------------------------
      if((digitalRead(spawnButt) == HIGH) && (lck == false))
      {
        digitalWrite(13,HIGH);
        
        timerTemp = millis();
        while(digitalRead(spawnButt) == HIGH){//Do nothing
        }
        
        if((millis() - timerTemp) > buttTimer)
        {
          if(role == role_ping_out)
            radioA.myints[2]--;
          else
            radioA.myints[3]--;
        }
          
        sendData();
        lck = true;
      }
      else if(digitalRead(spawnButt) == LOW){
        digitalWrite(13,LOW);
        lck = false;
      }
      //-------------------Timer-----------------------
      if(role == role_ping_out)
      {
         if((millis() - timerOld) >= timerStep)
         {
            radioA.myints[2]--;
            radioA.myints[3]--;
            timerOld = millis();
            sendData();
            updateTic();
         }
         updateTimer();
      }
      //------------Check win conditions----------------
      if(((millis() >= timerMax) && (role == role_ping_out)) || ((radioA.myints[2] <= 0) || (radioA.myints[3] <= 0)))//only for timer check
      {
        state = 3;
      }
      
    break;
    
    //=============================================================End conditions=================================================
    case 3:
      if(millis() >= timerMax)
      {
        lcd.clear();
        digitalWrite(buzzer, HIGH);
        delay(20);
        timer5(duty);
        while(1)
        {
         lcd.setCursor(0,0);
         lcd.print("Time over");
         lcd.setCursor(0,1);
         if(radioA.myints[3] <= radioA.myints[2])
           lcd.print("Team A Wins");
         else if(radioA.myints[2] <= radioA.myints[3])
           lcd.print("Team B wins");
         else
           lcd.print("Game Drawn");
         delay(1000);
         lcd.clear();
         delay(800);
        } 
      }
      if((radioA.myints[2] <= 0) && (radioA.myints[3] <= 0))
      {
        lcd.clear();
        digitalWrite(buzzer, HIGH);
        delay(20);
        timer5(duty);
        while(1)
        {
           lcd.setCursor(0,0);
           lcd.print("Tickets expired");
           lcd.setCursor(0,1);
           lcd.print("Game Drawn");
           delay(1000);
           lcd.clear();
           delay(800);
        }
      }
      if(radioA.myints[2] <= 0)
      {
        lcd.clear();
        digitalWrite(buzzer, HIGH);
        delay(20);
        timer5(duty);
        while(1)
        {
           lcd.setCursor(0,0);
           lcd.print("Tickets expired");
           lcd.setCursor(0,1);
           lcd.print("Team B Wins");
           delay(1000);
           lcd.clear();
           delay(800);
        }
      }
      if(radioA.myints[3] <= 0)
      {
        lcd.clear();
        digitalWrite(buzzer, HIGH);
        delay(20);
        timer5(duty);
        while(1)
        {
           lcd.setCursor(0,0);
           lcd.print("Tickets expired");
           lcd.setCursor(0,1);
           lcd.print("Team A Wins");
           delay(1000);
           lcd.clear();
           delay(800);
        }
      }
    break;
    
    //==============================================================Reserved for future use========================================
    case 4:
    
    break;
    
  }
  
}


//=================================================================================
//================================Background functions=============================
//=================================================================================


//Check the input data array is sensical 
boolean checkData(void)
{
  for(i = 0 ; i <= 15 ; i++)
  {
    if((radioA_in.myints[i] < -65535) || (radioA_in.myints[i] > 65535))
      return false;
  }  
  
//  lcd.setCursor(0, 1);
//  lcd.print(radioA_in.myints[2]);
    
  for(i = 0 ; i <= 15 ; i++)
  {
    radioA.myints[i] = radioA_in.myints[i];
  }
  
  updateTic();
  
  return true;
}


// Prepare and transmit the information in the data array
void sendData()
{
  boolean done = false;
  //Packet name
  int tempa = radioA.myints[0];
  tempa++;
  if(tempa > 800)
  {
    tempa = 0;
  }
  radioA.myints[0] = tempa;
  
  radioA.myints[14] = 0;
  
  radio.stopListening();
  boolean ok = radio.write( &radioA, 32);   
  radio.startListening();
  
  updateTic();
}

//
void sendReady(int q)
{
   boolean done = false;
  //Packet name
  int tempa = radioA.myints[0];
  tempa++;
  if(tempa > 800)
  {
    tempa = 0;
  }
  radioA.myints[0] = tempa;
  
  radioA.myints[4] = q; 
  
  radio.stopListening();
  boolean ok = radio.write( &radioA, 32);   
  radio.startListening();
}

// Update the top row of the lcd to display the ticket count
void updateTic()
{
  //LCD code
  lcd.setCursor(0, 0);
  lcd.print("A:");
  lcd.print(radioA.myints[2]);
  lcd.print("  ");
  lcd.setCursor(12, 0);
  lcd.print("B:");
  lcd.print(radioA.myints[3]);
  lcd.print(" ");
}

//Update the timer display on the LCD
void updateTimer()
{
  lcd.setCursor(0, 1);
  //lcd.print("Time");
  lcd.print(((millis()-startTime)/1000)/60);
  lcd.print(":");
  if((((millis()-startTime)/1000)%60) <= 9)
  {
    lcd.print("0");
    lcd.print(((millis()-startTime)/1000)%60);
  }
  else
  {
    lcd.print(((millis()-startTime)/1000)%60);
    lcd.print(" ");
  }
}

//Start/Stop timer 5 at a set duty cycle
//
// 0 - Stop the timer
// 1 - 255 Duty cycle
//
void timer5(int j)
{
//  if(j == 0)
//  {
//    TCCR5B &= (0 << CS51) | (0 << CS50);
//    delay(1);
//    digitalWrite(buzzer, LOW);
//    return;
//  }
//  TCCR5A |= ((1 << COM5A1) | (1 << WGM50));
//  TCCR5B |= (1 << WGM52);
//  //TIMSK5 |= ((1 << OCIE5A) | (1 << TOIE5));
//  OCR5A = j;
//  TCCR5B |= ((1 << CS51) | (1 << CS50));
}


// vim:cin:ai:sts=2 sw=2 ft=cpp
