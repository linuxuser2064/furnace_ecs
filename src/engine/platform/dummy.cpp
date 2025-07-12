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

#include "dummy.h"
#include "../engine.h"
#include <stdio.h>
#include <math.h>

#define CHIP_DIVIDER 1
int setVolume = 0;
int ch1Counter = 0;
int ch1PulsePhase = 0;
int ch1Output = 0;
int ch2Counter = 0;
int ch2Output = 0;
int noiseCounter = 0;
int noiseOutput = 0;
bool ringmodCh1 = false;
bool pulseModeCh1 = false;
bool shortNoiseMode = false;
short lfsrValue = 0x3FF;
int lfsrMode = 10;

void SetLFSRMode(int bits) {
  lfsrMode = bits;
  lfsrValue = (1 << bits) - 1;
}

int StepLFSR() {
  int feedback = 0;
  switch (lfsrMode) {
  case 10:
    // taps: bit 9 and 6
    feedback = ((lfsrValue >> 9) ^ (lfsrValue >> 6)) & 1;
    lfsrValue = ((lfsrValue << 1) | feedback) & 0x3FF;
    break;

  case 4:
    // taps: bit 3 and 2
    feedback = ((lfsrValue >> 3) ^ (lfsrValue >> 2)) & 1;
    lfsrValue = ((lfsrValue << 1) | feedback) & 0x0F;
    break;

  case 3:
    // taps: bit 2 and 1
    feedback = ((lfsrValue >> 2) ^ (lfsrValue >> 1)) & 1;
    lfsrValue = ((lfsrValue << 1) | feedback) & 0x07;
    break;

  default:
    break;
  }

  return lfsrValue & 1;
}
void DivPlatformDummy::acquire(short** buf, size_t len) {

  for (int i = 0; i < chans; i++) {
    oscBuf[i]->begin(len);
  }

  for (size_t i = 0; i < len; i++) {
    int out = 0;
    for (unsigned char j = 0; j < chans; j++) {
      if (chan[j].active) {
        if (!isMuted[j]) {
          if (j == 0) { // channel 1
            if (pulseModeCh1) { // pulse mode
              ch1Counter += 1;
              int highTime = chan[j].freq + 1;
              int cycleLength = 4 * (chan[j].freq + 2);
              if (ch1Counter >= cycleLength) {
                ch1Counter = 0;
                ch1PulsePhase = 0;
              }
              ch1Output = (ch1Counter < highTime) ? (chan[j].vol * 768) : 0;
            }
            else { // regular mode
              if (ch1Counter > chan[j].freq) { // toggle output
                ch1Counter = 0;
                ch1Output = (ch1Output > 0) ? 0 : (chan[j].vol * 768);
              }
              else {
                ch1Counter++;
              }
            }
            oscBuf[j]->putSample(i, ch1Output << 1);
          }
          else if (j == 1) {
            if (ch2Counter > chan[j].freq) { // toggle output
              ch2Counter = 0;
              ch2Output = (ch2Output > 0) ? 0 : (chan[j].vol * 768);
            }
            else {
              ch2Counter++;
            }
            oscBuf[j]->putSample(i, ch2Output << 1);
          }
          else if (j == 2) { // noise channel (WIP)
            if (noiseCounter > chan[j].freq) {
              noiseCounter = 0;
              noiseOutput = StepLFSR() * (chan[j].vol * 768);
            }
            else {
              noiseCounter++;
            }
            oscBuf[j]->putSample(i, noiseOutput << 1);
          }
        }
        else {
          oscBuf[j]->putSample(i, 0);
        }
        chan[j].pos += chan[j].freq;
      }
      else {
        oscBuf[j]->putSample(i, 0);
      }
    }
    if (ringmodCh1) { out = (ch1Output * ch2Output) + noiseOutput; }
    else { out = ch1Output + ch2Output + noiseOutput; }
    if (out < -32768) out = -32768;
    if (out > 32767) out = 32767;
    buf[0][i] = out;
  }
  for (int i = 0; i < chans; i++) {
    oscBuf[i]->end(len);
  }
}

void DivPlatformDummy::muteChannel(int ch, bool mute) {
  isMuted[ch] = mute;
}

