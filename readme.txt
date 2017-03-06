Sous vide machine
Anders Heed 2015
--------------------------------


1 Hardware

1.1 Raspberry Pi

model A

pi@hikaru /media/networkshare/public/src/sousvide $ uname -a
Linux hikaru 3.10.25+ #622 PREEMPT Fri Jan 3 18:41:00 GMT 2014 armv6l GNU/Linux


1.1.1 Schematics

GPIO17, 5V and ground connected to the relay board.

GPIO22 used for LED output

GPIO4 (input), ground and 3.3V connected to the temperature sensors.
Note that both sensors share the same data bus (GPIO4).


1.2 Heater
Taken from a Kettle, 230V/~2000W.


1.3 Relay for heater power
Beefcake Relay Control Kit
http://www.lawicel-shop.se/prod/Beefcake-Relay-Control-Kit_869396/Rela--Relakort_83015/SWE/SEK
The actual relay is a Tyco Electronics T9A series.
Electrical life: 100000 operations at max power.


1.4 Aquarium pump
Eheim compact 300
Note: this pump is specified for max 35 degrees Celsius and it is supposed to have built-in overheat protection but it seems to work fine at 65 degrees or more.


1.5 Temperature sensors
Two DS18B20 digital sensors with 1-wire support



2 Software

2.1 Prerequisites

To enable the 1-wire drivers used for temperature sensors add these lines to the end of the /etc/modules file:
w1-gpio
w1_therm

It may be necessary to add a line to the /boot/config.txt file to make the temp sensors work on a newer distro for Rpi.
See info below from this web page:  
http://www.reuk.co.uk/DS18B20-Temperature-Sensor-with-Raspberry-Pi.htm

"
NEW 6th May 2015 Update
Since the Raspbian operating system was updated back at the end of January 2015 (kernel 3.18.8 and higher) which enabled Device Tree, the above steps do not result in the temperature sensor(s) being detected.

Now you need to enter the following at the command prompt:
sudo nano /boot/config.txt
to open the /boot/config.txt file for editing. Then scroll down to the bottom of the file, and add the line:
dtoverlay=w1-gpio
Ctrl-X to save the amended file.
Finally reboot the Raspberry Pi so that the changes take effect.
"

The serial numbers of the temperature sensors used are hard coded in temperature.c.


2.2 Compiling
$ gcc main.c temperature.c -o control

2.3 Running, example (target temperature 56.5 degrees):
$ sudo ./control 56500

Low level GPIO => requires root access to run
http://www.susa.net/wordpress/2012/06/raspberry-pi-relay-using-gpio/

Error codes
0: normal operation, 1 LED flash per second
1: Temperature sanity check failed, 2 LED flashes per second
2: Temperature diff between sensors too high, 3 LED flashes per second

Logging example
$ ssh pi@192.168.0.19 | tee log.tx


2.4 Tips & tricks

To let the RPi mount an SMB drive on your PC:
http://geeks.noeit.com/mount-an-smb-network-drive-on-raspberry-pi/
This allows you to do all the source code editing, version control etc on your PC. Use ssh to run the program on the RPi.

example:
pi@hikaru ~ $ cd /media/networkshare/public/src/sousvide/
pi@hikaru /media/networkshare/public/src/sousvide $ sudo ./control 56000

----------------------


Todo:
Add usage*
Change input format for temp*
All output should be stdout*
Add config file support
Change max value for integrator*
Remove GPIO15 output*
Finish this readme file with distro info etc*
Rename the program*
Headers for printouts*
Fixed width output*
Comments in source code file(s) about compiling and running*
Use average sensor temp*
Status file support (HTML?)
Print date and time on startup*
Clean up file headers*
Catch ctrl+C: print date and switch off heater then quit*





