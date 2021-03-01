// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AmigaComponent.h"
#include "Chrono.h"

#ifdef __MACH__
#include <mach/mach_time.h>
#endif

class Oscillator : public AmigaComponent {

    /* The heart of this class is method sychronize() which puts the thread to
     * sleep for a certain interval. In order to calculate the delay, the
     * function needs to know the values of the Amiga clock and the Kernel
     * clock at the time the synchronization timer was started. The values are
     * stores in the following two variables and recorded in restart().
     */
    
    // Agnus clock (Amiga master cycles)
    Cycle clockBase = 0;

    // Kernel clock
    Time timeBase;

    
    //
    // Constructing
    //
    
public:
    
    Oscillator(Amiga& ref);

    const char *getDescription() const override;

private:
    
    void _reset(bool hard) override;
    
    
    //
    // Serializing
    //
    
private:
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
    }

    template <class T>
    void applyToHardResetItems(T& worker)
    {
        worker << clockBase;
    }
    
    template <class T>
    void applyToResetItems(T& worker)
    {
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    
    
    //
    // Managing emulation speed
    //
        
public:
    
    // Restarts the synchronization timer
    void restart();

    // Puts the emulator thread to rest
    void synchronize();
};
