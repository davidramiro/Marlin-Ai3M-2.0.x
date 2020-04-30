/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
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
#pragma once

/**
 * Release version. Leave the Marlin version or apply a custom scheme.
 */
#ifndef SHORT_BUILD_VERSION
  #define SHORT_BUILD_VERSION "2.0.5.3"
#endif

  /**
   * Verbose version identifier which should contain a reference to the location
   * from where the binary was downloaded or the source code was compiled.
   */
  #define DETAILED_BUILD_VERSION SHORT_BUILD_VERSION " (davidramiro, Github)"

/**
 * Defines the version of the Marlin-AI3M build. Not to be confused with
 * Marlin's own build number, e.g. 2.0.x.
 */
 #define CUSTOM_BUILD_VERSION "v2.0.1"

#ifndef STRING_DISTRIBUTION_DATE
  #define STRING_DISTRIBUTION_DATE "2020-03-31"
#endif

/**
 * Minimum Configuration.h and Configuration_adv.h file versions.
 * Set based on the release version number. Used to catch an attempt to use
 * older configurations. Override these if using a custom versioning scheme
 * to alert users to major changes.
 */

#define MARLIN_HEX_VERSION 020005
#ifndef REQUIRED_CONFIGURATION_H_VERSION
  #define REQUIRED_CONFIGURATION_H_VERSION MARLIN_HEX_VERSION
#endif
#ifndef REQUIRED_CONFIGURATION_ADV_H_VERSION
  #define REQUIRED_CONFIGURATION_ADV_H_VERSION MARLIN_HEX_VERSION
#endif

/**
 * The protocol for communication to the host. Protocol indicates communication
 * standards such as the use of ASCII, "echo:" and "error:" line prefixes, etc.
 * (Other behaviors are given by the firmware version and capabilities report.)
 */
#ifndef PROTOCOL_VERSION
  #define PROTOCOL_VERSION "1.0"
#endif

  /**
   * Defines a generic printer name to be output to the LCD after booting Marlin.
   */
  #define MACHINE_NAME "Anycubic i3 Mega"

  /**
   * The SOURCE_CODE_URL is the location where users will find the Marlin Source
   * Code which is installed on the device. In most cases —unless the manufacturer
   * has a distinct Github fork— the Source Code URL should just be the main
   * Marlin repository.
   */
  #define SOURCE_CODE_URL "https://github.com/davidramiro/Marlin-Ai3M-2.0.x"

/**
 * Default generic printer UUID.
 */
#ifndef DEFAULT_MACHINE_UUID
  #define DEFAULT_MACHINE_UUID "cede2a2f-41a2-4748-9b12-c55c62f367ff"
#endif

  /**
   * The WEBSITE_URL is the location where users can get more information such as
   * documentation about a specific Marlin release. Displayed in the Info Menu.
   */
#ifndef WEBSITE_URL
  #define WEBSITE_URL "http://marlinfw.org"
#endif

/**
 * Set the vendor info the serial USB interface, if changable
 * Currently only supported by DUE platform
 */
#ifndef USB_DEVICE_VENDOR_ID
  #define  USB_DEVICE_VENDOR_ID           0x03EB /* ATMEL VID */
#endif
#ifndef USB_DEVICE_PRODUCT_ID
  #define  USB_DEVICE_PRODUCT_ID          0x2424 /* MSC / CDC */
#endif
//! USB Device string definitions (Optional)
#ifndef USB_DEVICE_MANUFACTURE_NAME
  #define  USB_DEVICE_MANUFACTURE_NAME    WEBSITE_URL
#endif
#ifdef CUSTOM_MACHINE_NAME
  #define  USB_DEVICE_PRODUCT_NAME        CUSTOM_MACHINE_NAME
#else
  #define  USB_DEVICE_PRODUCT_NAME        MACHINE_NAME
#endif
#define  USB_DEVICE_SERIAL_NAME           "123985739853"
