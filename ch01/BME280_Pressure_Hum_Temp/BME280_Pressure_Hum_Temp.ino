#include "DFRobot_BME280.h"
#include "Wire.h"

typedef DFRobot_BME280_IIC    BME; //
BME bme(&Wire, 0x77);      // Select TwoWire peripheral and set sensor address
#define SEA_LEVEL_PRESSURE    1015.0f

void printLastOperateStatus(BME::eStatus_t eStatus) // show last sensor operate status
{
  switch(eStatus) {
    case BME::eStatusOK:  Serial.println("Everything ok"); break;
    case BME::eStatusErr: Serial.println("Unknown error"); break;
    case BME::eStatusErrDeviceNotDetected:  Serial.println("Device not detected"); break;
    case BME::eStatusErrParameter: Serial.println("Parameter error"); break;
    default: Serial.println("Unknown status");  break;
  }
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  bme.reset();
  Serial.println("bme read data test");
  while(bme.begin() != BME::eStatusOK) {
    Serial.println("bme begin failed");
    printLastOperateStatus(bme.lastOperateStatus);
    delay(2000);
  }
  Serial.println("bme begin success");
  delay(100);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  float temp = bme.getTemperature();
  unit32_t press = bme.getPressure();
  float alti = bme.calAltitude(SEA_LEVEL_PRESSURE, press);
  float humi = bme.getHumidity();
  Serial.println();
  Serial.println("=======start print=======");
  Serial.println("Temperature (unit Celsius):  "); Serial.println(temp);
  Serial.println("Pressure (unit pa):          "); Serial.println(press);
  Serial.println("Altitude (unit meter):       "); Serial.println(alti);
  Serial.println("Humidity (unit percent):     "); Serial.println(humi);
  Serial.println("========== End print ========"); 
  delay(1000);
}




