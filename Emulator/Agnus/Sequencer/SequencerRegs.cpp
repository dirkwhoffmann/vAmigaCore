// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Agnus.h"

template <Accessor s> void
Sequencer::pokeDDFSTRT(u16 value)
{
    trace(DDF_DEBUG, "pokeDDFSTRT(%x)\n", value);
    
    //      15 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // OCS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 -- --
    // ECS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 H2 --
    
    value &= agnus.ddfMask();
    
    // Schedule the write cycle
    if constexpr (s == ACCESSOR_CPU) {
        agnus.recordRegisterChange(DMA_CYCLES(3), SET_DDFSTRT, value);
    }
    if constexpr (s == ACCESSOR_AGNUS) {
        agnus.recordRegisterChange(DMA_CYCLES(4), SET_DDFSTRT, value);
    }
}

void
Sequencer::setDDFSTRT(u16 old, u16 value)
{
    trace(DDF_DEBUG, "setDDFSTRT(%d, %d)\n", old, value);

    ddfstrt = value;

    auto posh = agnus.pos.h;
    
    if (posh == old) {
        trace(XFILES, "setDDFSTRT: Old value matches trigger position\n");
    }
    if (posh == value) {
        trace(XFILES, "setDDFSTRT: New value matches trigger position\n");
    }
        
    // Remove the old start event if it hasn't been reached
    sigRecorder.invalidate(posh, SIG_BPHSTART);
    
    // Add the new start event if it will be reached
    if (ddfstrt > posh) sigRecorder.insert(ddfstrt, SIG_BPHSTART);
    
    // Recompute the event table
    computeBplEventsOld(sigRecorder);
    
    // Tell the hsync handler to recompute the table in the next line
    agnus.hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
}

template <Accessor s> void
Sequencer::pokeDDFSTOP(u16 value)
{
    trace(DDF_DEBUG, "pokeDDFSTOP(%x)\n", value);

    //      15 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // OCS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 -- --
    // ECS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 H2 --
    
    value &= agnus.ddfMask();
    
    // Schedule the write cycle
    if constexpr (s == ACCESSOR_CPU) {
        agnus.recordRegisterChange(DMA_CYCLES(3), SET_DDFSTOP, value);
    }
    if constexpr (s == ACCESSOR_AGNUS) {
        agnus.recordRegisterChange(DMA_CYCLES(4), SET_DDFSTOP, value);
    }
}

void
Sequencer::setDDFSTOP(u16 old, u16 value)
{
    trace(DDF_DEBUG, "setDDFSTOP(%d, %d)\n", old, value);

    ddfstop = value;

    auto posh = agnus.pos.h;
    
    if (posh == old) {
        trace(XFILES, "setDDFSTOP: Old value matches trigger position\n");
    }
    if (posh == value) {
        trace(XFILES, "setDDFSTOP: New value matches trigger position\n");
    }
    
    // Remove the old stop event if it hasn't been reached
    sigRecorder.invalidate(posh + 1, SIG_BPHSTOP);
    
    // Add the new stop event if it will be reached
    if (ddfstop > posh) sigRecorder.insert(ddfstop, SIG_BPHSTOP);
    
    // Recompute the event table
    computeBplEventsOld(sigRecorder);
    
    // Tell the hsync handler to recompute the table in the next line
    agnus.hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
}

void
Sequencer::setDIWSTRT(u16 value)
{
    trace(DIW_DEBUG, "setDIWSTRT(%X)\n", value);
    
    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // V7 V6 V5 V4 V3 V2 V1 V0 -- -- -- -- -- -- -- --  and  V8 = 0
    
    diwstrt = value;
    diwVstrt = HI_BYTE(value);
    
    if (agnus.pos.v == diwVstrt && agnus.pos.v != diwVstop) {
        
        sigRecorder.insert(agnus.pos.h + 2, SIG_VFLOP_SET);
        computeBplEventsOld(sigRecorder);
    }

    agnus.hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
}

void
Sequencer::setDIWSTOP(u16 value)
{
    trace(DIW_DEBUG, "setDIWSTOP(%X)\n", value);
    
    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // V7 V6 V5 V4 V3 V2 V1 V0 -- -- -- -- -- -- -- --  and  V8 = !V7
    
    diwstop = value;
    diwVstop = HI_BYTE(value) | ((value & 0x8000) ? 0 : 0x100);
    
    if (agnus.pos.v == diwVstop) {
        
        sigRecorder.insert(agnus.pos.h + 2, SIG_VFLOP_CLR);
        computeBplEventsOld(sigRecorder);
    }

    if (agnus.pos.v != diwVstop && agnus.pos.v == diwVstrt) {
            
        sigRecorder.insert(agnus.pos.h + 2, SIG_VFLOP_SET);
        computeBplEventsOld(sigRecorder);
    }

    agnus.hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
}

template void Sequencer::pokeDDFSTRT<ACCESSOR_CPU>(u16 value);
template void Sequencer::pokeDDFSTRT<ACCESSOR_AGNUS>(u16 value);
template void Sequencer::pokeDDFSTOP<ACCESSOR_CPU>(u16 value);
template void Sequencer::pokeDDFSTOP<ACCESSOR_AGNUS>(u16 value);