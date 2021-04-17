// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AmigaFile.h"

class Script : public AmigaFile {
    
public:
    
    //
    // Class methods
    //

    static bool isCompatiblePath(const string &path);
    static bool isCompatibleStream(std::istream &stream);
    
    
    //
    // Initializing
    //

public:

    Script();
    
    const char *getDescription() const override { return "Script"; }

    
    //
    // Methods from AmigaFile
    //
    
public:
    
    FileType type() const override { return FILETYPE_SCRIPT; }

    
    //
    // Processing
    //
    
    // Executes the script
    void execute(class Amiga &amiga);
};
