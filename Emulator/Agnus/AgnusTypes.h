// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// This file must conform to standard ANSI-C to be compatible with Swift.

#ifndef _AGNUS_TYPES_H
#define _AGNUS_TYPES_H

#include "BlitterTypes.h"
#include "CopperTypes.h"


typedef VA_ENUM(long, AgnusRevision)
{
    AGNUS_8367, // OCS Agnus
    AGNUS_8372, // ECS Agnus (up to 1MB Chip Ram)
    AGNUS_8375, // ECS Agnus (up to 2MB Chip Ram)
    AGNUS_CNT
};

inline bool isAgnusRevision(long value)
{
    return value >= 0 && value < AGNUS_CNT;
}

inline const char *agnusRevisionName(AgnusRevision type)
{
    assert(isAgnusRevision(type));

    switch (type) {
        case AGNUS_8367: return "AGNUS_8367";
        case AGNUS_8372: return "AGNUS_8372";
        case AGNUS_8375: return "AGNUS_8375";
        default:         return "???";
    }
}

typedef struct
{
    AgnusRevision revision;
}
AgnusConfig;

// Register change identifiers
typedef VA_ENUM(i32, RegChangeID)
{
    SET_NONE = 0,
    
    SET_BLTSIZE,
    SET_BLTSIZV,
    SET_BLTCON0,
    SET_BLTCON0L,
    SET_BLTCON1,
    
    SET_INTREQ,
    SET_INTENA,
    
    SET_AGNUS_BPLCON0,
    SET_DENISE_BPLCON0,
    SET_AGNUS_BPLCON1,
    SET_DENISE_BPLCON1,
    SET_BPLCON2,
    SET_DMACON,
    
    SET_DIWSTRT,
    SET_DIWSTOP,
    SET_DDFSTRT,
    SET_DDFSTOP,
    
    SET_BPL1MOD,
    SET_BPL2MOD,
    SET_BPL1PTH,
    SET_BPL2PTH,
    SET_BPL3PTH,
    SET_BPL4PTH,
    SET_BPL5PTH,
    SET_BPL6PTH,
    SET_BPL1PTL,
    SET_BPL2PTL,
    SET_BPL3PTL,
    SET_BPL4PTL,
    SET_BPL5PTL,
    SET_BPL6PTL,

    SET_SPR0DATA,
    SET_SPR1DATA,
    SET_SPR2DATA,
    SET_SPR3DATA,
    SET_SPR4DATA,
    SET_SPR5DATA,
    SET_SPR6DATA,
    SET_SPR7DATA,

    SET_SPR0DATB,
    SET_SPR1DATB,
    SET_SPR2DATB,
    SET_SPR3DATB,
    SET_SPR4DATB,
    SET_SPR5DATB,
    SET_SPR6DATB,
    SET_SPR7DATB,

    SET_SPR0POS,
    SET_SPR1POS,
    SET_SPR2POS,
    SET_SPR3POS,
    SET_SPR4POS,
    SET_SPR5POS,
    SET_SPR6POS,
    SET_SPR7POS,

    SET_SPR0CTL,
    SET_SPR1CTL,
    SET_SPR2CTL,
    SET_SPR3CTL,
    SET_SPR4CTL,
    SET_SPR5CTL,
    SET_SPR6CTL,
    SET_SPR7CTL,

    SET_SPR0PTH,
    SET_SPR1PTH,
    SET_SPR2PTH,
    SET_SPR3PTH,
    SET_SPR4PTH,
    SET_SPR5PTH,
    SET_SPR6PTH,
    SET_SPR7PTH,

    SET_SPR0PTL,
    SET_SPR1PTL,
    SET_SPR2PTL,
    SET_SPR3PTL,
    SET_SPR4PTL,
    SET_SPR5PTL,
    SET_SPR6PTL,
    SET_SPR7PTL,

    REG_COUNT
};

static inline bool isRegChangeID(long value)
{
    return value >= 0 && value < REG_COUNT;
}

typedef VA_ENUM(i8, BusOwner)
{
    BUS_NONE,
    BUS_CPU,
    BUS_REFRESH,
    BUS_DISK,
    BUS_AUDIO,
    BUS_BPL1,
    BUS_BPL2,
    BUS_BPL3,
    BUS_BPL4,
    BUS_BPL5,
    BUS_BPL6,
    BUS_SPRITE0,
    BUS_SPRITE1,
    BUS_SPRITE2,
    BUS_SPRITE3,
    BUS_SPRITE4,
    BUS_SPRITE5,
    BUS_SPRITE6,
    BUS_SPRITE7,
    BUS_COPPER,
    BUS_BLITTER,
    BUS_OWNER_COUNT
};

static inline bool isBusOwner(long value)
{
    return value >= 0 && value < BUS_OWNER_COUNT;
}

typedef enum
{
    DDF_OFF,
    DDF_READY,
    DDF_ON
}
DDFState;

typedef enum
{
    SPR_DMA_IDLE,
    SPR_DMA_ACTIVE
}
SprDMAState;

//
// Structures
//

typedef struct
{
    i16 vpos;
    i16 hpos;

    u16 dmacon;
    u16 bplcon0;
    u8  bpu;
    u16 ddfstrt;
    u16 ddfstop;
    u16 diwstrt;
    u16 diwstop;

    u16 bpl1mod;
    u16 bpl2mod;
    u16 bltamod;
    u16 bltbmod;
    u16 bltcmod;
    u16 bltdmod;
    u16 bltcon0;
    
    u32 coppc;
    u32 dskpt;
    u32 bplpt[6];
    u32 audpt[4];
    u32 audlc[4];
    u32 bltpt[4];
    u32 sprpt[8];

    bool bls;
}
AgnusInfo;

typedef struct
{
    long usage[BUS_OWNER_COUNT];
    
    double copperActivity;
    double blitterActivity;
    double diskActivity;
    double audioActivity;
    double spriteActivity;
    double bitplaneActivity;
}
AgnusStats;

#endif
