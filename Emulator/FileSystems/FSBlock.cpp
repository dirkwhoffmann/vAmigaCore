// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Utils.h"
#include "FSVolume.h"

FSBlock *
FSBlock::makeWithType(FSVolume &ref, u32 nr, FSBlockType type)
{
    switch (type) {

        case FS_EMPTY_BLOCK: return new FSEmptyBlock(ref, nr);
        case FS_BOOT_BLOCK: return new FSBootBlock(ref, nr);
        case FS_ROOT_BLOCK: return new FSRootBlock(ref, nr);
        case FS_BITMAP_BLOCK: return new FSBitmapBlock(ref, nr);
        case FS_USERDIR_BLOCK: return new FSUserDirBlock(ref, nr);
        case FS_FILEHEADER_BLOCK: return new FSFileHeaderBlock(ref, nr);
        case FS_FILELIST_BLOCK: return new FSFileListBlock(ref, nr);
        
        case FS_DATA_BLOCK:
            if (ref.isOFS()) {
                return new OFSDataBlock(ref, nr);
            } else {
                return new FFSDataBlock(ref, nr);
            }

        default: return nullptr;
    }
}

u32
FSBlock::typeID()
{
    return get32(0);
}

u32
FSBlock::subtypeID()
{
    return get32((volume.bsize / 4) - 1);
}

unsigned
FSBlock::check(bool strict)
{
    FSError error;
    unsigned count = 0;
    
    for (u32 i = 0; i < volume.bsize; i++) {

        if ((error = check(i, strict)) != FS_OK) {
            count++;
            if (FS_DEBUG) printf("Block %d [%d.%d]: %s\n", nr, i / 4, i % 4, sFSError(error));
        }
    }

    return count;
}

u8 *
FSBlock::addr(int nr)
{
    return (data + 4 * nr) + (nr < 0 ? volume.bsize : 0);
}

u32
FSBlock::read32(const u8 *p)
{
    return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}
    
void
FSBlock::write32(u8 *p, u32 value)
{
    p[0] = (value >> 24) & 0xFF;
    p[1] = (value >> 16) & 0xFF;
    p[2] = (value >>  8) & 0xFF;
    p[3] = (value >>  0) & 0xFF;
}

char *
FSBlock::assemblePath()
{
    FSBlock *parent = getParentBlock();
    if (!parent) return strdup("");
    
    FSName name = getName();
    
    char *prefix = parent->assemblePath();
    char *result = new char [strlen(prefix) + strlen(name.cStr) + 2];

    strcpy(result, prefix);
    strcat(result, "/");
    strcat(result, name.cStr);

    delete [] prefix;
    return result;
}

void
FSBlock::printPath()
{
    char *path = assemblePath();
    printf("%s", path);
    delete [] path;
}

void
FSBlock::dumpData()
{
    int cols = 32;

    printf("Block %d\n", nr);
    for (int y = 0; y < 512 / cols; y++) {
        for (int x = 0; x < cols; x++) {
            printf("%02X ", data[y*cols + x]);
            if ((x % 4) == 3) printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}

u32
FSBlock::checksum()
{
    u32 loc = checksumLocation();
    assert(loc <= 5);
    
    // Wipe out the old checksum
    u32 old = get32(loc);
    set32(loc, 0);
    
    // Compute the new checksum
    u32 result = 0;
    for (u32 i = 0; i < volume.bsize / 4; i++) result += get32(i);
    result = ~result + 1;
    
    // Rectify the buffer
    set32(loc, old);
    
    return result;
}

void
FSBlock::updateChecksum()
{
    u32 ref = checksumLocation();
    if (ref < volume.bsize / 4) set32(ref, checksum());
}

void
FSBlock::importBlock(const u8 *src, size_t bsize)
{    
    assert(bsize == volume.bsize);
    assert(src != nullptr);
    assert(data != nullptr);
        
    memcpy(data, src, bsize);
    if (nr == 0) dumpData();
}

void
FSBlock::exportBlock(u8 *dst, size_t bsize)
{
    assert(bsize == volume.bsize);
            
    // Rectify the checksum
    updateChecksum();

    // Export the block
    assert(dst != nullptr);
    assert(data != nullptr);
    memcpy(dst, data, bsize);
}

FSBlock *
FSBlock::getParentBlock()
{
    u32 ref = getParentDirRef();
    return ref ? volume.block(ref) : nullptr;
}

FSFileHeaderBlock *
FSBlock::getFileHeaderBlock()
{
    u32 ref = getFileHeaderRef();
    return ref ? volume.fileHeaderBlock(ref) : nullptr;
}

FSDataBlock *
FSBlock::getFirstDataBlock()
{
    u32 ref = getFirstDataBlockRef();
    return ref ? volume.dataBlock(ref) : nullptr;
}

FSDataBlock *
FSBlock::getNextDataBlock()
{
    u32 ref = getNextDataBlockRef();
    return ref ? volume.dataBlock(ref) : nullptr;
}

FSBlock *
FSBlock::getNextHashBlock()
{
    u32 ref = getNextHashRef();
    return ref ? volume.block(ref) : nullptr;
}

FSFileListBlock *
FSBlock::getNextExtensionBlock()
{
    u32 ref = getNextListBlockRef();
    return ref ? volume.fileListBlock(ref) : nullptr;
}

u32
FSBlock::hashLookup(u32 nr)
{
    return (nr < hashTableSize()) ? get32(6 + nr) : 0;
}

FSBlock *
FSBlock::hashLookup(FSName name)
{
    // Don't call this function if no hash table is present
    assert(hashTableSize() != 0);

    // Compute hash value and table position
    u32 hash = name.hashValue() % hashTableSize();
    
    // Read the entry
    u32 blockRef = hashLookup(hash);
    FSBlock *block = blockRef ? volume.block(blockRef) : nullptr;
    
    // Traverse the linked list until the item has been found
    for (int i = 0; block && i < searchLimit; i++) {

        if (block->isNamed(name)) return block;
        block = block->getNextHashBlock();
    }

    return nullptr;
}

void
FSBlock::addToHashTable(u32 ref)
{
    FSBlock *block = volume.block(ref);
    if (block == nullptr) return;
    
    // Don't call this function if no hash table is present
    assert(hashTableSize() != 0);
        
    // Compute hash value and table position
    u32 hash = block->hashValue() % hashTableSize();
    u8 *tableEntry = data + 24 + 4 * hash;
    
    // If the hash table slot is empty, put the reference there
    if (read32(tableEntry) == 0) { write32(tableEntry, ref); return; }
        
    // Otherwise, add the reference at the end of the linked list
    if (auto item = volume.block(read32(tableEntry))) {
        
        for (int i = 0; i < searchLimit; i++) {
            
            if (item->getNextHashBlock() == nullptr) {
                item->setNextHashRef(ref);
                return;
            }
            
            item = item->getNextHashBlock();
        }
    }
}

void
FSBlock::dumpHashTable()
{
    for (u32 i = 0; i < hashTableSize(); i++) {
        
        u32 value = read32(data + 24 + 4 * i);
        if (value) {
            printf("%d: %d ", i, value);
        }
    }
}

u32
FSBlock::getMaxDataBlockRefs()
{
    return volume.bsize / 4 - 56;
}
