// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------
// THIS FILE MUST CONFORM TO ANSI-C TO BE COMPATIBLE WITH SWIFT
// -----------------------------------------------------------------------------

#pragma once

#include "Aliases.h"

//
// Enumerations
//

enum_long(OPT)
{
    // Agnus
    OPT_AGNUS_REVISION,
    OPT_SLOW_RAM_MIRROR,
    
    // Denise
    OPT_DENISE_REVISION,
    
    // Pixel engine
    OPT_PALETTE,
    OPT_BRIGHTNESS,
    OPT_CONTRAST,
    OPT_SATURATION,
    
    // Real-time clock
    OPT_RTC_MODEL,

    // Memory
    OPT_CHIP_RAM,
    OPT_SLOW_RAM,
    OPT_FAST_RAM,
    OPT_EXT_START,
    OPT_SLOW_RAM_DELAY,
    OPT_BANKMAP,
    OPT_UNMAPPING_TYPE,
    OPT_RAM_INIT_PATTERN,
    
    // Disk controller
    OPT_DRIVE_CONNECT,
    OPT_DRIVE_SPEED,
    OPT_LOCK_DSKSYNC,
    OPT_AUTO_DSKSYNC,

    // Drives
    OPT_DRIVE_TYPE,
    OPT_EMULATE_MECHANICS,
    OPT_DRIVE_PAN,
    OPT_STEP_VOLUME,
    OPT_POLL_VOLUME,
    OPT_INSERT_VOLUME,
    OPT_EJECT_VOLUME,
    OPT_DEFAULT_FILESYSTEM,
    OPT_DEFAULT_BOOTBLOCK,
    
    // Ports
    OPT_SERIAL_DEVICE,

    // Compatibility
    OPT_HIDDEN_SPRITES,
    OPT_HIDDEN_LAYERS,
    OPT_HIDDEN_LAYER_ALPHA,
    OPT_CLX_SPR_SPR,
    OPT_CLX_SPR_PLF,
    OPT_CLX_PLF_PLF,
        
    // Blitter
    OPT_BLITTER_ACCURACY,
    
    // CIAs
    OPT_CIA_REVISION,
    OPT_TODBUG,
    OPT_ECLOCK_SYNCING,
    
    // Keyboard
    OPT_ACCURATE_KEYBOARD,
    
    // Mouse
    OPT_PULLUP_RESISTORS,
    OPT_SHAKE_DETECTION,
    OPT_MOUSE_VELOCITY,
    
    // Paula audio
    OPT_SAMPLING_METHOD,
    OPT_FILTER_TYPE,
    OPT_FILTER_ALWAYS_ON,
    OPT_AUDPAN,
    OPT_AUDVOL,
    OPT_AUDVOLL,
    OPT_AUDVOLR,
    
    OPT_COUNT
};
typedef OPT Option;


enum_long(EMULATOR_STATE)
{
    EMULATOR_STATE_OFF,
    EMULATOR_STATE_PAUSED,
    EMULATOR_STATE_RUNNING,

    EMULATOR_STATE_COUNT
};
typedef EMULATOR_STATE EmulatorState;


enum_long(ERROR_CODE)
{
    ERROR_OK,
    ERROR_UNKNOWN,
    
    // General
    ERROR_FILE_NOT_FOUND,
    ERROR_FILE_TYPE_MISMATCH,
    ERROR_FILE_CANT_READ,
    ERROR_FILE_CANT_WRITE,
    ERROR_FILE_CANT_CREATE,

    // Memory
    ERROR_OUT_OF_MEMORY,
    ERROR_CHIP_RAM_LIMIT,
    ERROR_AROS_RAM_LIMIT,

    // Roms
    ERROR_ROM_MISSING,
    ERROR_AROS_NO_EXTROM,
    
    // Floppy disks
    ERROR_DISK_CANT_DECODE,
    ERROR_DISK_INVALID_DIAMETER,
    ERROR_DISK_INVALID_DENSITY,
    
    // Snapshots
    ERROR_SNP_TOO_OLD,
    ERROR_SNP_TOO_NEW,
    ERROR_UNSUPPORTED_SNAPSHOT,  // DEPRECATED
    
    // Encrypted Roms
    ERROR_MISSING_ROM_KEY,
    ERROR_INVALID_ROM_KEY,
    
    // File system
    ERROR_FS_UNKNOWN,
    ERROR_FS_UNSUPPORTED,
    ERROR_FS_WRONG_BSIZE,
    ERROR_FS_WRONG_CAPACITY,
    ERROR_FS_HAS_CYCLES,
    ERROR_FS_CORRUPTED,

    // File system (export errors)
    ERROR_FS_DIRECTORY_NOT_EMPTY,
    ERROR_FS_CANNOT_CREATE_DIR,
    ERROR_FS_CANNOT_CREATE_FILE,

    // File system (block errors)
    ERROR_FS_EXPECTED_VALUE,
    ERROR_FS_EXPECTED_SMALLER_VALUE,
    ERROR_FS_EXPECTED_DOS_REVISION,
    ERROR_FS_EXPECTED_NO_REF,
    ERROR_FS_EXPECTED_REF,
    ERROR_FS_EXPECTED_SELFREF,
    ERROR_FS_PTR_TO_UNKNOWN_BLOCK,
    ERROR_FS_PTR_TO_EMPTY_BLOCK,
    ERROR_FS_PTR_TO_BOOT_BLOCK,
    ERROR_FS_PTR_TO_ROOT_BLOCK,
    ERROR_FS_PTR_TO_BITMAP_BLOCK,
    ERROR_FS_PTR_TO_BITMAP_EXT_BLOCK,
    ERROR_FS_PTR_TO_USERDIR_BLOCK,
    ERROR_FS_PTR_TO_FILEHEADER_BLOCK,
    ERROR_FS_PTR_TO_FILELIST_BLOCK,
    ERROR_FS_PTR_TO_DATA_BLOCK,
    ERROR_FS_EXPECTED_DATABLOCK_NR,
    ERROR_FS_INVALID_HASHTABLE_SIZE,
    
    ERROR_COUNT
};
typedef ERROR_CODE ErrorCode;


//
// Structures
//

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
