// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"
#include <new>

int OCSREG_DEBUG = 0;
int CIAREG_DEBUG = 0;

Memory::Memory(Amiga& ref) : AmigaComponent(ref)
{
    setDescription("Memory");

    memset(&config, 0, sizeof(config));
    config.extStart = 0xE0;
}

Memory::~Memory()
{
    dealloc();
}

void
Memory::dealloc()
{
    if (rom) { delete[] rom; rom = NULL; }
    if (wom) { delete[] wom; wom = NULL; }
    if (ext) { delete[] ext; ext = NULL; }
    if (chip) { delete[] chip; chip = NULL; }
    if (slow) { delete[] slow; slow = NULL; }
    if (fast) { delete[] fast; fast = NULL; }
}

void
Memory::setExtStart(u32 page)
{
    assert(page == 0xE0 || page == 0xF0);

    config.extStart = page;
    updateMemSrcTable();
}

void
Memory::_powerOn()
{
    // dump();
    
    // Erase WOM (if any)
    if (hasWom()) eraseWom();

    // Fill RAM with the proper startup pattern
    fillRamWithStartupPattern();

    // Set up the memory lookup table
    updateMemSrcTable();
}

void
Memory::_reset(bool hard)
{
    RESET_SNAPSHOT_ITEMS
    
    // Set up the memory lookup table
    updateMemSrcTable();
    
    // In hard-reset mode, we also initialize RAM
    if (hard) fillRamWithStartupPattern();
}

void
Memory::_dump()
{
    struct { u8 *addr; size_t size; const char *desc; } mem[7] = {
        { rom, config.romSize, "Rom" },
        { wom, config.womSize, "Wom" },
        { ext, config.extSize, "Ext" },
        { chip, config.chipSize, "Chip Ram" },
        { slow, config.slowSize, "Slow Ram" },
        { fast, config.fastSize, "Fast Ram" }
    };

    // Print a summary of the installed memory
    for (int i = 0; i < 6; i++) {

        size_t size = mem[i].size;
        u8 *addr = mem[i].addr;

        msg("     %s: ", mem[i].desc);
        if (size == 0) {
            assert(addr == 0);
            msg("not present\n");
        } else {
            assert(addr != 0);
            assert(size % KB(1) == 0);
            u32 check = fnv_1a_32(addr, size);
            msg("%3d KB at: %p Checksum: %x\n", size >> 10, addr, check);
        }
    }
}

size_t
Memory::_size()
{
    SerCounter counter;

    applyToPersistentItems(counter);
    applyToResetItems(counter);

    counter.count += sizeof(config.romSize) + config.romSize;
    counter.count += sizeof(config.womSize) + config.womSize;
    counter.count += sizeof(config.extSize) + config.extSize;
    counter.count += sizeof(config.chipSize) + config.chipSize;
    counter.count += sizeof(config.slowSize) + config.slowSize;
    counter.count += sizeof(config.fastSize) + config.fastSize;

    return counter.count;
}

size_t
Memory::didLoadFromBuffer(u8 *buffer)
{
    SerReader reader(buffer);

    // Load memory size information
    reader
    & config.romSize
    & config.womSize
    & config.extSize
    & config.chipSize
    & config.slowSize
    & config.fastSize;

    // Make sure that corrupted values do not cause any damage
    if (config.romSize > KB(512)) { config.romSize = 0; assert(false); }
    if (config.womSize > KB(256)) { config.womSize = 0; assert(false); }
    if (config.extSize > KB(512)) { config.extSize = 0; assert(false); }
    if (config.chipSize > MB(2)) { config.chipSize = 0; assert(false); }
    if (config.slowSize > KB(512)) { config.slowSize = 0; assert(false); }
    if (config.fastSize > MB(8)) { config.fastSize = 0; assert(false); }

    // Free previously allocated memory
    dealloc();

    // Allocate new memory
    if (config.romSize) rom = new (std::nothrow) u8[config.romSize];
    if (config.womSize) wom = new (std::nothrow) u8[config.womSize];
    if (config.extSize) ext = new (std::nothrow) u8[config.extSize];
    if (config.chipSize) chip = new (std::nothrow) u8[config.chipSize];
    if (config.slowSize) slow = new (std::nothrow) u8[config.slowSize];
    if (config.fastSize) fast = new (std::nothrow) u8[config.fastSize];

    // Load memory contents from buffer
    reader.copy(rom, config.romSize);
    reader.copy(wom, config.womSize);
    reader.copy(ext, config.extSize);
    reader.copy(chip, config.chipSize);
    reader.copy(slow, config.slowSize);
    reader.copy(fast, config.fastSize);

    return reader.ptr - buffer;
}

size_t
Memory::didSaveToBuffer(u8 *buffer)
{
    // Save memory size information
    SerWriter writer(buffer);
    writer
    & config.romSize
    & config.womSize
    & config.extSize
    & config.chipSize
    & config.slowSize
    & config.fastSize;

    // Save memory contents
    writer.copy(rom, config.romSize);
    writer.copy(wom, config.womSize);
    writer.copy(ext, config.extSize);
    writer.copy(chip, config.chipSize);
    writer.copy(slow, config.slowSize);
    writer.copy(fast, config.fastSize);

    return writer.ptr - buffer;
}

void
Memory::updateStats()
{
    const double w = 0.5;
    
    stats.chipReads.accumulated =
    w * stats.chipReads.accumulated + (1.0 - w) * stats.chipReads.raw;
    stats.chipWrites.accumulated =
    w * stats.chipWrites.accumulated + (1.0 - w) * stats.chipWrites.raw;
    stats.slowReads.accumulated =
    w * stats.slowReads.accumulated + (1.0 - w) * stats.slowReads.raw;
    stats.slowWrites.accumulated =
    w * stats.slowWrites.accumulated + (1.0 - w) * stats.slowWrites.raw;
    stats.fastReads.accumulated =
    w * stats.fastReads.accumulated + (1.0 - w) * stats.fastReads.raw;
    stats.fastWrites.accumulated =
    w * stats.fastWrites.accumulated + (1.0 - w) * stats.fastWrites.raw;
    stats.kickReads.accumulated =
    w * stats.kickReads.accumulated + (1.0 - w) * stats.kickReads.raw;
    stats.kickWrites.accumulated =
    w * stats.kickWrites.accumulated + (1.0 - w) * stats.kickWrites.raw;

    stats.chipReads.raw = 0;
    stats.chipWrites.raw = 0;
    stats.slowReads.raw = 0;
    stats.slowWrites.raw = 0;
    stats.fastReads.raw = 0;
    stats.fastWrites.raw = 0;
    stats.kickReads.raw = 0;
    stats.kickWrites.raw = 0;
}

bool
Memory::alloc(size_t bytes, u8 *&ptr, size_t &size, u32 &mask)
{
    // Check the invariants
    assert((ptr == NULL) == (size == 0));
    assert((ptr == NULL) == (mask == 0));
    assert((ptr != NULL) == (mask == size - 1));

    // Only proceed if memory layout changes
    if (bytes == size) return true;
    
    // Delete previous allocation
    if (ptr) { delete[] ptr; ptr = NULL; size = 0; mask = 0; }
    
    // Allocate memory
    if (bytes) {
        
        size_t allocSize = bytes;
        
        if (!(ptr = new (std::nothrow) u8[allocSize])) {
            warn("Cannot allocate %d KB of memory\n", bytes);
            return false;
        }
        size = bytes;
        mask = bytes - 1;
        memset(ptr, 0, allocSize);
        
#ifdef RANDOM_RAM_PATTERN
        srand(0);
        for (int i = 0; i < allocSize; i++) {
            ptr[i] = rand() & 0xFF;
        }
#endif
    }

    updateMemSrcTable();
    return true;
}

void
Memory::fillRamWithStartupPattern()
{
    if (chip) memset(chip, 0x0, config.chipSize);
    if (slow) memset(slow, 0x0, config.slowSize);
    if (fast) memset(fast, 0x0, config.fastSize);
}

const char *
Memory::romVersion()
{
    static char str[32];

    if (romIdentifier() == ROM_UNKNOWN) {
        sprintf(str, "CRC %x", romFingerprint());
        return str;
    }

    return RomFile::version(romIdentifier());
}

const char *
Memory::extVersion()
{
    static char str[32];

    if (extIdentifier() == ROM_UNKNOWN) {
        sprintf(str, "CRC %x", extFingerprint());
        return str;
    }

    return RomFile::version(extIdentifier());
}

