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
#include "Denise.h"
#include "Paula.h"

#include "Amiga.h"

u16
Agnus::peekDMACONR()
{
    u16 result = dmacon;
    
    assert((result & ((1 << 14) | (1 << 13))) == 0);
    
    if (blitter.isBusy()) result |= (1 << 14);
    if (blitter.isZero()) result |= (1 << 13);
    
    return result;
}

void
Agnus::pokeDMACON(u16 value)
{
    trace(DMA_DEBUG, "pokeDMACON(%X)\n", value);
    
    setDMACON(dmacon, value);
}

void
Agnus::setDMACON(u16 oldValue, u16 value)
{
    trace(DMA_DEBUG, "setDMACON(%x, %x)\n", oldValue, value);
    
    // Compute new value
    u16 newValue;
    if (value & 0x8000) {
        newValue = (dmacon | value) & 0x07FF;
    } else {
        newValue = (dmacon & ~value) & 0x07FF;
    }
    
    if (oldValue == newValue) return;
    
    dmacon = newValue;
    
    // Update variable dmaconAtDDFStrt if DDFSTRT has not been reached yet
    if (pos.h + 2 < ddfstrtReached) dmaconAtDDFStrt = newValue;
    
    // Check the lowest 5 bits
    bool oldDMAEN = (oldValue & DMAEN);
    bool oldBPLEN = (oldValue & BPLEN) && oldDMAEN;
    bool oldCOPEN = (oldValue & COPEN) && oldDMAEN;
    bool oldBLTEN = (oldValue & BLTEN) && oldDMAEN;
    bool oldSPREN = (oldValue & SPREN) && oldDMAEN;
    bool oldDSKEN = (oldValue & DSKEN) && oldDMAEN;
    bool oldAUD0EN = (oldValue & AUD0EN) && oldDMAEN;
    bool oldAUD1EN = (oldValue & AUD1EN) && oldDMAEN;
    bool oldAUD2EN = (oldValue & AUD2EN) && oldDMAEN;
    bool oldAUD3EN = (oldValue & AUD3EN) && oldDMAEN;
    
    bool newDMAEN = (newValue & DMAEN);
    bool newBPLEN = (newValue & BPLEN) && newDMAEN;
    bool newCOPEN = (newValue & COPEN) && newDMAEN;
    bool newBLTEN = (newValue & BLTEN) && newDMAEN;
    bool newSPREN = (newValue & SPREN) && newDMAEN;
    bool newDSKEN = (newValue & DSKEN) && newDMAEN;
    bool newAUD0EN = (newValue & AUD0EN) && newDMAEN;
    bool newAUD1EN = (newValue & AUD1EN) && newDMAEN;
    bool newAUD2EN = (newValue & AUD2EN) && newDMAEN;
    bool newAUD3EN = (newValue & AUD3EN) && newDMAEN;
        
    // Inform the delegates
    blitter.pokeDMACON(oldValue, newValue);
    
    // Bitplane DMA
    if (oldBPLEN ^ newBPLEN) {
        
        if (isOCS()) {
            newBPLEN ? enableBplDmaOCS() : disableBplDmaOCS();
        } else {
            newBPLEN ? enableBplDmaECS() : disableBplDmaECS();
        }
        
        hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
    }
    
    // Let Denise know about the change
    denise.pokeDMACON(oldValue, newValue);
        
    // Disk DMA and sprite DMA
    if ((oldDSKEN ^ newDSKEN) || (oldSPREN ^ newSPREN)) {
        
        // Note: We don't need to rebuild the table if audio DMA changes,
        // because audio events are always executed.
        
        if (oldSPREN ^ newSPREN)
            trace(DMA_DEBUG, "Sprite DMA %s\n", newSPREN ? "on" : "off");
        if (oldDSKEN ^ newDSKEN)
            trace(DMA_DEBUG, "Disk DMA %s\n", newDSKEN ? "on" : "off");
        
        u16 newDAS = newDMAEN ? (newValue & 0x3F) : 0;
        
        // Schedule the DAS DMA table to be rebuild
        hsyncActions |= HSYNC_UPDATE_DAS_TABLE;
        
        // Make the effect visible in the current rasterline as well
        for (isize i = pos.h; i < HPOS_CNT; i++) {
            dasEvent[i] = dasDMA[newDAS][i];
        }
        updateDasJumpTable();
        
        // Rectify the currently scheduled DAS event
        scheduleDasEventForCycle(pos.h);
    }
    
    // Copper DMA
    if (oldCOPEN ^ newCOPEN) {
        trace(DMA_DEBUG, "Copper DMA %s\n", newCOPEN ? "on" : "off");
        if (newCOPEN) copper.activeInThisFrame = true;
    }
    
    // Blitter DMA
    if (oldBLTEN ^ newBLTEN) {
        trace(DMA_DEBUG, "Blitter DMA %s\n", newBLTEN ? "on" : "off");
    }
    
    // Audio DMA
    if (oldAUD0EN ^ newAUD0EN) {
        newAUD0EN ? paula.channel0.enableDMA() : paula.channel0.disableDMA();
    }
    if (oldAUD1EN ^ newAUD1EN) {
        newAUD1EN ? paula.channel1.enableDMA() : paula.channel1.disableDMA();
    }
    if (oldAUD2EN ^ newAUD2EN) {
        newAUD2EN ? paula.channel2.enableDMA() : paula.channel2.disableDMA();
    }
    if (oldAUD3EN ^ newAUD3EN) {
        newAUD3EN ? paula.channel3.enableDMA() : paula.channel3.disableDMA();
    }
}

