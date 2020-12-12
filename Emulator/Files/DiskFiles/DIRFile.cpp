// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "DIRFile.h"
#include "FSDevice.h"

DIRFile::DIRFile()
{
}

bool
DIRFile::isDIRFile(const char *path)
{
    assert(path != nullptr);
    return isDirectory(path);
}

bool
DIRFile::matchingBuffer(const u8 *buffer, size_t length)
{
    assert(false);
    return false;
}

bool
DIRFile::readFromBuffer(const u8 *buffer, size_t length, FileError *error)
{
    assert(false);
    return false;
}

bool
DIRFile::readFromFile(const char *filename, FileError *error)
{
    debug(FS_DEBUG, "DIRFile::readFromFile(%s)\n", filename);
              
    // Only proceed if the provided filename points to a directory
    if (!isDIRFile(filename)) {
        warn("%s is not a directory\n", filename);
        if (error) *error = ERR_INVALID_TYPE;
        return false;
    }
    
    // Create a file system and import the directory
    FSDevice *volume = FSDevice::make(FS_OFS, filename);
    if (!volume) {
        warn("Contents of %s does not fit on a disk\n", filename);
        if (error) *error = ERR_UNKNOWN;
        return false;
    }
    
    // Check the file system for errors
    volume->info();
    volume->printDirectory(true);

    // Check the file system for consistency
    FSErrorReport report = volume->check(true);
    if (report.corruptedBlocks > 0) {
        warn("Found %ld corrupted blocks\n", report.corruptedBlocks);
    }
    // volume->dump();
    
    // Convert the file system into an ADF
    FSError fsError;
    assert(adf == nullptr);
    adf = ADFFile::makeWithVolume(*volume, &fsError);
    debug(FS_DEBUG, "makeWithVolume: %s\n", sFSError(fsError));
    delete volume;
    
    if (error) *error = adf != nullptr ? ERR_UNKNOWN : ERR_FILE_OK;
    return adf != nullptr;
}
