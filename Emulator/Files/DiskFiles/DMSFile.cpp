// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "DMSFile.h"

extern "C" {
unsigned short extractDMS(FILE *fi, FILE *fo);
}

DMSFile::DMSFile()
{
}

bool
DMSFile::isDMSBuffer(const u8 *buffer, size_t length)
{
    u8 signature[] = { 'D', 'M', 'S', '!' };
                                                                                            
    assert(buffer != nullptr);
    return matchingBufferHeader(buffer, signature, sizeof(signature));
}

bool
DMSFile::isDMSFile(const char *path)
{
    u8 signature[] = { 'D', 'M', 'S', '!' };
    
    assert(path != nullptr);
    return matchingFileHeader(path, signature, sizeof(signature));
}

bool
DMSFile::readFromBuffer(const u8 *buffer, size_t length, FileError *error)
{
    FILE *fpi, *fpo;
    char *pi, *po;
    size_t si, so;
    
    if (!isDMSBuffer(buffer, length)) {
        if (error) *error = ERR_INVALID_FILE_TYPE;
        return false;
    }

    if (!AmigaFile::readFromBuffer(buffer, length, error))
        return false;
    
    // We use a third-party tool called xdms to convert the DMS file into an
    // ADF file. Originally, xdms is a command line utility that is designed
    // to work with the file system. To ease the integration of this tool, we
    // utilize memory streams for getting data in and out. 

    // Setup input stream
    fpi = open_memstream(&pi, &si);
    for (size_t i = 0; i < size; i++) putc(data[i], fpi);
    fclose(fpi);
    
    // Setup output file
    fpi = fmemopen(pi, si, "r");
    fpo = open_memstream(&po, &so);
    extractDMS(fpi, fpo);
    fclose(fpi);
    fclose(fpo);
    
    // Create ADF
    fpo = fmemopen(po, so, "r");
    adf = AmigaFile::make <ADFFile> (fpo);
    fclose(fpo);
    
    if (error) *error = adf ? ERR_FILE_OK : ERR_UNKNOWN;
    return adf != nullptr;
}