u16
Agnus::peekVHPOSR()
{
    // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // V7 V6 V5 V4 V3 V2 V1 V0 H8 H7 H6 H5 H4 H3 H2 H1
    
    // Return the latched position if the counters are frozen
    if (ersy()) return HI_LO(latchedPos.v & 0xFF, latchedPos.h);
                     
    auto posh = pos.h + 4;
    auto posv = pos.v;
    
    // Check if posh has wrapped over (we just added 4)
    if (posh > HPOS_MAX) {
        posh -= HPOS_CNT;
        if (++posv >= frame.numLines()) posv = 0;
    }
    
    // The value of posv only shows up in cycle 2 and later
    if (posh > 1) {
        return HI_LO(posv & 0xFF, posh);
    }
    
    // In cycle 0 and 1, we need to return the old value of posv
    if (posv > 0) {
        return HI_LO((posv - 1) & 0xFF, posh);
    } else {
        return HI_LO(frame.prevLastLine() & 0xFF, posh);
    }
}

void
Agnus::pokeVHPOS(u16 value)
{
    trace(POSREG_DEBUG, "pokeVHPOS(%X)\n", value);
    
    setVHPOS(value);
}

void
Agnus::setVHPOS(u16 value)
{
    [[maybe_unused]] int v7v0 = HI_BYTE(value);
    [[maybe_unused]] int h8h1 = LO_BYTE(value);
    
    trace(XFILES, "XFILES (VHPOS): %x (%d,%d)\n", value, v7v0, h8h1);

    // Don't know what to do here ...
}

u16
Agnus::peekVPOSR()
{
    // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // LF I6 I5 I4 I3 I2 I1 I0 -- -- -- -- -- -- -- V8
 
    // I5 I4 I3 I2 I1 I0 (Chip Identification)
    u16 result = idBits();

    // LF (Long frame bit)
    if (frame.isLongFrame()) result |= 0x8000;

    // V8 (Vertical position MSB)
    result |= (ersy() ? latchedPos.v : pos.v) >> 8;
    
    trace(POSREG_DEBUG, "peekVPOSR() = %X\n", result);
    return result;
}

void
Agnus::pokeVPOS(u16 value)
{
    trace(POSREG_DEBUG, "pokeVPOS(%x) (%zd,%d)\n", value, pos.v, frame.lof);
    
    setVPOS(value);
}

void
Agnus::setVPOS(u16 value)
{
    /* I don't really know what exactly we are supposed to do here.
     * For the time being, I only take care of the LOF bit.
     */
    bool newlof = value & 0x8000;
    if (frame.lof == newlof) return;
    
    trace(XFILES, "XFILES (VPOS): %x (%zd,%d)\n", value, pos.v, frame.lof);

    /* If a long frame gets changed to a short frame, we only proceed if
     * Agnus is not in the last rasterline. Otherwise, we would corrupt the
     * emulators internal state (we would be in a line that is unreachable).
     */
    if (!newlof && inLastRasterline()) return;

    trace(XFILES, "XFILES (VPOS): Making a %s frame\n", newlof ? "long" : "short");
    frame.lof = newlof;
    
    /* Reschedule a pending VBL event with a trigger cycle that is consistent
     * with the new value of the LOF bit.
     */
    switch (scheduler.id[SLOT_VBL]) {

        case VBL_STROBE0: scheduleStrobe0Event(); break;
        case VBL_STROBE1: scheduleStrobe1Event(); break;
        case VBL_STROBE2: scheduleStrobe2Event(); break;
            
        default: break;
    }
}

void
Agnus::pokeBPLCON0(u16 value)
{
    trace(DMA_DEBUG, "pokeBPLCON0(%X)\n", value);

    recordRegisterChange(DMA_CYCLES(4), SET_BPLCON0_AGNUS, value);
}

