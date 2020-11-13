// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"  

AmigaComponent::AmigaComponent(Amiga& ref) :
agnus(ref.agnus),
amiga(ref),
audioUnit(ref.paula.audioUnit),
blitter(ref.agnus.blitter),
controlPort1(ref.controlPort1),
controlPort2(ref.controlPort2),
copper(ref.agnus.copper),
cpu(ref.cpu),
ciaa(ref.ciaA),
ciab(ref.ciaB),
denise(ref.denise),
df0(ref.df0),
df1(ref.df1),
df2(ref.df2),
df3(ref.df3),
diskController(ref.paula.diskController),
dmaDebugger(ref.agnus.dmaDebugger),
keyboard(ref.keyboard),
mem(ref.mem),
messageQueue(ref.messageQueue),
oscillator(ref.oscillator),
paula(ref.paula),
pixelEngine(ref.denise.pixelEngine),
rtc(ref.rtc),
serialPort(ref.serialPort),
uart(ref.paula.uart),
zorro(ref.zorro)
{

};

void
AmigaComponent::prefix()
{
    amiga.prefix();
}