bool
Memory::loadRom(RomFile *file)
{
    assert(file != NULL);

    // Allocate memory and load file
    if (!allocRom(file->getSize())) return false;
    loadRom(file, rom, config.romSize);

    // Add a Wom if a Boot Rom is installed instead of a Kickstart Rom
    hasBootRom() ? (void)allocWom(KB(256)) : deleteWom();

    // Remove extended Rom (if any)
    deleteExt();
    
    return true;
}

bool
Memory::loadRomFromBuffer(const u8 *buffer, size_t length)
{
    assert(buffer != NULL);
    
    RomFile *file = RomFile::makeWithBuffer(buffer, length);
    
    if (!file) {
        msg("Failed to read Kick Rom from buffer at %p\n", buffer);
        return false;
    }
    
    return loadRom(file);
}

bool
Memory::loadRomFromFile(const char *path)
{
    assert(path != NULL);
    
    RomFile *rom = RomFile::makeWithFile(path);
    if (rom == NULL) return false;
    
    bool success = loadRom(rom);
    delete rom;
    return success;
}

bool
Memory::loadEncryptedRomFromFile(const char *path)
{
    assert(path != NULL);
    
    EncryptedRomFile *encryptedRom = EncryptedRomFile::makeWithFile(path);
    if (encryptedRom == NULL) return false;
        
    RomFile *rom = encryptedRom->decrypt();
    if (rom == NULL) { delete encryptedRom; return false; }

    bool success = loadRom(rom);
    delete rom;
    return success;
}

bool
Memory::loadExt(ExtFile *file)
{
    assert(file != NULL);

    // Allocate memory and load file
    if (!allocExt(file->getSize())) return false;
    loadRom(file, ext, config.extSize);

    return true;
}

bool
Memory::loadExtFromBuffer(const u8 *buffer, size_t length)
{
    assert(buffer != NULL);

    ExtFile *file = ExtFile::makeWithBuffer(buffer, length);

    if (!file) {
        msg("Failed to read Extended Rom from buffer at %p\n", buffer);
        return false;
    }

    return loadExt(file);
}

bool
Memory::loadExtFromFile(const char *path)
{
    assert(path != NULL);

    ExtFile *file = ExtFile::makeWithFile(path);

    if (!file) {
        msg("Failed to read Extended Rom from file %s\n", path);
        return false;
    }

    return loadExt(file);
}

void
Memory::loadRom(AmigaFile *file, u8 *target, size_t length)
{
    if (file) {

        assert(target != NULL);
        memset(target, 0, length);

        file->seek(0);

        int c;
        for (size_t i = 0; i < length; i++) {
            if ((c = file->read()) == EOF) break;
            *(target++) = c;
        }
    }
}

bool
Memory::saveRom(const char *path)
{
    if (rom == NULL) return false;

    RomFile *file = RomFile::makeWithBuffer(rom, config.romSize);
    return file && file->writeToFile(path);
}

bool
Memory::saveWom(const char *path)
{
    if (wom == NULL) return false;
    
    RomFile *file = RomFile::makeWithBuffer(wom, config.womSize);
    return file && file->writeToFile(path);
}

bool
Memory::saveExt(const char *path)
{
    if (ext == NULL) return false;

    RomFile *file = RomFile::makeWithBuffer(ext, config.extSize);
    return file && file->writeToFile(path);
}