void
Agnus::setBPLCON0(u16 oldValue, u16 newValue)
{
    trace(DMA_DEBUG, "setBPLCON0(%X,%X)\n", oldValue, newValue);
    
    // Update variable bplcon0AtDDFStrt if DDFSTRT has not been reached yet
    if (pos.h < ddfstrtReached) bplcon0AtDDFStrt = newValue;
    
    // Update the bpl event table in the next rasterline
    hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
    
    // Check if the hires bit or one of the BPU bits have been modified
    if ((oldValue ^ newValue) & 0xF000) {
    
        /*
        if ((oldValue ^ newValue) & 0x8000) {
            if (newValue & 0x8000) {
                if (agnus.frame.nr > 2000) amiga.signalStop();
            }
        }
        */
        
        /* TODO:
         * BPLCON0 is usually written in each frame. To speed up, just check
         * hpos. If it is smaller than the start of the DMA window, a standard
         * update() is enough and the scheduled update in hsyncActions
         * (HSYNC_UPDATE_BPL_TABLE) can be omitted.
         */
        
        // Update the DMA allocation table
        updateBplEvents(dmaconAtDDFStrt, newValue, pos.h);
        
        // Since the table has changed, we also need to update the event slot
        scheduleBplEventForCycle(pos.h);
    }
    
    // Latch the position counters if the ERSY bit is set
    if ((newValue & 0b10) && !(oldValue & 0b10)) latchedPos = pos;

    bplcon0 = newValue;
}

void
Agnus::pokeBPLCON1(u16 value)
{
    trace(DMA_DEBUG, "pokeBPLCON1(%X)\n", value);
    
    if (bplcon1 != value) {
        recordRegisterChange(DMA_CYCLES(1), SET_BPLCON1_AGNUS, value);
    }
}

void
Agnus::setBPLCON1(u16 oldValue, u16 newValue)
{
    assert(oldValue != newValue);
    trace(DMA_DEBUG, "setBPLCON1(%X,%X)\n", oldValue, newValue);
    
    bplcon1 = newValue & 0xFF;
    
    // Compute comparision values for the hpos counter
    scrollLoresOdd  = (bplcon1 & 0b00001110) >> 1;
    scrollLoresEven = (bplcon1 & 0b11100000) >> 5;
    scrollHiresOdd  = (bplcon1 & 0b00000110) >> 1;
    scrollHiresEven = (bplcon1 & 0b01100000) >> 5;
    
    // Update the bitplane event table starting at the current hpos
    updateBplEvents(pos.h);
    
    // Update the scheduled bitplane event according to the new table
    scheduleBplEventForCycle(pos.h);
    
    // Schedule the bitplane event table to be recomputed
    agnus.hsyncActions |= HSYNC_UPDATE_BPL_TABLE;
}

template <Accessor s> void
Agnus::pokeDIWSTRT(u16 value)
{
    trace(DIW_DEBUG, "pokeDIWSTRT<%s>(%X)\n", AccessorEnum::key(s), value);
    recordRegisterChange(DMA_CYCLES(2), SET_DIWSTRT, value);
}

void
Agnus::setDIWSTRT(u16 value)
{
    trace(DIW_DEBUG, "setDIWSTRT(%X)\n", value);
    
    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // V7 V6 V5 V4 V3 V2 V1 V0 H7 H6 H5 H4 H3 H2 H1 H0  and  H8 = 0, V8 = 0
    
    diwstrt = value;
    
    // Extract the upper left corner of the display window
    isize newDiwVstrt = HI_BYTE(value);
    isize newDiwHstrt = LO_BYTE(value);
    
    trace(DIW_DEBUG, "newDiwVstrt = %zd newDiwHstrt = %zd\n", newDiwVstrt, newDiwHstrt);
    
    // Invalidate the horizontal coordinate if it is out of range
    if (newDiwHstrt < 2) {
        trace(DIW_DEBUG, "newDiwHstrt is too small\n");
        newDiwHstrt = -1;
    }
    
    /* Check if the change already takes effect in the current rasterline.
     *
     *     old: Old trigger coordinate (diwHstrt)
     *     new: New trigger coordinate (newDiwHstrt)
     *     cur: Position of the electron beam (derivable from pos.h)
     *
     * The following cases have to be taken into accout:
     *
     *    1) cur < old < new : Change takes effect in this rasterline.
     *    2) cur < new < old : Change takes effect in this rasterline.
     *    3) new < cur < old : Neither the old nor the new trigger hits.
     *    4) new < old < cur : Already triggered. Nothing to do in this line.
     *    5) old < cur < new : Already triggered. Nothing to do in this line.
     *    6) old < new < cur : Already triggered. Nothing to do in this line.
     */
    
    isize cur = 2 * pos.h;
    
    // (1) and (2)
    if (cur < diwHstrt && cur < newDiwHstrt) {
        
        trace(DIW_DEBUG, "Updating DIW hflop immediately at %zd\n", cur);
        diwHFlopOn = newDiwHstrt;
    }
    
    // (3)
    if (newDiwHstrt < cur && cur < diwHstrt) {
        
        trace(DIW_DEBUG, "DIW hflop not switched on in current line\n");
        diwHFlopOn = -1;
    }
    
    diwVstrt = newDiwVstrt;
    diwHstrt = newDiwHstrt;
    
    /* Update the vertical DIW flipflop
     * This is not 100% accurate. If the vertical DIW flipflop changes in the
     * middle of a rasterline, the effect is immediately visible on a real
     * Amiga. The current emulation code only evaluates the flipflop at the end
     * of the rasterline in the drawing routine of Denise. Hence, the whole
     * line will be blacked out, not just the rest of it.
     */
    if (pos.v == diwVstrt) diwVFlop = true;
    if (pos.v == diwVstop) diwVFlop = false;
}

