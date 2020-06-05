// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"

Drive::Drive(unsigned n, Amiga& ref) : nr(n), AmigaComponent(ref)
{
    assert(nr < 4);

    setDescription(nr == 0 ? "Df0" :
                   nr == 1 ? "Df1" :
                   nr == 2 ? "Df2" : "Df3");

    config.type = DRIVE_35_DD;
    config.speed = 1; 
}

void
Drive::_reset(bool hard)
{
    RESET_SNAPSHOT_ITEMS
}

void
Drive::_ping()
{
    amiga.putMessage(hasDisk() ?
                     MSG_DISK_INSERTED : MSG_DISK_EJECTED, nr);
    amiga.putMessage(hasWriteProtectedDisk() ?
                     MSG_DISK_PROTECTED : MSG_DISK_UNPROTECTED, nr);
    amiga.putMessage(hasModifiedDisk() ?
                     MSG_DISK_UNSAVED : MSG_DISK_SAVED, nr);
}

void
Drive::_inspect()
{
    pthread_mutex_lock(&lock);

    info.head = head;
    info.hasDisk = hasDisk();
    info.motor = motor();

    pthread_mutex_unlock(&lock);
}

void
Drive::_dumpConfig()
{
    msg("           Type: %s\n", driveTypeName(config.type));
    msg("          Speed: %d\n", config.speed);
    msg(" Original drive: %s\n", isOriginal() ? "yes" : "no");
    msg("    Turbo drive: %s\n", isTurbo() ? "yes" : "no");
}

void
Drive::_dump()
{
    msg("                Nr: %d\n", nr);
    msg("          Id count: %d\n", idCount);
    msg("            Id bit: %d\n", idBit);
    msg("      motorOnCycle: %s\n", motorOnCycle);
    msg("     motorOffCycle: %s\n", motorOffCycle);
    msg("           motor(): %s\n", motor() ? "on" : "off");
    msg(" motorSpeedingUp(): %s\n", motorSpeedingUp() ? "yes" : "no");
    msg("motorAtFullSpeed(): %s\n", motorAtFullSpeed() ? "yes" : "no");
    msg("motorSlowingDown(): %s\n", motorSlowingDown() ? "yes" : "no");
    msg("    motorStopped(): %s\n", motorStopped() ? "yes" : "no");
    msg("         dskchange: %d\n", dskchange);
    msg("            dsklen: %X\n", dsklen);
    msg("               prb: %X\n", prb);
    msg("              Side: %d\n", head.side);
    msg("         Cyclinder: %d\n", head.cylinder);
    msg("            Offset: %d\n", head.offset);
    msg("   cylinderHistory: %X\n", cylinderHistory);
    msg("              Disk: %s\n", disk ? "yes" : "no");
}

size_t
Drive::_size()
{
    SerCounter counter;

    applyToPersistentItems(counter);
    applyToResetItems(counter);

    // Add the size of the boolean indicating whether a disk is inserted
    counter.count += sizeof(bool);

    if (hasDisk()) {

        // Add the disk type and disk state
        counter & disk->getType();
        disk->applyToPersistentItems(counter);
    }

    return counter.count;
}

size_t
Drive::_load(u8 *buffer)
{
    SerReader reader(buffer);

    // Read own state
    applyToPersistentItems(reader);
    applyToResetItems(reader);

    // Delete the current disk
    if (disk) {
        delete disk;
        disk = NULL;
    }

    // Check if the snapshot includes a disk
    bool diskInSnapshot;
    reader & diskInSnapshot;

    // If yes, create recreate the disk
    if (diskInSnapshot) {
        DiskType diskType;
        reader & diskType;
        disk = Disk::makeWithReader(reader, diskType);
    }

    debug(SNP_DEBUG, "Recreated from %d bytes\n", reader.ptr - buffer);
    return reader.ptr - buffer;
}

