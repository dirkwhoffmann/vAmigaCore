// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "FSVolume.h"

FSRootBlock::FSRootBlock(FSVolume &ref, u32 nr) : FSBlock(ref, nr)
{
    data = new u8[ref.bsize]();
    
    assert(hashTableSize() == 72);
    
    set32(0, 2);                         // Type
    set32(3, hashTableSize());           // Hash table size
    set32(-49, volume.bitmapBlockNr());  // Location of the bitmap block
    set32(-50, 0xFFFFFFFF);              // Bitmap validity
    setCreationDate(time(NULL));         // Creation date
    setModificationDate(time(NULL));     // Modification date
    set32(-1, 1);                        // Sub type
}

FSRootBlock::FSRootBlock(FSVolume &ref, u32 nr, const char *name) : FSRootBlock(ref, nr)
{
    setName(FSName(name));
}

FSRootBlock::~FSRootBlock()
{
    delete [] data;
}

FSItemType
FSRootBlock::itemType(u32 pos)
{
    // Intercept some special locations
    if (pos == 432) return FSI_BCPL_STRING_LENGTH;

    // Translate 'pos' to a long word index
    i32 word = (i32)(pos & ~0b11) / 4;
    if (word >= 6) word -= volume.bsize / 4;

    printf("pos = %d word = %d\n", pos, word);
    
    switch (word) {
        case 0:   return FSI_TYPE_ID;
        case 1:
        case 2:   return FSI_UNUSED;
        case 3:   return FSI_HASHTABLE_SIZE;
        case 4:   return FSI_UNUSED;
        case 5:   return FSI_CHECKSUM;
        case -50: return FSI_BITMAP_VALIDITY;
        case -49: return FSI_BITMAP_BLOCK_REF;
        case -23: return FSI_MODIFIED_DAY;
        case -22: return FSI_MODIFIED_MIN;
        case -21: return FSI_MODIFIED_TICKS;
        case -7:  return FSI_CREATED_DAY;
        case -6:  return FSI_CREATED_MIN;
        case -5:  return FSI_CREATED_TICKS;
        case -4:
        case -3:
        case -2:  return FSI_UNUSED;
        case -1:  return FSI_SUBTYPE_ID;
    }
    
    if (word <= -51)                return FSI_HASH_REF;
    if (word >= -49 && word <= -24) return FSI_BITMAP_BLOCK_REF;
    if (word >= -20 && word <= -8)  return FSI_BCPL_DISK_NAME;

    assert(false);
    return FSI_UNKNOWN;
}

FSError
FSRootBlock::check(u32 pos)
{
    // Align pos to the long word raster
    pos &= ~0b11;

    // Translate 'pos' to a long word index
    i32 word = (pos <= 24 ? (i32)pos : (i32)pos - volume.bsize) / 4;

    u32 value = get32(word);
    
    switch (word) {
        case 0:
            return value == 2 ? FS_OK : FS_BLOCK_TYPE_ID_MISMATCH;
        case 1:
        case 2:
            return value == 0 ? FS_OK : FS_EXPECTED_00;
        case 3:
            return value == 72 ? FS_OK : FS_BLOCK_HASHTABLE_SIZE_MISMATCH;
        case 4:
            return value == 0 ? FS_OK : FS_EXPECTED_00;
        case 5:
            return value == checksum() ? FS_OK : FS_BLOCK_CHECKSUM_ERROR;
        case -4:
        case -3:
        case -2:
            return value == 0 ? FS_OK : FS_EXPECTED_00;
        case -1:
            return value == 1 ? FS_OK : FS_BLOCK_SUBTYPE_ID_MISMATCH;
        default:
            break;
    }
        
    // Check all hash table entries
    if (word >= 6 && word < 6 + (i32)hashTableSize()) {
        return checkHashTableItem(word - 6);
    }
    
    return FS_OK;
}

void
FSRootBlock::dump()
{
    printf("        Name : %s\n", getName().cStr);
    printf("     Created : "); getCreationDate().print(); printf("\n");
    printf("    Modified : "); getModificationDate().print(); printf("\n");
    printf("  Hash table : "); dumpHashTable(); printf("\n");
}

void
FSRootBlock::updateChecksum()
{
    set32(5, 0);
    set32(5, checksum());
}
