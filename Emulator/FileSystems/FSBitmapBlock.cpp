// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "FSVolume.h"

FSBitmapBlock::FSBitmapBlock(FSVolume &ref, u32 nr) : FSBlock(ref, nr)
{
    allocated = new bool[volume.capacity]();
    
    // The first two blocks are always allocated
    allocated[0] = true;
    allocated[1] = true;    
}

void
FSBitmapBlock::dump()
{
    /*
    for (int i = 0; i < volume.capacity; i++) {
        printf("%d(%c)", i, allocated[i] ? '1' : '0');
    }
    printf("\n");
    */
}

bool
FSBitmapBlock::check()
{
    bool result = true;
    
    for (u32 i = 2; i < volume.capacity; i++) {
                
        FSBlockType type = volume.blocks[i]->type();

        if (type == FS_EMPTY_BLOCK && isAllocated(i)) {
            printf("Empty block %d is marked as allocated.\n", i);
            result = false;
        }
        if (type != FS_EMPTY_BLOCK && !isAllocated(i)) {
            printf("Non-empty block %d is marked as free.\n", i);
            result = false;
        }
    }
    return result;
}

void
FSBitmapBlock::write(u8 *p)
{
    // Start from scratch
    memset(p, 0, 512);

    // Write allocation map
    for (long i = 2; i < volume.capacity; i++) {

        if (allocated[i]) continue;
        
        // Determine bit position for this block inside the bitmap. Layout:
        //
        //     Position: p[00] p[01] p[02] ... p[31] p[32] p[33] ... p[63]
        //       Sector:   29    28    27         2    61    60        34
        //
        // Remember: The first two sectors are always allocated and not part
        // the map.

        long bit = (i - 2) % 8;
        long byte = (i - 2) / 8;
        switch (byte % 4) {
            case 0: byte += 3; break;
            case 1: byte += 1; break;
            case 2: byte -= 1; break;
            case 3: byte -= 3; break;
        }
        
        SET_BIT(p[4 + byte], bit);
    }
        
    // Compute checksum
    write32(p, FSBlock::checksum(p));
    
    /*
    for (unsigned i = 0; i < 256; i++) {
        printf("%d:%x ", i, p[i]);
    }
    printf("\n");
     */
}

bool
FSBitmapBlock::isAllocated(u32 block)
{
    if (!volume.isBlockNumber(block)) return true;
    
    return allocated[block];
}

void
FSBitmapBlock::alloc(u32 block, bool value)
{
    if (!volume.isBlockNumber(block)) return;

    allocated[block] = value;
}

void
FSBitmapBlock::dealloc()
{
    memset(allocated, 0, sizeof(bool) * volume.capacity);
}
