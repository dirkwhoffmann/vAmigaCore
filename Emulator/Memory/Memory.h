// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AmigaComponent.h"
#include "RomFile.h"
#include "ExtendedRomFile.h"

// DEPRECATED. TODO: GET VALUE FROM ZORRO CARD MANANGER
const u32 FAST_RAM_STRT = 0x200000;

// Verifies address ranges
#define ASSERT_CHIP_ADDR(x) \
assert(((x) % config.chipSize) == ((x) & chipMask));
#define ASSERT_FAST_ADDR(x) \
assert(((x) - FAST_RAM_STRT) < config.fastSize);
#define ASSERT_SLOW_ADDR(x) \
assert(((x) % config.slowSize) == ((x) & slowMask));
#define ASSERT_ROM_ADDR(x) \
assert(((x) % config.romSize) == ((x) & romMask));
#define ASSERT_WOM_ADDR(x) \
assert(((x) % config.womSize) == ((x) & womMask));
#define ASSERT_EXT_ADDR(x)  \
assert(((x) % config.extSize) == ((x) & extMask));
#define ASSERT_CIA_ADDR(x) \
assert((x) >= 0xA00000 && (x) <= 0xBFFFFF);
#define ASSERT_RTC_ADDR(x) \
assert((x) >= 0xD80000 && (x) <= 0xDCFFFF);
#define ASSERT_CUSTOM_ADDR(x) \
assert((x) >= 0xC00000 && (x) <= 0xDFFFFF);
#define ASSERT_AUTO_ADDR(x) \
assert((x) >= 0xE80000 && (x) <= 0xE8FFFF);

//
// Reading
//

// Reads a value from Chip RAM in big endian format
#define READ_CHIP_8(x)  R8BE_ALIGNED (chip + ((x) & chipMask))
#define READ_CHIP_16(x) R16BE_ALIGNED(chip + ((x) & chipMask))

// Reads a value from Fast RAM in big endian format
#define READ_FAST_8(x)  R8BE_ALIGNED (fast + ((x) - FAST_RAM_STRT))
#define READ_FAST_16(x) R16BE_ALIGNED(fast + ((x) - FAST_RAM_STRT))

// Reads a value from Slow RAM in big endian format
#define READ_SLOW_8(x)  R8BE_ALIGNED (slow + ((x) & slowMask))
#define READ_SLOW_16(x) R16BE_ALIGNED(slow + ((x) & slowMask))

// Reads a value from Boot ROM or Kickstart ROM in big endian format
#define READ_ROM_8(x)  R8BE_ALIGNED (rom + ((x) & romMask))
#define READ_ROM_16(x) R16BE_ALIGNED(rom + ((x) & romMask))

// Reads a value from Kickstart WOM in big endian format
#define READ_WOM_8(x)  R8BE_ALIGNED (wom + ((x) & womMask))
#define READ_WOM_16(x) R16BE_ALIGNED(wom + ((x) & womMask))

// Reads a value from Extended ROM in big endian format
#define READ_EXT_8(x)  R8BE_ALIGNED (ext + ((x) & extMask))
#define READ_EXT_16(x) R16BE_ALIGNED(ext + ((x) & extMask))

//
// Writing
//

// Writes a value into Chip RAM in big endian format
#define WRITE_CHIP_8(x,y)  W8BE_ALIGNED (chip + ((x) & chipMask), (y))
#define WRITE_CHIP_16(x,y) W16BE_ALIGNED(chip + ((x) & chipMask), (y))

// Writes a value into Fast RAM in big endian format
#define WRITE_FAST_8(x,y)  W8BE_ALIGNED (fast + ((x) - FAST_RAM_STRT), (y))
#define WRITE_FAST_16(x,y) W16BE_ALIGNED(fast + ((x) - FAST_RAM_STRT), (y))

// Writes a value into Slow RAM in big endian format
#define WRITE_SLOW_8(x,y)  W8BE_ALIGNED (slow + ((x) & slowMask), (y))
#define WRITE_SLOW_16(x,y) W16BE_ALIGNED(slow + ((x) & slowMask), (y))

