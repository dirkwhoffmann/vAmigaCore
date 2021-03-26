// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "ControlPortTypes.h"
#include "AmigaComponent.hpp"
#include "Joystick.hpp"
#include "Mouse.hpp"

//
// Reflection APIs
//

struct PortNrEnum : util::Reflection<PortNrEnum, PortNr> {
    
    static bool isValid(long value)
    {
        return value == PORT_1 || value == PORT_2;
    }
    
    static const char *prefix() { return ""; }
    static const char *key(PortNr value)
    {
        switch (value) {
                
            case PORT_1:  return "PORT_1";
            case PORT_2:  return "PORT_2";
        }
        return "???";
    }
};

struct ControlPortDeviceEnum : util::Reflection<ControlPortDeviceEnum, ControlPortDevice> {
    
    static bool isValid(long value)
    {
        return (unsigned long)value <  CPD_COUNT;
    }
    
    static const char *prefix() { return "CPD"; }
    static const char *key(ControlPortDevice value)
    {
        switch (value) {
                
            case CPD_NONE:      return "NONE";
            case CPD_MOUSE:     return "MOUSE";
            case CPD_JOYSTICK:  return "JOYSTICK";
            case CPD_COUNT:     return "???";
        }
        return "???";
    }
};

//
// Class
//

class ControlPort : public AmigaComponent {

    friend class Mouse;
    friend class Joystick;
    
    // The represented control port
    PortNr nr;

    // The result of the latest inspection
    ControlPortInfo info;
    
    // The connected device
    ControlPortDevice device = CPD_NONE;
    
    // The two mouse position counters
    i64 mouseCounterX = 0;
    i64 mouseCounterY = 0;

    // Resistances on the potentiometer lines (specified as a delta charge)
    double chargeDX;
    double chargeDY;
    
    
    //
    // Sub components
    //

public:
    
    Mouse mouse = Mouse(amiga, *this);
    Joystick joystick = Joystick(amiga, *this);


    //
    // Initializing
    //
    
public:
    
    ControlPort(Amiga& ref, PortNr nr);

    const char *getDescription() const override;
    
private:
    
    void _reset(bool hard) override { RESET_SNAPSHOT_ITEMS(hard) }

    
    //
    // Configuring
    //

public:
    
    ControlPortInfo getInfo() { return HardwareComponent::getInfo(info); }

private:
    
    void _inspect() override;
    void _dump(Dump::Category category, std::ostream& os) const override;

    
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
        worker

        << mouseCounterX
        << mouseCounterY
        << chargeDX
        << chargeDY;
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }

    
    //
    // Accessing
    //

public:

    // Getter for the delta charges
    i16 getChargeDX() const { return (i16)chargeDX; }
    i16 getChargeDY() const { return (i16)chargeDY; }
    
    // Returns the control port bits showing up in the JOYxDAT register
    u16 joydat();

    // Emulates a write access to JOYTEST
    void pokeJOYTEST(u16 value);

    // Modifies the POTGOR bits according to the connected device
    void changePotgo(u16 &potgo) const;

    // Modifies the PRA bits of CIA A according to the connected device
    void changePra(u8 &pra) const;
};
