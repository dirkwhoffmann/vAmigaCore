// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "DiskFile.h"

/* This class is a dummy class for detecting extended ADFs. Once the emulator
 * detects such a file, it tells the user that this format is not supported.
 */

class EXTFile : public DiskFile {
    
    static const isize HEADER_SIZE = 160 * 4 + 8;
    
    // Accepted header signatures
    static const std::vector<string> extAdfHeaders;
    
public:
            
    static bool isCompatible(const string &path);
    static bool isCompatible(std::istream &stream);

 
    //
    // Initializing
    //

public:

    EXTFile(const string &path) throws { AmigaFile::init(path); }
    EXTFile(const u8 *buf, isize len) throws { AmigaFile::init(buf, len); }
    EXTFile(class Disk &disk) throws { init(disk); }
    EXTFile(class Drive &drive) throws { init(drive); }
    
    void init(Disk &disk) throws;
    void init(Drive &drive) throws;

    
    //
    // Methods from AmigaObject
    //

public:
    
    const char *getDescription() const override { return "EXT"; }

    
    //
    // Methods from AmigaFile
    //
    
public:
    
    bool isCompatiblePath(const string &path) const override { return isCompatible(path); }
    bool isCompatibleStream(std::istream &stream) const override { return isCompatible(stream); }
    FileType type() const override { return FILETYPE_EXT; }
    isize readFromStream(std::istream &stream) override;
    
    
    //
    // Methods from DiskFile
    //
    
    FSVolumeType getDos() const override { return FS_NODOS; }
    void setDos(FSVolumeType dos) override { };
    DiskDiameter getDiskDiameter() const override { return INCH_35; }
    DiskDensity getDiskDensity() const override { return DISK_DD; }
    isize numSides() const override { return 2; }
    isize numCyls() const override { return 80; }
    isize numSectors() const override { return 11; }
    
    u8 readByte(isize b, isize offset) const override { return 0; }
    u8 readByte(isize t, isize s, isize offset) const override { return 0; }
    void readSector(u8 *dst, isize b) const override { }
    void readSector(u8 *dst, isize t, isize s) const override { }
    void readSectorHex(char *dst, isize b, isize count) const override { }
    void readSectorHex(char *dst, isize t, isize s, isize count) const override { }    
};