template <Accessor s> void
Agnus::pokeDIWSTOP(u16 value)
{
    trace(DIW_DEBUG, "pokeDIWSTOP<%s>(%X)\n", AccessorEnum::key(s), value);
    recordRegisterChange(DMA_CYCLES(2), SET_DIWSTOP, value);
}

void
Agnus::setDIWSTOP(u16 value)
{
    trace(DIW_DEBUG, "setDIWSTOP(%X)\n", value);
    
    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // V7 V6 V5 V4 V3 V2 V1 V0 H7 H6 H5 H4 H3 H2 H1 H0  and  H8 = 1, V8 = !V7
    
    diwstop = value;
    
    // Extract the lower right corner of the display window
    isize newDiwVstop = HI_BYTE(value) | ((value & 0x8000) ? 0 : 0x100);
    isize newDiwHstop = LO_BYTE(value) | 0x100;
    
    trace(DIW_DEBUG, "newDiwVstop = %zd newDiwHstop = %zd\n", newDiwVstop, newDiwHstop);
    
    // Invalidate the coordinate if it is out of range
    if (newDiwHstop > 0x1C7) {
        trace(DIW_DEBUG, "newDiwHstop is too large\n");
        newDiwHstop = -1;
    }
    
    // Check if the change already takes effect in the current rasterline.
    isize cur = 2 * pos.h;
    
    // (1) and (2) (see setDIWSTRT)
    if (cur < diwHstop && cur < newDiwHstop) {
        
        trace(DIW_DEBUG, "Updating hFlopOff immediately at %zd\n", cur);
        diwHFlopOff = newDiwHstop;
    }
    
    // (3) (see setDIWSTRT)
    if (newDiwHstop < cur && cur < diwHstop) {
        
        trace(DIW_DEBUG, "hFlop not switched off in current line\n");
        diwHFlopOff = -1;
    }
    
    diwVstop = newDiwVstop;
    diwHstop = newDiwHstop;
    
    /* Update the vertical DIW flipflop
     * This is not 100% accurate. See comment in setDIWSTRT().
     */
    if (pos.v == diwVstrt) diwVFlop = true;
    if (pos.v == diwVstop) diwVFlop = false;
}

void
Agnus::pokeDDFSTRT(u16 value)
{
    trace(DDF_DEBUG, "pokeDDFSTRT(%X)\n", value);
    
    //      15 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // OCS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 -- --
    // ECS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 H2 --
    
    value &= ddfMask();
    recordRegisterChange(DMA_CYCLES(2), SET_DDFSTRT, value);
}

void
Agnus::setDDFSTRT(u16 old, u16 value)
{
    trace(DDF_DEBUG, "setDDFSTRT(%X, %X)\n", old, value);
    
    ddfstrt = value;
    
    // Tell the hsync handler to recompute the DDF window
    hsyncActions |= HSYNC_PREDICT_DDF;
    
    // Take immediate action if we haven't reached the old DDFSTRT cycle yet
    if (pos.h < ddfstrtReached) {
        
        // Check if the new position has already been passed
        if (ddfstrt <= pos.h + 2) {
            
            // DDFSTRT never matches in the current rasterline. Disable DMA
            ddfstrtReached = -1;
            clearBplEvents();
            scheduleNextBplEvent();
            
        } else {
            
            // Update the matching position and recalculate the DMA table
            ddfstrtReached = ddfstrt > HPOS_MAX ? -1 : ddfstrt;
            computeDDFWindow();
            updateBplEvents();
            scheduleNextBplEvent();
        }
    }
}

void
Agnus::pokeDDFSTOP(u16 value)
{
    trace(DDF_DEBUG, "pokeDDFSTOP(%X)\n", value);
    
    //      15 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // OCS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 -- --
    // ECS: -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H3 H2 --
    
    value &= ddfMask();
    recordRegisterChange(DMA_CYCLES(2), SET_DDFSTOP, value);
}

