// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _PAULA_H
#define _PAULA_H

#include "PaulaAudio.h"
#include "DiskController.h"
#include "UART.h"
#include "TimeDelayed.h"

class Paula : public AmigaComponent {
    
private:

    // Result of the latest inspection
    PaulaInfo info;
    
    
    //
    // Sub components
    //
    
public:

    // Sound circuitry
    PaulaAudio audioUnit = PaulaAudio(amiga);

    // Disk controller
    DiskController diskController = DiskController(amiga);

    // Universal Asynchronous Receiver Transmitter
    UART uart = UART(amiga);
    
    
    //
    // Counters
    //
    
    // Paula has been executed up to this clock cycle
    Cycle clock = 0;
    
    
    //
    // Interrupts
    //
    
    // The interrupt request register
    u16 intreq;
    
    // The interrupt enable register
    u16 intena;

    // Trigger cycle for setting a bit in INTREQ
    Cycle setIntreq[16];

    // Value pipe for emulating the four cycle delay on the IPL pins (DEPRECATED)
    u64 iplPipe;
    
    // Interrupt priority line (IPL)
    TimeDelayed <u8,4> ipl = TimeDelayed <u8,4> ();
    
    //
    // Control ports
    //
    
    // The pot control register
    u16 potgo;

    // Potentiometer counters for the first and the second control port
    u8 potCntX0;
    u8 potCntY0;
    u8 potCntX1;
    u8 potCntY1;

    // Current capacitor charge on all four potentiometer lines
    double chargeX0;
    double chargeY0;
    double chargeX1;
    double chargeY1;

    // The Audio and Disk Control Register
    u16 adkcon;

    
    //
    // Initializing
    //
    
public:

    Paula(Amiga& ref);
    
private:
    
    void _reset(bool hard) override;
    
    
    //
    // Analyzing
    //
    
public:
    
    PaulaInfo getInfo() { return HardwareComponent::getInfo(info); }
    
private:
    
    void _inspect() override;
    void _dump() override;
    
    
    //
    // Serializing
    //
    
private:
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
    }

    template <class T>
    void applyToHardResetItems(T& worker)
    {
        worker

        & clock;
    }

    template <class T>
    void applyToResetItems(T& worker)
    {
        worker
        
        & intreq
        & intena
        & setIntreq
        & iplPipe
        & potgo
        & potCntX0
        & potCntY0
        & potCntX1
        & potCntY1
        & chargeX0
        & chargeY0
        & chargeX1
        & chargeY1
        & adkcon;
    }

    size_t _size() override { COMPUTE_SNAPSHOT_SIZE }
    size_t _load(u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    size_t _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    
 
    //
    // Controlling
    //
    
private:
    
    void _setWarp(bool enable) override;


    //
    // Accessing registers
    //
    
public:
    
    // ADKCONR and ADKCON
    u16 peekADKCONR();
    void pokeADKCON(u16 value);

    bool UARTBRK() { return GET_BIT(adkcon, 11); }

    // INTREQR and INTREQ
    u16 peekINTREQR();
    template <Accessor s> void pokeINTREQ(u16 value);
    void setINTREQ(bool setclr, u16 value);
    void setINTREQ(u16 value) { setINTREQ(value & 0x8000, value & 0x7FFF); }

    // INTENAR and INTENA
    u16 peekINTENAR() { return intena; }
    template <Accessor s> void pokeINTENA(u16 value);
    void setINTENA(bool setclr, u16 value);
    void setINTENA(u16 value) { setINTENA(value & 0x8000, value & 0x7FFF); }

    // POTxDAT
    template <int x> u16 peekPOTxDAT();

    // POTGOR and POTGO
    u16 peekPOTGOR();
    bool OUTRY() { return potgo & 0x8000; }
    bool DATRY() { return potgo & 0x4000; }
    bool OUTRX() { return potgo & 0x2000; }
    bool DATRX() { return potgo & 0x1000; }
    bool OUTLY() { return potgo & 0x0800; }
    bool DATLY() { return potgo & 0x0400; }
    bool OUTLX() { return potgo & 0x0200; }
    bool DATLX() { return potgo & 0x0100; }
    void pokePOTGO(u16 value);


    //
    // Serving events
    //
    
public:

    // Triggers all pending interrupts
    void serviceIrqEvent();

    // Changes the CPU interrupt priority lines
    void serviceIplEvent();

    // Charges or discharges a potentiometer capacitor
    void servicePotEvent(EventID id);

    
    //
    // Managing interrupts
    //
    
public:
    
    // Schedules an interrupt
    void raiseIrq(IrqSource src);
    void scheduleIrqAbs(IrqSource src, Cycle trigger);
    void scheduleIrqRel(IrqSource src, Cycle trigger);

    // Checks intena and intreq and triggers an interrupt (if pending)
    void checkInterrupt();

private:
    
    // Computes the interrupt level of a pending interrupt.
    int interruptLevel();
};

#endif
