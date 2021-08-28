// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Paula.h"
#include "ControlPort.h"
#include "CPU.h"

void
Paula::serviceIrqEvent()
{
    assert(agnus.slot[SLOT_IRQ].id == IRQ_CHECK);

    Cycle clock = agnus.clock;
    Cycle next = NEVER;

    // Check all interrupt sources
    for (isize src = 0; src < 16; src++) {

        // Check if the interrupt source is due
        if (clock >= setIntreq[src]) {
            setINTREQ(true, (u16)(1 << src));
            setIntreq[src] = NEVER;
        } else {
             next = std::min(next, setIntreq[src]);
        }
    }

    // Schedule next event
    agnus.scheduleAbs<SLOT_IRQ>(next, IRQ_CHECK);
}

void
Paula::serviceIplEvent()
{
    assert(agnus.slot[SLOT_IPL].id == IPL_CHANGE);    

    // Update the value on the CPU's IPL pin
    cpu.setIPL((iplPipe >> 24) & 0xFF);
    
    // Shift the pipe
    iplPipe = (iplPipe << 8) | (iplPipe & 0xFF);
    
    // Reschedule the event until the pipe has been shifted through entirely
    i64 repeat = scheduler.slot[SLOT_IPL].data;
    if (repeat) {
        agnus.scheduleRel<SLOT_IPL>(DMA_CYCLES(1), IPL_CHANGE, repeat - 1);
    } else {
        agnus.cancel<SLOT_IPL>();
    }
}

void
Paula::servicePotEvent(EventID id)
{
    trace(POT_DEBUG, "servicePotEvent(%lld)\n", id);

    switch (id) {

        case POT_DISCHARGE:
        {
            if (--scheduler.slot[SLOT_POT].data) {

                // Discharge capacitors
                if (!OUTLY()) chargeY0 = 0.0;
                if (!OUTLX()) chargeX0 = 0.0;
                if (!OUTRY()) chargeY1 = 0.0;
                if (!OUTRX()) chargeX1 = 0.0;

                // Schedule the next discharge event
                agnus.scheduleRel<SLOT_POT>(DMA_CYCLES(HPOS_CNT), POT_DISCHARGE);

            } else {

                // Reset counters. For input pins, we need to set the couter
                // value to -1. It'll wrap over to 0 in the hsync handler.
                potCntY0 = OUTLY() ? 0 : -1;
                potCntX0 = OUTLX() ? 0 : -1;
                potCntY1 = OUTRY() ? 0 : -1;
                potCntX1 = OUTRX() ? 0 : -1;

                // Schedule the first charge event
                agnus.scheduleRel<SLOT_POT>(DMA_CYCLES(HPOS_CNT), POT_CHARGE);
            }
            break;
        }
        case POT_CHARGE:
        {
            bool cont = false;

            // Get delta charges for each line
            double dy0 = controlPort1.getChargeDY();
            double dx0 = controlPort1.getChargeDX();
            double dy1 = controlPort2.getChargeDY();
            double dx1 = controlPort2.getChargeDX();

            // Charge capacitors
            if (dy0 > 0 && chargeY0 < 1.0 && !OUTLY()) { chargeX0 += dy0; cont = true; }
            if (dx0 > 0 && chargeX0 < 1.0 && !OUTLX()) { chargeX0 += dx0; cont = true; }
            if (dy1 > 0 && chargeY1 < 1.0 && !OUTRY()) { chargeX0 += dy1; cont = true; }
            if (dx1 > 0 && chargeX1 < 1.0 && !OUTRX()) { chargeX0 += dx1; cont = true; }

            // Schedule next event
            if (cont) {
                agnus.scheduleRel<SLOT_POT>(DMA_CYCLES(HPOS_CNT), POT_CHARGE);
            } else {
                agnus.cancel<SLOT_POT>();
            }
            break;
        }
        default:
            fatalError;
    }
}