size_t
Drive::_save(u8 *buffer)
{
    SerWriter writer(buffer);

    // Write own state
    applyToPersistentItems(writer);
    applyToResetItems(writer);

    // Indicate whether this drive has a disk is inserted
    writer & hasDisk();

    if (hasDisk()) {

        // Write the disk type
        writer & disk->getType();

        // Write the disk's state
        disk->applyToPersistentItems(writer);
    }

    debug(SNP_DEBUG, "Serialized to %d bytes\n", writer.ptr - buffer);
    return writer.ptr - buffer;
}

void
Drive::setType(DriveType t)
{
    assert(isDriveType(t));
    
    config.type = t;
 
    debug("Setting drive type to %s\n", driveTypeName(config.type));
}

void
Drive::setSpeed(i16 value)
{
    assert(isValidDriveSpeed(value));
    debug(DSK_DEBUG, "Setting acceleration factor to %d\n", value);

    amiga.suspend();
    config.speed = value;
    amiga.resume();
}

bool
Drive::idMode()
{
    return motorStopped() || motorSpeedingUp();
}

u32
Drive::getDriveId()
{
    /* External floopy drives identify themselve with the following codes:
     *
     *   3.5" DD :  0xFFFFFFFF
     *   3.5" HD :  0xAAAAAAAA  if an HD disk is inserted
     *              0xFFFFFFFF  if no disk or a DD disk is inserted
     *   5.25"SD :  0x55555555
     *
     * An unconnected drive corresponds to ID 0x00000000. The internal drive
     * does not identify itself. Its ID is also read as 0x00000000.
     */
    
    assert(config.type == DRIVE_35_DD);

    if (nr == 0) {
        return 0x00000000;
    } else {
        return 0xFFFFFFFF;
    }
}

u8
Drive::driveStatusFlags()
{
    u8 result = 0xFF;
    
    if (isSelected()) {
        
        // PA5: /DSKRDY
        if (idMode()) {
            if (idBit) result &= 0b11011111;
        } else if (hasDisk()) {
            if (motorAtFullSpeed() || motorSlowingDown()) result &= 0b11011111;
        }
        
        // PA4: /DSKTRACK0
        // debug("Head is at cyclinder %d\n", head.cylinder);
        if (head.cylinder == 0) { result &= 0b11101111; }
        
        // PA3: /DSKPROT
        if (!hasWriteEnabledDisk()) { result &= 0b11110111; }
        
        /* PA2: /DSKCHANGE
         * "Disk has been removed from the drive. The signal goes low whenever
         *  a disk is removed. It remains low until a disk is inserted AND a
         *  step pulse is received." [HRM]
         */
        if (!dskchange) result &= 0b11111011;
    }
    
    return result;
}

void
Drive::setMotor(bool value)
{
    bool oldValue = motor();
    
    if (!oldValue && value) {
        
        motorOnCycle = cpu.getMasterClock(); // TODO: Use agnus.clock

        debug(DSK_DEBUG, "Motor on (Cycle: %d)\n", motorOnCycle);

        amiga.putMessage(MSG_DRIVE_LED_ON, nr);
        amiga.putMessage(MSG_DRIVE_MOTOR_ON, nr);
    }
    
    if (oldValue && !value) {

        idCount = 0; // Reset identification shift register counter
        motorOffCycle = cpu.getMasterClock(); // TODO: Use agnus.clock

        debug(DSK_DEBUG, "Motor off (Cycle: %d)\n", motorOffCycle);

        amiga.putMessage(MSG_DRIVE_LED_OFF, nr);
        amiga.putMessage(MSG_DRIVE_MOTOR_OFF, nr);
    }
    
}

Cycle
Drive::motorOnTime()
{
    return motor() ? cpu.getMasterClock() - motorOnCycle : 0;
}

Cycle
Drive::motorOffTime()
{
    return motor() ? 0 : (cpu.getMasterClock() - motorOffCycle);
}

bool
Drive::motorSpeedingUp()
{
    return motor() && !motorAtFullSpeed();
}

