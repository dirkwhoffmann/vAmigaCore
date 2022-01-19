// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Aliases.h"

#ifdef __cplusplus

typedef u16 DDFState;
static constexpr u16 DDF_BMAPEN         = 0b0000000000000001;
static constexpr u16 DDF_HW             = 0b0000000000000010;
static constexpr u16 DDF_BPH            = 0b0000000000000100;
static constexpr u16 DDF_BPRUN          = 0b0000000000001000;
static constexpr u16 DDF_LASTFU         = 0b0000000000010000;

static constexpr u16 SIG_NONE           = 0b0000000000000000;
static constexpr u16 SIG_CON_L          = 0b0000000000010000;
static constexpr u16 SIG_CON_L0         = 0b0000000000010000 | 0;
static constexpr u16 SIG_CON_L1         = 0b0000000000010000 | 1;
static constexpr u16 SIG_CON_L2         = 0b0000000000010000 | 2;
static constexpr u16 SIG_CON_L3         = 0b0000000000010000 | 3;
static constexpr u16 SIG_CON_L4         = 0b0000000000010000 | 4;
static constexpr u16 SIG_CON_L5         = 0b0000000000010000 | 5;
static constexpr u16 SIG_CON_L6         = 0b0000000000010000 | 6;
static constexpr u16 SIG_CON_L7         = 0b0000000000010000 | 7;
static constexpr u16 SIG_CON_H0         = 0b0000000000010000 | 8;
static constexpr u16 SIG_CON_H1         = 0b0000000000010000 | 9;
static constexpr u16 SIG_CON_H2         = 0b0000000000010000 | 10;
static constexpr u16 SIG_CON_H3         = 0b0000000000010000 | 11;
static constexpr u16 SIG_CON_H4         = 0b0000000000010000 | 12;
static constexpr u16 SIG_CON_H5         = 0b0000000000010000 | 13;
static constexpr u16 SIG_CON_H6         = 0b0000000000010000 | 14;
static constexpr u16 SIG_CON_H7         = 0b0000000000010000 | 15;
static constexpr u16 SIG_BMAPEN_CLR     = 0b0000000000100000;
static constexpr u16 SIG_BMAPEN_SET     = 0b0000000001000000;
static constexpr u16 SIG_VFLOP_CLR      = 0b0000000010000000;
static constexpr u16 SIG_VFLOP_SET      = 0b0000000100000000;
static constexpr u16 SIG_BPHSTART       = 0b0000001000000000;
static constexpr u16 SIG_BPHSTOP        = 0b0000010000000000;
static constexpr u16 SIG_SHW            = 0b0000100000000000;
static constexpr u16 SIG_RHW            = 0b0001000000000000;
static constexpr u16 SIG_DONE           = 0b0010000000000000;
#endif

#ifdef __cplusplus
// DEPRECATED
struct DDFFlipflops
{
    bool bpv = false;
    bool shw = false;
    bool rhw = false;
    bool bphstart = false;
    bool bphstop = false;
    bool bprun = false;
    bool lastFu = false;
        
    bool operator!=(const DDFFlipflops &rhs) const
    {
        return
        this->bpv != rhs.bpv ||
        this->shw != rhs.shw ||
        this->rhw != rhs.rhw ||
        this->bphstart != rhs.bphstop ||
        this->bprun != rhs.bprun ||
        this->lastFu != rhs.lastFu;
    }
    
    template <class W>
    void operator<<(W& worker)
    {
        worker << bpv << shw << rhw << bphstart << bphstop << bprun << lastFu;
    }
};

#endif
