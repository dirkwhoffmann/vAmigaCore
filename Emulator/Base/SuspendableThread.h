// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Thread.h"

/* This class extends class Thread by a suspend-resume mechanism for pausing
 * the thread temporarily. This functionality is utilized frequently by the GUI
 * to carry out atomic state-change operations that cannot be performed while
 * the emulator is running. To pause the emulator temporarily, the critical
 * code section can be embedded in a suspend/resume block like so:
 *
 *        suspend();
 *        do something with the internal state;
 *        resume();
 *
 * It it safe to nest multiple suspend/resume blocks, but it is essential
 * that each call to suspend() is followed by a call to resume(). As a result,
 * the critical code section must not be exited in the middle, e.g., by
 * throwing an exception. It is therefore recommended to use the "suspended"
 * macro which is exit-safe. It is used in the following way:
 *
 *        suspended {
 *            do something with the internal state;
 *            return or throw an exceptions as you like;
 *        }
 */

class SuspendableThread : public Thread {
    
private:
    
    isize suspendCounter = 0;

public:

    void suspend() override;
    void resume() override;
};

class AutoResume {

    AmigaComponent *comp;
    
public:

    bool active = true;

    AutoResume(AmigaComponent *c) : comp(c) { comp->suspend(); }
    ~AutoResume() { comp->resume(); }
};

#define suspended \
for (AutoResume _ar(this); _ar.active; _ar.active = false)
