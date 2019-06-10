/*
  xdrv_18_armtronix_dimmers.ino - Armtronix dimmers support for Sonoff-Tasmota

  Copyright (C) 2019  wvdv2002 and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_ARMTRONIX_DIMMERS
/*********************************************************************************************\
 * This code can be used for Armtronix dimmers.
 * The dimmers contain a Atmega328 to do the actual dimming.
 * Checkout the Tasmota Wiki for information on how to flash this Atmega328 with the firmware
 *   to work together with this driver.
\*********************************************************************************************/

#define XDRV_18                18

#include <TasmotaSerial.h>

TasmotaSerial *ArmtronixSerial = nullptr;

//bool armtronix_ignore_dim = false;            // Flag to skip serial send to prevent looping when processing inbound states from the faceplate interaction
int8_t armtronix_wifi_state = -2;                // Keep MCU wifi-status in sync with WifiState()
//int8_t armtronix_dimState[2];                    // Dimmer state values.
int8_t armtronix_oldKnobState[2];                   // Dimmer state values.
const int8_t ARMTRONIX_KNOBVARIATIONBEFORECHANGE = 5;        //The amount the knob needs to change before the change is applied.
bool armtronix_firstStatus = true;

/*********************************************************************************************\
 * Internal Functions
\*********************************************************************************************/

bool ArmtronixSetChannels(void)
{
  LightSerial2Duty(((uint8_t*)XdrvMailbox.data)[0], ((uint8_t*)XdrvMailbox.data)[1]);
  return true;
}

void LightSerial2Duty(uint8_t duty1, uint8_t duty2)
{
  if (ArmtronixSerial) {
    duty1 = changeUIntScale(duty1,0,255,0,100); //max 100
    duty2 = changeUIntScale(duty2,0,255,0,100); //max 100
//    armtronix_dimState[0] = duty1;
//    armtronix_dimState[1] = duty2;
    ArmtronixSerial->print("DIMMER:1,");
    ArmtronixSerial->println(duty1);
    ArmtronixSerial->print("DIMMER:2,");
    ArmtronixSerial->println(duty2);

    AddLog_P2(LOG_LEVEL_DEBUG, PSTR("ARM: Send Serial Packet Dim Values=%d,%d"), duty1,duty2);
  }
}

void ArmtronixRequestState(void)
{
  if (ArmtronixSerial) {
    // Get current status of MCU
    AddLog_P(LOG_LEVEL_DEBUG, PSTR("ARM: Request Dimmer state"));
    ArmtronixSerial->println("STATUS:");
  }
}

/*********************************************************************************************\
 * API Functions
\*********************************************************************************************/

bool ArmtronixModuleSelected(void)
{
  String answer;
 light_type=LT_SERIAL2;
  ArmtronixSerial = new TasmotaSerial(pin[GPIO_RXD], pin[GPIO_TXD], 2);
  if (ArmtronixSerial->begin(115200)) {
    if (ArmtronixSerial->hardwareSerial()) { ClaimSerial(); }
     ArmtronixSerial->println("DIMCOUNT:"); 
    int i=0;
    while (!ArmtronixSerial->available()){
        yield();
        delay(10);
        if(i++>100){break;}
    }
    answer = ArmtronixSerial->readStringUntil('\n');
    if (answer.substring(0,7) == "DIMCOUNT:") {
      int temp = answer.substring(7,8).toInt();
      if(temp==1){
        light_type = LT_SERIAL1;
      }
    }
  }
  return true;
}

void ArmtronixInit(void)
{
    ArmtronixSerial->println("STATUS:");
}

