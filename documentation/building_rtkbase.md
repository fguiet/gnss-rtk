# Building an RTK Base

In this tutorial, we are using this project : <https://github.com/Stefal/rtkbase> to buidl an RTK Base

## Config and burn a brand new Raspberry image

Use this tutorial to configure and burn a ready config Raspberry image : <https://github.com/fguiet/homelab/blob/main/packer/paker.md>

```bash
# For instance, command I use to generate the image
docker run --rm --privileged -v /dev:/dev -v /applications/mkaczanowski/packer-builder-arm:/build -v /applications/homelab/packer/packer-raspberry:/applications/homelab/packer/packer-raspberry -v /applications/raspios:/applications/raspios packer-builder-arm:20210418 build /applications/homelab/packer/packer-raspberry/raspios-lite-arm64.json
```

## Bill Of Material (BOM)

* Raspberry 4 (2 giga of RAM) - RaspiOS 64 bits Lite
* SimpleRTK2B-F9P 
  * [SimpleRTK2B-F9P](https://www.ardusimple.com/product/simplertk2b-f9p-v3/)
  * [ZED-F9P module](https://www.u-blox.com/en/product/zed-f9p-module)
  * [DA910 multi-band GNSS Antenna](https://store-drotek.com/910-da-910-multiband-gnss-antenna.html)

## Installation

Most of what is described below are taken from : <https://github.com/Stefal/rtkbase>

1. Get the source code from GitHub

As of 2021/04/30, I am using the [release 2.2.0](https://github.com/Stefal/rtkbase/releases/tag/2.2.0)

```bash
cd /applications
git clone https://github.com/Stefal/rtkbase.git

#Checkout tag 2.2.0
cd rtkbase
git checkout 2.2.0 -b 2.2.0
```

2. Install Dependencies

```bash
cd /applications/rtkbase/tools
sudo ./install.sh --dependencies

#I am adding netcat..so I can decode RTCM frame later
sudo apt-get install netcat
```

3. Installation RTKLib

```bash
cd /applications/rtkbase/tools
sudo ./install.sh --rtklib
```

4. Install the rtkbase web frontend

```bash
cd /applications/rtkbase/tools
sudo ./install.sh --rtkbase-release

# This command will download the release from this branch (see rtkbase.tar.gz)
# https://github.com/Stefal/rtkbase/releases/tag/2.2.0
# Source code will be placed in the folder /applications/rtkbase/tools/rtkbase
#
# The following /etc/environment file will be modified with the frond end path
# rtkbase_path=/applications/rtkbase/tools/rtkbase
```

5. Install Rtkbase web front requirements

```bash
cd /applications/rtkbase/tools
python3 -m pip install --upgrade pip setuptools wheel  --extra-index-url https://www.piwheels.org/simple
python3 -m pip install -r ../web_app/requirements.txt  --extra-index-url https://www.piwheels.org/simple
```

6. Install systemd services

```bash
cd /applications/rtkbase/tools
sudo ./install.sh --unit-files

#
# Following services are installed
#
#/etc/systemd/system/str2str_tcp.service
#/etc/systemd/system/str2str_rtcm_svr.service
#/etc/systemd/system/str2str_rtcm_serial.service
#/etc/systemd/system/str2str_ntrip.service
#/etc/systemd/system/str2str_file.service
#/etc/systemd/system/rtkbase_web.service
#/etc/systemd/system/rtkbase_archive.timer
#/etc/systemd/system/rtkbase_archive.service
```

```bash
# Get service status
#RTKBase Tcpservice
systemctl status str2str_tcp.service
#RTKBase rtcm server
systemctl status str2str_rtcm_svr.service
#RTKBase serial rtcm output
systemctl status str2str_rtcm_serial.service
#RTKBase Ntrip
systemctl status str2str_ntrip.service
#RTKBase File - Log data
systemctl status str2str_file.service
#RTKBase Web Server
systemctl status rtkbase_web.service
#Run rtkbase_archive.service everyday at 04H00
systemctl status rtkbase_archive.timer
#RTKBase - Archiving and cleaning raw data
systemctl status rtkbase_archive.service
```

7. Install chrony and gpsd

```bash
cd /applications/rtkbase/tools
sudo ./install.sh --gpsd-chrony
#Launch this one again, otherwise `/etc/chrony/chrony.conf` not correctly set
sudo ./install.sh --gpsd-chrony

# Check `/etc/chrony/chrony.conf`, the line below should be added
# refclock SHM 0 refid GPS precision 1e-1 offset 0.2 delay 0.2

# Check `chrony.service` => After=gpsd.service
#cat /etc/systemd/system/chrony.service

# Check `gpsd.service` => cat /etc/systemd/system/gpsd.service
#[Unit]
#Description=GPS (Global Positioning System) Daemon
#Requires=gpsd.socket
#BindsTo=str2str_tcp.service
#After=str2str_tcp.service

# Check `gpsd` conf file => cat /etc/default/gpsd
# DEVICES="tcp://127.0.0.1:5015"
# GPSD_OPTIONS="-n -b"

#systemctl status chrony.service
#systemctl status gpsd.service

#sudo systemctl deamon-reload
#sudo systemctl restart chrony.service
# it will not start because it depends on str2str_tcp.service
#sudo systemctl restart gpsd.service

#I want to install gpsd clients as well but it does not work at the moment...
#sudo apt-get install python-gps gpsd-clients gpsd gpsd-tools
```

8. Plug the U-Blox ZED-F9P via USB cable

```bash
# Which com port it uses ?
cd /applications/rtkbase/tools
sudo ./install.sh --detect-usb-gnss

# Here is the result for me
#Installation options:  --detect-usb-gnss
################################
#GNSS RECEIVER DETECTION
################################
#/dev/ttyACM0  -  u-blox_AG_-_www.u-blox.com_u-blox_GNSS_receiver

# Configure the ZED-F9P
sudo ./install.sh --detect-usb-gnss --configure-gnss
```

To connect to your ZED-F9P from a remote comuter with `u-center`

```bash
sudo socat tcp-listen:128,reuseaddr /dev/ttyACM0,b115200,raw,echo=0
```

9. Start all services 

```bash
cd /applications/rtkbase/tools
sudo ./install.sh --start-services

# Get service status
#RTKBase Tcpservice
systemctl status str2str_tcp.service
#RTKBase rtcm server
systemctl status str2str_rtcm_svr.service
#RTKBase serial rtcm output
systemctl status str2str_rtcm_serial.service
#RTKBase Ntrip
systemctl status str2str_ntrip.service
#RTKBase File - Log data
systemctl status str2str_file.service
#RTKBase Web Server
systemctl status rtkbase_web.service
#Run rtkbase_archive.service everyday at 04H00
systemctl status rtkbase_archive.timer
#RTKBase - Archiving and cleaning raw data
systemctl status rtkbase_archive.service
#GPSD
systemctl status gpsd.service
#Chrony
systemctl status chrony.service
```

Navigate to <http://your-raspberry-ip> (default password : `admin`)

![alt text](images/rtkbase.jpg)

## Compute accurate base position

Follow tutorial here : <https://docs.centipede.fr/docs/base/positionnement.html>

Creating a RINEX file

![alt text](images/rinex.jpg)

Go the IGN web site : <http://rgp.ign.fr/SERVICES/calcul_online.php> and fill in the form to get the accuracy computation.

![alt text](images/pivot-compute.jpg)

Time to set the accurate position of your base : see : <https://docs.centipede.fr/docs/base/param_positionnement>

## References

* Real-Time Kinematic Satellite Navigation : <https://www.petig.eu/rtk/>



Check RTCM frame transmission   
netcat localhost 5016  | gpsdecode