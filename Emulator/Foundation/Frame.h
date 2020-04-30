// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _FRAME_INC
#define _FRAME_INC

struct Frame
{
    // Frame count
    i64 nr;
    
    // Indicates if this frame is drawn in interlace mode
    bool interlaced;

    // The long frame flipflop
    bool lof;
    
    // Value of the frame flipflop in the previous frame
    bool prevlof;
    
    template <class T>
    void applyToItems(T& worker)
    {
        worker

        & nr
        & interlaced
        & lof
        & prevlof;
    }

    Frame() : nr(0), interlaced(false), lof(true) { }
    
    bool isLongFrame() { return lof; }
    bool isShortFrame() { return !lof; }
    int numLines() { return lof ? 313 : 312; }
    int lastLine() { return lof ? 312 : 311; }
    
    bool wasLongFrame() { return prevlof; }
    bool wasShortFrame() { return !prevlof; }
    int prevNumLines() { return prevlof ? 313 : 312; }
    int prevLastLine() { return prevlof ? 312 : 311; }

    // Advances one frame
    void next(bool laceBit)
    {
        nr++;
        prevlof = lof;
        
        // Update the long frame flipflop
        if ((interlaced = laceBit)) { lof = !lof; } else { lof = true; }
    }
};

#endif