void ArmtronixSerialInput(void)
{
  String answer;
  uint8_t newDimState[2];
  uint8_t newKnobState[2]; 
  uint8_t temp;
  int commaIndex;
  char scmnd[20];
  if (ArmtronixSerial->available()) {
    yield();
    answer = ArmtronixSerial->readStringUntil('\n');
    if (answer.substring(0,7) == "STATUS:") {
      commaIndex = 6;
      for (int i =0; i<2; i++) {
        newDimState[i] = answer.substring(commaIndex+1,answer.indexOf(',',commaIndex+1)).toInt();
 //       newDimState[i] = changeUIntScale(newDimState[i], 0, 100, 0, 255);
//          temp = ((float)newDimState[i]); //max 255
//          snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_CHANNEL "%d %d"),i+1, temp);
//          ExecuteCommand(scmnd,SRC_SWITCH);
//          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("ARM: Send CMND_CHANNEL=%s"), scmnd );
          commaIndex = answer.indexOf(',',commaIndex+1);
          int temp = answer.substring(commaIndex+1,answer.indexOf(',',commaIndex+1)).toInt();
          temp = changeUIntScale(temp,0,512,0,100);
          newKnobState[i] = min(temp,100);
          commaIndex = answer.indexOf(',',commaIndex+1);
      }
          AddLog_P2(LOG_LEVEL_DEBUG_MORE, PSTR("ARM: Received Status: %u, %u, %u, %u"), newDimState[0],newKnobState[0],newDimState[1],newKnobState[1]);
      if(armtronix_firstStatus){
        armtronix_firstStatus = false;
        armtronix_oldKnobState[0] = newKnobState[0];
        armtronix_oldKnobState[1] = newKnobState[1];
           AddLog_P2(LOG_LEVEL_DEBUG_MORE, PSTR("ARM: First knob setting"));
      }else{
        if(abs((int8_t)(armtronix_oldKnobState[0] - newKnobState[0]))>ARMTRONIX_KNOBVARIATIONBEFORECHANGE){
        armtronix_oldKnobState[0] = newKnobState[0];
           AddLog_P2(LOG_LEVEL_DEBUG_MORE, PSTR("ARM: Change detected in Chan1"));
          snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_CHANNEL "%u %u"),1, newKnobState[0]);
          ExecuteCommand(scmnd, SRC_BUTTON);
        }
        if(abs((int8_t)(armtronix_oldKnobState[1] - newKnobState[1]))>ARMTRONIX_KNOBVARIATIONBEFORECHANGE){
        armtronix_oldKnobState[1] = newKnobState[1];
          AddLog_P2(LOG_LEVEL_DEBUG_MORE, PSTR("ARM: Change detected in Chan2"));
          snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_CHANNEL "%u %u"),2, newKnobState[1]);
          ExecuteCommand(scmnd, SRC_BUTTON);
        }
      }   
    }
  }
}

void ArmtronixSetWifiLed(void)
{
  uint8_t wifi_state = 0x02;

  switch (WifiState()) {
    case WIFI_SMARTCONFIG:
      wifi_state = 0x00;
      break;
    case WIFI_MANAGER:
    case WIFI_WPSCONFIG:
      wifi_state = 0x01;
      break;
    case WIFI_RESTART:
      wifi_state =  0x03;
      break;
  }

  AddLog_P2(LOG_LEVEL_DEBUG, PSTR("ARM: Set WiFi LED to state %d (%d)"), wifi_state, WifiState());

  char state = '0' + ((wifi_state & 1) > 0);
  ArmtronixSerial->print("SETLEDS:");
  ArmtronixSerial->write(state);
  ArmtronixSerial->write(',');
  state = '0' + ((wifi_state & 2) > 0);
  ArmtronixSerial->write(state);
  ArmtronixSerial->write(10);
  armtronix_wifi_state = WifiState();
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv18(uint8_t function)
{
  bool result = false;

  if (ARMTRONIX_DIMMERS == my_module_type) {
    switch (function) {
      case FUNC_LOOP:
        if (ArmtronixSerial) { ArmtronixSerialInput(); }
        break;
      case FUNC_MODULE_INIT:
        result = ArmtronixModuleSelected();
        break;
      case FUNC_INIT:
        ArmtronixInit();
        break;
      case FUNC_EVERY_SECOND:
        if (ArmtronixSerial) {
          if (armtronix_wifi_state!=WifiState()) { ArmtronixSetWifiLed(); }
          ArmtronixSerial->println("STATUS:");
        }
        break;
      case FUNC_SET_CHANNELS:
        result = ArmtronixSetChannels();
        break;
    }
  }
  return result;
}

#endif  // USE_ARMTRONIX_DIMMERS
