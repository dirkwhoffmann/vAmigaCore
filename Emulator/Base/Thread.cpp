// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Thread.h"
#include "Chrono.h"
#include <iostream>

Thread::Thread()
{
    restartSyncTimer();
    
    // Start the thread and enter the main function
    thread = std::thread(&Thread::main, this);
}

Thread::~Thread()
{
    // Wait until the thread has terminated
    join();
}

template <> void
Thread::execute <Thread::SyncMode::Periodic> ()
{
    // Call the execution function
    loadClock.go();
    execute();
    loadClock.stop();
}

template <> void
Thread::execute <Thread::SyncMode::Pulsed> ()
{
    // Call the execution function
    loadClock.go();
    execute();
    loadClock.stop();
    
}

template <> void
Thread::sleep <Thread::SyncMode::Periodic> ()
{
    auto now = util::Time::now();

    // Only proceed if we're not running in warp mode
    if (warpMode) return;
        
    // Check if we're running too slow...
    if (now > targetTime) {
        
        // Check if we're completely out of sync...
        if ((now - targetTime).asMilliseconds() > 200) {
            
            warn("Emulation is way too slow: %f\n",(now - targetTime).asSeconds());
            restartSyncTimer();
        }
    }
    
    // Check if we're running too fast...
    if (now < targetTime) {
        
        // Check if we're completely out of sync...
        if ((targetTime - now).asMilliseconds() > 200) {
            
            warn("Emulation is way too slow: %f\n",(targetTime - now).asSeconds());
            restartSyncTimer();
        }
    }
        
    // Sleep for a while
    // std::cout << "Sleeping... " << targetTime.asMilliseconds() << std::endl;
    // std::cout << "Delay = " << delay.asNanoseconds() << std::endl;
    targetTime += delay;
    targetTime.sleepUntil();
}

template <> void
Thread::sleep <Thread::SyncMode::Pulsed> ()
{
    // Wait for the next pulse
    if (!warpMode) waitForCondition();
}

void
Thread::main()
{
    debug(RUN_DEBUG, "main()\n");
          
    while (++loops) {
           
        if (isRunning()) {
                        
            switch (mode) {
                case SyncMode::Periodic: execute<SyncMode::Periodic>(); break;
                case SyncMode::Pulsed: execute<SyncMode::Pulsed>(); break;
            }
        }
        
        if (!warpMode || isPaused()) {

            switch (mode) {
                case SyncMode::Periodic: sleep<SyncMode::Periodic>(); break;
                case SyncMode::Pulsed: sleep<SyncMode::Pulsed>(); break;
            }
        }
        
        // Are we requested to enter or exit warp mode?
        while (newWarpMode != warpMode) {
            
            AmigaComponent::warpOnOff(newWarpMode);
            warpMode = newWarpMode;
            break;
        }

        // Are we requested to enter or exit warp mode?
        while (newDebugMode != debugMode) {
            
            AmigaComponent::debugOnOff(newDebugMode);
            debugMode = newDebugMode;
            break;
        }

        // Are we requested to change state?
        while (newState != state) {
            
            if (state == EXEC_OFF && newState == EXEC_PAUSED) {
                
                AmigaComponent::powerOn();
                state = newState;
                break;
            }

            if (state == EXEC_OFF && newState == EXEC_RUNNING) {
                
                AmigaComponent::powerOn();
                AmigaComponent::run();
                state = newState;
                break;
            }

            if (state == EXEC_PAUSED && newState == EXEC_OFF) {
                
                AmigaComponent::powerOff();
                state = newState;
                break;
            }

            if (state == EXEC_PAUSED && newState == EXEC_RUNNING) {
                
                AmigaComponent::run();
                state = newState;
                break;
            }

            if (state == EXEC_RUNNING && newState == EXEC_OFF) {
                
                AmigaComponent::pause();
                AmigaComponent::powerOff();
                state = newState;
                break;
            }

            if (state == EXEC_RUNNING && newState == EXEC_PAUSED) {
                
                AmigaComponent::pause();
                state = newState;
                break;
            }
            
            if (newState == EXEC_HALTED) {
                
                AmigaComponent::halt();
                state = newState;
                return;
            }
            
            // Invalid state transition
            assert(false);
            break;
        }
        
        // Compute the CPU load once in a while
        if (loops % 32 == 0) {
            
            auto used  = loadClock.getElapsedTime().asSeconds();
            auto total = nonstopClock.getElapsedTime().asSeconds();
            
            cpuLoad = used / total;
            
            loadClock.restart();
            loadClock.stop();
            nonstopClock.restart();
            
            // printf("CPU load = %f\n", cpuLoad);
        }
    }
}

