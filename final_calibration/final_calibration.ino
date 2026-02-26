#include "FS.h"
#include "SD.h"
#include "GY521.h"
#include <SPI.h>
#include <Wire.h>

#define CS_PIN    4
#define RST       14
#define DIO0      27

#define SD_CS     33
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23

#define HOT_PIN   32
#define TEST_PIN  35

#define QMC5883L_ADDRESS 0x0D

GY521 sensor(0x68);

const char* labels[]={
"gx","gy","gz",
"ax","ay","az",
"gtemp",
"magx","magy","magz",
};

struct float3d{
  float x,y,z;};
struct int16_t3d{
  int16_t x,y,z;};
  
struct SensorData {
  float3d accel;
  float3d gyro;
  float gtemp;
  float3d mag;
};
SensorData data;

char row[512];

void getMag(float3d &output) {
    int16_t3d result;
  
    Wire.beginTransmission(QMC5883L_ADDRESS);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(QMC5883L_ADDRESS, 6);
  
    if (Wire.available() >= 6) {
      result.x = Wire.read() | (Wire.read() << 8);
      result.y = Wire.read() | (Wire.read() << 8);
      result.z = Wire.read() | (Wire.read() << 8);
      output.x=(float)result.x;
      output.y=(float)result.y;
      output.z=(float)result.z;
   }
}
bool i2cDevicePresent(uint8_t address) {
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}
void sdConnect(){
  Serial.println("sd connect");
  if(SD.begin(SD_CS)){
    if (!SD.exists("/final-calibration.csv")) {
      File file = SD.open("/final-calibration.csv",FILE_WRITE);
      if(file){
        for(int i=0;i<sizeof(labels)/sizeof(labels[0]);i++){
          file.print(labels[i]);
          if(i<sizeof(labels)/sizeof(labels[0])-1)file.print(",");
          else file.print("\n");
        }
        file.close();}
    }
  }
  
  Serial.println("sd connected");
}
void accelConnect(){
  
  Serial.println("accel connect");
  while(!sensor.wakeup());
    sensor.setAccelSensitivity(0);  //  2g
    sensor.setGyroSensitivity(0);   //  250 degrees/s
    sensor.setThrottle();
  
  
}
void magnConnect(){
  Serial.println("magn connect");
  Wire.beginTransmission(QMC5883L_ADDRESS);
  Wire.write(0x09);
  Wire.write(0b00011101); // OSR=512, 8G, 200Hz, continuous
  Wire.endTransmission();

  Wire.beginTransmission(QMC5883L_ADDRESS);
  Wire.write(0x0B);
  Wire.write(0x01);
  Wire.endTransmission();
}
void startI2CDevices(){
  delay(500);
  Wire.begin(21,22);
  Wire.setClock(100000); // Set to 100kHz (Standard Mode) for better stability
  Wire.setTimeOut(50);   // Give it 50ms to recover before erroring out
  delay(500);
  accelConnect();
  magnConnect();
}
void dataToCsv(SensorData data,char buffer[],int len){
  snprintf(buffer, len,
    "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f\n",
    data.gyro.x,data.gyro.y,data.gyro.z,
    data.accel.x,data.accel.y,data.accel.z,
    data.gtemp,
    data.mag.x,data.mag.y,data.mag.z
  );
}
void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(HOT_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(SD_CS, HIGH);
  startI2CDevices();
  sdConnect();
  delay(100);
}

void loop() {
  sensor.read();
    data.accel.x = sensor.getAccelX();
    data.accel.y = sensor.getAccelY();
    data.accel.z = sensor.getAccelZ();
  
    data.gyro.x = sensor.getGyroX();
    data.gyro.y = sensor.getGyroY();
    data.gyro.z = sensor.getGyroZ();

    data.gtemp = sensor.getTemperature();///340.0+36.53;
  
  
  getMag(data.mag);
  digitalWrite(HOT_PIN,LOW);
//  digitalWrite(HOT_PIN,millis()%1200000<600000);
  dataToCsv(data,row,sizeof(row));
  Serial.print(row);
    File file = SD.open("/final-calibration.csv", FILE_APPEND);
    if (file) {
      file.print(row);
      file.close();
    }
    delay(100);
}
