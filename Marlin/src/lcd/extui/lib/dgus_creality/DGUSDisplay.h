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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

//#define DEBUG_DGUSLCD
//#define DEBUG_DGUSLCD_OUTGOING_COMM


/* Creality DGUS implementation written by Sebastiaan Dammann in 2020 for Marlin */

#include "../../../../inc/MarlinConfigPre.h"

#include "../../../../MarlinCore.h"
#if HAS_BED_PROBE
  #include "../../../../module/probe.h"
#endif

#include "DGUSVPVariable.h"

enum DGUSLCD_Screens : uint8_t;

#define DEBUG_OUT ENABLED(DEBUG_DGUSLCD)
#include "../../../../core/debug_out.h"


typedef enum : uint8_t {
  DGUS_IDLE,           //< waiting for DGUS_HEADER1.
  DGUS_HEADER1_SEEN,   //< DGUS_HEADER1 received
  DGUS_HEADER2_SEEN,   //< DGUS_HEADER2 received
  DGUS_WAIT_TELEGRAM,  //< LEN received, Waiting for to receive all bytes.
} rx_datagram_state_t;

typedef void (*UPDATE_CURRENT_SCREEN_CALLBACK)(DGUSLCD_Screens screen);

// Low-Level access to the display.
class DGUSDisplay {
public:

  DGUSDisplay() = default;

  static void InitDisplay();
  static void ResetDisplay();

  // Variable access.
  static void WriteVariable(uint16_t adr, const void* values, uint8_t valueslen, bool isstr=false, char fillChar = ' ');
  static void WriteVariablePGM(uint16_t adr, const void* values, uint8_t valueslen, bool isstr=false, char fillChar = ' ');
  static void WriteVariable(uint16_t adr, int16_t value);
  static void WriteVariable(uint16_t adr, uint16_t value);
  static void WriteVariable(uint16_t adr, uint8_t value);
  static void WriteVariable(uint16_t adr, int8_t value);
  static void WriteVariable(uint16_t adr, long value);
  static void WriteVariable(uint16_t adr, float value);

  static void SetVariableDisplayColor(uint16_t sp, uint16_t color);

  static void ReadVariable(uint16_t adr);

  // Utility functions for bridging ui_api and dgus
  template<typename T, float(*Getter)(const T), T selector, typename WireType=uint16_t>
  static void SetVariable(DGUS_VP_Variable &var) {
    WriteVariable(var.VP, (WireType)Getter(selector));
  }

  template<typename T, void(*Setter)(const float V, const T), T selector>
  static void GetVariable(DGUS_VP_Variable &var, void *val_ptr) {
    uint16_t newvalue = swap16(*(uint16_t*)val_ptr);
    Setter(newvalue, selector);
  }

  // Force display into another screen.
  // (And trigger update of containing VPs)
  // (to implement a pop up message, which may not be nested)
  static void RequestScreen(DGUSLCD_Screens screen);

  static void SetTouchScreenConfiguration(bool enable_standby, bool enable_sound, uint8_t standby_brightness, uint8_t active_brightness, uint16_t standbyTimeSeconds);

  // Periodic tasks, eg. Rx-Queue handling.
  static void loop();

public:
  // Helper for users of this class to estimate if an interaction would be blocking.
  static size_t GetFreeTxBuffer();

  // Checks two things: Can we confirm the presence of the display and has we initiliazed it.
  // (both boils down that the display answered to our chatting)
  static inline bool isInitialized() { return Initialized; }

private:
  static void WriteHeader(uint16_t adr, uint8_t cmd, uint8_t payloadlen);
  static void WritePGM(const char str[], uint8_t len);
  static void ProcessRx();

  static inline uint16_t swap16(const uint16_t value) { return (value & 0xffU) << 8U | (value >> 8U); }
  static rx_datagram_state_t rx_datagram_state;
  static uint8_t rx_datagram_len;
  static bool Initialized, no_reentrance;

  static DGUSLCD_Screens displayRequest;
};

#define GET_VARIABLE(f, t, V...) (&DGUSDisplay::GetVariable<decltype(t), f, t, ##V>)
#define SET_VARIABLE(f, t, V...) (&DGUSDisplay::SetVariable<decltype(t), f, t, ##V>)

extern DGUSDisplay dgusdisplay;

// compile-time x^y
template<typename T>
constexpr T cpow(const T x, const int y) { return y == 0 ? 1.0 : x * cpow(x, y - 1); }

/// Find the flash address of a DGUS_VP_Variable for the VP.
const DGUS_VP_Variable* DGUSLCD_FindVPVar(const uint16_t vp);

/// Helper to populate a DGUS_VP_Variable for a given VP. Return false if not found.
bool populate_VPVar(const uint16_t VP, DGUS_VP_Variable * const ramcopy);
