
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

const int MPU=0x68;
int16_t AcX,AcY;
bool Voleur;
#define LED (11)
#define Button (10)
#define EnGPS (A4)
#define PN532_SS   (A0)
#define PN532_MOSI (A1)
#define PN532_MISO (A2)
#define PN532_SCK  (A3)

bool Unlock;
bool TimerEtat = 0;

// Use this line for a breakout with a SPI connection:
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

void setup(void) {

    pinMode(LED, OUTPUT);
    pinMode(EnGPS, OUTPUT);
    digitalWrite(EnGPS,1);
    Serial.begin(115200);
    timer_init();
    
    nfc.begin();
    nfc.SAMConfig();

    Wire.begin();
    Wire.beginTransmission(MPU);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);
    
  } 
  
  void loop(void) 
  {
    digitalWrite(LED,Voleur);
    //digitalWrite(LED,!Unlock); 
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU,14,true);
  
  int AcXoff,AcYoff,AcZoff;
  //Acceleration data correction
  AcXoff = -950;
  AcYoff = -300;
  AcZoff = 0;
  //read accel data
  AcX=(Wire.read()<<8|Wire.read()) + AcXoff;
  AcY=(Wire.read()<<8|Wire.read()) + AcYoff;
  
   if(abs(AcY) > 30000 || abs(AcX) > 30000 )
   {
     if(Unlock == 0)
     {
        ControlTime();
     }
   }
}


void ControlTime(void)
{
  for(int idx = 0; idx < 20 ; idx++)
  {
    if(Unlock == 1)
    {
      return;
    }
    Serial.println("VOLEUR");
    BlinkLed(200);
    Voleur = 1;
  }
  if(Unlock == 0)
  {
    while(digitalRead(Button) == 0)
    {
      TimerEtat = 1;
      BlinkLed(50);
    }
    TimerEtat = 0;
    return;
  }
  else
  {
    return;
  }
}

void SendData(void)
{
  Serial.println("Data");
}

void timer_init()
{
  cli();
  TCCR1A = 0x0000; // 0b00000000  - TOP is 0xFFFF
  TCCR1B = 0b00001100; //0x0C; // 0b00001XXX  // last 3 bits specify clock source: 0 = no clock source (stopped); 1 = no prescale; 2 = clock / 8; 3 = clock / 64
  // 4 = clock / 256; 5 = clock / 1024; 6 = ext clock on T0 pin falling edge; 7 = ext clock on T0 pin rising edge
  TCCR1C = 0; // not forcing output compare
  TCNT1 = 0x0000; // set timer counter initial value (16 bit value)

  // Interrupts on overflow (every ~1 second)
  OCR1A = 0x7A12; // 1/8 M * 256 * 0x7A12 = 1 S
  
  //TIMSK1 = 0; // Disable timer 
  //TIMSK1 = 1; // enable timer overflow interrupt
  TIMSK1 = 2; // enable timer compare match 1A interrupt
  sei(); // enable interrupts
}

ISR(TIMER1_COMPA_vect) 
{
  switch(TimerEtat)
  {
    case 0: // Lorsque le timer sert à detect. le badge RFID
          TCCR1B = 0b00001100; // Timer1 Prescaler 256
          OCR1A = 0x7A12; // Timer 1 Chaque 1s
          DetectRFID();
    break;
    case 1: // Lorsque le timer sert à récup. le GPS
          TCCR1B = 0b00001011; // Timer1 Prescaler 64
          OCR1A = 0x00CD; // Timer1 chaque 1ms
          SendData();
    break;
  }
}

void BlinkLed(int Time)
{
      digitalWrite(LED,0);
      delay(Time);
      digitalWrite(LED,1);
      delay(Time);
}

void DetectRFID(void) 
{
  int Code1[16] = { 'H', 'E', 'S', ' ', 'V', 'e', 'l', 'o', 's', ' ', 'I', 'd', 'e', 'n', 't', '.' };
  int Code2[16] = {'V', 'e','l', 'o', ' ', 'N', '-', ' ', '2',0 ,0 ,0 ,0 ,0 ,0 ,0};
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type) 
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
  if (success) { 
    
    if (uidLength == 4)
    {
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
      
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
      if (success)
      {
        uint8_t Data1[16];
        uint8_t Data2[16];
        success = nfc.mifareclassic_ReadDataBlock(4, Data1);    
        if (success)
        {
          success = nfc.mifareclassic_ReadDataBlock(5, Data2);
          if (success)
        {
          int CountCode1 = 0;
          int CountCode2 = 0;
          for(int Count = 0; Count < 16 ; Count++)
          {
            if(Data1[Count] == Code1[Count])
            {
              CountCode1++;
            }
            if(Data2[Count] == Code2[Count])
            {
              CountCode2++;
            }
          }
          if(CountCode1 == 16)
          {
            Serial.println("Carte des Velos de la HES");
            if(CountCode2 == 16)
            {
              Serial.println("C'est le bon Velo :)");
              Unlock = 1;
              Voleur = 0;
            }
            else
            {
              Serial.println("Ce n'est pas le bon Velo !");
              Unlock = 0;
            }
          }
          else
          {
            Serial.println("Pas la bonne Entreprise !");
            Unlock = 0;
          }
        }
        }
        else
        {
          Unlock = 0;
        }
      }
      else
      {
        Unlock = 0;
      }
    }
  }
  else
  {
    Unlock = 0;
  }
}