void
Agnus::setDDFSTOP(u16 old, u16 value)
{
    trace(DDF_DEBUG, "setDDFSTOP(%X, %X)\n", old, value);
    
    ddfstop = value;
    
    // Tell the hsync handler to recompute the DDF window
    hsyncActions |= HSYNC_PREDICT_DDF;
    
    // Take action if we haven't reached the old DDFSTOP cycle yet
    if (pos.h + 2 < ddfstopReached || ddfstopReached == -1) {
        
        // Check if the new position has already been passed
        if (ddfstop <= pos.h + 2) {
            
            // DDFSTOP won't match in the current rasterline
            ddfstopReached = -1;
            
        } else {
            
            // Update the matching position and recalculate the DMA table
            ddfstopReached = (ddfstop > HPOS_MAX) ? -1 : ddfstop;
            if (ddfstrtReached >= 0) {
                computeDDFWindow();
                updateBplEvents();
                scheduleNextBplEvent();
            }
        }
    }
}

void
Agnus::pokeBPL1MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "pokeBPL1MOD(%X)\n", value);
    recordRegisterChange(DMA_CYCLES(2), SET_BPL1MOD, value);
}

void
Agnus::setBPL1MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "setBPL1MOD(%X)\n", value);
    bpl1mod = (i16)(value & 0xFFFE);
}

void
Agnus::pokeBPL2MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "pokeBPL2MOD(%X)\n", value);
    recordRegisterChange(DMA_CYCLES(2), SET_BPL2MOD, value);
}

void
Agnus::setBPL2MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "setBPL2MOD(%X)\n", value);
    bpl2mod = (i16)(value & 0xFFFE);
}

template <int x> void
Agnus::pokeSPRxPOS(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dPOS(%X)\n", x, value);

    // Compute the value of the vertical counter that is seen here
    i16 v = (i16)(pos.h < 0xDF ? pos.v : (pos.v + 1));

    // Compute the new vertical start position
    sprVStrt[x] = ((value & 0xFF00) >> 8) | (sprVStrt[x] & 0x0100);

    // Update sprite DMA status
    if (sprVStrt[x] == v) sprDmaState[x] = SPR_DMA_ACTIVE;
    if (sprVStop[x] == v) sprDmaState[x] = SPR_DMA_IDLE;
}

template <int x> void
Agnus::pokeSPRxCTL(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dCTL(%X)\n", x, value);

    // Compute the value of the vertical counter that is seen here
    i16 v = (i16)(pos.h < 0xDF ? pos.v : (pos.v + 1));

    // Compute the new vertical start and stop position
    sprVStrt[x] = (i16)((value & 0b100) << 6 | (sprVStrt[x] & 0x00FF));
    sprVStop[x] = (i16)((value & 0b010) << 7 | (value >> 8));

    // Update sprite DMA status
    if (sprVStrt[x] == v) sprDmaState[x] = SPR_DMA_ACTIVE;
    if (sprVStop[x] == v) sprDmaState[x] = SPR_DMA_IDLE;
}

template <Accessor s>
void Agnus::pokeDSKPTH(u16 value)
{
    trace(DSKREG_DEBUG, "pokeDSKPTH(%04x) [%s]\n", value, AccessorEnum::key(s));

    // Schedule the write cycle
    auto delay = (s == ACCESSOR_CPU) ? DMA_CYCLES(1) : DMA_CYCLES(2);
    recordRegisterChange(delay, SET_DSKPTH_1, value, s);
}

void
Agnus::setDSKPTH1(u16 value, Accessor a)
{
    trace(DSKREG_DEBUG, "setDSKPTH1(%04x) [%s]\n", value, AccessorEnum::key(a));

    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BUS_DISK)) return;
    
    // Perform the write
    dskpt = REPLACE_HI_WORD(dskpt, value);
    
    if (dskpt & ~agnus.ptrMask) {
        trace(XFILES, "DSKPT out of range: %x\n", dskpt);
    }
}

void
Agnus::setDSKPTH2(u16 value, Accessor s)
{
    assert(false);
    
    trace(DSKREG_DEBUG, "setDSKPTH2(%04x)\n", value);

    // Perform the write
    dskpt = REPLACE_HI_WORD(dskpt, value);
    
    if (dskpt & ~agnus.ptrMask) {
        trace(XFILES, "DSKPT out of range: %x\n", dskpt);
    }    
}

template <Accessor s>
void Agnus::pokeDSKPTL(u16 value)
{
    trace(DSKREG_DEBUG, "pokeDSKPTL(%04x) [%s]\n", value, AccessorEnum::key(s));

    // Schedule the write cycle
    auto delay = (s == ACCESSOR_CPU) ? DMA_CYCLES(1) : DMA_CYCLES(2);
    recordRegisterChange(delay, SET_DSKPTL_1, value, s);
}

