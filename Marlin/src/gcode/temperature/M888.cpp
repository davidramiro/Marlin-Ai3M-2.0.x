/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfig.h"

#include "../gcode.h"
#include "../queue.h"
#include "../parser.h"
#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/temperature.h"
#include "../../lcd/ultralcd.h" // i2c-based BUZZ
#include "../../libs/buzzer.h"  // Buzzer, if possible

#if ENABLED(DELTA)
  #include "../../module/delta.h"
#elif ENABLED(SCARA)
  #include "../../module/scara.h"
#endif

ab_float_t cooldown_arc = { 50, 50 };
const uint8_t cooldown_target = _MAX((parser.ushortval('T', 30)), 15);

/**
 * M888: Cooldown routine for the Anycubic Ultrabase (EXPERIMENTAL):
 *       This is meant to be placed at the end Gcode of your slicer.
 *       It hovers over the print bed and does circular movements while
 *       running the fan. Works best with custom fan ducts.
 *
 *  T<int>   Target bed temperature (min 15°C), 30°C if not specified
 *  S<int>   Fan speed between 0 and 255, full speed if not specified
 */

void plan_arc(const xyze_pos_t &cart, const ab_float_t &offset, const uint8_t clockwise);

void GcodeSuite::M888() {

    // don't do this if the machine is not homed
    if (axis_unhomed_error()) return;

    // set hotbed temperate to zero
    thermalManager.setTargetBed(0);
    SERIAL_ECHOLNPGM("Ultrabase cooldown started");

    // set fan to speed <S>, if undefined blast at full speed
    uint8_t cooldown_fanspeed = parser.ushortval('S', 255);
    thermalManager.set_fan_speed(0, _MIN(cooldown_fanspeed, 255U));

    // raise z by 2mm and move to X50, Y50
    do_blocking_move_to_z(_MIN(current_position[Z_AXIS] + 2, Z_MAX_POS), 5);
    do_blocking_move_to_xy(50, 50, 100);

    while ((thermalManager.degBed() > cooldown_target)) {
      // queue arc movement
      get_destination_from_command();
      plan_arc(destination, cooldown_arc, true);
      SERIAL_ECHOLNPGM("Target not reached, queued an arc");

      // delay while arc is in progress
      while (planner.movesplanned()) {
        idle();
      }
    idle();
    }
    // the bed should be under <T> now
    thermalManager.set_fan_speed(0,0);
    do_blocking_move_to_xy(_MAX(X_MIN_POS, 10), _MIN(Y_MAX_POS, 190), 100);
    BUZZ(100, 659);
    BUZZ(150, 1318);
    queue.enqueue_now_P("M84");
    SERIAL_ECHOLNPGM("M888 cooldown routine done");
  }
