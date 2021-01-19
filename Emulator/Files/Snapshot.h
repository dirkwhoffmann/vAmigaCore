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

class Amiga;

struct Thumbnail {
    
    // Image size
    u16 width, height;
    
    // Raw texture data
    u32 screen[(HPIXELS / 2) * (VPIXELS / 1)];
    
    // Date and time of screenshot creation
    time_t timestamp;
    
    // Factory methods
    static Thumbnail *makeWithAmiga(Amiga *amiga, int dx = 2, int dy = 1);
    
    // Takes a screenshot from a given Amiga
    void take(Amiga *amiga, int dx = 2, int dy = 1);
};

struct SnapshotHeader {
    
    // Magic bytes ('V','A','S','N','A','P')
    char magic[6];
    
    // Version number (V major.minor.subminor)
    u8 major;
    u8 minor;
    u8 subminor;
    
    // Screenshot
    Thumbnail screenshot;
};

class Snapshot : public AmigaFile {
 
    //
    // Class methods
    //
    
public:
    
    static bool isCompatibleName(const std::string &name);
    static bool isCompatibleStream(std::istream &stream);

    // Returns true iff buffer contains a snapshot (of a specific version)
    static bool isSnapshot(const u8 *buf, size_t len);
    static bool isSnapshot(const u8 *buf, size_t len, u8 major, u8 minor, u8 subminor);
        
    // Returns true if path points to a snapshot file (of a specific version)
    [[deprecated]] static bool isSnapshotFile(const char *path);
    [[deprecated]] static bool isSnapshotFile(const char *path, u8 major, u8 minor, u8 subminor);
        
    
    //
    // Initializing
    //
    
    Snapshot();
    Snapshot(size_t capacity);
    
    const char *getDescription() const override { return "Snapshot"; }
    
    bool setCapacity(size_t size);
    
    static Snapshot *makeWithAmiga(Amiga *amiga);
    
    
    //
    // Methods from AmigaFile
    //
    
    FileType type() const override { return FILETYPE_SNAPSHOT; }
    [[deprecated]] bool matchingBuffer(const u8 *buffer, size_t length) override;
    [[deprecated]] bool matchingFile(const char *filename) override;
    
    // bool readFromBuffer(const u8 *buffer, size_t length, ErrorCode *error = nullptr) override;

    
    //
    // Accessing snapshot properties
    //
    
public:
    
    // Returns pointer to header data
    const SnapshotHeader *getHeader() const { return (SnapshotHeader *)data; }
    
    // Returns a pointer to the thumbnail image
    const Thumbnail &getThumbnail() const { return getHeader()->screenshot; }
    
    // Returns pointer to core data
    u8 *getData() { return data + sizeof(SnapshotHeader); }
    
    // Takes a screenshot
    void takeScreenshot(Amiga &amiga);
};