void DivPlatformDummy::tick(bool sysTick) { // sysTick every frame (specified tick rate)
  for (unsigned char i = 0; i < chans; i++) {
    if (sysTick) {

    }

    if (chan[i].freqChanged) {
      chan[i].freqChanged = false;
      if (i == 2) {
        chan[i].freq = parent->calcFreq(chan[i].baseFreq, chan[i].pitch, 0, false, true, 0, 0, chipClock, 7, 0, 0);
        if (chan[i].freq > 255) { chan[i].freq = 255; }
        if (chan[i].freq < 0) { chan[i].freq = 0; }
      }
      else {
        chan[i].freq = parent->calcFreq(chan[i].baseFreq, chan[i].pitch, 0, false, true, 0, 0, chipClock, 1, 0, 0);
        if (chan[i].freq > 255) { chan[i].freq = 255; }
      }
    }
  }
}

void* DivPlatformDummy::getChanState(int ch) {
  return &chan[ch];
}

DivDispatchOscBuffer* DivPlatformDummy::getOscBuffer(int ch) {
  return oscBuf[ch];
}

int DivPlatformDummy::dispatch(DivCommand c) {
  switch (c.cmd) {
  case DIV_CMD_NOTE_ON:
    if (c.value != DIV_NOTE_NULL) {
      chan[c.chan].baseFreq = NOTE_PERIODIC(c.value);
      chan[c.chan].freqChanged = true;
    }
    chan[c.chan].active = true;
    break;
  case DIV_CMD_NOTE_OFF:
    chan[c.chan].active = false;
    break;
  case DIV_CMD_VOLUME:
    chan[c.chan].vol = c.value;
    if (chan[c.chan].vol > 15) chan[c.chan].vol = 15;
    break;
  case DIV_CMD_GET_VOLUME:
    return chan[c.chan].vol;
    break;
  case DIV_CMD_PITCH:
    chan[c.chan].pitch = c.value;
    chan[c.chan].freqChanged = true;
    break;
  case DIV_CMD_NOTE_PORTA: {
    int destFreq = NOTE_PERIODIC(c.value2);
    bool return2 = false;
    if (destFreq > chan[c.chan].baseFreq) {
      chan[c.chan].baseFreq += c.value;
      if (chan[c.chan].baseFreq >= destFreq) {
        chan[c.chan].baseFreq = destFreq;
        return2 = true;
      }
    }
    else {
      chan[c.chan].baseFreq -= c.value;
      if (chan[c.chan].baseFreq <= destFreq) {
        chan[c.chan].baseFreq = destFreq;
        return2 = true;
      }
    }
    chan[c.chan].freqChanged = true;
    if (return2) return 2;
    break;
  }
  case DIV_CMD_LEGATO:
    chan[c.chan].baseFreq = NOTE_PERIODIC(c.value);
    chan[c.chan].freqChanged = true;
    break;
  case DIV_CMD_GET_VOLMAX:
    return 15;
    break;
  case DIV_CMD_STD_NOISE_MODE: // pulse mode toggle
    pulseModeCh1 = c.value2;
    break;
  case DIV_CMD_SID3_RING_MOD_SRC: // ringmod toggle
    ringmodCh1 = c.value2;
    break;
  case DIV_CMD_STD_NOISE_FREQ: // short noise toggle
    if (c.value2 > 0) {
      SetLFSRMode(3);
    }
    else {
      SetLFSRMode(10);
    }
    break;
  default:
    break;
  }
  return 1;
}

void DivPlatformDummy::notifyInsDeletion(void* ins) {
  // nothing
}

void DivPlatformDummy::reset() {
  for (int i = 0; i < chans; i++) {
    chan[i] = DivPlatformDummy::Channel();
    chan[i].vol = 0x0f;
  }
}

int DivPlatformDummy::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent = p;
  dumpWrites = false;
  skipRegisterWrites = false;
  for (int i = 0; i < DIV_MAX_CHANS; i++) {
    isMuted[i] = false;
    if (i < channels) {
      oscBuf[i] = new DivDispatchOscBuffer;
      oscBuf[i]->setRate(75000);
    }
  }
  rate = 75000;
  chipClock = 75000;
  chans = channels;
  reset();
  return channels;
}

void DivPlatformDummy::quit() {
  for (int i = 0; i < chans; i++) {
    delete oscBuf[i];
  }
}

DivPlatformDummy::~DivPlatformDummy() {
}
