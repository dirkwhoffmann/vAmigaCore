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
#include "Reflection.h"

// Checks if a slot number refers to a primary slot or a secondary slot
#define isPrimarySlot(s) ((s) <= SLOT_SEC)
#define isSecondarySlot(s) ((s) > SLOT_SEC && (s) <= SLOT_TER)
#define isTertiarySlot(s) ((s) > SLOT_TER)

// Time stamp used for messages that never trigger
#define NEVER INT64_MAX


//
// Enumerations
//

enum_long(SLOT)
{
    // Primary slots
    SLOT_REG,                       // Register changes
    SLOT_CIAA,                      // CIA A execution
    SLOT_CIAB,                      // CIA B execution
    SLOT_BPL,                       // Bitplane DMA
    SLOT_DAS,                       // Disk, Audio, and Sprite DMA
    SLOT_COP,                       // Copper
    SLOT_BLT,                       // Blitter
    SLOT_SEC,                       // Enables secondary slots

    // Secondary slots
    SLOT_CH0,                       // Audio channel 0
    SLOT_CH1,                       // Audio channel 1
    SLOT_CH2,                       // Audio channel 2
    SLOT_CH3,                       // Audio channel 3
    SLOT_DSK,                       // Disk controller
    SLOT_VBL,                       // Vertical blank
    SLOT_IRQ,                       // Interrupts
    SLOT_IPL,                       // CPU Interrupt Priority Lines
    SLOT_KBD,                       // Keyboard
    SLOT_TXD,                       // Serial data out (UART)
    SLOT_RXD,                       // Serial data in (UART)
    SLOT_POT,                       // Potentiometer
    SLOT_RAS,                       // HSYNC handler (End of Line)
    SLOT_TER,                       // Enables tertiary slots
    
    // Tertiary slots
    SLOT_DC0,                       // Disk change (Df0)
    SLOT_DC1,                       // Disk change (Df1)
    SLOT_DC2,                       // Disk change (Df2)
    SLOT_DC3,                       // Disk change (Df3)
    SLOT_MSE1,                      // Port 1 mouse
    SLOT_MSE2,                      // Port 2 mouse
    SLOT_KEY,                       // Auto-typing
    SLOT_SRV,                       // Remote server manager
    SLOT_SER,                       // Serial remote server
    SLOT_INS,                       // Handles periodic calls to inspect()

    SLOT_COUNT
};
typedef SLOT EventSlot;

#ifdef __cplusplus
struct EventSlotEnum : util::Reflection<EventSlotEnum, EventSlot>
{
    static long minVal() { return 0; }
    static long maxVal() { return SLOT_COUNT - 1; }
    static bool isValid(auto val) { return val >= minVal() && val <= maxVal(); }
    
    static const char *prefix() { return "SLOT"; }
    static const char *key(EventSlot value)
    {
        switch (value) {
                
            case SLOT_REG:   return "REG";
            case SLOT_CIAA:  return "CIAA";
            case SLOT_CIAB:  return "CIAB";
            case SLOT_BPL:   return "BPL";
            case SLOT_DAS:   return "DAS";
            case SLOT_COP:   return "COP";
            case SLOT_BLT:   return "BLT";
            case SLOT_SEC:   return "SEC";

            case SLOT_CH0:   return "CH0";
            case SLOT_CH1:   return "CH1";
            case SLOT_CH2:   return "CH2";
            case SLOT_CH3:   return "CH3";
            case SLOT_DSK:   return "DSK";
            case SLOT_VBL:   return "VBL";
            case SLOT_IRQ:   return "IRQ";
            case SLOT_IPL:   return "IPL";
            case SLOT_KBD:   return "KBD";
            case SLOT_TXD:   return "TXD";
            case SLOT_RXD:   return "RXD";
            case SLOT_POT:   return "POT";
            case SLOT_RAS:   return "RAS";
            case SLOT_TER:   return "TER";
                
            case SLOT_DC0:   return "DC0";
            case SLOT_DC1:   return "DC1";
            case SLOT_DC2:   return "DC2";
            case SLOT_DC3:   return "DC3";
            case SLOT_MSE1:  return "MSE1";
            case SLOT_MSE2:  return "MSE2";
            case SLOT_KEY:   return "KEY";
            case SLOT_SRV:   return "SRV";
            case SLOT_SER:   return "SER";
            case SLOT_INS:   return "INS";

            case SLOT_COUNT: return "???";
        }
        return "???";
    }
};
#endif