// Writes a value into Kickstart WOM in big endian format
#define WRITE_WOM_8(x,y)  W8BE_ALIGNED (wom + ((x) & womMask), (y))
#define WRITE_WOM_16(x,y) W16BE_ALIGNED(wom + ((x) & womMask), (y))

// Writes a value into Extended ROM in big endian format
#define WRITE_EXT_8(x,y)  W8BE_ALIGNED (ext + ((x) & extMask), (y))
#define WRITE_EXT_16(x,y) W16BE_ALIGNED(ext + ((x) & extMask), (y))


class Memory : public AmigaComponent {

    // Current configuration
    MemoryConfig config;

    // Current workload
    MemoryStats stats;

public:

    /* About
     *
     * There are 6 types of dynamically allocated memory:
     *
     *     rom: Read-only memory
     *          Holds a Kickstart Rom or a Boot Rom (A1000).

     *     wom: Write-once Memory
     *          If rom holds a Boot Rom, a wom is automatically created. It
     *          is the place where the A1000 stores the Kickstart loaded
     *          from disk.
     *
     *     ext: Extended Rom
     *          Such a Rom was added to newer Amiga models when the 512 KB
     *          Kickstart Rom became too small. It is emulated to support
     *          the Aros Kickstart replacement.
     *
     *    chip: Chip Ram
     *          Holds the memory which is shared by the CPU and the Amiga Chip
     *          set. The original Agnus chip is able to address 512 KB Chip
     *          memory. Newer models are able to address up to 2 MB.
     *
     *    slow: Slow Ram (aka Bogo Ram)
     *          This Ram is addressed by the same bus as Chip Ram, but it can
     *          used by the CPU only.
     *
     *    fast: Fast Ram
     *          Only the CPU can access this Ram. It is connected via a
     *          seperate bus and doesn't slow down the Chip set when the CPU
     *          addresses it.
     *
     * Each memory type is represented by three variables:
     *
     *    A pointer to the allocates memory.
     *    A variable storing the memory size in bytes (in MemoryConfig).
     *    A bit mask to emulate address mirroring.
     *
     * The following invariants hold:
     *
     *    pointer == nullptr <=> config.size == 0 <=> mask == 0
     *    pointer != nullptr <=> mask == config.size - 1
     *
     */
    u8 *rom = nullptr;
    u8 *wom = nullptr;
    u8 *ext = nullptr;
    u8 *chip = nullptr;
    u8 *slow = nullptr;
    u8 *fast = nullptr;

    u32 romMask = 0;
    u32 womMask = 0;
    u32 extMask = 0;
    u32 chipMask = 0;
    u32 slowMask = 0;
    u32 fastMask = 0;

    /* Indicates if the Kickstart Wom is writable. If an Amiga 1000 Boot Rom is
     * installed, a Kickstart WOM (Write Once Memory) is added automatically.
     * On startup, the WOM is unlocked which means that it is writable. During
     * the boot process, the WOM gets locked.
     */
    bool womIsLocked = false;
    
    /* The Amiga memory is divided into 256 banks of size 64KB. The following
     * tables indicate which memory type is seen in each bank by the CPU and
     * Agnus, respectively.
     * See also: updateMemSrcTables()
     */
    MemorySource cpuMemSrc[256];
    MemorySource agnusMemSrc[256];

    // The last value on the data bus
    u16 dataBus;

    // Static buffer for returning textual representations
    char str[256];
    

    //
    // Initializing
    //
    
public:
    
    Memory(Amiga& ref);
    ~Memory();

    const char *getDescription() const override { return "Memory"; }
    
private:
    
    void dealloc();
    void _reset(bool hard) override;
    
    
    //
    // Configuring
    //
    
public:
    
    const MemoryConfig &getConfig() const { return config; }
    
    long getConfigItem(Option option) const;
    bool setConfigItem(Option option, long value) override;

private:
    
