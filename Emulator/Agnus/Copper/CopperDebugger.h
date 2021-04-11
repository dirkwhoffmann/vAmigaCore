// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AmigaComponent.h"
#include <map>

struct CopperList {
    
    u32 start;
    u32 end;
};

class CopperDebugger: public AmigaComponent {
                
    // Cached Copper lists
    std::map<u32, CopperList> cache;
        
    // The most recently used Copper list 1
    CopperList *current1 = nullptr;

    // The most recently used Copper list 2
    CopperList *current2 = nullptr;

    // Storage for disassembled instruction
    char disassembly[128];

    
    //
    // Initializing
    //
    
public:
    
    using AmigaComponent::AmigaComponent;
    const char *getDescription() const override { return "CopperDebugger"; }
    void _reset(bool hard) override;
    
    
    //
    // Analyzing
    //

private:

    void _dump(dump::Category category, std::ostream& os) const override;
    
    
    //
    // Serialization
    //
    
private:
    
    isize _size() override { return 0; }
    isize _load(const u8 *buffer) override { return 0; }
    isize _save(u8 *buffer) override { return 0; }
    

    //
    // Tracking the Copper
    //
    
public:
    
    // Returns the start or end address of the currently processed Copper list
    u32 startOfCopperList(isize nr);
    u32 endOfCopperList(isize nr);

    // Notifies the debugger that the Copper has advanced the program counter
    void advanced();
    
    // Notifies the debugger that the Copper has jumped to a new Copper list
    void jumped();
    
    
    //
    // Disassembling instructions
    //
    
    // Disassembles a single Copper command
    string disassemble(u32 addr) const;
    string disassemble(isize list, isize offset) const;    
};
