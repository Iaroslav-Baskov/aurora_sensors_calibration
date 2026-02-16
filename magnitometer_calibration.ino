#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <Wire.h>

#define CS_PIN    4

#define SD_CS     5
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23

#define QMC5883L_ADDRESS 0x0D
#define SCALE_FACTOR 1.5

struct SensorData {
  int16_t magx, magy, magz;
};


char row[2048];

void getMag(){
  SensorData data;

  Wire.beginTransmission(QMC5883L_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(QMC5883L_ADDRESS, 6);

  if (Wire.available() >= 6) {
    data.magx = Wire.read() | (Wire.read() << 8);
    data.magy = Wire.read() | (Wire.read() << 8);
    data.magz = Wire.read() | (Wire.read() << 8);
  
    dataToCsv(data,row,sizeof(row));
    Serial.print(row);
  
    File file = SD.open("/calibration.csv", FILE_APPEND);
    if (file) {
      file.print(row);
      file.close();
    }else{
      SD.begin(SD_CS);
    }
  }
}
void dataToCsv(SensorData data,char buffer[],int len){
snprintf(buffer, len,
    "%d,%d,%d\n",
    data.magx,data.magy,data.magz
  );
}
void setup() {
  Serial.begin(9600);

  Wire.begin();
    Wire.beginTransmission(QMC5883L_ADDRESS);
  Wire.write(0x09);
  Wire.write(0b00011101); // OSR=512, 8G, 200Hz, continuous
  Wire.endTransmission();

  Wire.beginTransmission(QMC5883L_ADDRESS);
  Wire.write(0x0B);
  Wire.write(0x01);
  Wire.endTransmission();
  
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  
  pinMode(CS_PIN, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(CS_PIN, HIGH);
  digitalWrite(SD_CS, HIGH);

  SD.begin(SD_CS);
}

void loop() {
  getMag();
}
