int8_t TabTest[500];
int idx;
// what's the name of the hardware serial port?
#define GPSSerial Serial1


void setup() {
  pinMode(A4,OUTPUT);
  digitalWrite(A4,1);
  pinMode(11,OUTPUT);
  timer_init();
  // wait for hardware serial to appear
  while (!Serial);

  // make this baud rate fast enough to we aren't waiting on it
  Serial.begin(115200);

  // 9600 baud is the default rate for the Ultimate GPS
  GPSSerial.begin(9600);
  
}

     
void loop() {
    
}

void timer_init()
{
  cli();
  TCCR1A = 0x0000; // 0b00000000  - TOP is 0xFFFF
  TCCR1B = 0b00001011; //0x0C; // 0b00001XXX  // last 3 bits specify clock source: 0 = no clock source (stopped); 1 = no prescale; 2 = clock / 8; 3 = clock / 64
  // 4 = clock / 256; 5 = clock / 1024; 6 = ext clock on T0 pin falling edge; 7 = ext clock on T0 pin rising edge
  TCCR1C = 0; // not forcing output compare
  TCNT1 = 0x0000; // set timer counter initial value (16 bit value)

  // Interrupts on overflow (every ~1 second)
  OCR1A = 0x00CD; // 1/8 M * 256 * 0x7A12 = 1 S
  
  //TIMSK1 = 0; // Disable timer 
  //TIMSK1 = 1; // enable timer overflow interrupt
  TIMSK1 = 2; // enable timer compare match 1A interrupt
  sei(); // enable interrupts
}

ISR(TIMER1_COMPA_vect) // 16 bit timer 1 compare 1A match
{
   if (Serial.available()) {
    idx++;
    char c = Serial.read();
    TabTest[idx] = c;
    GPSSerial.write(TabTest[idx]);
  }
  if (GPSSerial.available()) {
    idx++;
    char c = GPSSerial.read();
    TabTest[idx] = c;
    Serial.write(TabTest[idx]);
  }
  if(idx == sizeof(TabTest)-1);
  {
    idx = 0;
  }
}

