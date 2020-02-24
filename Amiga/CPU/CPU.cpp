// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"

void
CPU::sync(int cycles)
{
    // Advance the CPU clock
    clock += cycles;

    // Emulate Agnus up to the same cycle
    agnus.executeUntil(CPU_CYCLES(clock));
}

u8
CPU::read8(u32 addr)
{
    return mem.peek8(addr);
}

u16
CPU::read16(u32 addr)
{
     return mem.peek16<BUS_CPU>(addr);
}

u16
CPU::read16Dasm(u32 addr)
{
    return mem.spypeek16(addr);
}

u16
CPU::read16OnReset(u32 addr)
{
    return mem.chip ? read16(addr) : 0;
}

void
CPU::write8(u32 addr, u8 val)
{
    mem.poke8(addr, val);
}

void
CPU::write16 (u32 addr, u16 val)
{
    mem.poke16<BUS_CPU>(addr, val);
}

void
CPU::irqOccurred(u8 level)
{
    // debug("**** INTERRUPT %d (intena = %x intreq = %x\n", level, paula.intena, paula.intreq);
}

void
CPU::breakpointReached(u32 addr)
{
    amiga.setControlFlags(RL_BREAKPOINT_REACHED);
}

void
CPU::watchpointReached(u32 addr)
{
    amiga.setControlFlags(RL_WATCHPOINT_REACHED);
}

CPU::CPU(Amiga& ref) : AmigaComponent(ref)
{
    setDescription("CPU");
}

void
CPU::_initialize()
{
    debug(CPU_DEBUG, "CPU::_initialize()\n");
}

void
CPU::_powerOn()
{
    debug(CPU_DEBUG, "CPU::_powerOn()\n");
}

void
CPU::_powerOff()
{

}

void
CPU::_run()
{
    debug(CPU_DEBUG, "CPU::_run()\n");
}

void
CPU::_reset()
{
    debug(CPU_DEBUG, "CPU::_reset()\n");

    RESET_SNAPSHOT_ITEMS

    // Reset the Moira core
    Moira::reset();

    // Remove all previously recorded instructions
    debugger.clearLog();
}

void
CPU::_inspect()
{
    uint32_t pc;

    // Prevent external access to variable 'info'
    pthread_mutex_lock(&lock);

    pc = getPC();

    // Registers
    info.pc = pc;

    for (int i = 0; i < 8; i++) {
        info.d[i] = getD(i);
        info.a[i] = getA(i);
    }
    info.usp = getUSP();
    info.ssp = getSSP();
    info.sr = getSR();

    // Disassemble the program starting at the program counter
    for (unsigned i = 0; i < CPUINFO_INSTR_COUNT; i++) {

        int bytes = disassemble(pc, info.instr[i].instr);
        disassemblePC(pc, info.instr[i].addr);
        disassembleMemory(pc, bytes / 2, info.instr[i].data);
        info.instr[i].sr[0] = 0;
        info.instr[i].bytes = bytes;
        pc += bytes;
    }

    // Disassemble the most recent entries in the trace buffer
    long count = debugger.loggedInstructions();
    for (int i = 0; i < count; i++) {

        moira::Registers r = debugger.logEntryAbs(i);
        disassemble(r.pc, info.loggedInstr[i].instr);
        disassemblePC(r.pc, info.loggedInstr[i].addr);
        disassembleSR(r.sr, info.loggedInstr[i].sr);
    }

    pthread_mutex_unlock(&lock);
}

void
CPU::_dumpConfig()
{
}

void
CPU::_dump()
{
    _inspect();
    
    msg("      PC: %8X\n", info.pc);
    msg(" D0 - D3: ");
    for (unsigned i = 0; i < 4; i++) msg("%8X ", info.d[i]);
    msg("\n");
    msg(" D4 - D7: ");
    for (unsigned i = 4; i < 8; i++) msg("%8X ", info.d[i]);
    msg("\n");
    msg(" A0 - A3: ");
    for (unsigned i = 0; i < 4; i++) msg("%8X ", info.a[i]);
    msg("\n");
    msg(" A4 - A7: ");
    for (unsigned i = 4; i < 8; i++) msg("%8X ", info.a[i]);
    msg("\n");
    msg("     SSP: %X\n", info.ssp);
    msg("   Flags: %X\n", info.sr);
}

CPUInfo
CPU::getInfo()
{
    CPUInfo result;
    
    pthread_mutex_lock(&lock);
    result = info;
    pthread_mutex_unlock(&lock);
    
    return result;
}

DisassembledInstr
CPU::getInstrInfo(long index)
{
    assert(index < CPUINFO_INSTR_COUNT);
    
    DisassembledInstr result;
    
    pthread_mutex_lock(&lock);
    result = info.instr[index];
    pthread_mutex_unlock(&lock);
    
    return result;
}

DisassembledInstr
CPU::getLoggedInstrInfo(long index)
{
    assert(index < CPUINFO_INSTR_COUNT);
    
    DisassembledInstr result;
    
    pthread_mutex_lock(&lock);
    result = info.loggedInstr[index];
    pthread_mutex_unlock(&lock);
    
    return result;
}

size_t
CPU::_size()
{
    SerCounter counter;

    applyToPersistentItems(counter);
    applyToResetItems(counter);

    return counter.count;
}

size_t
CPU::didLoadFromBuffer(uint8_t *buffer)
{
    SerReader reader(buffer);

    debug(SNP_DEBUG, "CPU state checksum: %x (%d bytes)\n",
          fnv_1a_64(buffer, reader.ptr - buffer), reader.ptr - buffer);

    return reader.ptr - buffer;
}

size_t
CPU::didSaveToBuffer(uint8_t *buffer)
{
    SerWriter writer(buffer);

    debug(SNP_DEBUG, "CPU state checksum: %x (%d bytes)\n",
          fnv_1a_64(buffer, writer.ptr - buffer), writer.ptr - buffer);

    return writer.ptr - buffer;
}