void
Agnus::setDSKPTL1(u16 value, Accessor a)
{
    trace(DSKREG_DEBUG, "setDSKPTH1(%04x) [%s]\n", value, AccessorEnum::key(a));

    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BUS_DISK)) return;
    
    // Perform the write
    dskpt = REPLACE_LO_WORD(dskpt, value);
}

void
Agnus::setDSKPTL2(u16 value, Accessor a)
{
    assert(false);
    
    trace(DSKREG_DEBUG, "setDSKPTL2(%04x)\n", value);

    // Perform the write
    dskpt = REPLACE_LO_WORD(dskpt, value);
}

template <int x, Accessor s> void
Agnus::pokeAUDxLCH(u16 value)
{
    debug(AUDREG_DEBUG, "pokeAUD%dLCH(%X)\n", x, value);

     audlc[x] = REPLACE_HI_WORD(audlc[x], value);
}

template <int x, Accessor s> void
Agnus::pokeAUDxLCL(u16 value)
{
    trace(AUDREG_DEBUG, "pokeAUD%dLCL(%X)\n", x, value);

    audlc[x] = REPLACE_LO_WORD(audlc[x], value & 0xFFFE);
}

template <int x, Accessor s> void
Agnus::pokeBPLxPTH(u16 value)
{
    trace(BPLREG_DEBUG, "pokeBPL%dPTH(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Execute or schedule the first execution cycle
    switch (s) {
            
        case ACCESSOR_CPU:
            
            setBPLxPTH1 <x> (value);
            break;
            
        case ACCESSOR_AGNUS:
            
            recordRegisterChange(DMA_CYCLES(1), SET_BPL1PTH_1 + x - 1, value);
            break;
    }
}

template <int x> void
Agnus::setBPLxPTH1(u16 value)
{
    trace(BPLREG_DEBUG, "setBPL%dPTH1(%X)\n", x, value);

    // Drop the write if the register is currently in use
    if (isBplDmaCycle<x>() && !NO_PTR_DROPS) {
        
        trace(XFILES, "Dropping write to BPL%dPTH\n", x);
        return;
    }
    
    // Schedule the second execution cycle
    recordRegisterChange(DMA_CYCLES(1), SET_BPL1PTH_2 + x - 1, value);
}

template <int x> void
Agnus::setBPLxPTH2(u16 value)
{
    trace(BPLREG_DEBUG, "setBPL%dPTH2(%04x)\n", x, value);

    // Perform the write
    bplpt[x - 1] = REPLACE_HI_WORD(bplpt[x - 1], value);
}

template <int x, Accessor s> void
Agnus::pokeBPLxPTL(u16 value)
{
    trace(BPLREG_DEBUG, "pokeBPL%dPTL(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Execute or schedule the first execution cycle
    switch (s) {
            
        case ACCESSOR_CPU:
            
            setBPLxPTL1 <x> (value);
            break;
            
        case ACCESSOR_AGNUS:
            
            recordRegisterChange(DMA_CYCLES(1), SET_BPL1PTL_1 + x - 1, value);
            break;
    }
}

template <int x> void
Agnus::setBPLxPTL1(u16 value)
{
    trace(BPLREG_DEBUG, "setBPL%dPTL1(%X)\n", x, value);

    // Drop the write if the register is currently in use
    if (isBplDmaCycle<x>() && !NO_PTR_DROPS) {
        
        trace(XFILES, "Dropping write to BPL%dPTL\n", x);
        return;
    }
    
    // Schedule the second execution cycle
    recordRegisterChange(DMA_CYCLES(1), SET_BPL1PTL_2 + x - 1, value);
}

template <int x> void
Agnus::setBPLxPTL2(u16 value)
{
    trace(BPLREG_DEBUG, "setBPL%dPTL2(%04x)\n", x, value);

    // Perform the write
    bplpt[x - 1] = REPLACE_LO_WORD(bplpt[x - 1], value);
}

template <int x, Accessor s> void
Agnus::pokeSPRxPTH(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dPTH(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Execute or schedule the first execution cycle
    switch (s) {
            
        case ACCESSOR_CPU:
            
            setSPRxPTH1 <x> (value);
            break;
            
        case ACCESSOR_AGNUS:
            
            recordRegisterChange(DMA_CYCLES(1), SET_SPR0PTH_1 + x, value);
            break;
    }
}

template <int x> void
Agnus::setSPRxPTH1(u16 value)
{
    trace(SPRREG_DEBUG, "setSPR%dPTH1(%%04x)\n", x, value);
    
    // Drop the write if the register is currently in use
    if (isSprDmaCycle<x>() && !NO_PTR_DROPS) {
        
        trace(XFILES, "Dropping write to SPR%dPTH\n", x);
        return;
    }
    
    // Schedule the second execution cycle
    recordRegisterChange(DMA_CYCLES(1), SET_SPR0PTH_2 + x, value);
}

template <int x> void
Agnus::setSPRxPTH2(u16 value)
{
    trace(BPLREG_DEBUG, "setSPR%dPTH2(%04x)\n", x, value);

    // Perform the write
    sprpt[x] = REPLACE_HI_WORD(sprpt[x], value);
}

template <int x, Accessor s> void
Agnus::pokeSPRxPTL(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dPTL(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Execute or schedule the first execution cycle
    switch (s) {
            
        case ACCESSOR_CPU:
            
            setSPRxPTL1 <x> (value);
            break;
            
        case ACCESSOR_AGNUS:
            
            recordRegisterChange(DMA_CYCLES(1), SET_SPR0PTL_1 + x, value);
            break;
    }
}

template <int x> void
Agnus::setSPRxPTL1(u16 value)
{
    trace(SPRREG_DEBUG, "setSPR%dPTL1(%04x)\n", x, value);
    
    // Drop the write if the register is currently in use
    if (isSprDmaCycle<x>() && !NO_PTR_DROPS) {
        
        trace(XFILES, "Dropping write to SPR%dPTL\n", x);
        return;
    }
    
    // Schedule the second execution cycle
    recordRegisterChange(DMA_CYCLES(1), SET_SPR0PTL_2 + x, value);
}

template <int x> void
Agnus::setSPRxPTL2(u16 value)
{
    trace(BPLREG_DEBUG, "setSPR%dPTL2(%04x)\n", x, value);

    // Perform the write
    sprpt[x] = REPLACE_LO_WORD(sprpt[x], value);
}

bool
Agnus::dropWrite(BusOwner owner)
{
    /* A write to a pointer register is dropped if the pointer was used one
     * cycle before the update would happen.
     */
    if (!NO_PTR_DROPS && pos.h >= 1 && busOwner[pos.h - 1] == owner) {
        
        trace(XFILES, "XFILES: Dropping pointer register write (%d)\n", owner);
        return true;
    }
    
    return false;
}

template void Agnus::pokeDSKPTH<ACCESSOR_CPU>(u16 value);
template void Agnus::pokeDSKPTH<ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeDSKPTL<ACCESSOR_CPU>(u16 value);
template void Agnus::pokeDSKPTL<ACCESSOR_AGNUS>(u16 value);

template void Agnus::pokeAUDxLCH<0,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeAUDxLCH<1,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeAUDxLCH<2,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeAUDxLCH<3,ACCESSOR_CPU>(u16 value);

template void Agnus::pokeAUDxLCH<0,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeAUDxLCH<1,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeAUDxLCH<2,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeAUDxLCH<3,ACCESSOR_AGNUS>(u16 value);

template void Agnus::pokeAUDxLCL<0,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeAUDxLCL<1,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeAUDxLCL<2,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeAUDxLCL<3,ACCESSOR_CPU>(u16 value);

template void Agnus::pokeAUDxLCL<0,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeAUDxLCL<1,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeAUDxLCL<2,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeAUDxLCL<3,ACCESSOR_AGNUS>(u16 value);

template void Agnus::pokeBPLxPTH<1,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTH<2,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTH<3,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTH<4,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTH<5,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTH<6,ACCESSOR_CPU>(u16 value);

template void Agnus::pokeBPLxPTH<1,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTH<2,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTH<3,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTH<4,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTH<5,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTH<6,ACCESSOR_AGNUS>(u16 value);

template void Agnus::setBPLxPTH1<1>(u16 value);
template void Agnus::setBPLxPTH1<2>(u16 value);
template void Agnus::setBPLxPTH1<3>(u16 value);
template void Agnus::setBPLxPTH1<4>(u16 value);
template void Agnus::setBPLxPTH1<5>(u16 value);
template void Agnus::setBPLxPTH1<6>(u16 value);

template void Agnus::setBPLxPTH2<1>(u16 value);
template void Agnus::setBPLxPTH2<2>(u16 value);
template void Agnus::setBPLxPTH2<3>(u16 value);
template void Agnus::setBPLxPTH2<4>(u16 value);
template void Agnus::setBPLxPTH2<5>(u16 value);
template void Agnus::setBPLxPTH2<6>(u16 value);

template void Agnus::pokeBPLxPTL<1,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTL<2,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTL<3,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTL<4,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTL<5,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeBPLxPTL<6,ACCESSOR_CPU>(u16 value);

template void Agnus::pokeBPLxPTL<1,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTL<2,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTL<3,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTL<4,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTL<5,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeBPLxPTL<6,ACCESSOR_AGNUS>(u16 value);

template void Agnus::setBPLxPTL1<1>(u16 value);
template void Agnus::setBPLxPTL1<2>(u16 value);
template void Agnus::setBPLxPTL1<3>(u16 value);
template void Agnus::setBPLxPTL1<4>(u16 value);
template void Agnus::setBPLxPTL1<5>(u16 value);
template void Agnus::setBPLxPTL1<6>(u16 value);

template void Agnus::setBPLxPTL2<1>(u16 value);
template void Agnus::setBPLxPTL2<2>(u16 value);
template void Agnus::setBPLxPTL2<3>(u16 value);
template void Agnus::setBPLxPTL2<4>(u16 value);
template void Agnus::setBPLxPTL2<5>(u16 value);
template void Agnus::setBPLxPTL2<6>(u16 value);

template void Agnus::pokeSPRxPTH<0,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<1,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<2,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<3,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<4,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<5,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<6,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTH<7,ACCESSOR_CPU>(u16 value);

template void Agnus::pokeSPRxPTH<0,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<1,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<2,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<3,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<4,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<5,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<6,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTH<7,ACCESSOR_AGNUS>(u16 value);

template void Agnus::setSPRxPTH1<0>(u16 value);
template void Agnus::setSPRxPTH1<1>(u16 value);
template void Agnus::setSPRxPTH1<2>(u16 value);
template void Agnus::setSPRxPTH1<3>(u16 value);
template void Agnus::setSPRxPTH1<4>(u16 value);
template void Agnus::setSPRxPTH1<5>(u16 value);
template void Agnus::setSPRxPTH1<6>(u16 value);
template void Agnus::setSPRxPTH1<7>(u16 value);

template void Agnus::setSPRxPTH2<0>(u16 value);
template void Agnus::setSPRxPTH2<1>(u16 value);
template void Agnus::setSPRxPTH2<2>(u16 value);
template void Agnus::setSPRxPTH2<3>(u16 value);
template void Agnus::setSPRxPTH2<4>(u16 value);
template void Agnus::setSPRxPTH2<5>(u16 value);
template void Agnus::setSPRxPTH2<6>(u16 value);
template void Agnus::setSPRxPTH2<7>(u16 value);

template void Agnus::pokeSPRxPTL<0,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<1,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<2,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<3,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<4,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<5,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<6,ACCESSOR_CPU>(u16 value);
template void Agnus::pokeSPRxPTL<7,ACCESSOR_CPU>(u16 value);

template void Agnus::pokeSPRxPTL<0,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<1,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<2,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<3,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<4,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<5,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<6,ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeSPRxPTL<7,ACCESSOR_AGNUS>(u16 value);

template void Agnus::setSPRxPTL1<0>(u16 value);
template void Agnus::setSPRxPTL1<1>(u16 value);
template void Agnus::setSPRxPTL1<2>(u16 value);
template void Agnus::setSPRxPTL1<3>(u16 value);
template void Agnus::setSPRxPTL1<4>(u16 value);
template void Agnus::setSPRxPTL1<5>(u16 value);
template void Agnus::setSPRxPTL1<6>(u16 value);
template void Agnus::setSPRxPTL1<7>(u16 value);

template void Agnus::setSPRxPTL2<0>(u16 value);
template void Agnus::setSPRxPTL2<1>(u16 value);
template void Agnus::setSPRxPTL2<2>(u16 value);
template void Agnus::setSPRxPTL2<3>(u16 value);
template void Agnus::setSPRxPTL2<4>(u16 value);
template void Agnus::setSPRxPTL2<5>(u16 value);
template void Agnus::setSPRxPTL2<6>(u16 value);
template void Agnus::setSPRxPTL2<7>(u16 value);

template void Agnus::pokeSPRxPOS<0>(u16 value);
template void Agnus::pokeSPRxPOS<1>(u16 value);
template void Agnus::pokeSPRxPOS<2>(u16 value);
template void Agnus::pokeSPRxPOS<3>(u16 value);
template void Agnus::pokeSPRxPOS<4>(u16 value);
template void Agnus::pokeSPRxPOS<5>(u16 value);
template void Agnus::pokeSPRxPOS<6>(u16 value);
template void Agnus::pokeSPRxPOS<7>(u16 value);

template void Agnus::pokeSPRxCTL<0>(u16 value);
template void Agnus::pokeSPRxCTL<1>(u16 value);
template void Agnus::pokeSPRxCTL<2>(u16 value);
template void Agnus::pokeSPRxCTL<3>(u16 value);
template void Agnus::pokeSPRxCTL<4>(u16 value);
template void Agnus::pokeSPRxCTL<5>(u16 value);
template void Agnus::pokeSPRxCTL<6>(u16 value);
template void Agnus::pokeSPRxCTL<7>(u16 value);

template void Agnus::pokeDIWSTRT<ACCESSOR_CPU>(u16 value);
template void Agnus::pokeDIWSTRT<ACCESSOR_AGNUS>(u16 value);
template void Agnus::pokeDIWSTOP<ACCESSOR_CPU>(u16 value);
template void Agnus::pokeDIWSTOP<ACCESSOR_AGNUS>(u16 value);
