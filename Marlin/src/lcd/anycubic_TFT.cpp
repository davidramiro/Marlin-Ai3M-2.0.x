/*
   AnycubicTFT.cpp  --- Support for Anycubic i3 Mega TFT
   Created by Christian Hopp on 09.12.17.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Arduino.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../MarlinCore.h"
#include "../core/language.h"
#include "../core/macros.h"
#include "../core/serial.h"
#include "../gcode/queue.h"
#include "../feature/pause.h"
#include "../inc/MarlinConfig.h"
#include "../libs/buzzer.h"
#include "../module/planner.h"
#include "../module/printcounter.h"
#include "../module/stepper.h"
#include "../module/temperature.h"
#include "../sd/cardreader.h"

#ifdef ANYCUBIC_TFT_MODEL
#include "anycubic_TFT.h"
#include "anycubic_serial.h"

char _conv[8];

char *itostr2(const uint8_t &x)
{
  //sprintf(conv,"%5.1f",x);
  int xx=x;
  _conv[0]=(xx/10)%10+'0';
  _conv[1]=(xx)%10+'0';
  _conv[2]=0;
  return _conv;
}

#ifndef ULTRA_LCD
  #define DIGIT(n) ('0' + (n))
  #define DIGIMOD(n, f) DIGIT((n)/(f) % 10)
  #define RJDIGIT(n, f) ((n) >= (f) ? DIGIMOD(n, f) : ' ')
  #define MINUSOR(n, alt) (n >= 0 ? (alt) : (n = -n, '-'))


  char* itostr3(const int x) {
    int xx = x;
    _conv[4] = MINUSOR(xx, RJDIGIT(xx, 100));
    _conv[5] = RJDIGIT(xx, 10);
    _conv[6] = DIGIMOD(xx, 1);
    return &_conv[4];
  }


  // Convert signed float to fixed-length string with 023.45 / -23.45 format

  char *ftostr32(const float &x) {
    long xx = x * 100;
    _conv[1] = MINUSOR(xx, DIGIMOD(xx, 10000));
    _conv[2] = DIGIMOD(xx, 1000);
    _conv[3] = DIGIMOD(xx, 100);
    _conv[4] = '.';
    _conv[5] = DIGIMOD(xx, 10);
    _conv[6] = DIGIMOD(xx, 1);
    return &_conv[1];
  }

#endif

AnycubicTFTClass::AnycubicTFTClass() {
}

void AnycubicTFTClass::Setup() {
  AnycubicSerial.begin(115200);
  //ANYCUBIC_SERIAL_START();
  ANYCUBIC_SERIAL_PROTOCOLPGM("J17"); // J17 Main board reset
  ANYCUBIC_SERIAL_ENTER();
  delay(10);
  ANYCUBIC_SERIAL_PROTOCOLPGM("J12"); // J12 Ready
  ANYCUBIC_SERIAL_ENTER();

  #if ENABLED(SDSUPPORT) && PIN_EXISTS(SD_DETECT)
    pinMode(SD_DETECT_PIN, INPUT);
    WRITE(SD_DETECT_PIN, HIGH);
  #endif

  #if ENABLED(ANYCUBIC_FILAMENT_RUNOUT_SENSOR)
    pinMode(FIL_RUNOUT_PIN,INPUT);
    WRITE(FIL_RUNOUT_PIN,HIGH);
    if(READ(FIL_RUNOUT_PIN)==true)
    {
      ANYCUBIC_SERIAL_PROTOCOLPGM("J15"); //J15 FILAMENT LACK
      ANYCUBIC_SERIAL_ENTER();
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("TFT Serial Debug: Filament runout... J15");
      #endif
    }
  #endif

  SelectedDirectory[0]=0;
  SpecialMenu=false;

  #ifdef STARTUP_CHIME
    buzzer.tone(250, 554); // C#5
    buzzer.tone(250, 740); // F#5
    buzzer.tone(250, 554); // C#5
    buzzer.tone(500, 831); // G#5
  #endif
}

void AnycubicTFTClass::WriteOutageEEPromData() {
  int pos=E2END-256;

}

void AnycubicTFTClass::ReadOutageEEPromData() {
  int pos=E2END-256;

}

void AnycubicTFTClass::KillTFT()
{
  ANYCUBIC_SERIAL_PROTOCOLPGM("J11"); // J11 Kill
  ANYCUBIC_SERIAL_ENTER();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("TFT Serial Debug: Kill command... J11");
  #endif
}


void AnycubicTFTClass::StartPrint() {
  // which kind of starting behaviour is needed?
  switch (ai3m_pause_state) {
    case 0:
      // no pause, just a regular start
      starttime=millis();
      card.startFileprint();
      TFTstate=ANYCUBIC_TFT_STATE_SDPRINT;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
        SERIAL_EOL();
        SERIAL_ECHOLNPGM("DEBUG: Regular Start");
      #endif
      break;
    case 1:
      // regular sd pause
      queue.enqueue_now_P(PSTR("M24")); // unpark nozzle
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
        SERIAL_EOL();
        SERIAL_ECHOLNPGM("DEBUG: M24 Resume from regular pause");
      #endif
      IsParked=false; // remove parked flag
      starttime=millis();
      card.startFileprint(); // resume regularly
      TFTstate=ANYCUBIC_TFT_STATE_SDPRINT;
      ai3m_pause_state = 0;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
        SERIAL_EOL();
      #endif
      break;
    case 2:
      // paused by M600
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
        SERIAL_EOL();
        SERIAL_ECHOLNPGM("DEBUG: Start M108 routine");
      #endif
      FilamentChangeResume(); // enter display M108 routine
      ai3m_pause_state = 0; // clear flag
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Filament Change Flag cleared");
      #endif
      break;
    case 3:
      // paused by filament runout
      queue.enqueue_now_P(PSTR("M24")); // unpark nozzle and resume
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: M24 Resume from Filament Runout");
      #endif
      IsParked = false; // clear flags
      ai3m_pause_state = 0;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Filament Pause Flag cleared");
      #endif
      break;
    case 4:
      // nozzle was timed out before (M600), do not enter printing state yet
      TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE_REQ;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Set Pause again because of timeout");
      #endif

      // clear the timeout flag to ensure the print continues on the
      // next push of CONTINUE
      ai3m_pause_state = 2;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Nozzle timeout flag cleared");
      #endif
      break;
    case 5:
      // nozzle was timed out before (runout), do not enter printing state yet
      TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE_REQ;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Set Pause again because of timeout");
      #endif

      // clear the timeout flag to ensure the print continues on the
      // next push of CONTINUE
      ai3m_pause_state = 3;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Nozzle timeout flag cleared");
      #endif
      break;
    default:
      break;
  }
}

void AnycubicTFTClass::PausePrint() {
  #ifdef SDSUPPORT
    if(ai3m_pause_state < 2) { // is this a regular pause?
      card.pauseSDPrint(); // pause print regularly
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
        SERIAL_EOL();
        SERIAL_ECHOLNPGM("DEBUG: Regular Pause");
      #endif
    } else { // pause caused by filament runout
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Filament Runout Pause");
      #endif
      // filament runout, retract and beep
      queue.enqueue_now_P(PSTR("G91")); // relative mode
      queue.enqueue_now_P(PSTR("G1 E-3 F1800")); // retract 3mm
      queue.enqueue_now_P(PSTR("G90")); // absolute mode
      buzzer.tone(200, 1567);
      buzzer.tone(200, 1174);
      buzzer.tone(200, 1567);
      buzzer.tone(200, 1174);
      buzzer.tone(2000, 1567);
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: Filament runout - Retract, beep and park.");
      #endif
      queue.enqueue_now_P(PSTR("M25")); // pause print and park nozzle
      ai3m_pause_state = 3;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: M25 sent, parking nozzle");
        SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
        SERIAL_EOL();
      #endif
      IsParked = true;
      // show filament runout prompt on screen
      ANYCUBIC_SERIAL_PROTOCOLPGM("J23");
      ANYCUBIC_SERIAL_ENTER();
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("DEBUG: J23 Show filament prompt");
      #endif
    }
  #endif
  TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE_REQ;
}

void AnycubicTFTClass::StopPrint(){
  // stop print, disable heaters
  wait_for_user = false;
  wait_for_heatup = false;
  card.endFilePrint();
  queue.clear();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: Stopped and cleared");
  #endif
  print_job_timer.stop();
  thermalManager.disable_all_heaters();
  // we are not parked yet, do it in the display state routine
  IsParked = false;
  // turn off fan, cancel any heatups and set display state
  ai3m_pause_state = 0;
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
    SERIAL_EOL();
  #endif
  #if FAN_COUNT > 0
    thermalManager.zero_fan_speeds();
  #endif
  TFTstate=ANYCUBIC_TFT_STATE_SDSTOP_REQ;
}

void AnycubicTFTClass::FilamentChangeResume(){
  // call M108 to break out of M600 pause
  queue.enqueue_now_P(PSTR("M108"));
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: M108 Resume called");
  #endif

  // remove waiting flags
  wait_for_heatup = false;
  wait_for_user = false;

  // resume with proper progress state
  card.startFileprint();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: M108 Resume done");
  #endif
}

void AnycubicTFTClass::FilamentChangePause(){
  // set filament change flag to ensure the M108 routine
  // gets used when the user hits CONTINUE
  ai3m_pause_state = 2;
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
    SERIAL_EOL();
  #endif

  // call M600 and set display state to paused
  queue.enqueue_now_P(PSTR("M600"));
  TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE_REQ;
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: M600 Pause called");
  #endif
}

void AnycubicTFTClass::ReheatNozzle(){
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: Send reheat M108");
  #endif
  queue.enqueue_now_P(PSTR("M108"));
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: Resume heating");
  #endif

  // enable heaters again
  HOTEND_LOOP() thermalManager.reset_hotend_idle_timer(e);
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("DEBUG: Clear flags");
  #endif
  // lower the pause flag by two to restore initial pause condition
  if (ai3m_pause_state > 3) {
  ai3m_pause_state -= 2;
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOPAIR(" DEBUG: NTO done, AI3M Pause State: ", ai3m_pause_state);
    SERIAL_EOL();
  #endif
  }

  // clear waiting flags
  wait_for_user = false;
  wait_for_heatup = false;

  // set pause state to show CONTINUE button again
  TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE_REQ;
}

void AnycubicTFTClass::ParkAfterStop(){
  // only park the nozzle if homing was done before
  if (!axis_unhomed_error()) {
    // raize nozzle by 25mm respecting Z_MAX_POS
    do_blocking_move_to_z(_MIN(current_position[Z_AXIS] + 25, Z_MAX_POS), 5);
    #ifdef ANYCUBIC_TFT_DEBUG
      SERIAL_ECHOLNPGM("DEBUG: SDSTOP: Park Z");
    #endif
    // move bed and hotend to park position
    do_blocking_move_to_xy((X_MIN_POS + 10), (Y_MAX_POS - 10), 100);
    #ifdef ANYCUBIC_TFT_DEBUG
      SERIAL_ECHOLNPGM("DEBUG: SDSTOP: Park XY");
    #endif
  }
  queue.enqueue_now_P(PSTR("M84")); // disable stepper motors
  queue.enqueue_now_P(PSTR("M27")); // force report of SD status
  ai3m_pause_state = 0;
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
    SERIAL_EOL();
  #endif
}

float AnycubicTFTClass::CodeValue()
{
  return (strtod(&TFTcmdbuffer[TFTbufindr][TFTstrchr_pointer - TFTcmdbuffer[TFTbufindr] + 1], NULL));
}

bool AnycubicTFTClass::CodeSeen(char code)
{
  TFTstrchr_pointer = strchr(TFTcmdbuffer[TFTbufindr], code);
  return (TFTstrchr_pointer != NULL); //Return True if a character was found
}

void AnycubicTFTClass::HandleSpecialMenu()
{
  if(strcmp(SelectedDirectory, "<special menu>")==0) {
    SpecialMenu=true;
  } else if (strcmp(SelectedDirectory, "<auto tune hotend pid>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Auto Tune Hotend PID");
    queue.enqueue_now_P(PSTR("M106 S204\nM303 E0 S210 C15 U1"));
  } else if (strcmp(SelectedDirectory, "<auto tune hotbed pid>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Auto Tune Hotbed Pid");
    queue.enqueue_now_P(PSTR("M303 E-1 S60 C6 U1"));
  } else if (strcmp(SelectedDirectory, "<save eeprom>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Save EEPROM");
    queue.enqueue_now_P(PSTR("M500"));
    buzzer.tone(105, 1108);
    buzzer.tone(210, 1661);
  } else if (strcmp(SelectedDirectory, "<load fw defaults>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Load FW Defaults");
    queue.enqueue_now_P(PSTR("M502"));
    buzzer.tone(105, 1661);
    buzzer.tone(210, 1108);
  } else if (strcmp(SelectedDirectory, "<preheat bed>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Preheat Bed");
    queue.enqueue_now_P(PSTR("M140 S60"));
  } else if (strcmp(SelectedDirectory, "<start mesh leveling>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Start Mesh Leveling");
    queue.enqueue_now_P(PSTR("G29 S1"));
  } else if (strcmp(SelectedDirectory, "<next mesh point>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Next Mesh Point");
    queue.enqueue_now_P(PSTR("G29 S2"));
  } else if (strcmp(SelectedDirectory, "<z up 0.1>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Z Up 0.1");
    queue.enqueue_now_P(PSTR("G91\nG1 Z+0.1\nG90"));
  } else if (strcmp(SelectedDirectory, "<z up 0.02>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Z Up 0.02");
    queue.enqueue_now_P(PSTR("G91\nG1 Z+0.02\nG90"));
  } else if (strcmp(SelectedDirectory, "<z down 0.02>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Z Down 0.02");
    queue.enqueue_now_P(PSTR("G91\nG1 Z-0.02\nG90"));
  } else if (strcmp(SelectedDirectory, "<z down 0.1>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: Z Down 0.1");
    queue.enqueue_now_P(PSTR("G91\nG1 Z-0.1\nG90"));
  } else if (strcmp(SelectedDirectory, "<filamentchange pause>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: FilamentChange Pause");
    FilamentChangePause();
  } else if (strcmp(SelectedDirectory, "<filamentchange resume>")==0) {
    SERIAL_ECHOLNPGM("Special Menu: FilamentChange Resume");
    FilamentChangeResume();
  } else if (strcmp(SelectedDirectory, "<exit>")==0) {
    SpecialMenu=false;
  }
}

void AnycubicTFTClass::Ls()
{
  if (SpecialMenu) {
    switch (filenumber) {
      case 0: // First Page
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Up 0.1>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Up 0.1>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Up 0.02>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Up 0.02>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Down 0.02>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Down 0.02>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Down 0.1>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Z Down 0.1>");
      break;

      case 4: // Second Page
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Preheat bed>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Preheat bed>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Start Mesh Leveling>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Start Mesh Leveling>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Next Mesh Point>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Next Mesh Point>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Save EEPROM>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Save EEPROM>");
      break;

      case 8: // Third Page
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Exit>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Exit>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Auto Tune Hotend PID>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Auto Tune Hotend PID>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Auto Tune Hotbed PID>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Auto Tune Hotbed PID>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Load FW Defaults>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Load FW Defaults>");
      break;

      case 12: // Fourth Page
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<FilamentChange Pause>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<FilamentChange Pause>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<FilamentChange Resume>");
      ANYCUBIC_SERIAL_PROTOCOLLNPGM("<FilamentChange Resume>");
      break;

      default:
      break;
    }
  }
  #ifdef SDSUPPORT
    else if(card.isMounted())
    {
      uint16_t cnt=filenumber;
      uint16_t max_files;
      uint16_t dir_files=card.countFilesInWorkDir();

      if((dir_files-filenumber)<4)
      {
        max_files=dir_files;
      } else {
        max_files=filenumber+3;
      }

      for(cnt=filenumber; cnt<=max_files; cnt++)
      {
        if (cnt==0) // Special Entry
        {
          if(strcmp(card.getWorkDirName(), "/") == 0) {
            ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Special Menu>");
            ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Special Menu>");
            SERIAL_ECHO(cnt);
            SERIAL_ECHOLNPGM("<Special_Menu>");
          } else {
            ANYCUBIC_SERIAL_PROTOCOLLNPGM("/..");
            ANYCUBIC_SERIAL_PROTOCOLLNPGM("/..");
            SERIAL_ECHO(cnt);
            SERIAL_ECHOLNPGM("/..");
          }
        } else {
          card.selectFileByIndex(cnt-1);
          //      card.selectFileByIndex(cnt);

          if(card.flag.filenameIsDir) {
            ANYCUBIC_SERIAL_PROTOCOLPGM("/");
            ANYCUBIC_SERIAL_PROTOCOLLN(card.filename);
            ANYCUBIC_SERIAL_PROTOCOLPGM("/");
            ANYCUBIC_SERIAL_PROTOCOLLN(card.longFilename);
            SERIAL_ECHO(cnt);
            SERIAL_ECHOPGM("/");
            SERIAL_ECHOLN(card.longFilename);
          } else {
            ANYCUBIC_SERIAL_PROTOCOLLN(card.filename);
            ANYCUBIC_SERIAL_PROTOCOLLN(card.longFilename);
            SERIAL_ECHO(cnt);
            SERIAL_ECHOLN(card.longFilename);
          }
        }
      }
    }
  #endif
  else {
    ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Special_Menu>");
    ANYCUBIC_SERIAL_PROTOCOLLNPGM("<Special_Menu>");
  }
}

void AnycubicTFTClass::CheckSDCardChange()
{
  #ifdef SDSUPPORT
    if (LastSDstatus != IS_SD_INSERTED())
    {
      LastSDstatus = IS_SD_INSERTED();

      if (LastSDstatus)
      {
        card.mount();
        ANYCUBIC_SERIAL_PROTOCOLPGM("J00"); // J00 SD Card inserted
        ANYCUBIC_SERIAL_ENTER();
        #ifdef ANYCUBIC_TFT_DEBUG
          SERIAL_ECHOLNPGM("TFT Serial Debug: SD card inserted... J00");
        #endif
      }
      else
      {
        ANYCUBIC_SERIAL_PROTOCOLPGM("J01"); // J01 SD Card removed
        ANYCUBIC_SERIAL_ENTER();
        #ifdef ANYCUBIC_TFT_DEBUG
          SERIAL_ECHOLNPGM("TFT Serial Debug: SD card removed... J01");
        #endif

      }
    }
  #endif
}

void AnycubicTFTClass::CheckHeaterError()
{
  if ((thermalManager.degHotend(0) < 5) || (thermalManager.degHotend(0) > 290))
  {
    if (HeaterCheckCount > 60000)
    {
      HeaterCheckCount = 0;
      ANYCUBIC_SERIAL_PROTOCOLPGM("J10"); // J10 Hotend temperature abnormal
      ANYCUBIC_SERIAL_ENTER();
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("TFT Serial Debug: Hotend temperature abnormal... J20");
      #endif

    }
    else
    {
      HeaterCheckCount++;
    }
  }
  else
  {
    HeaterCheckCount = 0;
  }
}

void AnycubicTFTClass::StateHandler()
{
  switch (TFTstate) {
    case ANYCUBIC_TFT_STATE_IDLE:
      #ifdef SDSUPPORT
        if(card.isPrinting()) {
          TFTstate=ANYCUBIC_TFT_STATE_SDPRINT;
          starttime=millis();

          // --> Send print info to display... most probably print started via gcode
        }
      #endif
      break;
    case ANYCUBIC_TFT_STATE_SDPRINT:
      #ifdef SDSUPPORT
        if(!card.isPrinting()) {
          // It seems that we are to printing anymore... pause or stopped?
          if (card.isFileOpen()) {
            // File is still open --> paused
            TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE;
          } else if ((!card.isFileOpen()) && (ai3m_pause_state == 0)) {
            // File is closed --> stopped
            TFTstate=ANYCUBIC_TFT_STATE_IDLE;
            ANYCUBIC_SERIAL_PROTOCOLPGM("J14");// J14 print done
            ANYCUBIC_SERIAL_ENTER();
            #ifdef ANYCUBIC_TFT_DEBUG
              SERIAL_ECHOLNPGM("TFT Serial Debug: SD print done... J14");
            #endif
            ai3m_pause_state = 0;
            #ifdef ANYCUBIC_TFT_DEBUG
              SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
              SERIAL_EOL();
            #endif
          }
        }
      #endif
      break;
    case ANYCUBIC_TFT_STATE_SDPAUSE:
      break;
    case ANYCUBIC_TFT_STATE_SDPAUSE_OOF:
      #ifdef ANYCUBIC_FILAMENT_RUNOUT_SENSOR
        if(!FilamentTestStatus) {
          // We got filament again
          TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE;
        }
      #endif
      break;
    case ANYCUBIC_TFT_STATE_SDPAUSE_REQ:
      ANYCUBIC_SERIAL_PROTOCOLPGM("J18");
      ANYCUBIC_SERIAL_ENTER();
      #ifdef SDSUPPORT
        if((!card.isPrinting()) && (!planner.movesplanned())) {
          // We have to wait until the sd card printing has been settled
          if(ai3m_pause_state < 2) {
            // no flags, this is a regular pause.
            ai3m_pause_state = 1;
            #ifdef ANYCUBIC_TFT_DEBUG
              SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
              SERIAL_EOL();
              SERIAL_ECHOLNPGM("DEBUG: Regular Pause requested");
            #endif
            if(!IsParked) {
              // park head and retract 2mm
              queue.enqueue_now_P(PSTR("M125 L2"));
              IsParked = true;
            }
          }
          #ifdef ANYCUBIC_FILAMENT_RUNOUT_SENSOR
            if(FilamentTestStatus) {
              TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE;
            } else {
              // Pause because of "out of filament"
              TFTstate=ANYCUBIC_TFT_STATE_SDPAUSE_OOF;
            }
          #endif
          #ifdef ANYCUBIC_TFT_DEBUG
            SERIAL_ECHOLNPGM("TFT Serial Debug: SD print paused done... J18");
          #endif
        }
      #endif
      break;
    case ANYCUBIC_TFT_STATE_SDSTOP_REQ:
      #ifdef SDSUPPORT
        ANYCUBIC_SERIAL_PROTOCOLPGM("J16");// J16 stop print
        ANYCUBIC_SERIAL_ENTER();
        if((!card.isPrinting()) && (!planner.movesplanned())) {
          // enter idle display state
          TFTstate=ANYCUBIC_TFT_STATE_IDLE;
          #ifdef ANYCUBIC_TFT_DEBUG
            SERIAL_ECHOLNPGM("TFT Serial Debug: SD print stopped... J16");
          #endif
          ai3m_pause_state = 0;
          #ifdef ANYCUBIC_TFT_DEBUG
            SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
            SERIAL_EOL();
          #endif
        }
        // did we park the hotend already?
        if((!IsParked) && (!card.isPrinting()) && (!planner.movesplanned())) {
          queue.enqueue_now_P(PSTR("G91\nG1 E-1 F1800\nG90"));  //retract
          ParkAfterStop();
          IsParked = true;
        }
      #endif
      break;
    default:
      break;
  }
}

void AnycubicTFTClass::FilamentRunout()
{
  #if ENABLED(ANYCUBIC_FILAMENT_RUNOUT_SENSOR)
    FilamentTestStatus=READ(FIL_RUNOUT_PIN)&0xff;

    if(FilamentTestStatus>FilamentTestLastStatus) {
      // filament sensor pin changed, save current timestamp.
      const millis_t fil_ms = millis();
      static millis_t fil_delay;

      // since this is inside a loop, only set delay time once
      if (FilamentSetMillis) {
        #ifdef ANYCUBIC_TFT_DEBUG
          SERIAL_ECHOLNPGM("DEBUG: Set filament trigger time");
        #endif
        // set the delayed timestamp to 3000ms later
        fil_delay = fil_ms + 3000UL;
        // this doesn't need to run until the filament is recovered again
        FilamentSetMillis=false;
      }

      // if three seconds passed and the sensor is still triggered,
      // we trigger the filament runout status
      if ((FilamentTestStatus>FilamentTestLastStatus) && (ELAPSED(fil_ms, fil_delay))) {
        if (!IsParked){
          #ifdef ANYCUBIC_TFT_DEBUG
            SERIAL_ECHOLNPGM("DEBUG: 3000ms delay done");
          #endif
          if(card.isPrinting()) {
            ai3m_pause_state = 3;; // set runout pause flag
            #ifdef ANYCUBIC_TFT_DEBUG
              SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
              SERIAL_EOL();
            #endif
            PausePrint();
          } else if(!card.isPrinting()) {
            ANYCUBIC_SERIAL_PROTOCOLPGM("J15"); //J15 FILAMENT LACK
            ANYCUBIC_SERIAL_ENTER();
            #ifdef ANYCUBIC_TFT_DEBUG
              SERIAL_ECHOLNPGM("TFT Serial Debug: Filament runout... J15");
            #endif
            FilamentTestLastStatus=FilamentTestStatus;
          }
        }
        FilamentTestLastStatus=FilamentTestStatus;
      }

    }
    else if(FilamentTestStatus!=FilamentTestLastStatus) {
      FilamentSetMillis=true; // set the timestamps on the next loop again
      FilamentTestLastStatus=FilamentTestStatus;
      #ifdef ANYCUBIC_TFT_DEBUG
        SERIAL_ECHOLNPGM("TFT Serial Debug: Filament runout recovered");
      #endif
    }
  #endif
}

void AnycubicTFTClass::GetCommandFromTFT()
{
  char *starpos = NULL;
  while( AnycubicSerial.available() > 0  && TFTbuflen < TFTBUFSIZE)
  {
    serial3_char = AnycubicSerial.read();
    if(serial3_char == '\n' ||
    serial3_char == '\r' ||
    serial3_char == ':'  ||
    serial3_count >= (TFT_MAX_CMD_SIZE - 1) )
    {
      if(!serial3_count) { //if empty line
        return;
      }

      TFTcmdbuffer[TFTbufindw][serial3_count] = 0; //terminate string

      if((strchr(TFTcmdbuffer[TFTbufindw], 'A') != NULL)) {
        int16_t a_command;
        TFTstrchr_pointer = strchr(TFTcmdbuffer[TFTbufindw], 'A');
        a_command=((int)((strtod(&TFTcmdbuffer[TFTbufindw][TFTstrchr_pointer - TFTcmdbuffer[TFTbufindw] + 1], NULL))));

        #ifdef ANYCUBIC_TFT_DEBUG
        if ((a_command>7) && (a_command != 20)) // No debugging of status polls, please!
        SERIAL_ECHOLNPAIR("TFT Serial Command: ", TFTcmdbuffer[TFTbufindw]);
        #endif

        switch(a_command) {

          case 0: //A0 GET HOTEND TEMP
            ANYCUBIC_SERIAL_PROTOCOLPGM("A0V ");
            ANYCUBIC_SERIAL_PROTOCOL(itostr3(int(thermalManager.degHotend(0) + 0.5)));
            ANYCUBIC_SERIAL_ENTER();
            break;

          case 1: //A1  GET HOTEND TARGET TEMP
            ANYCUBIC_SERIAL_PROTOCOLPGM("A1V ");
            ANYCUBIC_SERIAL_PROTOCOL(itostr3(int(thermalManager.degTargetHotend(0) + 0.5)));
            ANYCUBIC_SERIAL_ENTER();
            break;

          case 2: //A2 GET HOTBED TEMP
            ANYCUBIC_SERIAL_PROTOCOLPGM("A2V ");
            ANYCUBIC_SERIAL_PROTOCOL(itostr3(int(thermalManager.degBed() + 0.5)));
            ANYCUBIC_SERIAL_ENTER();
            break;

          case 3: //A3 GET HOTBED TARGET TEMP
            ANYCUBIC_SERIAL_PROTOCOLPGM("A3V ");
            ANYCUBIC_SERIAL_PROTOCOL(itostr3(int(thermalManager.degTargetBed() + 0.5)));
            ANYCUBIC_SERIAL_ENTER();
            break;

          case 4://A4 GET FAN SPEED
            {
              unsigned int temp;

              temp = ((thermalManager.fan_speed[0] * 100) / 255);
              temp=constrain(temp,0,100);

              ANYCUBIC_SERIAL_PROTOCOLPGM("A4V ");
              ANYCUBIC_SERIAL_PROTOCOL(temp);
              ANYCUBIC_SERIAL_ENTER();
            }
            break;
          case 5:// A5 GET CURRENT COORDINATE
            ANYCUBIC_SERIAL_PROTOCOLPGM("A5V");
            ANYCUBIC_SERIAL_SPACE();
            ANYCUBIC_SERIAL_PROTOCOLPGM("X: ");
            ANYCUBIC_SERIAL_PROTOCOL(current_position[X_AXIS]);
            ANYCUBIC_SERIAL_SPACE();
            ANYCUBIC_SERIAL_PROTOCOLPGM("Y: ");
            ANYCUBIC_SERIAL_PROTOCOL(current_position[Y_AXIS]);
            ANYCUBIC_SERIAL_SPACE();
            ANYCUBIC_SERIAL_PROTOCOLPGM("Z: ");
            ANYCUBIC_SERIAL_PROTOCOL(current_position[Z_AXIS]);
            ANYCUBIC_SERIAL_SPACE();
            ANYCUBIC_SERIAL_ENTER();
            break;
          case 6: //A6 GET SD CARD PRINTING STATUS
            #ifdef SDSUPPORT
              if(card.isPrinting()) {
                ANYCUBIC_SERIAL_PROTOCOLPGM("A6V ");
                if(card.isMounted())
                {
                  ANYCUBIC_SERIAL_PROTOCOL(itostr3(card.percentDone()));
                }
                else
                {
                  ANYCUBIC_SERIAL_PROTOCOLPGM("J02");
                }
              }
              else
              ANYCUBIC_SERIAL_PROTOCOLPGM("A6V ---");
              ANYCUBIC_SERIAL_ENTER();
            #endif
            break;
          case 7://A7 GET PRINTING TIME
            {
              ANYCUBIC_SERIAL_PROTOCOLPGM("A7V ");
              if(starttime != 0) // print time
              {
                uint16_t time = millis()/60000 - starttime/60000;
                ANYCUBIC_SERIAL_PROTOCOL(itostr2(time/60));
                ANYCUBIC_SERIAL_SPACE();
                ANYCUBIC_SERIAL_PROTOCOLPGM("H");
                ANYCUBIC_SERIAL_SPACE();
                ANYCUBIC_SERIAL_PROTOCOL(itostr2(time%60));
                ANYCUBIC_SERIAL_SPACE();
                ANYCUBIC_SERIAL_PROTOCOLPGM("M");
              }else{
                ANYCUBIC_SERIAL_SPACE();
                ANYCUBIC_SERIAL_PROTOCOLPGM("999:999");
              }
              ANYCUBIC_SERIAL_ENTER();

            break;
          }
          case 8: // A8 GET  SD LIST
            #ifdef SDSUPPORT
              SelectedDirectory[0]=0;
              if(!IS_SD_INSERTED())
              {
                ANYCUBIC_SERIAL_PROTOCOLPGM("J02");
                ANYCUBIC_SERIAL_ENTER();
              }
              else
              {
                if(CodeSeen('S'))
                filenumber=CodeValue();

                ANYCUBIC_SERIAL_PROTOCOLPGM("FN "); // Filelist start
                ANYCUBIC_SERIAL_ENTER();
                Ls();
                ANYCUBIC_SERIAL_PROTOCOLPGM("END"); // Filelist stop
                ANYCUBIC_SERIAL_ENTER();
              }
            #endif
            break;
          case 9: // A9 pause sd print
            #ifdef SDSUPPORT
              if(card.isPrinting())
              {
                PausePrint();
              }
              else
              {
                ai3m_pause_state = 0;
                #ifdef ANYCUBIC_TFT_DEBUG
                  SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
                  SERIAL_EOL();
                #endif
                StopPrint();
              }
            #endif
            break;
          case 10: // A10 resume sd print
            #ifdef SDSUPPORT
              if((TFTstate==ANYCUBIC_TFT_STATE_SDPAUSE) || (TFTstate==ANYCUBIC_TFT_STATE_SDOUTAGE))
              {
                StartPrint();
                ANYCUBIC_SERIAL_PROTOCOLPGM("J04");// J04 printing form sd card now
                ANYCUBIC_SERIAL_ENTER();
                #ifdef ANYCUBIC_TFT_DEBUG
                  SERIAL_ECHOLNPGM("TFT Serial Debug: SD print started... J04");
                #endif
              }
              if (ai3m_pause_state > 3) {
                ReheatNozzle();
              }
            #endif
            break;
          case 11: // A11 STOP SD PRINT
            #ifdef SDSUPPORT
              if((card.isPrinting()) || (TFTstate==ANYCUBIC_TFT_STATE_SDOUTAGE))
              {
                StopPrint();
              } else {
                ANYCUBIC_SERIAL_PROTOCOLPGM("J16");
                ANYCUBIC_SERIAL_ENTER();
                TFTstate=ANYCUBIC_TFT_STATE_IDLE;
                ai3m_pause_state = 0;
                #ifdef ANYCUBIC_TFT_DEBUG
                  SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
                  SERIAL_EOL();
                #endif
              }
              #endif
            break;
          case 12: // A12 kill
            kill(PSTR(STR_ERR_KILLED));
            break;
          case 13: // A13 SELECTION FILE
              #ifdef SDSUPPORT
              if((TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE))
              {
                starpos = (strchr(TFTstrchr_pointer + 4,'*'));
                if (TFTstrchr_pointer[4] == '/') {
                  strcpy(SelectedDirectory, TFTstrchr_pointer+5);
                } else if (TFTstrchr_pointer[4] == '<') {
                  strcpy(SelectedDirectory, TFTstrchr_pointer+4);
                } else {
                  SelectedDirectory[0]=0;

                  if(starpos!=NULL)
                  *(starpos-1)='\0';
                  card.openFileRead(TFTstrchr_pointer + 4);
                  if (card.isFileOpen()) {
                    ANYCUBIC_SERIAL_PROTOCOLPGM("J20"); // J20 Open successful
                    ANYCUBIC_SERIAL_ENTER();
                    #ifdef ANYCUBIC_TFT_DEBUG
                      SERIAL_ECHOLNPGM("TFT Serial Debug: File open successful... J20");
                    #endif
                  } else {
                    ANYCUBIC_SERIAL_PROTOCOLPGM("J21"); // J21 Open failed
                    ANYCUBIC_SERIAL_ENTER();
                    #ifdef ANYCUBIC_TFT_DEBUG
                      SERIAL_ECHOLNPGM("TFT Serial Debug: File open failed... J21");
                    #endif
                  }
                }
                ANYCUBIC_SERIAL_ENTER();
              }
              #endif
            break;
          case 14: // A14 START PRINTING
            #ifdef SDSUPPORT
              if((!planner.movesplanned()) && (TFTstate!=ANYCUBIC_TFT_STATE_SDPAUSE) && (TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE) && (card.isFileOpen()))
              {
                ai3m_pause_state = 0;
                #ifdef ANYCUBIC_TFT_DEBUG
                  SERIAL_ECHOPAIR(" DEBUG: AI3M Pause State: ", ai3m_pause_state);
                  SERIAL_EOL();
                #endif
                StartPrint();
                IsParked = false;
                ANYCUBIC_SERIAL_PROTOCOLPGM("J04"); // J04 Starting Print
                ANYCUBIC_SERIAL_ENTER();
                #ifdef ANYCUBIC_TFT_DEBUG
                  SERIAL_ECHOLNPGM("TFT Serial Debug: Starting SD Print... J04");
                #endif
              }
            #endif
            break;
          case 15: // A15 RESUMING FROM OUTAGE
            //if((!planner.movesplanned())&&(!TFTresumingflag))
            //  {
            //    if(card.isMounted())
            //      FlagResumFromOutage=true;
            //      ResumingFlag=1;
            //      card.startFileprint();
            //      starttime=millis();
            //      ANYCUBIC_SERIAL_SUCC_START;
            //  }
            //ANYCUBIC_SERIAL_ENTER();
            break;
          case 16: // A16 set hotend temp
            {
              unsigned int tempvalue;
              if(CodeSeen('S'))
              {
                tempvalue=constrain(CodeValue(),0,275);
                thermalManager.setTargetHotend(tempvalue,0);
              }
              else if((CodeSeen('C'))&&(!planner.movesplanned()))
              {
                if((current_position[Z_AXIS]<10))
                queue.enqueue_now_P(PSTR("G1 Z10")); //RASE Z AXIS
                tempvalue=constrain(CodeValue(),0,275);
                thermalManager.setTargetHotend(tempvalue,0);
              }
            }
            //  ANYCUBIC_SERIAL_ENTER();
            break;
          case 17:// A17 set heated bed temp
            {
              unsigned int tempbed;
              if(CodeSeen('S')) {tempbed=constrain(CodeValue(),0,150);
                thermalManager.setTargetBed(tempbed);}
            }
            //  ANYCUBIC_SERIAL_ENTER();
            break;
          case 18:// A18 set fan speed
            unsigned int temp;
            if (CodeSeen('S'))
            {
              temp=(CodeValue()*255/100);
              temp=constrain(temp,0,255);
              thermalManager.set_fan_speed(0, temp);
            }
            else thermalManager.set_fan_speed(0, 255);
            ANYCUBIC_SERIAL_ENTER();
            break;
          case 19: // A19 stop stepper drivers
            if((!planner.movesplanned())
            #ifdef SDSUPPORT
            &&(!card.isPrinting())
            #endif
            )
            {
              quickstop_stepper();
              disable_all_steppers();
            }
            ANYCUBIC_SERIAL_ENTER();
            break;
          case 20:// A20 read printing speed
          {
            if(CodeSeen('S')) {
              feedrate_percentage=constrain(CodeValue(),40,999);
            }
            else{
              ANYCUBIC_SERIAL_PROTOCOLPGM("A20V ");
              ANYCUBIC_SERIAL_PROTOCOL(feedrate_percentage);
              ANYCUBIC_SERIAL_ENTER();
            }
          }
          break;
          case 21: // A21 all home
            if((!planner.movesplanned()) && (TFTstate!=ANYCUBIC_TFT_STATE_SDPAUSE) && (TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE))
            {
              if(CodeSeen('X')||CodeSeen('Y')||CodeSeen('Z'))
              {
                if(CodeSeen('X')) queue.enqueue_now_P(PSTR("G28 X"));
                if(CodeSeen('Y')) queue.enqueue_now_P(PSTR("G28 Y"));
                if(CodeSeen('Z')) queue.enqueue_now_P(PSTR("G28 Z"));
              }
              else if(CodeSeen('C')) queue.enqueue_now_P(PSTR("G28"));
            }
            break;
          case 22: // A22 move X/Y/Z or extrude
            if((!planner.movesplanned()) && (TFTstate!=ANYCUBIC_TFT_STATE_SDPAUSE) && (TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE))
            {
              float coorvalue;
              unsigned int movespeed=0;
              char value[30];
              if(CodeSeen('F')) // Set feedrate
              movespeed = CodeValue();

              queue.enqueue_now_P(PSTR("G91")); // relative coordinates

              if(CodeSeen('X')) // Move in X direction
              {
                coorvalue=CodeValue();
                if((coorvalue<=0.2)&&coorvalue>0) {sprintf_P(value,PSTR("G1 X0.1F%i"),movespeed);}
                else if((coorvalue<=-0.1)&&coorvalue>-1) {sprintf_P(value,PSTR("G1 X-0.1F%i"),movespeed);}
                else {sprintf_P(value,PSTR("G1 X%iF%i"),int(coorvalue),movespeed);}
                queue.enqueue_one_now(value);
              }
              else if(CodeSeen('Y')) // Move in Y direction
              {
                coorvalue=CodeValue();
                if((coorvalue<=0.2)&&coorvalue>0) {sprintf_P(value,PSTR("G1 Y0.1F%i"),movespeed);}
                else if((coorvalue<=-0.1)&&coorvalue>-1) {sprintf_P(value,PSTR("G1 Y-0.1F%i"),movespeed);}
                else {sprintf_P(value,PSTR("G1 Y%iF%i"),int(coorvalue),movespeed);}
                queue.enqueue_one_now(value);
              }
              else if(CodeSeen('Z')) // Move in Z direction
              {
                coorvalue=CodeValue();
                if((coorvalue<=0.2)&&coorvalue>0) {sprintf_P(value,PSTR("G1 Z0.1F%i"),movespeed);}
                else if((coorvalue<=-0.1)&&coorvalue>-1) {sprintf_P(value,PSTR("G1 Z-0.1F%i"),movespeed);}
                else {sprintf_P(value,PSTR("G1 Z%iF%i"),int(coorvalue),movespeed);}
                queue.enqueue_one_now(value);
              }
              else if(CodeSeen('E')) // Extrude
              {
                coorvalue=CodeValue();
                if((coorvalue<=0.2)&&coorvalue>0) {sprintf_P(value,PSTR("G1 E0.1F%i"),movespeed);}
                else if((coorvalue<=-0.1)&&coorvalue>-1) {sprintf_P(value,PSTR("G1 E-0.1F%i"),movespeed);}
                else {sprintf_P(value,PSTR("G1 E%iF500"),int(coorvalue)); }
                queue.enqueue_one_now(value);
              }
              queue.enqueue_now_P(PSTR("G90")); // absolute coordinates
            }
            ANYCUBIC_SERIAL_ENTER();
            break;
          case 23: // A23 preheat pla
            if((!planner.movesplanned())&& (TFTstate!=ANYCUBIC_TFT_STATE_SDPAUSE) && (TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE))
            {
              if((current_position[Z_AXIS]<10)) queue.enqueue_now_P(PSTR("G1 Z10")); // RAISE Z AXIS
              thermalManager.setTargetBed(50);
              thermalManager.setTargetHotend(200, 0);
              ANYCUBIC_SERIAL_SUCC_START;
              ANYCUBIC_SERIAL_ENTER();
            }
            break;
          case 24:// A24 preheat abs
            if((!planner.movesplanned()) && (TFTstate!=ANYCUBIC_TFT_STATE_SDPAUSE) && (TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE))
            {
              if((current_position[Z_AXIS]<10)) queue.enqueue_now_P(PSTR("G1 Z10")); //RAISE Z AXIS
              thermalManager.setTargetBed(80);
              thermalManager.setTargetHotend(240, 0);

              ANYCUBIC_SERIAL_SUCC_START;
              ANYCUBIC_SERIAL_ENTER();
            }
            break;
          case 25: // A25 cool down
            if((!planner.movesplanned())&& (TFTstate!=ANYCUBIC_TFT_STATE_SDPAUSE) && (TFTstate!=ANYCUBIC_TFT_STATE_SDOUTAGE))
            {
              thermalManager.setTargetHotend(0,0);
              thermalManager.setTargetBed(0);
              ANYCUBIC_SERIAL_PROTOCOLPGM("J12"); // J12 cool down
              ANYCUBIC_SERIAL_ENTER();
              #ifdef ANYCUBIC_TFT_DEBUG
              SERIAL_ECHOLNPGM("TFT Serial Debug: Cooling down... J12");
              #endif
            }
            break;
          case 26: // A26 refresh SD
            #ifdef SDSUPPORT
              if (SelectedDirectory[0]==0) {
                card.mount();
              } else {
                if ((SelectedDirectory[0] == '.') && (SelectedDirectory[1] == '.')) {
                  card.cdup();
                } else {
                  if (SelectedDirectory[0] == '<') {
                    HandleSpecialMenu();
                  } else {
                    card.cd(SelectedDirectory);
                  }
                }
              }

              SelectedDirectory[0]=0;

              if(!IS_SD_INSERTED())
              {
                ANYCUBIC_SERIAL_PROTOCOLPGM("J02"); // J02 SD Card initilized
                ANYCUBIC_SERIAL_ENTER();
                #ifdef ANYCUBIC_TFT_DEBUG
                  SERIAL_ECHOLNPGM("TFT Serial Debug: SD card initialized... J02");
                #endif
              }
            #endif
            break;
          #ifdef SERVO_ENDSTOPS
            case 27: // A27 servos angles  adjust
              break;
          #endif
          case 28: // A28 filament test
            {
              if(CodeSeen('O'));
              else if(CodeSeen('C'));
            }
            ANYCUBIC_SERIAL_ENTER();
            break;
          case 33: // A33 get version info
            {
              ANYCUBIC_SERIAL_PROTOCOLPGM("J33 ");
              ANYCUBIC_SERIAL_PROTOCOLPGM(MSG_MY_VERSION);
              ANYCUBIC_SERIAL_ENTER();
            }
            break;
          default: break;
        }
      }
      TFTbufindw = (TFTbufindw + 1)%TFTBUFSIZE;
      TFTbuflen += 1;
      serial3_count = 0; //clear buffer
    }
    else
    {
      TFTcmdbuffer[TFTbufindw][serial3_count++] = serial3_char;
    }
  }
}

void AnycubicTFTClass::CommandScan()
{
  CheckHeaterError();
  CheckSDCardChange();
  StateHandler();

  if(TFTbuflen<(TFTBUFSIZE-1))
  GetCommandFromTFT();
  if(TFTbuflen)
  {
    TFTbuflen = (TFTbuflen-1);
    TFTbufindr = (TFTbufindr + 1)%TFTBUFSIZE;
  }
}

void AnycubicTFTClass::HeatingStart()
{
  ANYCUBIC_SERIAL_PROTOCOLPGM("J06"); // J07 hotend heating start
  ANYCUBIC_SERIAL_ENTER();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("TFT Serial Debug: Nozzle is heating... J06");
  #endif
}

void AnycubicTFTClass::HeatingDone()
{
  ANYCUBIC_SERIAL_PROTOCOLPGM("J07"); // J07 hotend heating done
  ANYCUBIC_SERIAL_ENTER();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("TFT Serial Debug: Nozzle heating is done... J07");
  #endif

  if(TFTstate==ANYCUBIC_TFT_STATE_SDPRINT)
  {
    ANYCUBIC_SERIAL_PROTOCOLPGM("J04"); // J04 printing from sd card
    ANYCUBIC_SERIAL_ENTER();
    #ifdef ANYCUBIC_TFT_DEBUG
      SERIAL_ECHOLNPGM("TFT Serial Debug: Continuing SD print after heating... J04");
    #endif
  }
}

void AnycubicTFTClass::BedHeatingStart()
{
  ANYCUBIC_SERIAL_PROTOCOLPGM("J08"); // J08 hotbed heating start
  ANYCUBIC_SERIAL_ENTER();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("TFT Serial Debug: Bed is heating... J08");
  #endif
}

void AnycubicTFTClass::BedHeatingDone()
{
  ANYCUBIC_SERIAL_PROTOCOLPGM("J09"); // J09 hotbed heating done
  ANYCUBIC_SERIAL_ENTER();
  #ifdef ANYCUBIC_TFT_DEBUG
    SERIAL_ECHOLNPGM("TFT Serial Debug: Bed heating is done... J09");
  #endif
}


AnycubicTFTClass AnycubicTFT;
#endif
