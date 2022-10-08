# Raspberry-pi-and-libwebsockets
A real time, embedded system for analyzing trade transactions. Embedded on a Raspberry pi zero 2w. Implemented using the libwebsocket library in c.
A client that connects to a Finnhub web socket and receives information about transactions.

#For Real Time and Embedded Systems course, AUTH, 2022

#Raspberry pi zero 2w preparations
1. Buy a raspberry pi zero 2w (check: https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/).
2. Buy a memory card (myne is a Sandisk Ultra 32GB micro SD).
3. Download the Raspberry Pi Imager on your pc (https://www.raspberrypi.com/software/).
4. Connect the SD card into your pc.
5. Using the Raspberry Pi Imager, install Raspberry Pi OS on your SD card.
6. Remebmer to connect the Raspberry to the wifi.
7. Transfer the myclient.c and the makefile to the raspberry.


# libwebsockets installation on raspberry
1. `git clone https://github.com/warmcat/libwebsockets.git`
2. `cd to the libwebsockets directory` (Should be `cd libwebsockets`)
3. `sudo apt-get install libssl-dev`
4. `mkdir build`
5. `cd build`
6. `cmake ..`
7. `make`
8. `sudo make install`
9. `sudo ldconfig`

# Finnhub
1. Sign up to finnhub.io to get a key (https://finnhub.io/)
2. Put the key into the right place at the myclient.c file. (row: 533)


# Compile and run (into the raspberry terminal)
1. `cd to the myclient.c and makefile directory`
2. `make -f Makefile.mk` (to compile)
3. `./myclient`
4. `ctrl + C` (to terminate)