void
Thread::setSyncDelay(util::Time newDelay)
{
    delay = newDelay;
}

void
Thread::setMode(SyncMode newMode)
{
    mode = newMode;
}

void
Thread::setWarpLock(bool value)
{
    warpLock = value;
}

void
Thread::setDebugLock(bool value)
{
    debugLock = value;
}

void
Thread::powerOn(bool blocking)
{
    debug(RUN_DEBUG, "powerOn()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (isPoweredOff() && isReady()) {
        
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_PAUSED, blocking);
    }
}

void
Thread::powerOff(bool blocking)
{
    debug(RUN_DEBUG, "powerOff()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (!isPoweredOff()) {
                
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_OFF, blocking);
    }
}

void
Thread::run(bool blocking)
{
    debug(RUN_DEBUG, "run()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (!isRunning() && isReady()) {
        
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_RUNNING, blocking);
    }
}

void
Thread::pause(bool blocking)
{
    debug(RUN_DEBUG, "pause()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (isRunning()) {
                
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_PAUSED, blocking);
    }
}

void
Thread::halt(bool blocking)
{
    changeStateTo(EXEC_HALTED, blocking);
}

void
Thread::suspend()
{
    debug(RUN_DEBUG, "Suspending (%zu)...\n", suspendCounter);
    
    if (suspendCounter || isRunning()) {
        pause();
        suspendCounter++;
    }
}

void
Thread::resume()
{
    debug(RUN_DEBUG, "Resuming (%zu)...\n", suspendCounter);
    
    if (suspendCounter && --suspendCounter == 0) {
        run();
    }
}

void
Thread::warpOn(bool blocking)
{
    if (!warpLock) changeWarpTo(true, blocking);
}

void
Thread::warpOff(bool blocking)
{
    if (!warpLock) changeWarpTo(false, blocking);
}

void
Thread::debugOn(bool blocking)
{
    if (!debugLock) changeDebugTo(true, blocking);
}

void
Thread::debugOff(bool blocking)
{
    if (!debugLock) changeDebugTo(false, blocking);
}

void
Thread::changeStateTo(ExecutionState requestedState, bool blocking)
{
    newState = requestedState;
    if (blocking) while (state != newState) { };
}

void
Thread::changeWarpTo(bool value, bool blocking)
{
    newWarpMode = value;
    if (blocking) while (warpMode != newWarpMode) { };
}

void
Thread::changeDebugTo(bool value, bool blocking)
{
    newDebugMode = value;
    if (blocking) while (debugMode != newDebugMode) { };
}

void
Thread::waitForCondition()
{
    std::unique_lock<std::mutex> lock(condMutex);
    condFlag = false;
    cond.wait_for(lock,
                  std::chrono::seconds(1000),
                  [this]() { return condFlag; } );
}

void
Thread::signalCondition()
{
    std::lock_guard<std::mutex> lock(condMutex);
    condFlag = true;
    cond.notify_one();
}

void
Thread::pulse()
{
    if (mode == SyncMode::Pulsed) {
        signalCondition();
    }
}

void
Thread::restartSyncTimer()
{
    targetTime = util::Time::now();
}
