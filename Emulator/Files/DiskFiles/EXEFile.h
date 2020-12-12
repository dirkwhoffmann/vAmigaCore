// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _EXE_FILE_H
#define _EXE_FILE_H

#include "ADFFile.h"

class EXEFile : public DiskFile {
    
public:
    
    ADFFile *adf = nullptr;
    
    
    //
    // Class methods
    //
    
    // Returns true iff the provided buffer contains an Amiga executable
    static bool isEXEBuffer(const u8 *buffer, size_t length);
    
    // Returns true iff if the provided path points to an Amiga executable
    static bool isEXEFile(const char *path);
    
    
    //
    // Initializing
    //
    
    EXEFile();
    
    const char *getDescription() override { return "EXE"; }
    
    static EXEFile *makeWithFile(const char *path);
    
    
    //
    // Methods from AmigaFile
    //
    
    AmigaFileType fileType() override { return FILETYPE_EXE; }
    const char *typeAsString() override { return "EXE"; }
    u64 fnv() override { return adf->fnv(); }
    bool bufferHasSameType(const u8 *buffer, size_t length) override {
        return isEXEBuffer(buffer, length); }
    bool fileHasSameType(const char *path) override { return isEXEFile(path); }
    bool readFromBuffer(const u8 *buffer, size_t length) override;
    
    
    //
    // Methods from DiskFile
    //
    
    FSVolumeType getDos() override { return adf->getDos(); }
    void setDos(FSVolumeType dos) override { adf->setDos(dos); }
    DiskType getDiskType() override { return adf->getDiskType(); }
    DiskDensity getDiskDensity() override { return adf->getDiskDensity(); }
    long numSides() override { return adf->numSides(); }
    long numCyls() override { return adf->numCyls(); }
    long numSectors() override { return adf->numSectors(); }
    BootBlockType bootBlockType() override { return adf->bootBlockType(); }
    const char *bootBlockName() override { return adf->bootBlockName(); }
    void killVirus() override { adf->killVirus(); }
    void readSector(u8 *target, long s) override { return adf->readSector(target, s); }
    void readSector(u8 *target, long t, long s) override { return adf->readSector(target, t, s); }
    bool encodeDisk(class Disk *disk) override { return adf->encodeDisk(disk); }
};

#endif