bool
Drive::motorAtFullSpeed()
{
    Cycle delay = 380 * 28000; // 380 msec
    return emulateMechanics() ? (motorOnTime() > delay) : motor();
}

bool
Drive::motorSlowingDown()
{
    return !motor() && !motorStopped();
}

bool
Drive::motorStopped()
{
    Cycle delay = 80 * 28000; // 80 msec
    return emulateMechanics() ? (motorOffTime() > delay) : !motor();
}

void
Drive::selectSide(int side)
{
    assert(side < 2);
    if (head.side != side) debug("*** Select side %d\n", side);

    head.side = side;
}

u8
Drive::readHead()
{
    u8 result = 0xFF;
    
    if (disk) {
        result = disk->readByte(head.cylinder, head.side, head.offset);
    }
    rotate();

    return result;
}

u16
Drive::readHead16()
{
    u8 byte1 = readHead();
    u8 byte2 = readHead();
    
    return HI_LO(byte1, byte2);
}

void
Drive::writeHead(u8 value)
{
    if (disk) {
        disk->writeByte(value, head.cylinder, head.side, head.offset);
    }
    rotate();
}

void
Drive::writeHead16(u16 value)
{
    writeHead(HI_BYTE(value));
    writeHead(LO_BYTE(value));
}

void
Drive::rotate()
{
    if (++head.offset == disk->trackSize) {
        
        // Start over at the beginning of the current cyclinder
        head.offset = 0;

        // If this drive is selected, we emulate a falling edge on the flag pin
        // of CIA B. This causes the CIA to trigger the INDEX interrupt if the
        // corresponding enable bit is set.
        if (isSelected()) ciab.emulateFallingEdgeOnFlagPin();
    }

    assert(head.offset < Disk::trackSize);
}

void
Drive::findSyncMark()
{
    for (unsigned i = 0; i < disk->trackSize; i++) {
        
        if (readHead() != 0x44) continue;
        if (readHead() != 0x89) continue;
        break;
    }

    debug(DSK_DEBUG, "Moving to SYNC mark at offset %d\n", head.offset);
}

bool
Drive::readyToStep()
{
    Cycle delay = agnus.clock - stepCycle;
    return emulateMechanics() ? delay > 1060 : true;
}

void
Drive::moveHead(int dir)
{
    // Update disk change signal
    if (hasDisk()) dskchange = true;
 
    // Only proceed if the last head step was a while ago
    if (!readyToStep()) return;
    
    if (dir) {
        
        // Move drive head outwards (towards the lower tracks)
        if (head.cylinder > 0) {
            head.cylinder--;
            recordCylinder(head.cylinder);
        }
        if (DSK_CHECKSUM)
            plaindebug("Stepping down to cylinder %d\n", head.cylinder);

    } else {
        
        // Move drive head inwards (towards the upper tracks)
        if (head.cylinder < 83) {
            head.cylinder++;
            recordCylinder(head.cylinder);
        }
        if (DSK_CHECKSUM)
            plaindebug("Stepping up to cylinder %d\n", head.cylinder);
    }
    
    // Reset the head position in debug mode to generate reproducable results
    if (DRIVE_DEBUG) head.offset = 0;
    
    // Inform the GUI
    if (pollsForDisk()) {
        amiga.putMessage(MSG_DRIVE_HEAD_POLL, (nr << 8) | head.cylinder);
    } else {
        amiga.putMessage(MSG_DRIVE_HEAD, (nr << 8) | head.cylinder);
    }
    
    // Remember when we've performed the step
    stepCycle = agnus.clock;
}

void
Drive::recordCylinder(u8 cylinder)
{
    cylinderHistory = (cylinderHistory << 8) | cylinder;
}

