/*
  SerialID.h v1.0 - Tags your source code with the filename, build date, and version - which it prints out to serial upon each startup - so you always know what code is in your MCU!
  (C) Copyright 2018 (GNU GENERAL PUBLIC LICENSE, Version 3) - Chris Drake <cdrake@cpan.org>
*/

#ifndef SerialID_h
#define SerialID_h

#define SerialIDset(VER) const char VERSION[] PROGMEM=VER;
#ifndef TEENSYDUINO
#define SerialIDshow(BAUD) Serial.begin(BAUD); \
  while((!Serial)&&(micros()<2000000)); \
  Serial.println((__FlashStringHelper*)VERSION);Serial.flush();
#else
#define SerialIDshow(BAUD) Serial.begin(BAUD); \
  while((!Serial)&&(micros()<2000000)); \
  Serial.println((__FlashStringHelper*)VERSION); \
  delay(5000); \
  Serial.println((__FlashStringHelper*)VERSION);Serial.flush(); // wait 5s for PC to re-connect serial
#endif

#endif

