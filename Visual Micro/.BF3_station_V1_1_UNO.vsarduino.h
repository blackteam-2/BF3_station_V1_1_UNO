/* 
	Editor: http://www.visualmicro.com
	        arduino debugger, visual micro +, free forum and wiki
	
	Hardware: Arduino Uno, Platform=avr, Package=arduino
*/

#define __AVR_ATmega328P__
#define ARDUINO 101
#define ARDUINO_MAIN
#define F_CPU 16000000L
#define __AVR__
#define __cplusplus
extern "C" void __cxa_pure_virtual() {;}

void setup(void);
void loop(void);
boolean checkData(void);
void sendData();
void sendReady(int q);
void updateTic();
void updateTimer();
void timer5(int j);

#include "E:\Dropbox\Projects\arduino-1.0.5\hardware\arduino\variants\standard\pins_arduino.h" 
#include "E:\Dropbox\Projects\arduino-1.0.5\hardware\arduino\cores\arduino\arduino.h"
#include "E:\Dropbox\Projects\BF3 station\Code\BF3_station_V1_1_UNO\BF3_station_V1_1_UNO.ino"
