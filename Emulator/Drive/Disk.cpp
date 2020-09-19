// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"

Disk::Disk(DiskType type)
{
    setDescription("Disk");
    
    this->type = type;
    writeProtected = false;
    modified = false;
    clearDisk();
}

long
Disk::numSides(DiskType type)
{
    return 2;
}

long
Disk::numCylinders(DiskType type)
{
    assert(isDiskType(type));
    
    switch (type) {
        case DISK_35_DD:    return 84;
        case DISK_35_HD:    return 84;
        case DISK_525_SD:   return 40;
        default:            return 0;
    }
}

long
Disk::numTracks(DiskType type)
{
    return numSides(type) * numCylinders(type);
}

long
Disk::numSectorsPerTrack(DiskType type)
{
    assert(isDiskType(type));
    
    switch (type) {
        case DISK_35_DD:    return 11;
        case DISK_35_HD:    return 22;
        case DISK_525_SD:   return 9;
        default:            return 0;
    }
}

long
Disk::numSectorsTotal(DiskType type)
{
    return numTracks(type) * numSectorsPerTrack(type);
}

Disk *
Disk::makeWithADFFile(ADFFile *file)
{
    Disk *disk = new Disk(file->getDiskType());
    
    if (!disk->encodeAmigaDisk(file)) {
        delete disk;
        return NULL;
    }
    
    disk->fnv = file->fnv();
    
    return disk;
}

Disk *
Disk::makeWithDMSFile(DMSFile *file)
{
    return makeWithADFFile(file->adf);
}

Disk *
Disk::makeWithIMGFile(IMGFile *file)
{
    Disk *disk = new Disk(file->getDiskType());
    
    if (!disk->encodeDosDisk(file)) {
        delete disk;
        return NULL;
    }
    
    disk->fnv = file->fnv();
    
    return disk;
}

Disk *
Disk::makeWithReader(SerReader &reader, DiskType diskType)
{
    Disk *disk = new Disk(diskType);
    disk->applyToPersistentItems(reader);
    
    return disk;
}

u8
Disk::readByte(Cylinder cylinder, Side side, u16 offset)
{
    assert(isValidCylinderNr(cylinder));
    assert(isValidSideNr(side));
    assert(offset < trackSize);

    return data.cyclinder[cylinder][side][offset];
}

void
Disk::writeByte(u8 value, Cylinder cylinder, Side side, u16 offset)
{
    assert(isValidCylinderNr(cylinder));
    assert(isValidSideNr(side));
    assert(offset < trackSize);
    
    data.cyclinder[cylinder][side][offset] = value;
}

void
Disk::clearDisk()
{
    assert(sizeof(data) == sizeof(data.raw));

    srand(0);
    for (int i = 0; i < sizeof(data); i++)
        data.raw[i] = rand() & 0xFF;
    
    /* We are allowed to place random data here.
     * In order to make some copy protected game titles work, we place
     * add some magic values.
     * Crunch factory: Looks for 0x44A2 on cylinder 80
     */
    for (int t = 0; t < 2*84; t++) {
        data.track[t][0] = 0x44;
        data.track[t][1] = 0xA2;
    }
    
    fnv = 0;
}

void
Disk::clearTrack(Track t)
{
    assert(isValidTrack(t));

    srand(0);
    for (int i = 0; i < sizeof(data.track[t]); i++)
        data.track[t][i] = rand() & 0xFF;
}

void
Disk::clearTrack(Track t, u8 value)
{
    assert(isValidTrack(t));

    memset(data.track[t], value, trackSize);
}

bool
Disk::encodeAmigaDisk(ADFFile *adf)
{
    assert(adf != NULL);
    assert(adf->getDiskType() == getType());

    long tracks = adf->numTracks();
    long sectors = adf->numSectorsPerTrack();

    debug("Encoding disk (%d tracks, %d sectors each)...\n", tracks, sectors);
    assert(tracks <= numTracks());
    assert(sectors == numSectorsPerTrack());

    // Initialize disk with random data
    clearDisk();

    // Encode all tracks
    bool result = true;
    for (Track t = 0; t < tracks; t++) result &= encodeAmigaTrack(adf, t, sectors);

    return result;
}

bool
Disk::encodeAmigaTrack(ADFFile *adf, Track t, long smax)
{
    assert(isValidTrack(t));

    debug(MFM_DEBUG, "Encoding track %d\n", t);

    // Format track
    clearTrack(t, 0xAA);

    // Encode all sectors
    bool result = true;
    for (Sector s = 0; s < smax; s++) result &= encodeAmigaSector(adf, t, s);
    
    // Get the clock bit right at offset position 0
    if (data.track[t][trackSize - 1] & 1) data.track[t][0] &= 0x7F;

    // Compute a debugging checksum
    u8 *p = data.track[t] + (0 * sectorSize);
    if (DSK_CHECKSUM) {
        u32 check = fnv_1a_init32();
        for (unsigned i = 0; i < trackSize / 2; i+=2) {
            check = fnv_1a_it32(check, HI_LO(p[i],p[i+1]));
        }
        plaindebug(MFM_DEBUG, "Track %d checksum = %X\n", t, check);
    }

    return result;
}

