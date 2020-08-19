// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// This file must conform to standard ANSI-C to be compatible with Swift.

#ifndef _AMIGA_TYPES_H
#define _AMIGA_TYPES_H

#include "Aliases.h"

#include "AmigaEnums.h"
#include "AgnusTypes.h"
#include "CPUTypes.h"
#include "CIATypes.h"
#include "DeniseTypes.h"
#include "DiskTypes.h"
#include "DmaDebuggerTypes.h"
#include "DriveTypes.h"
#include "EventHandlerTypes.h"
#include "FileTypes.h"
#include "KeyboardTypes.h"
#include "MemoryTypes.h"
#include "MessageQueueTypes.h"
#include "PaulaTypes.h"
#include "PortTypes.h"
#include "RTCTypes.h"

//
// Enumerations
//

typedef VA_ENUM(long, ConfigOption)
{
    OPT_AGNUS_REVISION,
    OPT_DENISE_REVISION,
    OPT_RTC,
    OPT_CHIP_RAM,
    OPT_SLOW_RAM,
    OPT_FAST_RAM,
    OPT_EXT_START,
    
    OPT_DRIVE_CONNECT,
    OPT_DRIVE_TYPE,
    OPT_DRIVE_SPEED,
    
    OPT_SERIAL_DEVICE,

    OPT_HIDDEN_SPRITES,
    OPT_HIDDEN_LAYERS,
    OPT_HIDDEN_LAYER_ALPHA,
    OPT_CLX_SPR_SPR,
    OPT_CLX_SPR_PLF,
    OPT_CLX_PLF_PLF,
        
    OPT_BLITTER_ACCURACY,
    OPT_ASYNC_FIFO,
    OPT_LOCK_DSKSYNC,
    OPT_AUTO_DSKSYNC,
    
    OPT_TODBUG,
    OPT_ECLOCK_SYNCING,
    OPT_ACCURATE_KEYBOARD,
    
    OPT_SAMPLING_METHOD,
    OPT_FILTER_TYPE,
    OPT_FILTER_ALWAYS_ON,
    OPT_AUDVOLL,
    OPT_AUDVOLR,
    OPT_AUDVOL0,
    OPT_AUDVOL1,
    OPT_AUDVOL2,
    OPT_AUDVOL3,
    OPT_AUDPAN0,
    OPT_AUDPAN1,
    OPT_AUDPAN2,
    OPT_AUDPAN3,
};

inline bool isConfigOption(long value)
{
    return value >= OPT_AGNUS_REVISION && value <= OPT_FILTER_ALWAYS_ON;
}

typedef enum
{
    STATE_OFF,
    STATE_PAUSED,
    STATE_RUNNING
}
EmulatorState;

inline bool isEmulatorState(long value) {
    return value >= STATE_OFF && value <= STATE_RUNNING;
}

typedef enum
{
    RL_STOP               = 0b000001,
    RL_INSPECT            = 0b000010,
    RL_BREAKPOINT_REACHED = 0b000100,
    RL_WATCHPOINT_REACHED = 0b001000,
    RL_AUTO_SNAPSHOT      = 0b010000,
    RL_USER_SNAPSHOT      = 0b100000
}
RunLoopControlFlag;

typedef VA_ENUM(long, ErrorCode)
{
    ERR_OK,
    ERR_ROM_MISSING,
    ERR_AROS_NO_EXTROM,
    ERR_AROS_RAM_LIMIT,
    ERR_CHIP_RAM_LIMIT
};

//
// Structures
//

typedef struct
{
    int cpuSpeed;
    CIAConfig ciaA;
    CIAConfig ciaB;
    RTCConfig rtc;
    AudioConfig audio;
    MemoryConfig mem;
    AgnusConfig agnus;
    DeniseConfig denise;
    BlitterConfig blitter;
    SerialPortConfig serialPort;
    KeyboardConfig keyboard;
    DiskControllerConfig diskController;
    DriveConfig df0;
    DriveConfig df1;
    DriveConfig df2;
    DriveConfig df3;
}
AmigaConfiguration;

typedef struct
{
    Cycle cpuClock;
    Cycle dmaClock;
    Cycle ciaAClock;
    Cycle ciaBClock;
    long frame;
    long vpos;
    long hpos;
}
AmigaInfo;

#endif
