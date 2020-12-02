// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _FS_DESCRIPTORS_H
#define _FS_DESCRIPTORS_H

#include "FSObjects.h"
#include "FSBlock.h"
#include "FSEmptyBlock.h"
#include "FSBootBlock.h"
#include "FSRootBlock.h"
#include "FSBitmapBlock.h"
#include "FSBitmapExtBlock.h"
#include "FSUserDirBlock.h"
#include "FSFileHeaderBlock.h"
#include "FSFileListBlock.h"
#include "FSDataBlock.h"

struct FSDeviceDescriptor : AmigaObject {
    
    // Number of physical cylinders
    u32 numCyls = 0;

    // Number of physical heads
    u32 numHeads = 0;
        
    // Number of blocks per track
    u32 numSectors = 0;
    
    // Total number of blocks
    u32 blocks = 0;

    // Number of reserved blocks at the beginning of the block list
    u32 numReserved = 0;
    
    // Size of a single block in bytes
    u32 bsize = 0;
    
    // The list of partitions
    std::vector<struct FSPartitionDescriptor> part;
    
    
    //
    // Initializing
    //
    
    FSDeviceDescriptor() { }
    FSDeviceDescriptor(DiskType type, DiskDensity density, FSVolumeType dos = FS_OFS);
    FSDeviceDescriptor(class ADFFile *adf);
    FSDeviceDescriptor(class HDFFile *adf);

    const char *getDescription() override { return "FSLayout"; }
    
    void dump();
};

struct FSPartitionDescriptor : AmigaObject {
    
    // File system type
    FSVolumeType dos;
    
    // Cylinder boundaries
    u32 lowCyl = 0;
    u32 highCyl = 0;
    
    // Block boundaries
    u32 firstBlock = 0;
    u32 lastBlock = 0;
    
    // Location of the root block
    u32 rootBlock = 0;
    
    // References to all bitmap blocks and bitmap extension blocks
    vector<u32> bmBlocks;
    vector<u32> bmExtBlocks;

    
    //
    // Initializing
    //
    
    FSPartitionDescriptor(struct FSDeviceDescriptor &layout,
                FSVolumeType dos, u32 firstCyl, u32 lastCyl, u32 root);

    const char *getDescription() override { return "FSPartition"; }
    
    void dump();

    
    //
    // Querying partition properties
    //
    
    // Returns the number of cylinders in this partition
    u32 numCyls() { return highCyl - lowCyl + 1; }
};

#endif
