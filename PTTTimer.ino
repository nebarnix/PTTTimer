//K7PHI Icom 27-A PTT Timer

#include <SPI.h>
#include <Wire.h>
#include <toneAC.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
//#include <Fonts/FreeSans12pt7b.h>

#define SSD1306_EXTERNALVCC
#define PTTPIN 3 //PTT pin is 3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PTTTIME  170000 //170 seconds, 10 seconds shy of 3 minutes
#define IDTIME   600000 //600 seconds (10min)
#define IDLETIME 900000 //900 seconds (15 min)

//#define PTTTIME 10000 //10 seconds, for testing
//#define IDTIME 60000 //60 seconds (1min, for testing)
//#define IDLETIME 120000 //120 seconds, for testing


//Global Variables
unsigned long screenRefreshTimer = 0, PTTTimer = 0, IDTimer = 0;
bool FirstPTT = false, PTTMode = false, IDMode = false;
int PTTAlarm = 0;
const int screenRefreshInterval=250;

void splashScreen()
{
  //display splash screen
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0, 0);            // Start at top-left corner
  display.println(F("   ICOM   ")); //10
  display.println(F(" PTT-Piggy"));
  display.display();
  display.clearDisplay();
  delay(1500);

  display.setCursor(0, 0);            // Start at top-left corner
  display.println( F("  K7PHI   "));
  display.println( F("   2021   "));
  display.display(); //wipe clear could be fun and useless :P
  display.clearDisplay();
  delay(1500);
}

void initOLED()
{
  //Init OLED display
  //FAIL BEEP IF FAIL

  //arduino isnt seeing these in the header?? WTH?
#define SSD1306_EXTERNALVCC 0x01  ///< External display voltage source
#define SSD1306_SWITCHCAPVCC 0x02 ///< Gen. display voltage from 3.3V

  if (!display.begin(SSD1306_EXTERNALVCC, 0x3C))  // Address 0x3C for 128x32  Address 0x3C for 128x64
  {
    Serial.println(F("SSD1306 allocation failed"));
    //toneAC( frequency [, volume [, length [, background ]]] )
    toneAC(783.99, 10, 200); //pins 9 and 10 on a nano hooked to a buzzer
    toneAC(739.99, 10, 200); //pins 9 and 10 on a nano hooked to a buzzer
    toneAC(698.46, 10, 200); //pins 9 and 10 on a nano hooked to a buzzer
    toneAC(659.25, 10, 500); //pins 9 and 10 on a nano hooked to a buzzer
    //for (;;); // Don't proceed, loop forever
  }
  display.setRotation(180);
}

