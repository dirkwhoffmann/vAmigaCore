// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "ThreadTypes.h"
#include "AmigaComponent.h"
#include "Chrono.h"
#include "Concurrency.h"

/* The Thread class manages the emulator thread that runs side by side to
 * the graphical user interface. The thread exists during the lifetime of
 * the emulator instance, but does not have to be active all the time. The
 * behavior of the thread is controlled by its internal state, which defines
 * the "emulator state". During the lifetime of the thread, four possible
 * states are distinguished:
 *
 *        Off: The emulator is turned off.
 *     Paused: The emulator is turned on, but not running.
 *    Running: The emulator is turned on and running.
 *     Halted: The emulator is shutting down.
 *     
 *
 *          -----------------------------------------------
 *         |                     run()                     |
 *         |                                               V
 *     ---------   powerOn()   ---------     run()     ---------
 *    |   Off   |------------>| Paused  |------------>| Running |
 *    |         |<------------|         |<------------|         |
 *     ---------   powerOff()  ---------    pause()    ---------
 *         ^                                               |
 *         |                   powerOff()                  |
 *          -----------------------------------------------
 *
 *     isPoweredOff()         isPaused()          isRunning()
 * |-------------------||-------------------||-------------------|
 *                      |----------------------------------------|
 *                                     isPoweredOn()
 *
 * State changes are triggered by the following functions:
 *
 * Command    | Current   | Next      | Actions on the delegate
 * ------------------------------------------------------------------------
 * powerOn()  | off       | paused    | _powerOn()
 *            | paused    | paused    | none
 *            | running   | running   | none
 * ------------------------------------------------------------------------
 * powerOff() | off       | off       | none
 *            | paused    | off       | _powerOff()
 *            | running   | off       | _powerOff() + _pause()
 * ------------------------------------------------------------------------
 * run()      | off       | running   | _powerOn() + _run()
 *            | paused    | running   | _run()
 *            | running   | running   | none
 * ------------------------------------------------------------------------
 * pause()    | off       | off       | none
 *            | paused    | paused    | none
 *            | running   | paused    | _pause()
 * ------------------------------------------------------------------------
 * halt()     | --        | halted    | _halt()
 *
 * When an instance of the Thread class has been created, a new thread is
 * started which executes the thread's main() function. This function executes
 * a loop which periodically calls function execute(). After each iteration,
 * the thread is put to sleep to synchronize timing. Two synchronization modes
 * are supported: Periodic or Pulsed. In periodic mode, the thread is put to
 * sleep for a certain amout of time and wakes up automatically. The second
 * mode puts the thread to sleep indefinitely and waits for an external signal
 * (a call to pulse()) to continue.
 *
 * To speed up emulation (e.g., during disk accesses), the emulator may be put
 * into warp mode. In this mode, timing synchronization is disabled causing the
 * emulator to run as fast as possible. The current warp mode setting can be
 * "locked" which means that it can't be changed any more. This lock is utilized
 * by the regression tester to prevent the GUI from disabling warp mode during
 * an ongoing test.
 *
 * Similar to warp mode, the emulator may be put into debug mode. This mode is
 * enabled when the GUI debugger is opend and disabled when the debugger is
 * closed. In debug mode, several time-consuming tasks are performed that are
 * usually left out. E.g., the CPU checks for breakpoints and records the
 * executed instruction in it's trace buffer.
 *
 * In many occasions, the thread needs to be paused temporarily (e.g., when the
 * GUI changes state). This can be done easily by embedding the code inside a
 * suspend / resume block like so:
 *
 *            suspend();
 *            do something with the internal state;
 *            resume();
 *
 *  It it safe to nest multiple suspend() / resume() blocks.
 */

class Thread : public AmigaComponent, util::Wakeable {
    
    friend class Amiga;
    
    // The thread object
    std::thread thread;

    // The current synchronization mode
    enum class SyncMode { Periodic, Pulsed };
    volatile SyncMode mode = SyncMode::Periodic;
    
    // The current and the next thread state
    volatile ExecutionState state = EXEC_OFF;
    volatile ExecutionState newState = EXEC_OFF;

    // The current and the next warp state
    volatile bool warpMode = false;
    volatile bool newWarpMode = false;

    // The current and the next warp state
    volatile bool debugMode = false;
    volatile bool newDebugMode = false;

    // Indicates if the warp mode or debug mode is locked
    bool warpLock = false;
    bool debugLock = false;
    
    // Variables needed to implement "periodic" mode
    util::Time delay = util::Time(1000000000 / 50);
    util::Time targetTime;
        
    // Counters
    isize loopCounter = 0;
    isize suspendCounter = 0;

    // The current CPU load (%)
    double cpuLoad = 0.0;
    
    // Clocks for measuring the CPU load
    util::Clock nonstopClock;
    util::Clock loadClock;

    
    //
    // Initializing
    //

public:
    
    Thread();
    ~Thread();
    
    const char *getDescription() const override { return "Thread"; }

    
    //
    // Executing
    //

private:
    
    template <SyncMode M> void execute();
    template <SyncMode M> void sleep();
    void main();

    // Returns true if this functions is called from within the emulator thread
    bool isEmulatorThread() { return std::this_thread::get_id() == thread.get_id(); }

    // Delegation functions
    virtual void execute() = 0;

    
    //
    // Configuring
    //

public:
    
    void setSyncDelay(util::Time newDelay);
    void setMode(SyncMode newMode);
    void setWarpLock(bool value);
    void setDebugLock(bool value);

    
    //
    // Analyzing
    //
    
    // Returns the current CPU load
    double getCpuLoad() { return cpuLoad; }
    
    
    //
    // Managing states
    //
    
public:
    
    bool isPoweredOn() const override { return state != EXEC_OFF; }
    bool isPoweredOff() const override { return state == EXEC_OFF; }
    bool isRunning() const override { return state == EXEC_RUNNING; }
    bool isPaused() const override { return state == EXEC_PAUSED; }

    bool isReady() throws { return AmigaComponent::isReady(); }
    void powerOn(bool blocking = true) throws;
    void powerOff(bool blocking = true);
    void run(bool blocking = true) throws;
    void pause(bool blocking = true);
    void halt(bool blocking = true);

    void suspend();
    void resume();
    
    bool inWarpMode() const { return warpMode; }
    void warpOn(bool blocking = true);
    void warpOff(bool blocking = true);

    bool inDebugMode() const { return debugMode; }
    void debugOn(bool blocking = true);
    void debugOff(bool blocking = true);

private:

    void changeStateTo(ExecutionState requestedState, bool blocking);
    void changeWarpTo(bool value, bool blocking);
    void changeDebugTo(bool value, bool blocking);
    
    
    //
    // Synchronizing
    //

public:
    
    // Awakes the thread if it runs in pulse mode
    void pulse();

private:
    
    // Resynchonizes a periodic thread that got out-of-sync
    void restartSyncTimer();

    // Wait until the thread has terminated
    void join() { if (thread.joinable()) thread.join(); }
};
