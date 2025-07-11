/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2025 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _CHIP_UTILS_H
#define _CHIP_UTILS_H

#include "defines.h"
#include "macroInt.h"

// custom clock limits
#define MIN_CUSTOM_CLOCK 44100
#define MAX_CUSTOM_CLOCK 2147483647

// common shared channel struct
template<typename T> struct SharedChannel {
  int freq, baseFreq, baseNoteOverride, pitch, pitch2, arpOff;
  int ins, note, sampleNote, sampleNoteDelta;
  bool active, insChanged, freqChanged, fixedArp, keyOn, keyOff, portaPause, inPorta;
  T vol, outVol;
  DivMacroInt std;
  void handleArp(int offset=0) {
    if (std.arp.had) {
      if (std.arp.val<0) {
        if (!(std.arp.val&0x40000000)) {
          baseNoteOverride=(std.arp.val|0x40000000)+offset;
          fixedArp=true;
        } else {
          arpOff=std.arp.val;
          fixedArp=false;
        }
      } else {
        if (std.arp.val&0x40000000) {
          baseNoteOverride=(std.arp.val&(~0x40000000))+offset;
          fixedArp=true;
        } else {
          arpOff=std.arp.val;
          fixedArp=false;
        }
      }
      freqChanged=true;
    }
  }
  void macroInit(DivInstrument* which) {
    std.init(which);
    pitch2=0;
    arpOff=0;
    baseNoteOverride=0;
    fixedArp=false;
  }
  SharedChannel(T initVol):
    freq(0),
    baseFreq(0),
    baseNoteOverride(0),
    pitch(0),
    pitch2(0),
    arpOff(0),
    ins(-1),
    note(0),
    sampleNote(DIV_NOTE_NULL),
    sampleNoteDelta(0),
    active(false),
    insChanged(true),
    freqChanged(false),
    fixedArp(false),
    keyOn(false),
    keyOff(false),
    portaPause(false),
    inPorta(false),
    vol(initVol),
    outVol(initVol),
    std() {} 
};

#endif