enum_i8(EventID)
{
    EVENT_NONE          = 0,
    
    //
    // Events in the primary event table
    //

    // REG slot
    REG_CHANGE          = 1,
    REG_EVENT_COUNT,

    // CIA slots
    CIA_EXECUTE         = 1,
    CIA_WAKEUP,
    CIA_EVENT_COUNT,
    
    // BPL slot
    BPL_L1              = 0x04,
    BPL_L1_MOD          = 0x08,
    BPL_L2              = 0x0C,
    BPL_L2_MOD          = 0x10,
    BPL_L3              = 0x14,
    BPL_L3_MOD          = 0x18,
    BPL_L4              = 0x1C,
    BPL_L4_MOD          = 0x20,
    BPL_L5              = 0x24,
    BPL_L5_MOD          = 0x28,
    BPL_L6              = 0x2C,
    BPL_L6_MOD          = 0x30,
    BPL_H1              = 0x34,
    BPL_H1_MOD          = 0x38,
    BPL_H2              = 0x3C,
    BPL_H2_MOD          = 0x40,
    BPL_H3              = 0x44,
    BPL_H3_MOD          = 0x48,
    BPL_H4              = 0x4C,
    BPL_H4_MOD          = 0x50,
    BPL_EOL             = 0x54,
    BPL_EVENT_COUNT     = 0x58,

    // DAS slot
    DAS_REFRESH         = 1,
    DAS_D0,
    DAS_D1,
    DAS_D2,
    DAS_A0,
    DAS_A1,
    DAS_A2,
    DAS_A3,
    DAS_S0_1,
    DAS_S0_2,
    DAS_S1_1,
    DAS_S1_2,
    DAS_S2_1,
    DAS_S2_2,
    DAS_S3_1,
    DAS_S3_2,
    DAS_S4_1,
    DAS_S4_2,
    DAS_S5_1,
    DAS_S5_2,
    DAS_S6_1,
    DAS_S6_2,
    DAS_S7_1,
    DAS_S7_2,
    DAS_SDMA,
    DAS_TICK,
    DAS_EVENT_COUNT,

    // Copper slot
    COP_REQ_DMA         = 1,
    COP_WAKEUP,
    COP_WAKEUP_BLIT,
    COP_FETCH,
    COP_MOVE,
    COP_WAIT_OR_SKIP,
    COP_WAIT1,
    COP_WAIT2,
    COP_WAIT_BLIT,
    COP_SKIP1,
    COP_SKIP2,
    COP_JMP1,
    COP_JMP2,
    COP_VBLANK,
    COP_EVENT_COUNT,
    
    // Blitter slot
    BLT_STRT1           = 1,
    BLT_STRT2,
    BLT_COPY_SLOW,
    BLT_COPY_FAKE,
    BLT_LINE_SLOW,
    BLT_LINE_FAKE,
    BLT_EVENT_COUNT,
        
    // SEC slot
    SEC_TRIGGER         = 1,
    SEC_EVENT_COUNT,
    
    //
    // Events in secondary event table
    //

    // Audio channels
    CHX_PERFIN          = 1,
    CHX_EVENT_COUNT,

    // Disk controller slot
    DSK_ROTATE          = 1,
    DSK_EVENT_COUNT,

    // Strobe slot
    VBL_STROBE0         = 1,
    VBL_STROBE1,
    VBL_STROBE2,
    VBL_EVENT_COUNT,
    
    // IRQ slot
    IRQ_CHECK           = 1,
    IRQ_EVENT_COUNT,

    // IPL slot
    IPL_CHANGE          = 1,
    IPL_EVENT_COUNT,

    // Keyboard
    KBD_TIMEOUT         = 1,
    KBD_DAT,
    KBD_CLK0,
    KBD_CLK1,
    KBD_SYNC_DAT0,
    KBD_SYNC_CLK0,
    KBD_SYNC_DAT1,
    KBD_SYNC_CLK1,
    KBD_EVENT_COUNT,

    // Serial data out (UART)
    TXD_BIT             = 1,
    TXD_EVENT_COUNT,

    // Serial data out (UART)
    RXD_BIT             = 1,
    RXD_EVENT_COUT,

    // Potentiometer
    POT_DISCHARGE       = 1,
    POT_CHARGE,
    POT_EVENT_COUNT,
    
    // Screenshots
    SCR_TAKE            = 1,
    SCR_EVENT_COUNT,
    
    // Rasterline slot
    RAS_HSYNC           = 1,
    RAS_EVENT_COUNT,
    
    // SEC slot
    TER_TRIGGER         = 1,
    TER_EVENT_COUNT,
    
    //
    // Events in tertiary event table
    //
    
    // Disk change slot
    DCH_INSERT          = 1,
    DCH_EJECT,
    DCH_EVENT_COUNT,

    // Mouse
    MSE_PUSH_LEFT       = 1,
    MSE_RELEASE_LEFT,
    MSE_PUSH_RIGHT,
    MSE_RELEASE_RIGHT,
    MSE_EVENT_COUNT,

    // Auto typing
    KEY_PRESS           = 1,
    KEY_RELEASE,

    // Remote server manager
    SRV_LAUNCH_DAEMON   = 1,
    
    // Serial remote server
    SER_RECEIVE         = 1,
    
    // Inspector slot
    INS_AMIGA           = 1,
    INS_CPU,
    INS_MEM,
    INS_CIA,
    INS_AGNUS,
    INS_PAULA,
    INS_DENISE,
    INS_PORTS,
    INS_EVENTS,
    INS_EVENT_COUNT
};

