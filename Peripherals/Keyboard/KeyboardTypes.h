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

//
// Enumerations
//

enum_long(KB_STATE)
{
    KB_SELFTEST,
    KB_SYNC,
    KB_STRM_ON,
    KB_STRM_OFF,
    KB_SEND
};
typedef KB_STATE KeyboardState;

#ifdef __cplusplus
struct KeyboardStateEnum : util::Reflection<KeyboardStateEnum, KeyboardState>
{
    static long minVal() { return 0; }
    static long maxVal() { return KB_SEND; }
    static bool isValid(auto val) { return val >= minVal() && val <= maxVal(); }
    
    static const char *prefix() { return "KB"; }
    static const char *key(KeyboardState value)
    {
        switch (value) {
                
            case KB_SELFTEST:  return "SELFTEST";
            case KB_SYNC:      return "SYNC";
            case KB_STRM_ON:   return "STRM_ON";
            case KB_STRM_OFF:  return "STRM_OFF";
            case KB_SEND:      return "SEND";
        }
        return "???";
    }
};
#endif

//
// Structures
//

typedef struct
{
    bool accurate;
}
KeyboardConfig;
