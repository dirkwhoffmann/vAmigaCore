// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "FSVolume.h"

FSFileHeaderBlock::FSFileHeaderBlock(FSVolume &ref, u32 nr) : FSBlock(ref, nr)
{
    data = new u8[ref.bsize]();
   
    // Setup constant values
    
    set32(0, 2);                   // Type
    set32(1, nr);                  // Block pointer to itself
    setCreationDate(time(NULL));   // Creation date
    set32(-1, (u32)-3);            // Sub type
}

FSFileHeaderBlock::FSFileHeaderBlock(FSVolume &ref, u32 nr, const char *name) :
FSFileHeaderBlock(ref, nr)
{
    setName(FSName(name));
}

void
FSFileHeaderBlock::dump()
{
    printf("           Name : %s\n", getName().cStr);
    printf("           Path : ");    printPath(); printf("\n");
    printf("        Comment : %s\n", getComment().cStr);
    printf("        Created : ");    getCreationDate().print(); printf("\n");
    printf("           Next : %d\n", getNextHashRef());
    printf("      File size : %d\n", getFileSize());

    printf("    Block count : %d / %d\n", getNumDataBlockRefs(), getMaxDataBlockRefs());
    printf("          First : %d\n", getFirstDataBlockRef());
    printf("     Parent dir : %d\n", getParentDirRef());
    printf(" FileList block : %d\n", getNextListBlockRef());
    
    printf("    Data blocks : ");
    for (u32 i = 0; i < getNumDataBlockRefs(); i++) printf("%d ", getDataBlockRef(i));
    printf("\n");
}

FSError
FSFileHeaderBlock::check(u32 pos)
{
    // Make sure 'pos' points to the beginning of a long word
    assert(pos % 4 == 0);

    // Translate 'pos' to a long word index
    i32 word = (pos <= 24 ? (i32)pos : (i32)pos - volume.bsize) / 4;

    u32 value = get32(word);
    
    switch (word) {
        case 0:
            return value == 2 ? FS_OK : FS_BLOCK_TYPE_ID_MISMATCH;
        case 1:
            return value != nr ? FS_OK : FS_BLOCK_MISSING_SELFREF;
        case 3:
            return value == 0 ? FS_OK : FS_EXPECTED_00;
        case 4:
            if (value) {
                if (!volume.block(value)) return FS_BLOCK_REF_OUT_OF_RANGE;
                if (!volume.dataBlock(value)) return FS_BLOCK_REF_TYPE_MISMATCH;
            }
            return FS_OK;
        case 5:
            return value == checksum() ? FS_OK : FS_BLOCK_CHECKSUM_ERROR;
        case -50:
            return value == 0 ? FS_OK : FS_EXPECTED_00;
        case -4:
            return volume.block(value) ? FS_OK : FS_BLOCK_REF_OUT_OF_RANGE;
        case -3:
            if (value == 0) return FS_BLOCK_REF_MISSING;
            if (!volume.userDirBlock(value) &&
                !volume.rootBlock(value)) return FS_BLOCK_REF_TYPE_MISMATCH;
            return FS_OK;
        case -2:
            if (value == 0) return FS_OK;
            if (!volume.fileListBlock(value)) return FS_BLOCK_REF_TYPE_MISMATCH;
            return FS_OK;
        case -1:
            return value == (u32)-3 ? FS_OK : FS_BLOCK_SUBTYPE_ID_MISMATCH;
        default:
            break;
    }
        
    // Data block reference area
    if (word >= -51 && word <= 6 && value) {
        if (!volume.dataBlock(word)) return FS_BLOCK_REF_TYPE_MISMATCH;
    }
    if (word == -51) {
        if (value == 0 && getNumDataBlockRefs() > 0) {
            return FS_BLOCK_REF_MISSING;
        }
        if (value != 0 && getNumDataBlockRefs() == 0) {
            return FS_BLOCK_UNEXPECTED_REF;
        }
    }
    
    return FS_OK;
}

/*
bool
FSFileHeaderBlock::check(bool verbose)
{
    bool result = FSBlock::check(verbose);
    
    result &= assertNotNull(getParentDirRef(), verbose);
    result &= assertInRange(getParentDirRef(), verbose);
    result &= assertInRange(getFirstDataBlockRef(), verbose);
    result &= assertInRange(getNextListBlockRef(), verbose);

    for (u32 i = 0; i < getMaxDataBlockRefs(); i++) {
        result &= assertInRange(getDataBlockRef(i), verbose);
    }
    
    if (getNumDataBlockRefs() > 0 && getFirstDataBlockRef() == 0) {
        if (verbose) fprintf(stderr, "Missing reference to first data block\n");
        return false;
    }
    
    if (getNumDataBlockRefs() < getMaxDataBlockRefs() && getNextListBlockRef() != 0) {
        if (verbose) fprintf(stderr, "Unexpectedly found an extension block\n");
        return false;
    }
    
    return result;
}
*/

void
FSFileHeaderBlock::updateChecksum()
{
    set32(5, 0);
    set32(5, checksum());
}

size_t
FSFileHeaderBlock::addData(const u8 *buffer, size_t size)
{
    assert(getFileSize() == 0);
    
    // Compute the required number of DataBlocks
    u32 bytes = volume.getDataBlockCapacity();
    u32 numDataBlocks = (size + bytes - 1) / bytes;

    // Compute the required number of FileListBlocks
    u32 numDataListBlocks = 0;
    u32 max = getMaxDataBlockRefs();
    if (numDataBlocks > max) {
        numDataListBlocks = 1 + (numDataBlocks - max) / max;
    }

    printf("Required data blocks : %d\n", numDataBlocks);
    printf("Required list blocks : %d\n", numDataListBlocks);
    printf("         Free blocks : %d out of %d\n", volume.freeBlocks(), volume.numBlocks());
    
    if (volume.freeBlocks() < numDataBlocks + numDataListBlocks) {
        printf("Not enough free blocks\n");
        return 0;
    }
    
    for (u32 ref = nr, i = 0; i < numDataListBlocks; i++) {

        // Add a new file list block
        ref = volume.addFileListBlock(nr, ref);
    }
    
    for (u32 ref = nr, i = 1; i <= numDataBlocks; i++) {

        // Add a new data block
        ref = volume.addDataBlock(i, nr, ref);

        // Add references to the new data block
        addDataBlockRef(ref);
        
        // Add data
        FSBlock *block = volume.block(ref);
        if (block) {
            size_t written = block->addData(buffer, size);
            setFileSize(getFileSize() + written);
            buffer += written;
            size -= written;
        }
    }

    return getFileSize();
}


bool
FSFileHeaderBlock::addDataBlockRef(u32 ref)
{
    return addDataBlockRef(nr, ref);
}

bool
FSFileHeaderBlock::addDataBlockRef(u32 first, u32 ref)
{
    // If this block has space for more references, add it here
    if (getNumDataBlockRefs() < getMaxDataBlockRefs()) {

        if (getNumDataBlockRefs() == 0) setFirstDataBlockRef(ref);
        setDataBlockRef(getNumDataBlockRefs(), ref);
        incNumDataBlockRefs();
        return true;
    }

    // Otherwise, add it to an extension block
    FSFileListBlock *item = getNextExtensionBlock();
    
    for (int i = 0; item && i < searchLimit; i++) {
        
        if (item->addDataBlockRef(first, ref)) return true;
        item = item->getNextExtensionBlock();
    }
    
    assert(false);
    return false;
}
