#include "TinyGPS++.h"
#include <Wire.h>

#include <TinyGPS++.h>

//  BMA180
#define ACC (0x40)    //define BMA180 address
#define A_TO_READ (6)        //read bytes (2) every time
 
// gryo ITG3205 
#define GYRO 0x68 // configue address, connect AD0 to GND, address in Binary is 11101000 (this please refer to your schematic of sensor)
#define G_SMPLRT_DIV 0x15
#define G_DLPF_FS 0x16
#define G_INT_CFG 0x17
#define G_PWR_MGM 0x3E
 
#define G_TO_READ 8 // x,y,z every axis 2 bytes
 
// for the offest of gyroscope
int g_offx = 0;
int g_offy = 0;
int g_offz = 0;

 // for the offest of accelerometer
int a_offx = 0;
int a_offy = 0;
int a_offz = 0;

//GPS
static const uint32_t GPSBaud = 115200;

// The TinyGPS++ object
TinyGPSPlus gps;


char str[512]; 
void AccelerometerInit(){
  /*writeTo(ACC, 0x10, 0xB6);
  delay(10);
  
  writeTo(ACC, 0x0D, 0x10);
  
  Wire.beginTransmission(0x40); // address of the accelerometer
  Wire.write(0x35); // read from here
  Wire.endTransmission();
  Wire.requestFrom(0x40, 1);
  byte data = Wire.read();
  writeTo(ACC, 0x20, (data & 0x0F));
  
  Wire.beginTransmission(0x40); // address of the accelerometer
  Wire.write(0x35); // read from here
  Wire.endTransmission();
  Wire.requestFrom(0x40, 1);
  data = Wire.read();
  writeTo(ACC, 0x35, ((data & 0xF1) | 0x04));*/
   Wire.beginTransmission(0x40); // address of the accelerometer
  // reset the accelerometer
  Wire.write(0x10);
  Wire.write(0xB6);
  Wire.endTransmission();
  delay(10); 
 
  Wire.beginTransmission(0x40); // address of the accelerometer
  // low pass filter, range settings
  Wire.write(0x0D);
  Wire.write(0x10);
  Wire.endTransmission(); 
 
  Wire.beginTransmission(0x40); // address of the accelerometer
  Wire.write(0x20); // read from here
  Wire.endTransmission();
  Wire.requestFrom(0x40, 1);
  byte data = Wire.read();
  Wire.beginTransmission(0x40); // address of the accelerometer
  Wire.write(0x20);
  Wire.write(data & 0x0F); // low pass filter to 10 Hz
  Wire.endTransmission(); 
 
  Wire.beginTransmission(0x40); // address of the accelerometer
  Wire.write(0x35); // read from here
  Wire.endTransmission();
  Wire.requestFrom(0x40, 1);
  data = Wire.read();
  Wire.beginTransmission(0x40); // address of the accelerometer
  Wire.write(0x35);
  Wire.write((data & 0xF1) | 0x04); // range +/- 2g
  Wire.endTransmission();
   
}
 
void getAccelerometerData(float * result) {
  int regAddress = 0x02;    //The setting of data ofthe first axis of ADXL345
  byte buff[A_TO_READ];
 
  readFrom(ACC, regAddress, A_TO_READ, buff); //read the data from adxl345
 
  //the value of every axis has 10 resolution, which means 2 bytes
  //we have to convert 2 bytes into 1 int value.
  result[0] = (((int)buff[1]) << 8) | buff[0] + a_offx; 
  result[0] =result[0]*0.25*0.001/4; 
  result[1] = (((int)buff[3]) << 8) | buff[2] + a_offy;
  result[1] =result[1]*0.25*0.001/4; 
  result[2] = (((int)buff[5]) << 8) | buff[4] + a_offz;
  result[2] =result[2]*0.25*0.001/4; 
}

 
//initilize gyro
void initGyro()
{
  /*****************************************
   * ITG 3200
   * power managerment setting
   * clock setting = internal clock
   * no reset mode, no sleep mode
   * no standby mode
   * resolution = 125Hz
   * parameter is + / - 2000 degreee/second
   * low pass filter=5HZ
   * no interruption
   ******************************************/
  writeTo(GYRO, G_PWR_MGM, 0x00);
  writeTo(GYRO, G_SMPLRT_DIV, 0x07); // EB, 50, 80, 7F, DE, 23, 20, FF
  writeTo(GYRO, G_DLPF_FS, 0x1E); // +/- 2000 dgrs/sec, 1KHz, 1E, 19
  writeTo(GYRO, G_INT_CFG, 0x00);
}


 
void getGyroscopeData(float * result)
{
  /**************************************
   * gyro ITG- 3200 I2C
   * registers：
   * temp MSB = 1B, temp LSB = 1C
   * x axis MSB = 1D, x axis LSB = 1E
   * y axis MSB = 1F, y axis LSB = 20
   * z axis MSB = 21, z axis LSB = 22
   *************************************/
 
  int regAddress = 0x1B;
  int temp, x, y, z;
  byte buff[G_TO_READ];
 
  readFrom(GYRO, regAddress, G_TO_READ, buff); //read gyro itg3200 data
 
  result[0] = ((buff[2] << 8) | buff[3]) + g_offx;
  result[0] = result[0]*0.061;
  result[1] = ((buff[4] << 8) | buff[5]) + g_offy;
  result[1] = result[1]*0.061;
  result[2] = ((buff[6] << 8) | buff[7]) + g_offz;
  result[2] = result[2]*0.061;
  result[3] = (buff[0] << 8) | buff[1]; // temperature
 
}
 
 
void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  Wire.begin();
  initGyro();
  AccelerometerInit();
  
  //GPS settings
  Serial2.begin(GPSBaud);

  Serial.println(F("FullExample.ino"));
  Serial.println(F("An extensive example of many interesting TinyGPS++ features"));
  Serial.print(F("Testing TinyGPS++ library v. ")); 
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();
  Serial.println(F("Sats HDOP Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card Chars Sentences Checksum  Accelerometer          Gyroscope"));
  Serial.println(F("          (deg)      (deg)       Age                      Age  (m)    --- from GPS ---- RX    RX        Fail      x       y       z      x       y       z"));
  Serial.println(F("-------------------------------------------------------------------------------------------------------------------------------------------------------------"));

}

