/*  GPLv3 License
 *  
 *  	Copyright (c) Divergence Meter Project by waicool20
 *  	
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "clockMode.h"

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

#include "../DivergenceMeter.h"
#include "../settings.h"
#include "../constants.h"
#include "../util/display.h"
#include "../util/BCD.h"

/* Prototypes */

static void clockMode_displayCurrentTime();
static void clockMode_displayDates(bool roll);
static void clockMode_displayCurrentDate(bool roll);
static void clockMode_displayCurrentDayOfWeek();
static void clockMode_displayArmedAlarms();

/* Clock Mode Code */

void clockMode_run() {
  if (justEnteredMode[CLOCK_MODE]) {
    justEnteredMode[CLOCK_MODE] = false;
  }
  switch (settings.time[SECONDS]) {
    case 0x00:
      if (!((settings.main[REST_ON_MINUTE] == settings.main[WAKE_ON_MINUTE])
          && (settings.main[REST_ON_HOUR] == settings.main[WAKE_ON_HOUR]))) {
        if (settings.time[MINUTES] == settings.main[REST_ON_MINUTE]
            && settings.time[HOURS] == settings.main[REST_ON_HOUR]
            && settings.time[SECONDS] == 0x00) {
          DivergenceMeter_switchMode(REST_MODE, false);
          return;
        }
      }
      if (!ringDuration) {
        clockMode_displayDates(true);
      }
      break;
  }
  if (buttonShortPressed[BUTTON2]) {
    clockMode_displayDates(false);
  } else if (buttonLongPressed[BUTTON2]) {
    DivergenceMeter_switchMode(CLOCK_SET_MODE, false);
  } else if (buttonShortPressed[BUTTON3]) {
    if (ringDuration) {
      ringDuration = 0;
      DivergenceMeter_delayCS(s2cs(0.2));
    } else {
      uint8_t alarmArmState = settings.control & 0x03;
      alarmArmState = alarmArmState < 3 ? alarmArmState + 1 : 0;
      settings.control &= 0xFC;
      settings.control |= alarmArmState;
      settings_writeControlDS3232();
      clockMode_displayArmedAlarms();
      DivergenceMeter_delayCS(s2cs(ALARM_ARM_DISPLAY_S));
    }
  } else if (buttonLongPressed[BUTTON3]) {
    DivergenceMeter_switchMode(ALARM_SET_MODE, false);
  } else if (buttonIsPressed[BUTTON4]) {
    shouldRoll = true;
    DivergenceMeter_rollRandomWorldLineWithDelay(false);
  } else if (buttonIsPressed[BUTTON5]) {
    display_toggleBrightness();
    DivergenceMeter_showBrightness();
    return;
  }
  clockMode_displayCurrentTime();
}

static void clockMode_displayCurrentTime() {
  if (settings.main[TIME_FORMAT_12H]) {
    uint8_t hour = settings.time[HOURS];
    if (hour >= 0x13) {
      hour = BCD_sub(hour, 0x12);  //PM
    } else if (hour >= 0x12) {
      //PM
    } else if (hour >= 0x1) {
      //AM
    } else {
      hour = BCD_add(hour, 0x12);  //AM
    }
    display_setTube(TUBE1, ((hour >> 4) & 0x03), false, false);
    display_setTube(TUBE2, (hour & 0x0F), false, false);
  } else {
    display_setTube(TUBE1, ((settings.time[HOURS] >> 4) & 0x03), false, false);
    display_setTube(TUBE2, (settings.time[HOURS] & 0x0F), false, false);
  }
  bool oddSecond = (settings.time[SECONDS] & 0x01);
  display_setTube(TUBE3, BLANK, oddSecond, !oddSecond);

  display_setTube(TUBE4, (settings.time[MINUTES] >> 4), false, false);
  display_setTube(TUBE5, (settings.time[MINUTES] & 0x0F), false, false);
  display_setTube(TUBE6, BLANK, oddSecond, !oddSecond);
  display_setTube(TUBE7, (settings.time[SECONDS] >> 4), false, false);
  display_setTube(TUBE8, (settings.time[SECONDS] & 0x0F), false, false);
  display_update();
}

static void clockMode_displayDates(bool roll) {
  clockMode_displayCurrentDate(roll);
  DivergenceMeter_delayCS(s2cs(DATE_DISPLAY_S));
  clockMode_displayCurrentDayOfWeek();
  DivergenceMeter_delayCS(s2cs(DAY_DISPLAY_S));
}

static void clockMode_displayCurrentDate(bool roll) {
  uint8_t result[8] = { (settings.time[
      settings.main[DATE_FORMAT_DD_MM] ? DATE : MONTH] >> 4) & 0x03, settings
      .time[settings.main[DATE_FORMAT_DD_MM] ? DATE : MONTH] & 0x0F,
  BLANK, (settings.time[settings.main[DATE_FORMAT_DD_MM] ? MONTH : DATE] >> 4)
      & 0x03, settings.time[settings.main[DATE_FORMAT_DD_MM] ? MONTH : DATE]
      & 0x0F,
  BLANK, settings.time[YEAR] >> 4, settings.time[YEAR] & 0x0F };
  if (roll) {
    shouldRoll = true;
    DivergenceMeter_rollWorldLine(true, result);
    shouldRoll = false;
  } else {
    display_setTube(TUBE1, result[0], false, false);
    display_setTube(TUBE2, result[1], false, false);
    display_setTube(TUBE3, result[2], false, false);
    display_setTube(TUBE4, result[3], false, false);
    display_setTube(TUBE5, result[4], false, false);
    display_setTube(TUBE6, result[5], false, false);
    display_setTube(TUBE7, result[6], false, false);
    display_setTube(TUBE8, result[7], false, false);
    display_update();
  }
}

static void clockMode_displayCurrentDayOfWeek() {
  display_setTube(TUBE1, BLANK, false, false);
  display_setTube(TUBE2, BLANK, false, false);
  display_setTube(TUBE3, BLANK, false, false);
  display_setTube(TUBE4, 0, false, false);
  display_setTube(TUBE5, (settings.time[DAY_OF_WEEK]), false, false);
  display_setTube(TUBE6, BLANK, false, false);
  display_setTube(TUBE7, BLANK, false, false);
  display_setTube(TUBE8, BLANK, false, false);
  display_update();
}

static void clockMode_displayArmedAlarms() {
  display_setTube(TUBE1, BLANK, false, false);
  display_setTube(TUBE2, BLANK, false, false);
  display_setTube(TUBE3, BLANK, false, false);
  display_setTube(TUBE4, settings.control & 0x01, false, false);
  display_setTube(TUBE5, (settings.control >> 1) & 0x01, false, false);
  display_setTube(TUBE6, BLANK, false, false);
  display_setTube(TUBE7, BLANK, false, false);
  display_setTube(TUBE8, BLANK, false, false);
  display_update();
}