bool
Disk::encodeAmigaSector(ADFFile *adf, Track t, Sector s)
{
    assert(isValidTrack(t));
    assert(isValidSector(s));
    
    debug(MFM_DEBUG, "Encoding sector %d\n", s);
    
    /* Block header layout:
     *                     Start  Size   Value
     * Bytes before SYNC   00      4     0xAA 0xAA 0xAA 0xAA
     * SYNC mark           04      4     0x44 0x89 0x44 0x89
     * Track & sector info 08      8     Odd/Even encoded
     * Unused area         16     32     0xAA
     * Block checksum      48      8     Odd/Even encoded
     * Data checksum       56      8     Odd/Even encoded
     */
    
    u8 *p = data.track[t] + (s * sectorSize) + trackGapSize;
    
    // Bytes before SYNC
    p[0] = (p[-1] & 1) ? 0x2A : 0xAA;
    p[1] = 0xAA;
    p[2] = 0xAA;
    p[3] = 0xAA;
    
    // SYNC mark
    u16 sync = 0x4489;
    p[4] = HI_BYTE(sync);
    p[5] = LO_BYTE(sync);
    p[6] = HI_BYTE(sync);
    p[7] = LO_BYTE(sync);
    
    // Track and sector information
    u8 info[4] = { 0xFF, (u8)t, (u8)s, (u8)(11 - s) };
    encodeOddEven(&p[8], info, sizeof(info));
    
    // Unused area
    for (unsigned i = 16; i < 48; i++)
    p[i] = 0xAA;
    
    // Data
    u8 bytes[512];
    adf->readSector(bytes, t, s);
    encodeOddEven(&p[64], bytes, sizeof(bytes));
    
    // Block checksum
    u8 bcheck[4] = { 0, 0, 0, 0 };
    for(unsigned i = 8; i < 48; i += 4) {
        bcheck[0] ^= p[i];
        bcheck[1] ^= p[i+1];
        bcheck[2] ^= p[i+2];
        bcheck[3] ^= p[i+3];
    }
    encodeOddEven(&p[48], bcheck, sizeof(bcheck));
    
    // Data checksum
    u8 dcheck[4] = { 0, 0, 0, 0 };
    for(unsigned i = 64; i < 1088; i += 4) {
        dcheck[0] ^= p[i];
        dcheck[1] ^= p[i+1];
        dcheck[2] ^= p[i+2];
        dcheck[3] ^= p[i+3];
    }
    encodeOddEven(&p[56], dcheck, sizeof(bcheck));
    
    // Add clock bits
    for(unsigned i = 8; i < 1088; i++) {
        p[i] = addClockBits(p[i], p[i-1]);
    }
    
    return true;
}

bool
Disk::decodeAmigaDisk(u8 *dst, int tracks, int sectors)
{
    bool result = true;
        
    debug("Decoding disk (%d tracks, %d sectors each)...\n", tracks, sectors);
    
    for (Track t = 0; t < tracks; t++) {
        result &= decodeAmigaTrack(dst, t, sectors);
        dst += sectors * 512;
    
    }
    
    return result;
}

bool
Disk::decodeAmigaTrack(u8 *dst, Track t, long smax)
{
    assert(isValidTrack(t));
        
    debug(MFM_DEBUG, "Decoding track %d\n", t);
    
    // Create a local (double) copy of the track to simply the analysis
    u8 local[2 * trackSize];
    memcpy(local, data.track[t], trackSize);
    memcpy(local + trackSize, data.track[t], trackSize);
    
    // Seek all sync marks
    int sectorStart[smax], index = 0, nr = 0;
    while (index < trackSize + sectorSize && nr < smax) {

        if (local[index++] != 0x44) continue;
        if (local[index++] != 0x89) continue;
        if (local[index++] != 0x44) continue;
        if (local[index++] != 0x89) continue;
        
        sectorStart[nr++] = index;
        // debug("   Sector %d starts at index %d\n", nr-1, sectorStart[nr-1]);
    }
    
    if (nr != smax) {
        warn("Track %d: Found %d sectors, expected %d. Aborting.\n", t, nr, smax);
        return false;
    }
    
    // Encode all sectors
    for (Sector s = 0; s < smax; s++) {
        decodeAmigaSector(dst, local + sectorStart[s]);
        dst += 512;
    }
    
    return true;
}

void
Disk::decodeAmigaSector(u8 *dst, u8 *src)
{
    assert(dst != NULL);
    assert(src != NULL);

    // Skip sector header
    src += 56;
    
    // Decode sector data
    decodeOddEven(dst, src, 512);
}

bool
Disk::encodeDosDisk(IMGFile *img)
{
    assert(false);
    return false;
}

bool
Disk::encodeDosTrack(IMGFile *img, Track t, long smax)
{
    assert(false);
    return false;
}

bool encodeDosSector(IMGFile *img, Track t, Sector s)
{
    assert(false);
    return false;
}

void
Disk::encodeOddEven(u8 *dst, u8 *src, size_t count)
{
    // Encode odd bits
    for(size_t i = 0; i < count; i++)
        dst[i] = (src[i] >> 1) & 0x55;
    
    // Encode even bits
    for(size_t i = 0; i < count; i++)
        dst[i + count] = src[i] & 0x55;
}

void
Disk::decodeOddEven(u8 *dst, u8 *src, size_t count)
{
    // Decode odd bits
    for(size_t i = 0; i < count; i++)
        dst[i] = (src[i] & 0x55) << 1;
    
    // Decode even bits
    for(size_t i = 0; i < count; i++)
        dst[i] |= src[i + count] & 0x55;
}

u8
Disk::addClockBits(u8 value, u8 previous)
{
    // Clear all previously set clock bits
    value &= 0x55;

    // Compute clock bits (clock bit values are inverted)
    u8 lShifted = (value << 1);
    u8 rShifted = (value >> 1) | (previous << 7);
    u8 cBitsInv = lShifted | rShifted;

    // Reverse the computed clock bits
    u64 cBits = cBitsInv ^ 0xAA;
    
    // Return original value with the clock bits added
    return value | cBits;
}