static inline bool isRegEvent(EventID id) { return id < REG_EVENT_COUNT; }
static inline bool isCiaEvent(EventID id) { return id < CIA_EVENT_COUNT; }
static inline bool isBplEvent(EventID id) { return id < BPL_EVENT_COUNT; }
static inline bool isDasEvent(EventID id) { return id < DAS_EVENT_COUNT; }
static inline bool isCopEvent(EventID id) { return id < COP_EVENT_COUNT; }
static inline bool isBltEvent(EventID id) { return id < BLT_EVENT_COUNT; }

static inline bool isBplxEvent(EventID id, int x)
{
    switch(id & ~0b11) {

        case BPL_L1: case BPL_H1: return x == 1;
        case BPL_L2: case BPL_H2: return x == 2;
        case BPL_L3: case BPL_H3: return x == 3;
        case BPL_L4: case BPL_H4: return x == 4;
        case BPL_L5:              return x == 5;
        case BPL_L6:              return x == 6;

        default:
            return false;
    }
}


//
// Structures
//

typedef struct
{
    EventSlot slot;
    EventID eventId;
    const char *eventName;

    // Trigger cycle of the event
    Cycle trigger;
    Cycle triggerRel;

    // Trigger relative to the current frame
    // -1 = earlier frame, 0 = current frame, 1 = later frame
    long frameRel;

    // The trigger cycle translated to a beam position.
    long vpos;
    long hpos;
}
EventSlotInfo;

typedef struct
{
    Cycle cpuClock;
    Cycle cpuCycles;
    Cycle dmaClock;
    Cycle ciaAClock;
    Cycle ciaBClock;
    i64 frame;
    long vpos;
    long hpos;
}
EventInfo;