void
Memory::updateMemSrcTable()
{
    MemorySource mem_rom = rom ? MEM_ROM : MEM_NONE_FAST;
    MemorySource mem_wom = wom ? MEM_WOM : mem_rom;

    int chipRamPages = hasChipRam() ? 32 : 0;
    int slowRamPages = config.slowSize / 0x10000;
    int fastRamPages = config.fastSize / 0x10000;
    int extRomPages = hasExt() ? 8 : 0;
    
    assert(config.chipSize % 0x10000 == 0);
    assert(config.slowSize % 0x10000 == 0);
    assert(config.fastSize % 0x10000 == 0);

    bool ovl = ciaa.getPA() & 1;
        
    // Start from scratch
    for (unsigned i = 0x00; i <= 0xFF; i++)
        memSrc[i] = MEM_NONE_FAST;

    // Chip Ram
    for (unsigned i = 0x00; i <= 0x1F; i++)
        memSrc[i] = i < chipRamPages ? MEM_CHIP : MEM_NONE_SLOW;
    
    // Fast Ram
    for (unsigned i = 0x20; i <= 0x9F; i++)
        memSrc[i] = (i - 0x20) < fastRamPages ? MEM_FAST : MEM_NONE_FAST;

    // CIA range
    for (unsigned i = 0xA0; i <= 0xBF; i++)
        memSrc[i] = MEM_CIA;

    // Slow Ram
    for (unsigned i = 0xC0; i <= 0xD7; i++)
        memSrc[i] = (i - 0xC0) < slowRamPages ? MEM_SLOW : MEM_CUSTOM;
    for (unsigned i = 0xD8; i <= 0xDB; i++)
        memSrc[i] = (i - 0xCF) < slowRamPages ? MEM_SLOW : MEM_NONE_SLOW;

    // Real-time clock
    for (unsigned i = 0xDC; i <= 0xDC; i++)
        memSrc[i] = rtc.getModel() != RTC_NONE ? MEM_RTC : MEM_CUSTOM;

    // Reserved
    memSrc[0xDD] = MEM_NONE_FAST;

    // Custom chip set
    for (unsigned i = 0xDE; i <= 0xDF; i++)
        memSrc[i] = MEM_CUSTOM;

    // Reserved
    for (unsigned i = 0xE0; i <= 0xE7; i++)
        memSrc[i] = MEM_NONE_FAST;
    
    // Auto-config (Zorro II)
    for (unsigned i = 0xE8; i <= 0xEF; i++)
        memSrc[i] = hasFastRam() ? MEM_AUTOCONF : MEM_NONE_FAST;
    
    // Extended Rom
    for (unsigned i = 0; i < extRomPages; i++)
        memSrc[config.extStart + i] = MEM_EXT;

    // Kickstart Wom or Kickstart Rom
    for (unsigned i = 0xF8; i <= 0xFF; i++)
        memSrc[i] = mem_wom;

    // Blend in Boot Rom if a writeable Wom is present
    if (hasWom() && !womIsLocked) {
        for (unsigned i = 0xF8; i <= 0xFB; i++)
            memSrc[i] = mem_rom;
    }

    // Blend in Rom in lower memory area if the overlay line (OVL) is high
    if (ovl) {
        for (unsigned i = 0; i < 8 && memSrc[0xF8 + i] != MEM_NONE_FAST; i++)
            memSrc[i] = memSrc[0xF8 + i];
    }

    amiga.putMessage(MSG_MEM_LAYOUT);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_NONE_FAST> (u32 addr)
{
    debug(MEM_DEBUG, "peek8(%x [NONE_FAST]) = %x\n", addr, dataBus);
    return (u8)dataBus;
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_NONE_SLOW> (u32 addr)
{
    agnus.executeUntilBusIsFree();
    
    debug(MEM_DEBUG, "peek8(%x [NONE_SLOW]) = %x\n", addr, dataBus);
    return (u8)dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_NONE_FAST> (u32 addr)
{
    debug(MEM_DEBUG, "peek16(%x [NONE_FAST]) = %x\n", addr, dataBus);
    return dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_NONE_SLOW> (u32 addr)
{
    agnus.executeUntilBusIsFree();
    
    debug(MEM_DEBUG, "peek16(%x [NONE_SLOW]) = %x\n", addr, dataBus);
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_NONE_FAST> (u32 addr)
{
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_NONE_SLOW> (u32 addr)
{
    return dataBus;
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_CHIP> (u32 addr)
{
    ASSERT_CHIP_ADDR(addr);
    agnus.executeUntilBusIsFree();
    
    stats.chipReads.raw++;
    dataBus = READ_CHIP_8(addr);
    return dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_CHIP> (u32 addr)
{
    ASSERT_CHIP_ADDR(addr);
    agnus.executeUntilBusIsFree();
    
    stats.chipReads.raw++;
    dataBus = READ_CHIP_16(addr);
    return dataBus;
}

template<> u16
Memory::peek16 <AGNUS_ACCESS, MEM_CHIP> (u32 addr)
{
    ASSERT_CHIP_ADDR(addr);
    
    dataBus = READ_CHIP_16(addr);
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_CHIP> (u32 addr)
{
    return READ_CHIP_16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_SLOW> (u32 addr)
{
    ASSERT_SLOW_ADDR(addr);
    agnus.executeUntilBusIsFree();
    
    stats.slowReads.raw++;
    dataBus = READ_SLOW_8(addr);
    return dataBus;
}
    
template<> u16
Memory::peek16 <CPU_ACCESS, MEM_SLOW> (u32 addr)
{
    ASSERT_SLOW_ADDR(addr);
    agnus.executeUntilBusIsFree();
    
    stats.slowReads.raw++;
    dataBus = READ_SLOW_16(addr);
    return dataBus;
}

template<> u16
Memory::peek16 <AGNUS_ACCESS, MEM_SLOW> (u32 addr)
{
    ASSERT_SLOW_ADDR(addr);
    
    dataBus = READ_SLOW_16(addr);
    return dataBus;
}

template<> u16
Memory::peek16 <AGNUS_ACCESS, MEM_NONE_SLOW> (u32 addr)
{
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_SLOW> (u32 addr)
{
    return READ_SLOW_16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_FAST> (u32 addr)
{
    ASSERT_FAST_ADDR(addr);
    
    stats.fastReads.raw++;
    return READ_FAST_8(addr);
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_FAST> (u32 addr)
{
    ASSERT_FAST_ADDR(addr);
    
    stats.fastReads.raw++;
    return READ_FAST_16(addr);
}

template<> u16
Memory::spypeek16 <MEM_FAST> (u32 addr)
{
    return READ_FAST_16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_CIA> (u32 addr)
{
    ASSERT_CIA_ADDR(addr);
    
    agnus.executeUntilBusIsFreeForCIA();

    dataBus = peekCIA8(addr);
    return dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_CIA> (u32 addr)
{
    ASSERT_CIA_ADDR(addr);
    debug(XFILES, "XFILES (CIA): Reading a WORD from %x\n", addr);

    agnus.executeUntilBusIsFreeForCIA();
    
    dataBus = peekCIA16(addr);
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_CIA> (u32 addr)
{
    return spypeekCIA16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_RTC> (u32 addr)
{
    ASSERT_RTC_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    dataBus = peekRTC8(addr);
    return dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_RTC> (u32 addr)
{
    ASSERT_RTC_ADDR(addr);
    
    agnus.executeUntilBusIsFree();

    dataBus = peekRTC16(addr);
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_RTC> (u32 addr)
{
    return spypeekRTC16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_CUSTOM> (u32 addr)
{
    ASSERT_CUSTOM_ADDR(addr);
            
    agnus.executeUntilBusIsFree();

    if (IS_EVEN(addr)) {
        dataBus = HI_BYTE(peekCustom16(addr));
    } else {
        dataBus = LO_BYTE(peekCustom16(addr & 0x1FE));
    }
    return dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_CUSTOM> (u32 addr)
{
    ASSERT_CUSTOM_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    dataBus = peekCustom16(addr);
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_CUSTOM> (u32 addr)
{
    return spypeekCustom16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_AUTOCONF> (u32 addr)
{
    ASSERT_AUTO_ADDR(addr);
    
    // agnus.executeUntilBusIsFree();
    
    dataBus = zorro.peekFastRamDevice(addr) << 4;         
    return dataBus;
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_AUTOCONF> (u32 addr)
{
    ASSERT_AUTO_ADDR(addr);
    
    // agnus.executeUntilBusIsFree();
    
    u8 hi = zorro.peekFastRamDevice(addr) << 4;
    u8 lo = zorro.peekFastRamDevice(addr + 1) << 4;
    
    dataBus = HI_LO(hi,lo);
    return dataBus;
}

template<> u16
Memory::spypeek16 <MEM_AUTOCONF> (u32 addr)
{
    u8 hi = zorro.spypeekFastRamDevice(addr) << 4;
    u8 lo = zorro.spypeekFastRamDevice(addr + 1) << 4;
    
    return HI_LO(hi,lo);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_ROM> (u32 addr)
{
    ASSERT_ROM_ADDR(addr);
    
    stats.kickReads.raw++;
    return READ_ROM_8(addr);
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_ROM> (u32 addr)
{
    ASSERT_ROM_ADDR(addr);
    
    stats.kickReads.raw++;
    return READ_ROM_16(addr);
}

template<> u16
Memory::spypeek16 <MEM_ROM> (u32 addr)
{
    return READ_ROM_16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_WOM> (u32 addr)
{
    ASSERT_WOM_ADDR(addr);
    
    stats.kickReads.raw++;
    return READ_WOM_8(addr);
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_WOM> (u32 addr)
{
    ASSERT_WOM_ADDR(addr);
    
    stats.kickReads.raw++;
    return READ_WOM_16(addr);
}

template<> u16
Memory::spypeek16 <MEM_WOM> (u32 addr)
{
    return READ_WOM_16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS, MEM_EXT> (u32 addr)
{
    ASSERT_EXT_ADDR(addr);
    
    stats.kickReads.raw++;
    return READ_EXT_8(addr);
}

template<> u16
Memory::peek16 <CPU_ACCESS, MEM_EXT> (u32 addr)
{
    ASSERT_EXT_ADDR(addr);
    
    stats.kickReads.raw++;
    return READ_EXT_16(addr);
}

template<> u16
Memory::spypeek16 <MEM_EXT> (u32 addr)
{
    return READ_EXT_16(addr);
}

template<> u8
Memory::peek8 <CPU_ACCESS> (u32 addr)
{    
    switch (memSrc[(addr & 0xFFFFFF) >> 16]) {
            
        case MEM_NONE_FAST: return peek8 <CPU_ACCESS, MEM_NONE_FAST> (addr);
        case MEM_NONE_SLOW: return peek8 <CPU_ACCESS, MEM_NONE_SLOW> (addr);
        case MEM_CHIP:      return peek8 <CPU_ACCESS, MEM_CHIP>      (addr);
        case MEM_SLOW:      return peek8 <CPU_ACCESS, MEM_SLOW>      (addr);
        case MEM_FAST:      return peek8 <CPU_ACCESS, MEM_FAST>      (addr);
        case MEM_CIA:       return peek8 <CPU_ACCESS, MEM_CIA>       (addr);
        case MEM_RTC:       return peek8 <CPU_ACCESS, MEM_RTC>       (addr);
        case MEM_CUSTOM:    return peek8 <CPU_ACCESS, MEM_CUSTOM>    (addr);
        case MEM_AUTOCONF:  return peek8 <CPU_ACCESS, MEM_AUTOCONF>  (addr);
        case MEM_ROM:       return peek8 <CPU_ACCESS, MEM_ROM>       (addr);
        case MEM_WOM:       return peek8 <CPU_ACCESS, MEM_WOM>       (addr);
        case MEM_EXT:       return peek8 <CPU_ACCESS, MEM_EXT>       (addr);
            
        default: assert(false); return 0;
    }
}

template<> u16
Memory::peek16 <CPU_ACCESS> (u32 addr)
{
    assert(IS_EVEN(addr));
    
    switch (memSrc[(addr & 0xFFFFFF) >> 16]) {
            
        case MEM_NONE_FAST: return peek16 <CPU_ACCESS, MEM_NONE_FAST> (addr);
        case MEM_NONE_SLOW: return peek16 <CPU_ACCESS, MEM_NONE_SLOW> (addr);
        case MEM_CHIP:      return peek16 <CPU_ACCESS, MEM_CHIP>      (addr);
        case MEM_SLOW:      return peek16 <CPU_ACCESS, MEM_SLOW>      (addr);
        case MEM_FAST:      return peek16 <CPU_ACCESS, MEM_FAST>      (addr);
        case MEM_CIA:       return peek16 <CPU_ACCESS, MEM_CIA>       (addr);
        case MEM_RTC:       return peek16 <CPU_ACCESS, MEM_RTC>       (addr);
        case MEM_CUSTOM:    return peek16 <CPU_ACCESS, MEM_CUSTOM>    (addr);
        case MEM_AUTOCONF:  return peek16 <CPU_ACCESS, MEM_AUTOCONF>  (addr);
        case MEM_ROM:       return peek16 <CPU_ACCESS, MEM_ROM>       (addr);
        case MEM_WOM:       return peek16 <CPU_ACCESS, MEM_WOM>       (addr);
        case MEM_EXT:       return peek16 <CPU_ACCESS, MEM_EXT>       (addr);
            
        default: assert(false); return 0;
    }
}

u16
Memory::spypeek16 (u32 addr)
{
    assert(IS_EVEN(addr));
    
    switch (memSrc[(addr & 0xFFFFFF) >> 16]) {
            
        case MEM_NONE_FAST: return spypeek16 <MEM_NONE_FAST> (addr);
        case MEM_NONE_SLOW: return spypeek16 <MEM_NONE_SLOW> (addr);
        case MEM_CHIP:      return spypeek16 <MEM_CHIP>      (addr);
        case MEM_SLOW:      return spypeek16 <MEM_SLOW>      (addr);
        case MEM_FAST:      return spypeek16 <MEM_FAST>      (addr);
        case MEM_CIA:       return spypeek16 <MEM_CIA>       (addr);
        case MEM_RTC:       return spypeek16 <MEM_RTC>       (addr);
        case MEM_CUSTOM:    return spypeek16 <MEM_CUSTOM>    (addr);
        case MEM_AUTOCONF:  return spypeek16 <MEM_AUTOCONF>  (addr);
        case MEM_ROM:       return spypeek16 <MEM_ROM>       (addr);
        case MEM_WOM:       return spypeek16 <MEM_WOM>       (addr);
        case MEM_EXT:       return spypeek16 <MEM_EXT>       (addr);
            
        default: assert(false); return 0;
    }
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_NONE_FAST> (u32 addr, u8 value)
{
    debug(MEM_DEBUG, "poke8(%x [NONE_FAST], %x)\n", addr, value);
    dataBus = value;
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_NONE_SLOW> (u32 addr, u8 value)
{
    debug(MEM_DEBUG, "poke8(%x [NONE_SLOW], %x)\n", addr, value);
    agnus.executeUntilBusIsFree();
    dataBus = value;
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_NONE_FAST> (u32 addr, u16 value)
{
    debug(MEM_DEBUG, "poke16(%x [NONE_FAST], %x)\n", addr, value);
    dataBus = value;
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_NONE_SLOW> (u32 addr, u16 value)
{
    debug(MEM_DEBUG, "poke16(%x [NONE_SLOW], %x)\n", addr, value);
    agnus.executeUntilBusIsFree();
    dataBus = value;
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_CHIP> (u32 addr, u8 value)
{
    ASSERT_CHIP_ADDR(addr);
    
    if (BLT_GUARD && blitter.memguard[addr & mem.chipMask]) {
        debug("CPU(8) OVERWRITES BLITTER AT ADDR %x\n", addr);
    }

    agnus.executeUntilBusIsFree();
    
    stats.chipWrites.raw++;
    dataBus = value;
    WRITE_CHIP_8(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_CHIP> (u32 addr, u16 value)
{
    ASSERT_CHIP_ADDR(addr);
    
    if (BLT_GUARD && blitter.memguard[addr & mem.chipMask]) {
        debug("CPU OVERWRITES BLITTER AT ADDR %x\n", addr);
    }
    
    agnus.executeUntilBusIsFree();
    
    stats.chipWrites.raw++;
    dataBus = value;
    WRITE_CHIP_16(addr, value);
}

template <> void
Memory::poke16 <AGNUS_ACCESS, MEM_CHIP> (u32 addr, u16 value)
{
    ASSERT_CHIP_ADDR(addr);
 
    if (BLT_GUARD && blitter.memguard[addr & mem.chipMask]) {
        debug("AGNUS OVERWRITES BLITTER AT ADDR %x\n", addr);
    }

    dataBus = value;
    WRITE_CHIP_16(addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_SLOW> (u32 addr, u8 value)
{
    ASSERT_SLOW_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    stats.slowWrites.raw++;
    dataBus = value;
    WRITE_SLOW_8(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_SLOW> (u32 addr, u16 value)
{
    ASSERT_SLOW_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    stats.slowWrites.raw++;
    dataBus = value;
    WRITE_SLOW_16(addr, value);
}

template <> void
Memory::poke16 <AGNUS_ACCESS, MEM_SLOW> (u32 addr, u16 value)
{
    ASSERT_SLOW_ADDR(addr);
    
    dataBus = value;
    WRITE_SLOW_16(addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_FAST> (u32 addr, u8 value)
{
    ASSERT_FAST_ADDR(addr);
    
    stats.fastWrites.raw++;
    WRITE_FAST_8(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_FAST> (u32 addr, u16 value)
{
    ASSERT_FAST_ADDR(addr);
    
    stats.fastWrites.raw++;
    WRITE_FAST_16(addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_CIA> (u32 addr, u8 value)
{
    ASSERT_CIA_ADDR(addr);

    agnus.executeUntilBusIsFreeForCIA();
    
    dataBus = value;
    pokeCIA8(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_CIA> (u32 addr, u16 value)
{
    ASSERT_CIA_ADDR(addr);
    debug(XFILES, "XFILES (CIA): Writing a WORD into %x\n", addr);

    agnus.executeUntilBusIsFreeForCIA();
    
    dataBus = value;
    pokeCIA16(addr, value);
}
    
template <> void
Memory::poke8 <CPU_ACCESS, MEM_RTC> (u32 addr, u8 value)
{
    ASSERT_RTC_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    dataBus = value;
    pokeRTC8(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_RTC> (u32 addr, u16 value)
{
    ASSERT_RTC_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    dataBus = value;
    pokeRTC16(addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_CUSTOM> (u32 addr, u8 value)
{
    ASSERT_CUSTOM_ADDR(addr);
    
    agnus.executeUntilBusIsFree();
    
    dataBus = value;
    // http://eab.abime.net/showthread.php?p=1156399
    pokeCustom16<CPU_ACCESS>(addr & 0x1FE, HI_LO(value, value));
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_CUSTOM> (u32 addr, u16 value)
{
    ASSERT_CUSTOM_ADDR(addr);

    agnus.executeUntilBusIsFree();

    dataBus = value;
    pokeCustom16<CPU_ACCESS>(addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_AUTOCONF> (u32 addr, u8 value)
{
    ASSERT_AUTO_ADDR(addr);
    
    // agnus.executeUntilBusIsFree();
    
    dataBus = value;
    zorro.pokeFastRamDevice(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_AUTOCONF> (u32 addr, u16 value)
{
    ASSERT_AUTO_ADDR(addr);
    
    // agnus.executeUntilBusIsFree();

    dataBus = value;
    zorro.pokeFastRamDevice(addr, HI_BYTE(value));
    zorro.pokeFastRamDevice(addr + 1, LO_BYTE(value));
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_ROM> (u32 addr, u8 value)
{
    ASSERT_ROM_ADDR(addr);
    
    stats.kickWrites.raw++;
    
    // On Amigas with a WOM, writing into ROM space locks the WOM
    if (hasWom() && !womIsLocked) {
        debug("Locking WOM\n");
        womIsLocked = true;
        updateMemSrcTable();
    }
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_ROM> (u32 addr, u16 value)
{
    poke8 <CPU_ACCESS, MEM_ROM> (addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_WOM> (u32 addr, u8 value)
{
    ASSERT_WOM_ADDR(addr);
    
    stats.kickWrites.raw++;
    if (!womIsLocked) WRITE_WOM_8(addr, value);
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_WOM> (u32 addr, u16 value)
{
    ASSERT_WOM_ADDR(addr);

    stats.kickWrites.raw++;
    if (!womIsLocked) WRITE_WOM_16(addr, value);
}

template <> void
Memory::poke8 <CPU_ACCESS, MEM_EXT> (u32 addr, u8 value)
{
    ASSERT_EXT_ADDR(addr);
    stats.kickWrites.raw++;
}

template <> void
Memory::poke16 <CPU_ACCESS, MEM_EXT> (u32 addr, u16 value)
{
    ASSERT_EXT_ADDR(addr);
    stats.kickWrites.raw++;
}

template<> void
Memory::poke8 <CPU_ACCESS> (u32 addr, u8 value)
{
    switch (memSrc[(addr & 0xFFFFFF) >> 16]) {
            
        case MEM_NONE_FAST: poke8 <CPU_ACCESS, MEM_NONE_FAST> (addr, value); return;
        case MEM_NONE_SLOW: poke8 <CPU_ACCESS, MEM_NONE_SLOW> (addr, value); return;
        case MEM_CHIP:      poke8 <CPU_ACCESS, MEM_CHIP>      (addr, value); return;
        case MEM_SLOW:      poke8 <CPU_ACCESS, MEM_SLOW>      (addr, value); return;
        case MEM_FAST:      poke8 <CPU_ACCESS, MEM_FAST>      (addr, value); return;
        case MEM_CIA:       poke8 <CPU_ACCESS, MEM_CIA>       (addr, value); return;
        case MEM_RTC:       poke8 <CPU_ACCESS, MEM_RTC>       (addr, value); return;
        case MEM_CUSTOM:    poke8 <CPU_ACCESS, MEM_CUSTOM>    (addr, value); return;
        case MEM_AUTOCONF:  poke8 <CPU_ACCESS, MEM_AUTOCONF>  (addr, value); return;
        case MEM_ROM:       poke8 <CPU_ACCESS, MEM_ROM>       (addr, value); return;
        case MEM_WOM:       poke8 <CPU_ACCESS, MEM_WOM>       (addr, value); return;
        case MEM_EXT:       poke8 <CPU_ACCESS, MEM_EXT>       (addr, value); return;
            
        default: assert(false);
    }
}

template<> void
Memory::poke16 <CPU_ACCESS> (u32 addr, u16 value)
{
    assert(IS_EVEN(addr));
    
    switch (memSrc[(addr & 0xFFFFFF) >> 16]) {
            
        case MEM_NONE_FAST: poke16 <CPU_ACCESS, MEM_NONE_FAST> (addr, value); return;
        case MEM_NONE_SLOW: poke16 <CPU_ACCESS, MEM_NONE_SLOW> (addr, value); return;
        case MEM_CHIP:      poke16 <CPU_ACCESS, MEM_CHIP>      (addr, value); return;
        case MEM_SLOW:      poke16 <CPU_ACCESS, MEM_SLOW>      (addr, value); return;
        case MEM_FAST:      poke16 <CPU_ACCESS, MEM_FAST>      (addr, value); return;
        case MEM_CIA:       poke16 <CPU_ACCESS, MEM_CIA>       (addr, value); return;
        case MEM_RTC:       poke16 <CPU_ACCESS, MEM_RTC>       (addr, value); return;
        case MEM_CUSTOM:    poke16 <CPU_ACCESS, MEM_CUSTOM>    (addr, value); return;
        case MEM_AUTOCONF:  poke16 <CPU_ACCESS, MEM_AUTOCONF>  (addr, value); return;
        case MEM_ROM:       poke16 <CPU_ACCESS, MEM_ROM>       (addr, value); return;
        case MEM_WOM:       poke16 <CPU_ACCESS, MEM_WOM>       (addr, value); return;
        case MEM_EXT:       poke16 <CPU_ACCESS, MEM_EXT>       (addr, value); return;
            
        default: assert(false);
    }
}

u8
Memory::peekCIA8(u32 addr)
{
    // debug("peekCIA8(%6X)\n", addr);
    
    u32 reg = (addr >> 8)  & 0b1111;
    u32 sel = (addr >> 12) & 0b11;
    bool a0 = addr & 1;
    
    switch (sel) {
            
        case 0b00:
            return a0 ? ciaa.peek(reg) : ciab.peek(reg);
            
        case 0b01:
            return a0 ? LO_BYTE(cpu.getIRD()) : ciab.peek(reg);
            
        case 0b10:
            return a0 ? ciaa.peek(reg) : HI_BYTE(cpu.getIRD());
            
        case 0b11:
            return a0 ? LO_BYTE(cpu.getIRD()) : HI_BYTE(cpu.getIRD());
    }
    assert(false);
    return 0;
}

u16
Memory::peekCIA16(u32 addr)
{
    // debug(CIA_DEBUG, "peekCIA16(%6X)\n", addr);
    
    u32 reg = (addr >> 8)  & 0b1111;
    u32 sel = (addr >> 12) & 0b11;
    
    switch (sel) {
            
        case 0b00:
            return HI_LO(ciab.peek(reg), ciaa.peek(reg));
            
        case 0b01:
            return HI_LO(ciab.peek(reg), 0xFF);
            
        case 0b10:
            return HI_LO(0xFF, ciaa.peek(reg));
            
        case 0b11:
            return cpu.getIRD();
            
    }
    assert(false);
    return 0;
}

u8
Memory::spypeekCIA8(u32 addr)
{
    u32 reg = (addr >> 8)  & 0b1111;
    u32 sel = (addr >> 12) & 0b11;
    bool a0 = addr & 1;
    
    switch (sel) {
            
        case 0b00:
            return a0 ? ciaa.spypeek(reg) : ciab.spypeek(reg);
            
        case 0b01:
            return a0 ? LO_BYTE(cpu.getIRD()) : ciab.spypeek(reg);
            
        case 0b10:
            return a0 ? ciaa.spypeek(reg) : HI_BYTE(cpu.getIRD());
            
        case 0b11:
            return a0 ? LO_BYTE(cpu.getIRD()) : HI_BYTE(cpu.getIRD());
    }
    assert(false);
    return 0;
}

u16
Memory::spypeekCIA16(u32 addr)
{
    u32 reg = (addr >> 8) & 0b1111;
    u32 sel = (addr >> 12) & 0b11;
    
    switch (sel) {
            
        case 0b00:
            return HI_LO(ciab.spypeek(reg), ciaa.spypeek(reg));
            
        case 0b01:
            return HI_LO(ciab.spypeek(reg), 0xFF);
            
        case 0b10:
            return HI_LO(0xFF, ciaa.spypeek(reg));
            
        case 0b11:
            return cpu.getIRD();
            
    }
    assert(false);
    return 0;
}

void
Memory::pokeCIA8(u32 addr, u8 value)
{
    // debug(CIA_DEBUG, "pokeCIA8(%6X, %X)\n", addr, value);
    
    u32 reg = (addr >> 8) & 0b1111;
    u32 selA = (addr & 0x1000) == 0;
    u32 selB = (addr & 0x2000) == 0;

    if (selA) ciaa.poke(reg, value);
    if (selB) ciab.poke(reg, value);
}

void
Memory::pokeCIA16(u32 addr, u16 value)
{
    // debug(CIA_DEBUG, "pokeCIA16(%6X, %X)\n", addr, value);
    
    assert(IS_EVEN(addr));
    
    u32 reg = (addr >> 8) & 0b1111;
    u32 selA = (addr & 0x1000) == 0;
    u32 selB = (addr & 0x2000) == 0;
    
    if (selA) ciaa.poke(reg, LO_BYTE(value));
    if (selB) ciab.poke(reg, HI_BYTE(value));
}

u8
Memory::peekRTC8(u32 addr)
{
    /* Addr: 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011
     * Reg:   --   -0   --   -0   --   -1   --   -1   --   -2   --   -2
     */
    if (IS_EVEN(addr)) return HI_BYTE(dataBus);
    
    /* Addr: 0001 0011 0101 0111 1001 1011
     * Reg:   -0   -0   -1   -1   -2   -2
     */
    return rtc.peek((addr >> 2) & 0b1111);
}

u16
Memory::peekRTC16(u32 addr)
{
    return HI_LO(peekRTC8(addr), peekRTC8(addr + 1));
}

void
Memory::pokeRTC8(u32 addr, u8 value)
{
    /* Addr: 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011
     * Reg:   --   -0   --   -0   --   -1   --   -1   --   -2   --   -2
     */
    if (IS_EVEN(addr)) return;
    
    /* Addr: 0001 0011 0101 0111 1001 1011
     * Reg:   -0   -0   -1   -1   -2   -2
     */
    rtc.poke((addr >> 2) & 0b1111, value);
}

void
Memory::pokeRTC16(u32 addr, u16 value)
{
    pokeRTC8(addr, HI_BYTE(value));
    pokeRTC8(addr + 1, LO_BYTE(value));
}

u16
Memory::peekCustom16(u32 addr)
{
    u32 result;

    assert(IS_EVEN(addr));

    switch ((addr >> 1) & 0xFF) {
            
        case 0x000 >> 1: // BLTDDAT
            result = 0x00; break;
        case 0x002 >> 1: // DMACONR
            result = agnus.peekDMACONR(); break;
        case 0x004 >> 1: // VPOSR
            result = agnus.peekVPOSR(); break;
        case 0x006 >> 1: // VHPOSR
            result = agnus.peekVHPOSR(); break;
        case 0x008 >> 1: // DSKDATR
            result = paula.diskController.peekDSKDATR(); break;
        case 0x00A >> 1: // JOY0DAT
            result = denise.peekJOY0DATR(); break;
        case 0x00C >> 1: // JOY1DAT
            result = denise.peekJOY1DATR(); break;
        case 0x00E >> 1: // CLXDAT
            result = denise.peekCLXDAT(); break;
        case 0x010 >> 1: // ADKCONR
            result = paula.peekADKCONR(); break;
        case 0x012 >> 1: // POT0DAT
            result = paula.peekPOTxDAT<0>(); break;
        case 0x014 >> 1: // POT1DAT
            result = paula.peekPOTxDAT<1>(); break;
        case 0x016 >> 1: // POTGOR
            result = paula.peekPOTGOR(); break;
        case 0x018 >> 1: // SERDATR
            result = uart.peekSERDATR(); break;
        case 0x01A >> 1: // DSKBYTR
            result = diskController.peekDSKBYTR(); break;
        case 0x01C >> 1: // INTENAR
            result = paula.peekINTENAR(); break;
        case 0x01E >> 1: // INTREQR
            result = paula.peekINTREQR(); break;
        case 0x07C >> 1: // DENISEID
            result = denise.peekDENISEID(); break;
        default:
            result = peekCustomFaulty16(addr);

    }

    debug(OCSREG_DEBUG, "peekCustom16(%X [%s]) = %X\n", addr, regName(addr), result);

    dataBus = result;
    return result;
}

u16
Memory::peekCustomFaulty16(u32 addr)
{
    /* This functions is called when a write-only register or a
     * non-existing chipset register is read.
     *
     * Derived from the UAE source code documentation:
     *
     * Reading a write-only OCS register causes the last value of the
     * data bus to be written into this registers.
     *
     * Return values:
     *
     * - BLTDDAT (0x000) always returns the last data bus value.
     * - All other registers return
     *   - DMA cycle data (if DMA happened on the bus).
     *   - 0xFFFF or some some ANDed old data otherwise.
     */
    
    debug(INVREG_DEBUG, "Reading a non-readable chipset register\n");
    
    pokeCustom16<CPU_ACCESS>(addr, dataBus);
    
    return dataBus;
}

u16
Memory::spypeekCustom16(u32 addr)
{
    assert(IS_EVEN(addr));
    
    switch (addr & 0x1FE) {
            
    }

    return 42;
}

template <Accessor s> void
Memory::pokeCustom16(u32 addr, u16 value)
{

    if ((addr & 0xFFF) == 0x30) {
        debug(OCSREG_DEBUG, "pokeCustom16(SERDAT, '%c')\n", (char)value);
    } else {
        debug(OCSREG_DEBUG, "pokeCustom16(%X [%s], %X)\n", addr, regName(addr), value);
    }

    assert(IS_EVEN(addr));

    dataBus = value;

    switch ((addr >> 1) & 0xFF) {

        case 0x020 >> 1: // DSKPTH
            agnus.pokeDSKPTH(value); return;
        case 0x022 >> 1: // DSKPTL
            agnus.pokeDSKPTL(value); return;
        case 0x024 >> 1: // DSKLEN
            diskController.pokeDSKLEN(value); return;
        case 0x026 >> 1: // DSKDAT
            diskController.pokeDSKDAT(value); return;
        case 0x028 >> 1: // REFPTR
            return;
        case 0x02A >> 1: // VPOSW
            agnus.pokeVPOS(value); return;
        case 0x02C >> 1: // VHPOSW
            agnus.pokeVHPOS(value); return;
        case 0x02E >> 1: // COPCON
            copper.pokeCOPCON(value); return;
        case 0x030 >> 1: // SERDAT
            uart.pokeSERDAT(value); return;
        case 0x032 >> 1: // SERPER
            uart.pokeSERPER(value); return;
        case 0x034 >> 1: // POTGO
            paula.pokePOTGO(value); return;
        case 0x036 >> 1: // JOYTEST
            denise.pokeJOYTEST(value); return;
        case 0x038 >> 1: // STREQU
        case 0x03A >> 1: // STRVBL
        case 0x03C >> 1: // STRHOR
        case 0x03E >> 1: // STRLONG
            debug(XFILES, "XFILES (STROBE): %x\n", addr);
            return; // ignore
        case 0x040 >> 1: // BLTCON0
            blitter.pokeBLTCON0(value); return;
        case 0x042 >> 1: // BLTCON1
            blitter.pokeBLTCON1(value); return;
        case 0x044 >> 1: // BLTAFWM
            blitter.pokeBLTAFWM(value); return;
        case 0x046 >> 1: // BLTALWM
            blitter.pokeBLTALWM(value); return;
        case 0x048 >> 1: // BLTCPTH
            blitter.pokeBLTCPTH(value); return;
        case 0x04A >> 1: // BLTCPTL
            blitter.pokeBLTCPTL(value); return;
        case 0x04C >> 1: // BLTBPTH
            blitter.pokeBLTBPTH(value); return;
        case 0x04E >> 1: // BLTBPTL
            blitter.pokeBLTBPTL(value); return;
        case 0x050 >> 1: // BLTAPTH
            blitter.pokeBLTAPTH(value); return;
        case 0x052 >> 1: // BLTAPTL
            blitter.pokeBLTAPTL(value); return;
        case 0x054 >> 1: // BLTDPTH
            blitter.pokeBLTDPTH(value); return;
        case 0x056 >> 1: // BLTDPTL
            blitter.pokeBLTDPTL(value); return;
        case 0x058 >> 1: // BLTSIZE
            blitter.pokeBLTSIZE<s>(value); return;
        case 0x05A >> 1: // BLTCON0L (ECS)
            blitter.pokeBLTCON0L(value); return;
        case 0x05C >> 1: // BLTSIZV (ECS)
            blitter.pokeBLTSIZV(value); return;
        case 0x05E >> 1: // BLTSIZH (ECS)
            blitter.pokeBLTSIZH(value); return;
        case 0x060 >> 1: // BLTCMOD
            blitter.pokeBLTCMOD(value); return;
        case 0x062 >> 1: // BLTBMOD
            blitter.pokeBLTBMOD(value); return;
        case 0x064 >> 1: // BLTAMOD
            blitter.pokeBLTAMOD(value); return;
        case 0x066 >> 1: // BLTDMOD
            blitter.pokeBLTDMOD(value); return;
        case 0x068 >> 1: // unused
        case 0x06A >> 1: // unused
        case 0x06C >> 1: // unused
        case 0x06E >> 1: // unused
            break;
        case 0x070 >> 1: // BLTCDAT
            blitter.pokeBLTCDAT(value); return;
        case 0x072 >> 1: // BLTBDAT
            blitter.pokeBLTBDAT(value); return;
        case 0x074 >> 1: // BLTADAT
            blitter.pokeBLTADAT(value); return;
        case 0x076 >> 1: // unused
        case 0x078 >> 1: // unused
        case 0x07A >> 1: // unused
        case 0x07C >> 1: // unused
            break;
        case 0x07E >> 1: // DSKSYNC
            diskController.pokeDSKSYNC(value); return;
        case 0x080 >> 1: // COP1LCH
            copper.pokeCOP1LCH(value); return;
        case 0x082 >> 1: // COP1LCL
            copper.pokeCOP1LCL(value); return;
        case 0x084 >> 1: // COP2LCH
            copper.pokeCOP2LCH(value); return;
        case 0x086 >> 1: // COP2LCL
            copper.pokeCOP2LCL(value); return;
        case 0x088 >> 1: // COPJMP1
            copper.pokeCOPJMP1<s>(); return;
        case 0x08A >> 1: // COPJMP2
            copper.pokeCOPJMP2<s>(); return;
        case 0x08C >> 1: // COPINS
            copper.pokeCOPINS(value); return;
        case 0x08E >> 1: // DIWSTRT
            agnus.pokeDIWSTRT<s>(value); return;
        case 0x090 >> 1: // DIWSTOP
            agnus.pokeDIWSTOP<s>(value); return;
        case 0x092 >> 1: // DDFSTRT
            agnus.pokeDDFSTRT(value); return;
        case 0x094 >> 1: // DDFSTOP
            agnus.pokeDDFSTOP(value); return;
        case 0x096 >> 1: // DMACON
            agnus.pokeDMACON(value); return;
        case 0x098 >> 1:  // CLXCON
            denise.pokeCLXCON(value); return;
        case 0x09A >> 1: // INTENA
            paula.pokeINTENA(value); return;
        case 0x09C >> 1: // INTREQ
            paula.pokeINTREQ(value); return;
        case 0x09E >> 1: // ADKCON
            paula.pokeADKCON(value); return;
        case 0x0A0 >> 1: // AUD0LCH
            agnus.pokeAUDxLCH<0>(value); return;
        case 0x0A2 >> 1: // AUD0LCL
            agnus.pokeAUDxLCL<0>(value); return;
        case 0x0A4 >> 1: // AUD0LEN
            audioUnit.channel0.pokeAUDxLEN(value); return;
        case 0x0A6 >> 1: // AUD0PER
            audioUnit.channel0.pokeAUDxPER(value); return;
        case 0x0A8 >> 1: // AUD0VOL
            audioUnit.channel0.pokeAUDxVOL(value); return;
        case 0x0AA >> 1: // AUD0DAT
            audioUnit.channel0.pokeAUDxDAT(value); return;
        case 0x0AC >> 1: // Unused
        case 0x0AE >> 1: // Unused
            break;
        case 0x0B0 >> 1: // AUD1LCH
            agnus.pokeAUDxLCH<1>(value); return;
        case 0x0B2 >> 1: // AUD1LCL
            agnus.pokeAUDxLCL<1>(value); return;
        case 0x0B4 >> 1: // AUD1LEN
            audioUnit.channel1.pokeAUDxLEN(value); return;
        case 0x0B6 >> 1: // AUD1PER
            audioUnit.channel1.pokeAUDxPER(value); return;
        case 0x0B8 >> 1: // AUD1VOL
            audioUnit.channel1.pokeAUDxVOL(value); return;
        case 0x0BA >> 1: // AUD1DAT
            audioUnit.channel1.pokeAUDxDAT(value); return;
        case 0x0BC >> 1: // Unused
        case 0x0BE >> 1: // Unused
            break;
        case 0x0C0 >> 1: // AUD2LCH
            agnus.pokeAUDxLCH<2>(value); return;
        case 0x0C2 >> 1: // AUD2LCL
            agnus.pokeAUDxLCL<2>(value); return;
        case 0x0C4 >> 1: // AUD2LEN
            audioUnit.channel2.pokeAUDxLEN(value); return;
        case 0x0C6 >> 1: // AUD2PER
            audioUnit.channel2.pokeAUDxPER(value); return;
        case 0x0C8 >> 1: // AUD2VOL
            audioUnit.channel2.pokeAUDxVOL(value); return;
        case 0x0CA >> 1: // AUD2DAT
            audioUnit.channel2.pokeAUDxDAT(value); return;
        case 0x0CC >> 1: // Unused
        case 0x0CE >> 1: // Unused
            break;
        case 0x0D0 >> 1: // AUD3LCH
            agnus.pokeAUDxLCH<3>(value); return;
        case 0x0D2 >> 1: // AUD3LCL
            agnus.pokeAUDxLCL<3>(value); return;
        case 0x0D4 >> 1: // AUD3LEN
            audioUnit.channel3.pokeAUDxLEN(value); return;
        case 0x0D6 >> 1: // AUD3PER
            audioUnit.channel3.pokeAUDxPER(value); return;
        case 0x0D8 >> 1: // AUD3VOL
            audioUnit.channel3.pokeAUDxVOL(value); return;
        case 0x0DA >> 1: // AUD3DAT
            audioUnit.channel3.pokeAUDxDAT(value); return;
        case 0x0DC >> 1: // Unused
        case 0x0DE >> 1: // Unused
            break;
        case 0x0E0 >> 1: // BPL1PTH
            agnus.pokeBPLxPTH<1>(value); return;
        case 0x0E2 >> 1: // BPL1PTL
            agnus.pokeBPLxPTL<1>(value); return;
        case 0x0E4 >> 1: // BPL2PTH
            agnus.pokeBPLxPTH<2>(value); return;
        case 0x0E6 >> 1: // BPL2PTL
            agnus.pokeBPLxPTL<2>(value); return;
        case 0x0E8 >> 1: // BPL3PTH
            agnus.pokeBPLxPTH<3>(value); return;
        case 0x0EA >> 1: // BPL3PTL
            agnus.pokeBPLxPTL<3>(value); return;
        case 0x0EC >> 1: // BPL4PTH
            agnus.pokeBPLxPTH<4>(value); return;
        case 0x0EE >> 1: // BPL4PTL
            agnus.pokeBPLxPTL<4>(value); return;
        case 0x0F0 >> 1: // BPL5PTH
            agnus.pokeBPLxPTH<5>(value); return;
        case 0x0F2 >> 1: // BPL5PTL
            agnus.pokeBPLxPTL<5>(value); return;
        case 0x0F4 >> 1: // BPL6PTH
            agnus.pokeBPLxPTH<6>(value); return;
        case 0x0F6 >> 1: // BPL6PTL
            agnus.pokeBPLxPTL<6>(value); return;
        case 0x0F8 >> 1: // Unused
        case 0x0FA >> 1: // Unused
        case 0x0FC >> 1: // Unused
        case 0x0FE >> 1: // Unused
            break;
        case 0x100 >> 1: // BPLCON0
            agnus.pokeBPLCON0(value);
            denise.pokeBPLCON0(value);
            return;
        case 0x102 >> 1: // BPLCON1
            agnus.pokeBPLCON1(value);
            denise.pokeBPLCON1(value);
            return;
        case 0x104 >> 1: // BPLCON2
            denise.pokeBPLCON2(value); return;
        case 0x106 >> 1: // Unused
            break;
        case 0x108 >> 1: // BPL1MOD
            agnus.pokeBPL1MOD(value); return;
        case 0x10A >> 1: // BPL2MOD
            agnus.pokeBPL2MOD(value); return;
        case 0x10C >> 1: // Unused
        case 0x10E >> 1: // Unused
            break;
        case 0x110 >> 1: // BPL1DAT
            denise.pokeBPLxDAT<0>(value); return;
        case 0x112 >> 1: // BPL2DAT
            denise.pokeBPLxDAT<1>(value); return;
        case 0x114 >> 1: // BPL3DAT
            denise.pokeBPLxDAT<2>(value); return;
        case 0x116 >> 1: // BPL4DAT
            denise.pokeBPLxDAT<3>(value); return;
        case 0x118 >> 1: // BPL5DAT
            denise.pokeBPLxDAT<4>(value); return;
        case 0x11A >> 1: // BPL6DAT
            denise.pokeBPLxDAT<5>(value); return;
        case 0x11C >> 1: // Unused
        case 0x11E >> 1: // Unused
            break;
        case 0x120 >> 1: // SPR0PTH
            agnus.pokeSPRxPTH<0>(value); return;
        case 0x122 >> 1: // SPR0PTL
            agnus.pokeSPRxPTL<0>(value); return;
        case 0x124 >> 1: // SPR1PTH
            agnus.pokeSPRxPTH<1>(value); return;
        case 0x126 >> 1: // SPR1PTL
            agnus.pokeSPRxPTL<1>(value); return;
        case 0x128 >> 1: // SPR2PTH
            agnus.pokeSPRxPTH<2>(value); return;
        case 0x12A >> 1: // SPR2PTL
            agnus.pokeSPRxPTL<2>(value); return;
        case 0x12C >> 1: // SPR3PTH
            agnus.pokeSPRxPTH<3>(value); return;
        case 0x12E >> 1: // SPR3PTL
            agnus.pokeSPRxPTL<3>(value); return;
        case 0x130 >> 1: // SPR4PTH
            agnus.pokeSPRxPTH<4>(value); return;
        case 0x132 >> 1: // SPR4PTL
            agnus.pokeSPRxPTL<4>(value); return;
        case 0x134 >> 1: // SPR5PTH
            agnus.pokeSPRxPTH<5>(value); return;
        case 0x136 >> 1: // SPR5PTL
            agnus.pokeSPRxPTL<5>(value); return;
        case 0x138 >> 1: // SPR6PTH
            agnus.pokeSPRxPTH<6>(value); return;
        case 0x13A >> 1: // SPR6PTL
            agnus.pokeSPRxPTL<6>(value); return;
        case 0x13C >> 1: // SPR7PTH
            agnus.pokeSPRxPTH<7>(value); return;
        case 0x13E >> 1: // SPR7PTL
            agnus.pokeSPRxPTL<7>(value); return;
        case 0x140 >> 1: // SPR0POS
            agnus.pokeSPRxPOS<0>(value);
            denise.pokeSPRxPOS<0>(value);
            return;
        case 0x142 >> 1: // SPR0CTL
            agnus.pokeSPRxCTL<0>(value);
            denise.pokeSPRxCTL<0>(value);
            return;
        case 0x144 >> 1: // SPR0DATA
            denise.pokeSPRxDATA<0>(value); return;
        case 0x146 >> 1: // SPR0DATB
            denise.pokeSPRxDATB<0>(value); return;
        case 0x148 >> 1: // SPR1POS
            agnus.pokeSPRxPOS<1>(value);
            denise.pokeSPRxPOS<1>(value);
            return;
        case 0x14A >> 1: // SPR1CTL
            agnus.pokeSPRxCTL<1>(value);
            denise.pokeSPRxCTL<1>(value);
            return;
        case 0x14C >> 1: // SPR1DATA
            denise.pokeSPRxDATA<1>(value); return;
        case 0x14E >> 1: // SPR1DATB
            denise.pokeSPRxDATB<1>(value); return;
        case 0x150 >> 1: // SPR2POS
            agnus.pokeSPRxPOS<2>(value);
            denise.pokeSPRxPOS<2>(value);
            return;
        case 0x152 >> 1: // SPR2CTL
            agnus.pokeSPRxCTL<2>(value);
            denise.pokeSPRxCTL<2>(value);
            return;
        case 0x154 >> 1: // SPR2DATA
            denise.pokeSPRxDATA<2>(value); return;
        case 0x156 >> 1: // SPR2DATB
            denise.pokeSPRxDATB<2>(value); return;
        case 0x158 >> 1: // SPR3POS
            agnus.pokeSPRxPOS<3>(value);
            denise.pokeSPRxPOS<3>(value);
            return;
        case 0x15A >> 1: // SPR3CTL
            agnus.pokeSPRxCTL<3>(value);
            denise.pokeSPRxCTL<3>(value);
            return;
        case 0x15C >> 1: // SPR3DATA
            denise.pokeSPRxDATA<3>(value); return;
        case 0x15E >> 1: // SPR3DATB
            denise.pokeSPRxDATB<3>(value); return;
        case 0x160 >> 1: // SPR4POS
            agnus.pokeSPRxPOS<4>(value);
            denise.pokeSPRxPOS<4>(value);
            return;
        case 0x162 >> 1: // SPR4CTL
            agnus.pokeSPRxCTL<4>(value);
            denise.pokeSPRxCTL<4>(value);
            return;
        case 0x164 >> 1: // SPR4DATA
            denise.pokeSPRxDATA<4>(value); return;
        case 0x166 >> 1: // SPR4DATB
            denise.pokeSPRxDATB<4>(value); return;
        case 0x168 >> 1: // SPR5POS
            agnus.pokeSPRxPOS<5>(value);
            denise.pokeSPRxPOS<5>(value);
            return;
        case 0x16A >> 1: // SPR5CTL
            agnus.pokeSPRxCTL<5>(value);
            denise.pokeSPRxCTL<5>(value);
            return;
        case 0x16C >> 1: // SPR5DATA
            denise.pokeSPRxDATA<5>(value); return;
        case 0x16E >> 1: // SPR5DATB
            denise.pokeSPRxDATB<5>(value); return;
        case 0x170 >> 1: // SPR6POS
            agnus.pokeSPRxPOS<6>(value);
            denise.pokeSPRxPOS<6>(value);
            return;
        case 0x172 >> 1: // SPR6CTL
            agnus.pokeSPRxCTL<6>(value);
            denise.pokeSPRxCTL<6>(value);
            return;
        case 0x174 >> 1: // SPR6DATA
            denise.pokeSPRxDATA<6>(value); return;
        case 0x176 >> 1: // SPR6DATB
            denise.pokeSPRxDATB<6>(value); return;
        case 0x178 >> 1: // SPR7POS
            agnus.pokeSPRxPOS<7>(value);
            denise.pokeSPRxPOS<7>(value);
            return;
        case 0x17A >> 1: // SPR7CTL
            agnus.pokeSPRxCTL<7>(value);
            denise.pokeSPRxCTL<7>(value);
            return;
        case 0x17C >> 1: // SPR7DATA
            denise.pokeSPRxDATA<7>(value); return;
        case 0x17E >> 1: // SPR7DATB
            denise.pokeSPRxDATB<7>(value); return;
        case 0x180 >> 1: // COLOR00
            denise.pokeCOLORxx<s,0>(value); return;
        case 0x182 >> 1: // COLOR01
            denise.pokeCOLORxx<s,1>(value); return;
        case 0x184 >> 1: // COLOR02
            denise.pokeCOLORxx<s,2>(value); return;
        case 0x186 >> 1: // COLOR03
            denise.pokeCOLORxx<s,3>(value); return;
        case 0x188 >> 1: // COLOR04
            denise.pokeCOLORxx<s,4>(value); return;
        case 0x18A >> 1: // COLOR05
            denise.pokeCOLORxx<s,5>(value); return;
        case 0x18C >> 1: // COLOR06
            denise.pokeCOLORxx<s,6>(value); return;
        case 0x18E >> 1: // COLOR07
            denise.pokeCOLORxx<s,7>(value); return;
        case 0x190 >> 1: // COLOR08
            denise.pokeCOLORxx<s,8>(value); return;
        case 0x192 >> 1: // COLOR09
            denise.pokeCOLORxx<s,9>(value); return;
        case 0x194 >> 1: // COLOR10
            denise.pokeCOLORxx<s,10>(value); return;
        case 0x196 >> 1: // COLOR11
            denise.pokeCOLORxx<s,11>(value); return;
        case 0x198 >> 1: // COLOR12
            denise.pokeCOLORxx<s,12>(value); return;
        case 0x19A >> 1: // COLOR13
            denise.pokeCOLORxx<s,13>(value); return;
        case 0x19C >> 1: // COLOR14
            denise.pokeCOLORxx<s,14>(value); return;
        case 0x19E >> 1: // COLOR15
            denise.pokeCOLORxx<s,15>(value); return;
        case 0x1A0 >> 1: // COLOR16
            denise.pokeCOLORxx<s,16>(value); return;
        case 0x1A2 >> 1: // COLOR17
            denise.pokeCOLORxx<s,17>(value); return;
        case 0x1A4 >> 1: // COLOR18
            denise.pokeCOLORxx<s,18>(value); return;
        case 0x1A6 >> 1: // COLOR19
            denise.pokeCOLORxx<s,19>(value); return;
        case 0x1A8 >> 1: // COLOR20
            denise.pokeCOLORxx<s,20>(value); return;
        case 0x1AA >> 1: // COLOR21
            denise.pokeCOLORxx<s,21>(value); return;
        case 0x1AC >> 1: // COLOR22
            denise.pokeCOLORxx<s,22>(value); return;
        case 0x1AE >> 1: // COLOR23
            denise.pokeCOLORxx<s,23>(value); return;
        case 0x1B0 >> 1: // COLOR24
            denise.pokeCOLORxx<s,24>(value); return;
        case 0x1B2 >> 1: // COLOR25
            denise.pokeCOLORxx<s,25>(value); return;
        case 0x1B4 >> 1: // COLOR26
            denise.pokeCOLORxx<s,26>(value); return;
        case 0x1B6 >> 1: // COLOR27
            denise.pokeCOLORxx<s,27>(value); return;
        case 0x1B8 >> 1: // COLOR28
            denise.pokeCOLORxx<s,28>(value); return;
        case 0x1BA >> 1: // COLOR29
            denise.pokeCOLORxx<s,29>(value); return;
        case 0x1BC >> 1: // COLOR30
            denise.pokeCOLORxx<s,30>(value); return;
        case 0x1BE >> 1: // COLOR31
            denise.pokeCOLORxx<s,31>(value); return;
        case 0x1FE >> 1: // NO-OP (NULL)
            copper.pokeNOOP(value); return;
    }
    
    if (addr <= 0x1E) {
        debug(INVREG_DEBUG,
              "pokeCustom16(%X [%s]): READ-ONLY\n", addr, regName(addr));
    } else {
        debug(INVREG_DEBUG,
              "pokeCustom16(%X [%s]): NON-OCS\n", addr, regName(addr));
    }
}

const char *
Memory::ascii(u32 addr)
{
    for (unsigned i = 0; i < 16; i += 2) {
        u16 word = spypeek16(addr + i);
        str[i] = isprint(HI_BYTE(word)) ? HI_BYTE(word) : '.';
        str[i+1] = isprint(LO_BYTE(word)) ? LO_BYTE(word) : '.';
    }
    str[16] = 0;
    return str;
}

const char *
Memory::hex(u32 addr, size_t bytes)
{
    cpu.disassembleMemory(addr, bytes / 2, str);
    return str;
}

template void Memory::pokeCustom16<CPU_ACCESS>(u32 addr, u16 value);
template void Memory::pokeCustom16<AGNUS_ACCESS>(u32 addr, u16 value);