bool
Drive::pollsForDisk()
{
    // Disk polling is only performed if no disk is inserted
    if (hasDisk()) return false;

    /* Head polling sequences of different Kickstart versions:
     *
     * Kickstart 1.2 and 1.3: 0-1-0-1-0-1-...
     * Kickstart 2.0:         0-1-2-3-2-1-...
     */
    static const u64 signature[] = {

        // Kickstart 1.2 and 1.3
        0x010001000100,
        0x000100010001,

        // Kickstart 2.0
        0x020302030203,
        0x030203020302,
    };

    u64 mask = 0xFFFFFFFF;
    for (unsigned i = 0; i < sizeof(signature) / 8; i++) {
        if ((cylinderHistory & mask) == (signature[i] & mask)) return true;
    }

    return false;
}

bool
Drive::hasWriteEnabledDisk()
{
    return hasDisk() ? !disk->isWriteProtected() : false;
}

bool
Drive::hasWriteProtectedDisk()
{
    return hasDisk() ? disk->isWriteProtected() : false;
}

void
Drive::setWriteProtection(bool value)
{
    if (disk) {
        
        if (value && !disk->isWriteProtected()) {
            
            disk->setWriteProtection(true);
            amiga.putMessage(MSG_DISK_PROTECTED);
        }
        if (!value && disk->isWriteProtected()) {
            
            disk->setWriteProtection(false);
            amiga.putMessage(MSG_DISK_UNPROTECTED);
        }
    }
}

void
Drive::toggleWriteProtection()
{
    if (hasDisk()) {
        disk->setWriteProtection(!disk->isWriteProtected());
    }
}

void
Drive::ejectDisk()
{
    debug(DSK_DEBUG, "ejectDisk()\n");

    if (disk) {
        
        // Flag disk change in the CIAA::PA
        dskchange = false;
        
        // Get rid of the disk
        delete disk;
        disk = NULL;
        
        // Notify the GUI
        amiga.putMessage(MSG_DISK_EJECT, nr);
    }
}

void
Drive::insertDisk(Disk *disk)
{
    debug(DSK_DEBUG, "insertDisk(%p)", disk);

    if (disk) {

        // Don't insert a disk if there is already one
        assert(!hasDisk());

        // Insert the disk and inform the GUI
        this->disk = disk;
        amiga.putMessage(MSG_DISK_INSERT, nr);
    }
}

u64
Drive::fnv()
{
    return disk ? disk->getFnv() : 0;
}

void
Drive::PRBdidChange(u8 oldValue, u8 newValue)
{
    // -----------------------------------------------------------------
    // | /MTR  | /SEL3 | /SEL2 | /SEL1 | /SEL0 | /SIDE |  DIR  | STEP  |
    // -----------------------------------------------------------------

    bool oldMtr = oldValue & 0x80;
    bool oldSel = oldValue & (0b1000 << nr);
    bool oldStep = oldValue & 0x01;

    bool newMtr = newValue & 0x80;
    bool newSel = newValue & (0b1000 << nr);
    bool newStep = newValue & 0x01;
    
    bool newDir = newValue & 0x02;
    
    // Store a copy of the new PRB value
    prb = newValue;
    
    //
    // Drive motor
    //

    // The motor state can only change on a falling edge on the select line
    if (FALLING_EDGE(oldSel, newSel)) {
        
        // Emulate the identification shift register
        idCount = (idCount + 1) % 32;
        idBit = GET_BIT(getDriveId(), 31 - idCount);
        
        // Drive motor logic from SAE / UAE
        if (!oldMtr || !newMtr) {
            switchMotorOn();
        } else if (oldMtr) {
            switchMotorOff();
        }
        
        // plaindebug("disk.select() sel df%d ($%08X) [$%08x, bit #%02d: %d]\n",
        //       nr, getDriveId(), getDriveId() << idCount, 31 - idCount, idBit);
    }
    
    //
    // Drive head
    //
    
    // Move head if STEP goes high and drive was selected
    if (RISING_EDGE(oldStep, newStep) && !oldSel) moveHead(newDir);
    
    // Evaluate the side selection bit
    if (head.side != !(newValue & 0b100)) {
        // debug("Switching to side %d\n", !(newValue & 0b100));
    }
    head.side = !(newValue & 0b100);
}
