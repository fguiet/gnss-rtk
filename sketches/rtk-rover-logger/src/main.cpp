/*
* GNSS-RTK-LOGGER
* 
* Boards : 
*   - WEMOS D1 Mini - https://docs.platformio.org/en/latest/boards/espressif32/wemos_d1_mini32.html
    - LDO Regulator on WEMOS D1 Mini board : https://www.mcucity.com/product/3240/xc6204b332mr-g-3-3v-4b2x-4b2y-positive-voltage-regulator
*   - ArduSimple - SimpleRTK2B
*
* References:
*  
*  - ESP32 Wemos D1 Mini Pinout : https://github.com/r0oland/ESP32_mini_KiCad_Library
*  - Reading SD Card with ESP32 : https://randomnerdtutorials.com/esp32-microsd-card-arduino/
*  - SprkFun u-blox Arduino Library : http://librarymanager/All#SparkFun_u-blox_GNSS
*
*
* Frédéric Guiet - 07/2021
*
*/

// ****** External librairies used ******

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
#include <HardwareSerial.h>
//SD Card Library
#include <FS.h>
#include <SD.h>

// ****** DEBUG *************************

#define DEBUG 1
#define SERIAL_USB_SPEED  115200

//

/*
* OLED SSD1306 128x64
* U8G2LIB Library
*
* Manual  : https://github.com/olikraus/u8g2/wiki
* Library : https://github.com/olikraus/u8g2/wiki/u8g2reference
*
* I used this library instead of the Adafruit one because of the memory footprint.
* With Adafruit GFX Library this program will NOT work
*
* I am using a SSD1306 NO_NAME I2C, it supports both 5V or 3.3V
* 
* Pins mapping (SSD1306 => ESP32 Wemos pin)
* -----------
*
* GND => GND
* VDD => 5V or 3.3V
* SDA => SDA (on my Chinese Arduino clone I got 2 PINs SCL and SDA, on standard Arduino board one should use A5 and A4 pins)
* SCL/SCK => SCL (on my Chinese Arduino clone I got 2 PINs SCL and SDA, on standard Arduino board one should use A5 and A4 pins)
*/

//https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
//1 -  	Only one page of the display memory is stored in the microcontroller RAM. Use a firstPage()/nextPage() loop for drawing on the display.
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);  // High speed I2C

//https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
//2 -Same as 1, but maintains two pages in the microcontroller RAM. This will be up to two times faster than 1.
//U8G2_SSD1306_128X64_NONAME_2_HW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);  // High speed I2C

//Fastest solution
//https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
//F - Keep a copy of the full display frame buffer in the microcontroller RAM. Use clearBuffer() to clear the RAM and sendBuffer() to transfer the microcontroller RAM to the display.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);  // High speed I2C

/* 
* Chinese SD Card reader
* I am using the Adafruit SD Card Reader library
* https://fr.aliexpress.com/item/32867572635.html?spm=a2g0s.9042311.0.0.27426c370vOacP
*
* Pins mapping (SD Card => ESP32 D1 Wemos pin)
* ------------
* 
* GND  => GND
* VDD  => 3.3V
* CS   => 5
* MOSI => 23
* CLK  => 18
* MISO => 19
*/

//CS Pin for SD card
#define SD_CS_PIN 5

/* 
* ArduSimple - simpleRTK2b
* https://www.ardusimple.com/product/simplertk2b/
*
* Pins mapping (SimpleRTK2b => ESP32 D1 Wemos pin)
* ------------
* 
* GND  => GND
* 5V IN  => 5V
* <TX1 => 16
* >RX1 => 17
* OIREF => 5V (Very important)
*/

SFE_UBLOX_GNSS simpleRTK2B;

long lastTime = 0; //Simple local timer. Limits amount of I2C traffic to u-blox module.

/*
* Debug function
*/
void debug_message(String message, bool doReturnLine) {
  if (DEBUG) {
    if (doReturnLine)
      Serial.println(message);
    else
      Serial.print(message);
    
    Serial.flush();
    delay(10);
  }
}

/*
* Gets X coordinate depending on message to display so it is centered on screen
*/
int getCenterX(int msgLength, int stringPixelWidth) {
    return (display.getWidth() / 2) - ((stringPixelWidth) / 2);
}

/*
*  Gets Y coordinate depending on message to display so it is centered on screen
*/
int getCenterY(int totalLineToDisplay, int lineNumber, int charHeightPixelSize) {
    
    //Gets the center position
    int centerY = (display.getHeight() / 2);

    //Total lines height
    int totalHeight = (totalLineToDisplay * charHeightPixelSize);

    //Start offset
    int startOffset = (centerY + charHeightPixelSize) - (totalHeight / 2);

    return startOffset + ((lineNumber - 1) * charHeightPixelSize);
}

/**
 * Display a message centered on the screen
 * */