    void _dumpConfig() const override;

    
    //
    // Analyzing
    //
    
public:
    
    MemoryStats getStats() { return stats; }
    
    void clearStats() { memset(&stats, 0, sizeof(stats)); }
    void updateStats();

private:
    
    void _dump() const override;

    
    //
    // Serializing
    //
    
private:
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
        worker

        & romMask
        & womMask
        & extMask
        & chipMask
        & slowMask
        & fastMask

        & config.extStart;
    }

    template <class T>
    void applyToHardResetItems(T& worker)
    {
    }

    template <class T>
    void applyToResetItems(T& worker)
    {
        worker

        & womIsLocked
        & cpuMemSrc
        & dataBus;
    }

    size_t _size() override;
    size_t _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    size_t _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    size_t didLoadFromBuffer(const u8 *buffer) override;
    size_t didSaveToBuffer(u8 *buffer) const override;

    
    //
    // Controlling
    //
    
private:

    void _powerOn() override;

        
    //
    // Allocating memory
    //
    
private:
    
    /* Dynamically allocates Ram or Rom. As side effects, the memory table is
     * updated and the GUI is informed about the changed memory layout.
     */
    bool alloc(u64 bytes, u8 *&ptr, u64 &size, u32 &mask);

public:

    bool allocChip(u64 bytes) { return alloc(bytes, chip, config.chipSize, chipMask); }
    bool allocSlow(u64 bytes) { return alloc(bytes, slow, config.slowSize, slowMask); }
    bool allocFast(u64 bytes) { return alloc(bytes, fast, config.fastSize, fastMask); }

    void deleteChip() { allocChip(0); }
    void deleteSlow() { allocSlow(0); }
    void deleteFast() { allocFast(0); }

    bool allocRom(size_t bytes) { return alloc(bytes, rom, config.romSize, romMask); }
    bool allocWom(size_t bytes) { return alloc(bytes, wom, config.womSize, womMask); }
    bool allocExt(size_t bytes) { return alloc(bytes, ext, config.extSize, extMask); }

    void deleteRom() { allocRom(0); }
    void deleteWom() { allocWom(0); }
    void deleteExt() { allocExt(0); }


    //
    // Managing RAM
    //
    
public:

    // Check if a certain Ram is present
    bool hasChipRam() const { return chip != nullptr; }
    bool hasSlowRam() const { return slow != nullptr; }
    bool hasFastRam() const { return fast != nullptr; }

    // Returns the size of a certain Ram in bytes
    size_t chipRamSize() const { return config.chipSize; }
    size_t slowRamSize() const { return config.slowSize; }
    size_t fastRamSize() const { return config.fastSize; }
    size_t ramSize() const { return config.chipSize + config.slowSize + config.fastSize; }

private:
    
    void fillRamWithInitPattern();

    
    //
    // Managing ROM
    //

public:

    // Computes a CRC-32 checksum
    u32 romFingerprint() { return crc32(rom, config.romSize); }
    u32 extFingerprint() { return crc32(ext, config.extSize); }

    // Returns the ROM identifiers of the currently installed ROMs
    RomIdentifier romIdentifier() { return RomFile::identifier(romFingerprint()); }
    RomIdentifier extIdentifier() { return RomFile::identifier(extFingerprint()); }

    const char *romTitle() { return RomFile::title(romIdentifier()); }
    const char *romVersion();
    const char *romReleased()  { return RomFile::released(romIdentifier()); }

    const char *extTitle() { return RomFile::title(extIdentifier()); }
    const char *extVersion();
    const char *extReleased()  { return RomFile::released(extIdentifier()); }

    // Checks if a certain Rom is present
    bool hasRom() { return rom != nullptr; }
    bool hasBootRom() { return hasRom() && config.romSize <= KB(16); }
    bool hasKickRom() { return hasRom() && config.romSize >= KB(256); }
    bool hasArosRom() { return RomFile::isArosRom(romIdentifier()); }
    bool hasWom() { return wom != nullptr; }
    bool hasExt() { return ext != nullptr; }

    // Erases an installed Rom
    void eraseRom() { assert(rom); memset(rom, 0, config.romSize); }
    void eraseWom() { assert(wom); memset(wom, 0, config.womSize); }
    void eraseExt() { assert(ext); memset(ext, 0, config.extSize); }

    // Installs a Boot Rom or Kickstart Rom
    bool loadRom(RomFile *rom);
    bool loadRomFromFile(const char *path) throws;
    bool loadRomFromFile(const char *path, ErrorCode *ec);
    bool loadRomFromBuffer(const u8 *buf, size_t len) throws;
    bool loadRomFromBuffer(const u8 *buf, size_t len, ErrorCode *ec);

    bool loadExt(ExtendedRomFile *rom);
    bool loadExtFromFile(const char *path) throws;
    bool loadExtFromFile(const char *path, ErrorCode *ec);
    bool loadExtFromBuffer(const u8 *buf, size_t len) throws;
    bool loadExtFromBuffer(const u8 *buf, size_t len, ErrorCode *ec);

