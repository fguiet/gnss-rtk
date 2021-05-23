# Building an RTK Rover

## Hardware

In order to build an RTK Rover you need :

* a simplertk2b board with the ZED-F9P ublox chip
* a WiFi NTRIP Master clone attached to the simplertk2b board (See [building ESP32 XBEE](building_esp32_xbee.md))

## Building the rover

* Connect the WiFi NTRIP Master clone to the simplertk2b board.

* Configure the simplertk2b board (See [building ESP32 XBEE](building_esp32_xbee.md) and [README](../README.md)

If everything's ok, you should see the `GPS Fix` led blinking, `NO TRK` led OFF, `XBEE>GPS` blinking (ie RTCM message are received)

## Arduino connexion

* Configure the `UART1` or `UART2` port to output NMEA messages.

* Connect the PIN 8 (default AltSerial RX pin) to `<TX1` (UART1) or `<TX2` (UART2)

* Upload the following sketch, you should see your latitude and longitude on the Serial monitor

* Add the NeoGPS Library : <https://github.com/SlashDevin/NeoGPS/>
* Add the AltSoftSeral Library : <https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html>

```c++
#include <NMEAGPS.h>
#include <GPSport.h>

NMEAGPS  gps; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values

void setup()
{
  DEBUG_PORT.begin(9600);
  while (!Serial)
    ;
  DEBUG_PORT.print( F("NMEAsimple.INO: started\n") );

  gpsPort.begin(9600);
}

//--------------------------

void loop()
{
  while (gps.available( gpsPort )) {
    fix = gps.read();

    DEBUG_PORT.print( F("Location: ") );
    if (fix.valid.location) {
      DEBUG_PORT.print( fix.latitude(), 6 );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( fix.longitude(), 6 );
    }

    DEBUG_PORT.print( F(", Altitude: ") );
    if (fix.valid.altitude)
      DEBUG_PORT.print( fix.altitude() );

    DEBUG_PORT.println();
  }
}
```


NMEA Enabled High precision may be needed, see <https://learn.sparkfun.com/tutorials/setting-up-a-rover-base-rtk-system/all>

Arduino Library to decode NMEA message


Active only some message to the output
https://wiki.ardumower.de/index.php?title=Ardumower_Sunray#Rover_configuration_.28messages.29

## References

* <http://agrilab.unilasalle.fr/projets/projects/autoguidage-rtk-libre/wiki/Ressources>
* <https://learn.sparkfun.com/tutorials/setting-up-a-rover-base-rtk-system/all>
* <https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library>
* <https://www.instructables.com/RTK-GPS-Driven-Mower/>
* Faucheuse guidée par GPS RTK : <https://wikifab.org/wiki/Faucheuse_guid%C3%A9e_par_GPS_RTK>

