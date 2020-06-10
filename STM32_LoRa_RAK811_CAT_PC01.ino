#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <TSL2561.h>
#include <CayenneLPP.h>

TSL2561 tsl(TSL2561_ADDR_FLOAT); 

int txInterval = 60;

char buff[51];
CayenneLPP lpp(51);

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C #define BME280_ADDRESS                (0x76)

#define I2C_ADDRESS 0x3C
#define RST_PIN -1
SSD1306AsciiWire oled;

float temperature = 0;
float humidity = 0;
uint16_t tempC_int = 0;
uint8_t hum_int = 0;
uint16_t lux_int = 0;
int vbat_int;

String response = "";
String data_send ="";

void readData(){  
  
    data_send = "";
    char payload_1[50] = "";
 
//    uint32_t lum = tsl.getFullLuminosity();
//    uint16_t ir, full;
//    ir = lum >> 16;
//    full = lum & 0xFFFF;
//    lux_int = tsl.calculateLux(full, ir);
//
//    temperature = bme.readTemperature();
//    humidity = bme.readHumidity();
//    tempC_int = temperature*10;
//    hum_int = humidity*2;

   temperature = 25.5;
   humidity = 85.0;
   
   adc_enable(ADC1);
   vbat_int = 120 * 4096 / adc_read(ADC1, 17);
   adc_disable(ADC1);
   float vbat_f = vbat_int*0.01;  
   
   lpp.reset();
   lpp.addAnalogInput(1, vbat_f);
   lpp.addLuminosity(1, 20304);
   lpp.addTemperature(1, temperature);
   lpp.addRelativeHumidity(1, humidity);
   
   uint8_t buff = *lpp.getBuffer();
    
   Serial.print("Buffer size:" );
   Serial.println(lpp.getSize());

   for (int i = 0; i < lpp.getSize(); i++) {
      char tmp[16]; 
      sprintf(tmp, "%.2X",(lpp.getBuffer())[i]);
      strcat(payload_1, tmp);
      data_send += tmp;
    }

    Serial.print("Buffer content:" );
    Serial.println(data_send);

    Serial.print("Temp:");Serial.print(temperature);Serial.print(" Humid:");Serial.println(humidity);
    Serial.print("Vbat:");Serial.println(vbat_int*0.01);
    //Serial.print("Lux:");Serial.println(tsl.calculateLux(full, ir));
    Serial.println();
}

void setup_vdd_sensor() {
    adc_reg_map *regs = ADC1->regs;
    regs->CR2 |= ADC_CR2_TSVREFE; // enable VREFINT and temp sensor
    regs->SMPR1 = (ADC_SMPR1_SMP17 /* | ADC_SMPR1_SMP16 */); // sample rate for VREFINT ADC channel
}

void setup() {

setup_vdd_sensor();

Serial.begin(9600);
delay(3000);
Serial.println("Starting.....");
Serial2.begin(115200);
delay(100);


//  if (tsl.begin()) {
//    Serial.println("Found TLS2561 sensor");
//  } else {
//    Serial.println("No TLS2561 sensor?");
//    while (1);
//  }  
//
//  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
//  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)  
//  
//  bool status = bme.begin();  
//    if (!status) {
//        Serial.println("Could not find a valid BME280 sensor, check wiring!");
//        while (1);
//    }
//
//  #if RST_PIN >= 0
//    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
//  #else // RST_PIN >= 0
//    oled.begin(&Adafruit128x64, I2C_ADDRESS);
//  #endif // RST_PIN >= 0
//
//  oled.setFont(Adafruit5x7);

//  sendCommand("at+set_config=lora:work_mode:0\r\n"); //Lorawan 
    delay(200);
    sendCommand("at+set_config=lora:region:AS923\r\n");
    delay(200);
    sendCommand("at+set_config=lora:class:0\r\n"); // Class A 
    delay(200);
    sendCommand("at+set_config=lora:join_mode:1\r\n"); //0:OTAA 1:ABP 
    delay(2000);
    sendCommand("at+set_config=lora:dev_addr:260411E2\r\n");
    delay(200);
    sendCommand("at+set_config=lora:nwks_key:28AED22B7E1516A609CFABF715884F3C\r\n");
    delay(200);
    sendCommand("at+set_config=lora:apps_key:1628AE2B7E15D2A6ABF7CF4F3C158809\r\n");
    delay(200);
    sendCommand("at+set_config=lora:dev_eui:0016B93B00F5B12F\r\n");
    delay(200);  
    
//  join the connection
  sendJoinReq();
  delay(200);

  Serial.println(F("Leave setup"));
}


void loop() {
    readData(); 
    sendCommand("at+send=lora:1:"+data_send+"\r\n");
    delay(60000);
}

void sendCommand(String atComm){
response = "";
Serial2.print(atComm);
  while(Serial2.available()){
    char ch = Serial2.read();
    response += ch;
  }
  Serial.println(response);
}

void sleep(unsigned long milliseconds){
  sendCommand("at+sleep\r\n");
  delay(milliseconds);
  //send any charcater to wakeup;
  sendCommand("***\r\n");
}

void resetChip(int mode, unsigned long delaySec=0){
  delay(delaySec);
  String command = (String)"at+reset=" + mode + (String)"\r\n";
  sendCommand(command);
}

void reload(unsigned long delaySec){
  delay(delaySec);
  sendCommand("at+reload\r\n");
}

void setMode(int mode){
  String command = (String)"at+mode=" + mode + (String)"\r\n";
  sendCommand(command);  
}

void sendData(int port, String data){
  String command = (String)"at+send=lora," + port + "," + data + (String)"\r\n";
  sendCommand(command);
}

void setConnConfig(String key, String value){
  sendCommand("at+set_config=" + key + ":" + value + "\r\n");
}


void sendJoinReq(){
  sendCommand("at+join\r\n");
 }
