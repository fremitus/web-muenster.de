//Code provided by Ivan Sudarikov, St. Petersburg, Russia

#include <DS3231.h>
#include <Wire.h>

// outputs for K155ID1
int out1 = A3;
int out2 = A1;
int out4 = A0;
int out8 = A2;
// outputs for transistors
int key1 = 4;
int key2 = 6;
int key3 = 7;
int key4 = 8;

int neon = 12; // NE output (seconds)
int keyb = A6; // keyboard pin


int keynum = 0; //key pressed
int mode = 0; //current mode: 0 - just show time, 1 - configure time
int currentdigit = 0; //0 - configuring hour digits, 1 - configuring minute digits
bool blinking = false; // flag defining blinking while hour or minutes configuring
bool timeout = false; // timeout after button pressed (avoids contact bounce)
unsigned long startTime; //for second NE lamp blinking
unsigned long startTime2; // for reading input buttons
int sec; //For seconds blinking

// Configuring DS3231
DS3231 Clock;
bool Century = false;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;

//global parameters
//int second, minute, hour, date, month, year, temperature;

int lastMinute;

const int DIGITS_OTPUTS [10][4] =
{
  { LOW, LOW, LOW, LOW },   // 0
  { HIGH, LOW, LOW, LOW},   // 1
  { LOW, HIGH, LOW, LOW},   // 2
  { HIGH, HIGH, LOW, LOW},  // 3
  { LOW, LOW, HIGH, LOW},   // 4
  { HIGH, LOW, HIGH, LOW},  // 5
  { LOW, HIGH, HIGH, LOW},  // 6
  { HIGH, HIGH, HIGH, LOW}, // 7
  { LOW, LOW, LOW, HIGH},   // 8
  { HIGH, LOW, LOW, HIGH}   // 9
};



void setup() {
  // set PWM frequency equal to 50kHz at PIN = 9
  TCCR1B = TCCR1B & 0b11111000 | 0x01;

  analogWrite(9, 165);

  // Start the I2C interface
  Wire.begin();
  // Start the serial interface
  Serial.begin(9600);

  //задаем режим работы выходов микроконтроллера
  pinMode(out1, OUTPUT);
  pinMode(out2, OUTPUT);
  pinMode(out4, OUTPUT);
  pinMode(out8, OUTPUT);

  pinMode(key1, OUTPUT);
  pinMode(key2, OUTPUT);
  pinMode(key3, OUTPUT);
  pinMode(key4, OUTPUT);

  pinMode(neon, OUTPUT);

  pinMode(keyb, INPUT);


}

struct Time {
  int minute;
  int hour;
};

// function prototype
struct Time readTimeFromDS3231();

struct Time readTimeFromDS3231()   // reads current hour and minutes in global variables
{
  struct Time currentTimeStmp;
  currentTimeStmp.minute = Clock.getMinute();
  currentTimeStmp.hour = Clock.getHour(h12, PM);
  return currentTimeStmp;
}


