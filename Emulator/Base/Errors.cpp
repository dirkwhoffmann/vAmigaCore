// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "AmigaTypes.h"
#include "Errors.h"

namespace va {

const char *
VAError::what() const throw() {
    return  ErrorCodeEnum::key(data);
}

}
