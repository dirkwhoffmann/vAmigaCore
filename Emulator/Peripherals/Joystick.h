// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "JoystickTypes.h"
#include "AmigaComponent.h"

class Joystick : public AmigaComponent {

    // Reference to control port this device belongs to
    ControlPort &port;

    // Current configuration
    JoystickConfig config;

    // Button state
    bool button = false;
    
    // Horizontal joystick position (-1 = left, 1 = right, 0 = released)
    int axisX = 0;
    
    // Vertical joystick position (-1 = up, 1 = down, 0 = released)
    int axisY = 0;
        
    // Bullet counter used in multi-fire mode
    i64 bulletCounter = 0;
    
    // Next frame to auto-press or auto-release the fire button
    i64 nextAutofireFrame = 0;
    
    
    //
    // Initializing
    //
    
public:
    
    Joystick(Amiga& ref, ControlPort& pref);

    const char *getDescription() const override;
    
private:
    
    void _initialize() override;
    void _reset(bool hard) override;

    
    //
    // Configuring
    //
    
public:

    const JoystickConfig &getConfig() const { return config; }

    i64 getConfigItem(Option option) const;
    bool setConfigItem(Option option, i64 value) override;
    bool setConfigItem(Option option, long id, i64 value) override;
    
    
    //
    // Analyzing
    //
    
private:
    
    void _dump(dump::Category category, std::ostream& os) const override;

    
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
    }

    template <class T>
    void applyToResetItems(T& worker)
    {
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    isize didLoadFromBuffer(const u8 *buffer) override;
    

    //
    // Using the device
    //
    
public:

    // Modifies the PRA bits of CIA A according to the current button state
    void changePra(u8 &pra) const;

    // Callback handler for function ControlPort::joydat()
    u16 joydat() const;

    // Callback handler for function ControlPort::ciapa()
    u8 ciapa() const;
    
    // Triggers a gamepad event
    void trigger(GamePadAction event);

    /* Execution function for this control port. This method needs to be
     * invoked at the end of each frame to make the auto-fire mechanism work.
     */
    void execute();
    
private:

    // Reloads the autofire magazine
    void reload();
    
    // Updates variable nextAutofireFrame
    void scheduleNextShot();
};