void loop() {
  startTime = 0;
  startTime2 = 0;
  sec = 0;
  // read current time
  struct Time currentTimestamp = readTimeFromDS3231();
  delay(1000);

  lastMinute = (currentTimestamp.minute) % 10;
  int digits[4]; // array for current digits from time
  int keyval = 0;
  unsigned long currentTime = 0;

  while (1 == 1) {
    keyval = analogRead(keyb); // read input button pressed
    Serial.print("Keyval=");
    Serial.println(keyval);

    currentTime = millis(); //current time
    if (currentTime >= (startTime2 + 200)) // if 200 ms passed from last button pressed
    {
      timeout = false;
      // check input keys
      if (keynum == 3) // check if mode between "time" and "setup time" pressed
      {
        timeout = true;
        if (mode == 0) mode = 1; // if it was "time mode" go to "setup time" mode
        else if (mode == 1) mode = 0; //and vice versa
      }
      if (keynum == 2) // configure hours and minutes pressed
      {
        timeout = true;
        // currentdigit either = 0 - configuring hours, or = 1 - configuring minutes
        currentdigit++;
        if (currentdigit == 2) currentdigit = 0;
      }
      if (keynum == 1 && mode == 1) // increase current digit (minutes or hours)
      {
        blinking = false; // stop blinking
        startTime = millis();
        timeout = true;
        if (currentdigit == 0) // change hour
        {
          int hour = currentTimestamp.hour;
          if (hour <= 22) hour++;
          else
            hour = 0;
          Clock.setHour(hour);
        }
        if (currentdigit == 1) // change minute
        {
          int minute = currentTimestamp.minute;
          if (minute <= 58) minute++;
          else
            minute = 0;
          Clock.setMinute(minute);//Set the minute
          Clock.setSecond(0);
        }
      }
      keynum = 0;
      startTime2 = millis();
    }

    if (currentTime >= (startTime + 500)) // if passed 500 ms
    {
      blinking = !blinking; // inverts blinking mode
      startTime = currentTime;
      currentTimestamp = readTimeFromDS3231();
      sec = (sec + 1);
      if (sec >= 2) sec = 0;

    }

    if (sec >= 1) // NE seconds should be shown
    {
      digitalWrite(neon, HIGH);
      delay(2);
      digitalWrite(neon, LOW);
      delay(1);
    }

    digits[0] = (currentTimestamp.hour) / 10;
    digits[1] = (currentTimestamp.hour) % 10;
    digits[2] = (currentTimestamp.minute) / 10;
    digits[3] = (currentTimestamp.minute) % 10;

    //saving cathods
    if ((currentTimestamp.minute) % 10 != lastMinute && mode == 0)
    {

      digits[3] = lastMinute;
      lastMinute = (currentTimestamp.minute) % 10;

      saveCathods(digits);
      digits[3] = lastMinute;
    }
    //

    show(digits); // show time

    // check keybord pressed
    if (!timeout) //if timeout reached read input key value
    {
      /*keyval - value of analogRead functions
         if first button pressed keyval is 200
         if second button pressed keyval is around 700
         if third button pressed keyvalue is around 1000
         depends on resistors in key bouard
      */
      if (keyval > 150 && keyval < 400) keynum = 1;
      if (keyval > 700 && keyval < 900) keynum = 2;
      if (keyval > 960) keynum = 3;
    }
  }
}

void show(int a[])
{
  //show first hour digit
  setNumber(a[0]);
  if (!(mode == 1 && currentdigit == 0 && blinking == true)) //if we are in configure time mode then digit should blink
  {
    digitalWrite(key1, HIGH);
    delay(2);
    digitalWrite(key1, LOW);
    delay(1);
  }

  // a[1] show second hour digit
  setNumber(a[1]);
  if (!(mode == 1 && currentdigit == 0 && blinking == true))
  {
    digitalWrite(key2, HIGH);
    delay(2);
    digitalWrite(key2, LOW);
    delay(1);
  }

  //show first minute digit
  setNumber(a[2]);
  if (!(mode == 1 && currentdigit == 1 && blinking == true))
  {
    digitalWrite(key3, HIGH);
    delay(2);
    digitalWrite(key3, LOW);
    delay(1);
  }

  //show second minute digit
  setNumber(a[3]);
  if (!(mode == 1 && currentdigit == 1 && blinking == true))
  {
    digitalWrite(key4, HIGH);
    delay(2);
    digitalWrite(key4, LOW);
    delay(1);
  }
}

// pass digit to K155ID highvoltage processor
void setNumber(int num)
{
  digitalWrite (out1, DIGITS_OTPUTS[num][0]);
  digitalWrite (out2, DIGITS_OTPUTS[num][1]);
  digitalWrite (out4, DIGITS_OTPUTS[num][2]);
  digitalWrite (out8, DIGITS_OTPUTS[num][3]);
}


void saveCathods(int n[])
{
  int h1 = n[0];
  int h2 = n[1];
  int m1 = n[2];
  int m2 = n[3];

  n[0]++;
  while (n[0] != h1)
  {
    for (int x = 0; x < 2; x++)
    {
      show(n);
    }
    n[0]++;
    if (n[0] > 9) n[0] = 0;
  }

  n[1]++;
  while (n[1] != h2)
  {

    for (int x = 0; x < 2; x++)
    {
      show(n);
    }
    n[1]++;
    if (n[1] > 9) n[1] = 0;
  }

  n[2]++;
  while (n[2] != m1)
  {

    for (int x = 0; x < 2; x++)
    {
      show(n);
    }
    n[2]++;
    if (n[2] > 9) n[2] = 0;
  }

  n[3]++;
  while (n[3] != m2)
  {

    for (int x = 0; x < 2; x++)
    {
      show(n);
    }
    n[3]++;
    if (n[3] > 9) n[3] = 0;
  }

}