private:

    // Loads Rom data from a file
    // DEPRECATED: USE AnyAmigaFile::flash(...) instead
    void loadRom(AmigaFile *rom, u8 *target, size_t length);

public:
    
    // Saves a Rom to disk
    bool saveRom(const char *path);
    bool saveWom(const char *path);
    bool saveExt(const char *path);

    
    //
    // Maintaining the memory source table
    //
    
public:
        
    // Returns the memory source for a given address
    template <Accessor A> MemorySource getMemSrc(u32 addr);
    // MemorySource getMemSource(Accessor accessor, u32 addr);
    
    // Updates both memory source lookup tables
    void updateMemSrcTables();
    
private:

    void updateCpuMemSrcTable();
    void updateAgnusMemSrcTable();

    
    //
    // Accessing memory
    //
    
public:

    template <Accessor acc, MemorySource src> u8 peek8(u32 addr);
    template <Accessor acc, MemorySource src> u16 peek16(u32 addr);
    template <Accessor acc, MemorySource src> u16 spypeek16(u32 addr) const;
    template <Accessor acc> u8 peek8(u32 addr);
    template <Accessor acc> u16 peek16(u32 addr);
    template <Accessor acc> u16 spypeek16(u32 addr) const;

    template <Accessor acc, MemorySource src> void poke8(u32 addr, u8 value);
    template <Accessor acc, MemorySource src> void poke16(u32 addr, u16 value);
    template <Accessor acc> void poke8(u32 addr, u8 value);
    template <Accessor acc> void poke16(u32 addr, u16 value);
    

    //
    // Accessing the CIA space
    //
    
    u8 peekCIA8(u32 addr);
    u16 peekCIA16(u32 addr);
    
    u8 spypeekCIA8(u32 addr) const;
    u16 spypeekCIA16(u32 addr) const;
    
    void pokeCIA8(u32 addr, u8 value);
    void pokeCIA16(u32 addr, u16 value);


    //
    // Accessing the RTC space
    //
    
    u8 peekRTC8(u32 addr) const;
    u16 peekRTC16(u32 addr) const;
    
    // u8 spypeekRTC8(u32 addr) { return peekRTC8(addr); }
    // u16 spypeekRTC16(u32 addr) { return peekRTC16(addr); }

    void pokeRTC8(u32 addr, u8 value);
    void pokeRTC16(u32 addr, u16 value);

    
    //
    // Accessing the custom chip space
    //
    
    u16 peekCustom16(u32 addr);
    u16 peekCustomFaulty16(u32 addr);
    
    u16 spypeekCustom16(u32 addr) const;
 
    template <Accessor s> void pokeCustom16(u32 addr, u16 value);
    
    
    //
    // Debugging
    //
    
public:
    
    // Returns 16 bytes of memory as an ASCII string
    template <Accessor A> const char *ascii(u32 addr);
    
    // Returns a certain amount of bytes as a string containing hex words
    template <Accessor A> const char *hex(u32 addr, size_t bytes);
};