void displaySimpleCenteredMessage(String msg) {
  

  /*Serial.print(F("Max char height : "));
  Serial.println(display.getMaxCharHeight());

  Serial.print(F("Max char width : "));
  Serial.println(display.getMaxCharWidth());*/
    
  int x = getCenterX(msg.length(), display.getStrWidth(msg.c_str()));
  int y = getCenterY(1,1,display.getMaxCharHeight());

  //This version consume more RAM but it is faster (only work with _F_ Constructor)
  display.clearBuffer();                   // clear the internal memory    
  display.drawStr(x,y,msg.c_str());    // write something to the internal memory
  display.sendBuffer();                    // transfer internal memory to the display  

  //This version consume more RAM but it is slower
  /*display.firstPage();
  do {
    display.setCursor(x, y);
    display.print(msg);    
  } while ( display.nextPage() );*/  
}

//Expose Hardware Serial to pin 16 & 17
HardwareSerial hardwareSerial1(1);
#define SERIAL1_RX_PIN 16
#define SERIAL1_TX_PIN 17

void setup() {

  if (DEBUG) {
    // Open serial communications and wait for port to open:  
    Serial.begin(SERIAL_USB_SPEED);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }

  // Check that this platform supports 64-bit (8 byte) double
  if (sizeof(double) < 8)
  {
    Serial.println(F("Warning! Your platform does not support 64-bit double."));
    Serial.println(F("The latitude and longitude will be inaccurate."));
  }

  //Initialize graphic interface!
  display.begin();
  //Set default font 
  //u8g2_font_6x10_tf => 10 pixel height, 6 pixel width
  display.setFont(u8g2_font_6x10_tf);

  //Initialize SD Card
  displaySimpleCenteredMessage("SD Card initializing");    
  delay(2000);

  if(!SD.begin(SD_CS_PIN)) {
    debug_message("Card Mount Failed", true);
    displaySimpleCenteredMessage("Card Mount Failed");    
    //Freeze
    while(1);
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){    
    debug_message("No SD card attached", true);
    displaySimpleCenteredMessage("No SD card attached");    
    //Freeze
    while(1);
  }

  debug_message("SD Card Type: ", false);  
  if(cardType == CARD_MMC){
    debug_message("MMC", true);      
  } else if(cardType == CARD_SD){
    debug_message("SDSC", true);  
  } else if(cardType == CARD_SDHC){
    debug_message("SDHC", true);    
  } else {
    debug_message("UNKNOWN", true);      
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  displaySimpleCenteredMessage("SD Card initialized");    
  delay(2000);

  displaySimpleCenteredMessage("GNSS RTK initializing");    
  delay(2000);

  //Assume that the U-Blox GNSS is running at 9600 baud (the default) or at 38400 baud.
  //Loop until we're in sync and then ensure it's at 38400 baud.
  do {
    debug_message("GNSS: trying 38400 baud", true);               
    hardwareSerial1.begin(38400, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);    
    if (simpleRTK2B.begin(hardwareSerial1) == true) break;

    delay(100);

    debug_message("GNSS: trying 9600 baud", true);       
    hardwareSerial1.begin(9600, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);
    if (simpleRTK2B.begin(hardwareSerial1) == true) {        
        debug_message("GNSS: connected at 9600 baud, switching to 38400", true);               
        simpleRTK2B.setSerialRate(38400);
        delay(100);
    } else {
        //myGNSS.factoryReset();
        delay(2000); //Wait a bit before trying again to limit the Serial output
    }
  } while(1);
    
  simpleRTK2B.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
  //simpleRTK2B.setUART1Output(COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3);
  //simpleRTK2B.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  simpleRTK2B.setNavigationFrequency(2); //Set output in Hz. 200ms
  simpleRTK2B.saveConfiguration(); //Save the current settings to flash and BBR*/

  debug_message("GNSS serial connected", true);               
  displaySimpleCenteredMessage("GNSS RTK initialized");    
  delay(2000);

  byte rate = simpleRTK2B.getNavigationFrequency(); //Get the update rate of this module
  Serial.print("Current update rate (in Hz) : ");
  Serial.println(rate);   
}

void loop() {
  //Query module only every second. Doing it more often will just cause I2C traffic.
  //The module only responds when a new position is available
  if (millis() - lastTime > 1000)
  {
    lastTime = millis(); //Update the timer
    
    /*long latitude = simpleRTK2B.getLatitude();
    Serial.print(F("Lat: "));
    Serial.print(latitude);

    long longitude = simpleRTK2B.getLongitude();
    Serial.print(F(" Long: "));
    Serial.print(longitude);
    Serial.print(F(" (degrees * 10^-7)"));

    long altitude = simpleRTK2B.getAltitude();
    Serial.print(F(" Alt: "));
    Serial.print(altitude);
    Serial.print(F(" (mm)"));

    byte SIV = simpleRTK2B.getSIV();
    Serial.print(F(" SIV: "));
    Serial.print(SIV);

    Serial.println();*/

    // getHighResLatitude: returns the latitude from HPPOSLLH as an int32_t in degrees * 10^-7
    // getHighResLatitudeHp: returns the high resolution component of latitude from HPPOSLLH as an int8_t in degrees * 10^-9
    // getHighResLongitude: returns the longitude from HPPOSLLH as an int32_t in degrees * 10^-7
    // getHighResLongitudeHp: returns the high resolution component of longitude from HPPOSLLH as an int8_t in degrees * 10^-9
    // getElipsoid: returns the height above ellipsoid as an int32_t in mm
    // getElipsoidHp: returns the high resolution component of the height above ellipsoid as an int8_t in mm * 10^-1
    // getMeanSeaLevel: returns the height above mean sea level as an int32_t in mm
    // getMeanSeaLevelHp: returns the high resolution component of the height above mean sea level as an int8_t in mm * 10^-1
    // getHorizontalAccuracy: returns the horizontal accuracy estimate from HPPOSLLH as an uint32_t in mm * 10^-1

    // First, let's collect the position data
    int32_t latitude = simpleRTK2B.getHighResLatitude();
    int8_t latitudeHp = simpleRTK2B.getHighResLatitudeHp();
    int32_t longitude = simpleRTK2B.getHighResLongitude();
    int8_t longitudeHp = simpleRTK2B.getHighResLongitudeHp();
    int32_t ellipsoid = simpleRTK2B.getElipsoid();
    int8_t ellipsoidHp = simpleRTK2B.getElipsoidHp();
    int32_t msl = simpleRTK2B.getMeanSeaLevel();
    int8_t mslHp = simpleRTK2B.getMeanSeaLevelHp();
    uint32_t accuracy = simpleRTK2B.getHorizontalAccuracy();

    // Defines storage for the lat and lon as double
    double d_lat; // latitude
    double d_lon; // longitude

    // Assemble the high precision latitude and longitude
    d_lat = ((double)latitude) / 10000000.0; // Convert latitude from degrees * 10^-7 to degrees
    d_lat += ((double)latitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )
    d_lon = ((double)longitude) / 10000000.0; // Convert longitude from degrees * 10^-7 to degrees
    d_lon += ((double)longitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )

   // Print the lat and lon
    Serial.print("Lat (deg): ");
    Serial.print(d_lat, 9);
    Serial.print(", Lon (deg): ");
    Serial.print(d_lon, 9);

    // Now define float storage for the heights and accuracy
    float f_ellipsoid;
    float f_msl;
    float f_accuracy;

    // Calculate the height above ellipsoid in mm * 10^-1
    f_ellipsoid = (ellipsoid * 10) + ellipsoidHp;
    // Now convert to m
    f_ellipsoid = f_ellipsoid / 10000.0; // Convert from mm * 10^-1 to m

    // Calculate the height above mean sea level in mm * 10^-1
    f_msl = (msl * 10) + mslHp;
    // Now convert to m
    f_msl = f_msl / 10000.0; // Convert from mm * 10^-1 to m

    // Convert the horizontal accuracy (mm * 10^-1) to a float
    f_accuracy = accuracy;
    // Now convert to m
    f_accuracy = f_accuracy / 10000.0; // Convert from mm * 10^-1 to m

    // Finally, do the printing
    Serial.print(", Ellipsoid (m): ");
    Serial.print(f_ellipsoid, 4); // Print the ellipsoid with 4 decimal places

    Serial.print(", Mean Sea Level (m): ");
    Serial.print(f_msl, 4); // Print the mean sea level with 4 decimal places

    Serial.print(", Accuracy (m): ");
    Serial.println(f_accuracy, 4); // Print the accuracy with 4 decimal places

    long accuracy1 = simpleRTK2B.getPositionAccuracy();
    Serial.print(F(" 3D Positional Accuracy: "));
    Serial.print(accuracy1);
    Serial.println(F(" (mm)"));

    /*Serial.println();
    Serial.print(simpleRTK2B.getYear());
    Serial.print("-");
    Serial.print(simpleRTK2B.getMonth());
    Serial.print("-");
    Serial.print(simpleRTK2B.getDay());
    Serial.print(" ");
    Serial.print(simpleRTK2B.getHour());
    Serial.print(":");
    Serial.print(simpleRTK2B.getMinute());
    Serial.print(":");
    Serial.print(simpleRTK2B.getSecond());*/

    byte fixType = simpleRTK2B.getFixType();
    Serial.print(F(" Fix: "));
    if(fixType == 0) Serial.print(F("No fix"));
    else if(fixType == 1) Serial.print(F("Dead reckoning"));
    else if(fixType == 2) Serial.print(F("2D"));
    else if(fixType == 3) Serial.print(F("3D"));
    else if(fixType == 4) Serial.print(F("GNSS + Dead reckoning"));
    else if(fixType == 5) Serial.print(F("Time only"));

    byte RTK = simpleRTK2B.getCarrierSolutionType();
    Serial.print(" RTK: ");
    Serial.print(RTK);
    if (RTK == 0) Serial.print(F(" (No solution)"));
    else if (RTK == 1) Serial.print(F(" (High precision floating fix)"));
    else if (RTK == 2) Serial.print(F(" (High precision fix)"));

    //Serial.flush();

    display.clearBuffer();                   // clear the internal memory        

    //Latitude High Precision
    display.setCursor(0, 10);
    display.print("Lat : ");
    display.print(d_lat, 9);

    display.setCursor(0, 20);
    display.print("Lon : ");
    display.print(d_lon, 9);

    display.sendBuffer();                    // transfer internal memory to the display  

  }
  
}




