// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "FSRootBlock.h"

void
RootBlock::write(u8 *p)
{
    // Start from scratch
    memset(p, 0, 512);

    // Type
    p[3] = 0x02;
    
    // Hashtable size
    p[15] = 0x48;
    
    // Hashtable
    hashTable.write(p + 24);
    
    // BM flag (true if bitmap on disk is valid)
    p[312] = p[313] = p[314] = p[315] = 0xFF;
    
    // BM pages (indicates the blocks containing the bitmap)
    p[318] = HI_BYTE(881);
    p[319] = LO_BYTE(881);
    
    // Last recent change of the root directory of this volume
    lastModified.write(p + 420);
    
    // Date and time when this volume was formatted
    created.write(p + 484);
    
    // Volume name
    name.write(p + 432);
    
    // Secondary block type
    p[511] = 0x01;
    
    // Compute checksum
    write32(p + 20, Block::checksum(p));
}