void setup()
{

  Serial.begin(115200);
  initOLED();

  //PTT Status Pin
  pinMode(PTTPIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //Boot-up beep
  toneAC(2600, 5, 50); //pins 9 and 10 on a nano hooked to a buzzer

  splashScreen();//takes 3 seconds, would be nice to have it non blocking / interruptable with PTT
}

void chirp(int a, int b, int chirpTone, int chirpVol = 10)
{
  for (int i = 0; i < a; i++)
  {
    for (int j = 0; j < b; j++) //3 and 5 sounds like a cricket
    {
      //toneAC( frequency [, volume [, length [, background ]]] )
      toneAC(chirpTone, chirpVol);
      delay(10);
      toneAC();
      delay(10);
    }
    delay(100);
  }
}

void loop() {
  int PTTPinState;
  int minutes = 0;
  unsigned long nowTime, calcInterval;
  char buf[50];
  /* THIS CODE IS NOT I REPEAT NOT NEEDED OR USEFUL! ALREADY ROLLS OVER GRACEFULLY AS IS!!
    if (screenRefreshTimer > millis())
    screenRefreshTimer = 0;
    if (IDTimer > millis())
    IDTimer = 0;
    if (PTTTimer > millis())
    IDTimer = 0;

    //code to handle millis rollover wrt PTTTimer and IDTimer
  */

  PTTPinState = digitalRead(PTTPIN);

  if (PTTPinState == LOW ) //we're transmitting
  {
    if (PTTMode == false) //Suddenly!
    {
      PTTMode = true; //set the PTTMode to true
      
      //round to nearest second
      PTTTimer = (millis()/1000)*1000;
      //force screen refresh      
      screenRefreshTimer = millis() + screenRefreshInterval;
      FirstPTT = true; //use this to keep track of initial timers and stuff

      digitalWrite(LED_BUILTIN, HIGH);
      if (IDMode == false) //we started transmitting, so now we need to ID in 10 minutes
      {
        IDMode = true;
        //round to nearest second
        IDTimer = (millis()/1000)*1000;
      }
    }

    //check for alarm timer, set alarm variable but only if alarm isn't already going off
    if ((millis() - PTTTimer) > PTTTIME && PTTAlarm == 0)
      PTTAlarm = 1;
  }
  else if (PTTPinState == HIGH && PTTMode == true) // we've stopped transmitting
  {
    PTTMode = false;
    //round to nearest second
    PTTTimer = (millis()/1000)*1000;

    //force immediate screen refresh
    screenRefreshTimer = millis() + screenRefreshInterval; 
    PTTAlarm = 0;
    digitalWrite(LED_BUILTIN, LOW);
  }

  //ID TIMER RAN OUT, CHIRP PLZ
  if ((millis() - IDTimer) > IDTIME && IDMode == true)
  {
    IDMode = false; // turn off reminders. PTT will activate them again.
    chirp(1, 3, 3000, 10); //A,B, Freq, Vol
    Serial.print("ID REMINDER - ");
  }

  if ((millis() - screenRefreshTimer) > screenRefreshInterval) //update screen once per interval
  {
    screenRefreshTimer = millis();
    if (PTTAlarm > 0) //alarming!
    {
      Serial.print("TIMEOUT! - ");
      noToneAC(); // shut up
      if (PTTAlarm == 1)
        toneAC(750, 5, 250, true); //beep
      else if (PTTAlarm == 2)
        toneAC(500, 5, 250, true); //boop

      //toneAC( frequency [, volume [, length [, background ]]] )
      PTTAlarm++;
      if (PTTAlarm > 2)
        PTTAlarm = 1;
    }

    // only hide timers when ID expires BY 30 min (radio assumed idle)
    // keep track of initial condition where millis is close to the timers but PTT never got touched.
    // So display IDLE at startup.
    if ((millis() - IDTimer) < IDLETIME  && FirstPTT == true)
    {
      nowTime = millis(); // now we can stop calling it all the time

      //display.setRotation(0);            // Start at top-left corner
      display.setCursor(0, 0);            // Start at top-left corner
      display.setTextSize(3);             // Normal 1:1 pixel scale

      //show PTT Timer
      if (PTTMode == true)
      {
        if (PTTAlarm == 0 || PTTAlarm == 2) //if alarm is off or is alternate, makes it flash
        {
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
          display.print(" "); //print white block then shift right so we have a 1px boder to the left
          display.setCursor(1, 0);
        }
        else
          display.setCursor(1, 0); //match offset during blinking


        Serial.print(((nowTime - IDTimer) / 1000.0), 1);
        Serial.print(" - ");
        if ((nowTime - PTTTimer) < PTTTIME)
        {

          //display.println(((PTTTIME - (nowTime - PTTTimer)) / 1000.0), 1); //make it a countdown timer
          calcInterval = (PTTTIME - (nowTime - PTTTimer)) / 1000;
          minutes = 0;

          //is this faster than division? I don't know if I care :P
          while (calcInterval > 60)
          {
            calcInterval -= 60;
            minutes++;
          }

          sprintf(buf, "%02d:%02d", minutes, calcInterval);
          display.println(buf);

          Serial.print(((PTTTIME - (nowTime - PTTTimer)) / 1000.0), 1); //make it a countdown timer
        }
        else // stupid signed operations
        {
          //display.println(((-PTTTIME + (nowTime - PTTTimer)) / 1000.0), 1); //make it a countdown timer
          calcInterval = (-PTTTIME + (nowTime - PTTTimer)) / 1000;
          minutes = 0;

          //is this faster than division? I don't know if I care :P
          while (calcInterval > 60)
          {
            calcInterval -= 60;
            minutes++;
          }

          sprintf(buf, "%02d:%02d", minutes, calcInterval);
          display.println(buf);

          Serial.print(((-PTTTIME + (nowTime - PTTTimer)) / 1000.0), 1); //make it a countdown timer
        }
        //Serial.print((((PTTTIME) - (nowTime - PTTTimer)) / 1000.0), 1); //make it a countdown timer
        Serial.println(" - PTT");
      }

      //show count-up time since last PTT
      else
      {
        //decimal seconds
        //display.println(((nowTime - PTTTimer) / 1000.0), 1); //count UP timer

        calcInterval = (nowTime - PTTTimer) / 1000;
        minutes = 0;

        //is this faster than division? I don't know if I care :P
        while (calcInterval > 60)
        {
          calcInterval -= 60;
          minutes++;
        }

        //count UP timer
        sprintf(buf, "%02d:%02d", minutes, calcInterval);
        display.println(buf);

        Serial.print(((nowTime - IDTimer) / 1000.0), 1);
        Serial.print(" - ");
        Serial.println(((nowTime - PTTTimer) / 1000.0), 1);
      }

      display.setTextColor(SSD1306_WHITE);

      //show ID timer if its active
      if (IDMode == true)
      {
        display.setCursor(0, 40);
        //display.println(((IDTIME - (nowTime - IDTimer)) / 1000.0), 1); //make it a countdown timer
        calcInterval = (IDTIME - (nowTime - IDTimer)) / 1000;
        minutes = 0;

        //is this faster than division? I don't know if I care :P
        while (calcInterval > 60)
        {
          calcInterval -= 60;
          minutes++;
        }

        //count UP timer
        sprintf(buf, "%02d:%02d", minutes, calcInterval);
        display.println(buf);

      }

      display.display();
      display.clearDisplay(); //clears the BUFFER only, not the whole screen
    }
    else
    {
      display.setCursor(0, 0);            // Start at top-left corner
      display.setTextSize(3);             // Normal 1:1 pixel scale
      display.println(" IDLE "); //REPLACE THIS WITH A SCREEN SAVER!!
      Serial.println(" IDLE ");
      display.display();
      display.setTextColor(SSD1306_WHITE);
      display.clearDisplay(); //clears the BUFFER only, not the whole screen
    }
  }
}
