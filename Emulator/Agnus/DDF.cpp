// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "DDF.h"
#include <algorithm>

template <bool hires> void
DDF<hires>::compute(i16 ddfstrt, i16 ddfstop)
{
    if (hires) {
               
        // Compute the beginning of the fetch window
        strt = ddfstrt & ~0b11;
        
        // Compute the number of fetch units
        int fetchUnits = ((ddfstop - ddfstrt) + 15) >> 3;
        
        // Compute the end of the DDF window
        stop = std::min(strt + 8 * fetchUnits, 0xE0);

    } else {
        
        // Compute the beginning of the fetch window
        strt = ddfstrt & ~0b111;

        // Compute the number of fetch units
        int fetchUnits = ((ddfstop - ddfstrt) + 15) >> 3;
        
        // Compute the end of the DDF window
        stop = std::min(strt + 8 * fetchUnits, 0xE0);
    }
}

template void DDF<true>::compute(i16 ddfstrt, i16 ddfstop);
template void DDF<false>::compute(i16 ddfstrt, i16 ddfstop);