String one;
String data;
String index;
char number[20];
unsigned long nowTime, elapsedTime;
unsigned long startTime;
float acc[3];
float gyro[4];
static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

void loop()
{
  startTime = millis();
 
  getAccelerometerData(acc);
  getGyroscopeData(gyro);
  

  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);

  index=String("g");
  Serial1.print(index);
  /*printInt(gps.hdop.value(), gps.hdop.isValid(), 5);
  index=String("h");
  Serial1.print(index);*/
  data=String(gps.location.isValid());
  Serial1.print(data);
  index=String("v");
  Serial1.print(index);
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  index=String("i");
  Serial1.print(index);
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  index=String("j");
  Serial1.print(index);
  //printInt(gps.location.age(), gps.location.isValid(), 5);
  printDateTime(gps.date, gps.time);//Index k, l and m for Date Age
  data=String(gps.altitude.isValid());
  Serial1.print(data);
  index=String("w");
  Serial1.print(index);
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
  index=String("n");
  Serial1.print(index);
  //printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
  //printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
  //printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "*** ", 6);
  
 
  //printInt(gps.charsProcessed(), true, 6);
  //printInt(gps.sentencesWithFix(), true, 10);
  //printInt(gps.failedChecksum(), true, 9);
  /*
  printFloat(acc[0], 1, 7, 3);

  index=String("a");
  Serial1.print(index);
  
  printFloat(acc[1], 1, 7, 3);

  index=String("b");
  Serial1.print(index);
  
  printFloat(acc[2], 1, 7, 3);

  index=String("c");
  Serial1.print(index);
  
  printFloat(gyro[0], 1, 7, 3);

  index=String("d");
  Serial1.print(index);
  
  printFloat(gyro[1], 1, 7, 3);

  index=String("e");
  Serial1.print(index);
  
  printFloat(gyro[2], 1, 7, 3);
  */
  index=String("f");
  Serial1.print(index);
  nowTime=millis();
  elapsedTime=nowTime-startTime;
  printInt(elapsedTime, 1, 9);
  index=String("o");
  Serial1.print(index);
  Serial.println();

  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}
 
 
//---------------- function

/*void TSendInt(int input, String index){
  String one=String(input);
  String data=String(one+index);
  Serial1.print(data);
}*/
//write val into the address register of accelerometer
void writeTo(int DEVICE, byte address, byte val) {
  Wire.beginTransmission(DEVICE); //send to sensor
  Wire.write(address);        // send register address 
  Wire.write(val);        // send the value which needed to write
  Wire.endTransmission(); //end transmission 
}
 
 
//read data from the buffer array of address registers in the accelerometer sensor
void readFrom(int DEVICE, byte address, int num, byte buff[]) {
  Wire.beginTransmission(DEVICE); //start to send to accelerometer sensor
  Wire.write(address);        //send address which are read
  Wire.endTransmission(); //end transmission
 
  Wire.beginTransmission(DEVICE); //start to send to ACC
  Wire.requestFrom(DEVICE, num);    // require sending 6 bytes data from accelerometer sensor
 
  int i = 0;
  while(Wire.available())    //Error when the return value is smaller than required value
  { 
    buff[i] = Wire.read(); // receive data
    i++;
  }
  Wire.endTransmission(); //end transmission
}


// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial2.available())
      gps.encode(Serial2.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  data="";
  if (!valid)
  {
    
    while (len-- > 1){
      Serial.print('*');
      data=String(data+"*");
    }
    Serial.print(' ');
    data=String(data+" ");
  }
  else
  {
    Serial.print(val, prec);
    dtostrf(val,prec+5,prec,number);
    data=String(number);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i){
      Serial.print(' ');
      data=String(data+" ");
    }
  }
  Serial1.print(data);
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  data="";
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  data=String(sz);
  //data=String(one);
  Serial1.print(data);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  data="";
  if (!d.isValid())
  {
    Serial.print(F("********** "));
    data=String("********** ");
    
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
    data=String(sz);
    data=String(data+" ");
  }
  Serial1.print(data);
  index=String("k");
  Serial1.print(index);
  
  
  data="";
  if (!t.isValid())
  {
    Serial.print(F("******** "));
    data=String("********");
    
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d%02d%02d.%02d ", t.hour(), t.minute(), t.second(),t.centisecond());
    Serial.print(sz);
    data=String(sz);
    data=String(data+" ");
  }
  Serial1.print(data);
  index=String("l");
  Serial1.print(index);

  printInt(d.age(), d.isValid(), 5);
  index=String("m");
  Serial1.print(index);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}
