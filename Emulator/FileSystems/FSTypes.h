// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _FS_TYPES_H
#define _FS_TYPES_H

#include "Aliases.h"

VAMIGA_ENUM(long, FSVolumeType)
{
    FS_NONE = -1,
    FS_OFS = 0,         // Original File System
    FS_FFS = 1,         // Fast File System
    FS_OFS_INTL = 2,    // "International" (not supported)
    FS_FFS_INTL = 3,    // "International" (not supported)
    FS_OFS_DC = 4,      // "Directory Cache" (not supported)
    FS_FFS_DC = 5,      // "Directory Cache" (not supported)
    FS_OFS_LNFS = 6,    // "Long Filenames" (not supported)
    FS_FFS_LNFS = 7     // "Long Filenames" (not supported)
};

inline bool isFSVolumeType(long value)
{
    return value >= FS_NONE && value <= FS_FFS_LNFS;
}

inline const char *sFSVolumeType(FSVolumeType value)
{
    switch (value) {
        case FS_NONE:     return "None";
        case FS_OFS:      return "OFS";
        case FS_FFS:      return "FFS";
        case FS_OFS_INTL: return "OFS_INTL";
        case FS_FFS_INTL: return "FFS_INTL";
        case FS_OFS_DC:   return "OFS_DC";
        case FS_FFS_DC:   return "FFS_DC";
        case FS_OFS_LNFS: return "OFS_LNFS";
        case FS_FFS_LNFS: return "FFS_LNFS";
        default:          return "???";
    }
}

VAMIGA_ENUM(long, FSBlockType)
{
    FS_UNKNOWN_BLOCK,
    FS_EMPTY_BLOCK,
    FS_BOOT_BLOCK,
    FS_ROOT_BLOCK,
    FS_BITMAP_BLOCK,
    FS_USERDIR_BLOCK,
    FS_FILEHEADER_BLOCK,
    FS_FILELIST_BLOCK,
    FS_DATA_BLOCK
};

inline bool
isFSBlockType(long value)
{
    return value >= FS_UNKNOWN_BLOCK && value <= FS_DATA_BLOCK;
}

inline const char *
sFSBlockType(FSBlockType type)
{
    assert(isFSBlockType(type));

    switch (type) {
        case FS_EMPTY_BLOCK:      return "FS_EMPTY_BLOCK";
        case FS_BOOT_BLOCK:       return "FS_BOOT_BLOCK";
        case FS_ROOT_BLOCK:       return "FS_ROOT_BLOCK";
        case FS_BITMAP_BLOCK:     return "FS_BITMAP_BLOCK";
        case FS_USERDIR_BLOCK:    return "FS_USERDIR_BLOCK";
        case FS_FILEHEADER_BLOCK: return "FS_FILEHEADER_BLOCK";
        case FS_FILELIST_BLOCK:   return "FS_FILELIST_BLOCK";
        case FS_DATA_BLOCK:       return "FS_DATA_BLOCK";
        default:                  return "???";
    }
}

VAMIGA_ENUM(long, FSError)
{
    FS_OK,
    
    // File system errors
    FS_UNKNOWN,
    FS_UNSUPPORTED,
    FS_WRONG_BSIZE,
    FS_WRONG_CAPACITY,
    FS_CORRUPTED,
    
    // Block errros
    FS_BLOCK_TYPE_ERROR,
    FS_BLOCK_TYPE_ID_MISMATCH,
    FS_BLOCK_SUBTYPE_ID_MISMATCH,
    FS_BLOCK_REF_MISSING,
    FS_BLOCK_REF_OUT_OF_RANGE,
    FS_BLOCK_REF_TYPE_MISMATCH,
    FS_EXPECTED_00,
    FS_EXPECTED_FF,
    FS_BLOCK_CHECKSUM_ERROR,

};

inline bool isFSError(FSError value)
{
    return value >= FS_OK && value <= FS_CORRUPTED;
}

inline const char *sFSError(FSError value)
{
    switch (value) {
            
        case FS_OK:                     return "FS_OK";
            
        case FS_UNKNOWN:                return "FS_UNKNOWN";
        case FS_UNSUPPORTED:            return "FS_UNSUPPORTED";
        case FS_WRONG_BSIZE:            return "FS_WRONG_BSIZE";
        case FS_WRONG_CAPACITY:         return "FS_WRONG_CAPACITY";
            
        case FS_BLOCK_TYPE_ERROR:       return "FS_BLOCK_TYPE_ERROR";
        // TODO: COMPLETE
        case FS_BLOCK_CHECKSUM_ERROR:   return "FS_BLOCK_CHECKSUM_ERROR";
        case FS_CORRUPTED:              return "FS_CORRUPTED";
            
        default:                        return "???";
    }
}

typedef struct
{
    long numErrors;
    long numErroneousBlocks;
}
FSErrorReport;

#endif
