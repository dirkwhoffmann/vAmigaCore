// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"

Thumbnail *
Thumbnail::makeWithAmiga(Amiga *amiga, int dx, int dy)
{
    Thumbnail *screenshot = new Thumbnail();
    screenshot->take(amiga, dx, dy);
    
    return screenshot;
}

void
Thumbnail::take(Amiga *amiga, int dx, int dy)
{
    u32 *source = (u32 *)amiga->denise.pixelEngine.getStableBuffer().data;
    u32 *target = screen;
    
    int xStart = 4 * HBLANK_MAX + 1, xEnd = HPIXELS + 4 * HBLANK_MIN;
    int yStart = VBLANK_CNT, yEnd = VPIXELS - 2;
    
    width  = (xEnd - xStart) / dx;
    height = (yEnd - yStart) / dy;
    
    source += xStart + yStart * HPIXELS;
    
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            target[x] = source[x * dx];
        }
        source += dy * HPIXELS;
        target += width;
    }
    
    timestamp = time(nullptr);
}

bool
Snapshot::isSnapshot(const u8 *buf, size_t len)
{
    assert(buf != nullptr);

    u8 signature[] = { 'V', 'A', 'S', 'N', 'A', 'P' };
        
    if (len < sizeof(SnapshotHeader)) return false;
    return matchingBufferHeader(buf, signature, sizeof(signature));
}

bool
Snapshot::isSnapshot(const u8 *buf, size_t len, u8 major, u8 minor, u8 subminor)
{
    assert(buf != nullptr);

    u8 signature[] = { 'V', 'A', 'S', 'N', 'A', 'P', major, minor, subminor };
    
    if (len < sizeof(SnapshotHeader)) return false;
    return matchingBufferHeader(buf, signature, sizeof(signature));
}

/*
bool
Snapshot::isSupportedSnapshot(const u8 *buffer, size_t length)
{
    return isSnapshot(buffer, length, V_MAJOR, V_MINOR, V_SUBMINOR);
}

bool
Snapshot::isUnsupportedSnapshot(const u8 *buffer, size_t length)
{
    return isSnapshot(buffer, length) && !isSupportedSnapshot(buffer, length);
}
*/

bool
Snapshot::isSnapshotFile(const char *path)
{
    assert(path != nullptr);

    u8 signature[] = { 'V', 'A', 'S', 'N', 'A', 'P' };
    
    return matchingFileHeader(path, signature, sizeof(signature));
}

bool
Snapshot::isSnapshotFile(const char *path, u8 major, u8 minor, u8 subminor)
{
    assert(path != nullptr);

    u8 signature[] = { 'V', 'A', 'S', 'N', 'A', 'P', major, minor, subminor };
        
    return matchingFileHeader(path, signature, sizeof(signature));
}

/*
bool
Snapshot::isSupportedSnapshotFile(const char *path)
{
    return isSnapshotFile(path, V_MAJOR, V_MINOR, V_SUBMINOR);
}

bool
Snapshot::isUnsupportedSnapshotFile(const char *path)
{
    return isSnapshotFile(path) && !isSupportedSnapshotFile(path);
}
*/

Snapshot::Snapshot()
{
}

Snapshot::Snapshot(size_t capacity)
{
    u8 signature[] = { 'V', 'A', 'S', 'N', 'A', 'P' };
    
    size = capacity + sizeof(SnapshotHeader);
    data = new u8[size];
    
    SnapshotHeader *header = (SnapshotHeader *)data;
    
    for (unsigned i = 0; i < sizeof(signature); i++)
        header->magic[i] = signature[i];
    header->major = V_MAJOR;
    header->minor = V_MINOR;
    header->subminor = V_SUBMINOR;
}

Snapshot *
Snapshot::makeWithAmiga(Amiga *amiga)
{
    Snapshot *snapshot = new Snapshot(amiga->size());

    snapshot->getHeader()->screenshot.take(amiga);
    amiga->save(snapshot->getData());

    return snapshot;
}

bool
Snapshot::matchingBuffer(const u8* buffer, size_t length)
{
    return Snapshot::isSnapshot(buffer, length);
}

bool
Snapshot::matchingFile(const char *path)
{
    return Snapshot::isSnapshotFile(path);
}

bool
Snapshot::readFromBuffer(const u8 *buf, size_t len, FileError *err)
{
    // Read from buffer
    if (AmigaFile::readFromBuffer(buf, len, err)) {

        // Check the version number of this snapshot
        if (!isSnapshot(buf, len, V_MAJOR, V_MINOR, V_SUBMINOR)) {
            *err = ERR_UNSUPPORTED_SNAPSHOT;
        }
    }
    
    return *err == ERR_FILE_OK;
}
