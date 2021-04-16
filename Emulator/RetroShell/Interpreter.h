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
#include "Command.h"
#include "Exception.h"
#include "Error.h"
#include "Parser.h"

typedef std::list<string> Arguments;

enum class Token
{
    none,
    
    // Components
    agnus, amiga, audio, blitter, cia, controlport, copper, cpu, dc, denise,
    dfn, dmadebugger, keyboard, memory, monitor, mouse, paula, serial, rtc,

    // Commands
    about, audiate, autosync, clear, config, connect, debug, disable,
    disconnect, dsksync, easteregg, eject, enable, close, hide, insert, inspect,
    list, load, lock, off, on, open, pause, power, reset, run, screenshot, set,
    show, source,
    
    // Categories
    checksums, devices, events, registers, state,
        
    // Keys
    accuracy, bankmap, bitplanes, brightness, channel, chip, clxsprspr,
    clxsprplf, clxplfplf, color, contrast, defaultbb, defaultfs, device, disk,
    esync, extrom, extstart, fast, filter, joystick, keyset, mechanics, mode,
    model, opacity, palette, pan, poll, pullup, raminitpattern, refresh,
    revision, rom, sampling, saturation, searchpath, shakedetector, slow,
    slowramdelay, slowrammirror, speed, sprites, step, tod, todbug,
    unmappingtype, velocity, volume, wom
};

struct TooFewArgumentsError : public util::ParseError {
    using ParseError::ParseError;
};

struct TooManyArgumentsError : public util::ParseError {
    using ParseError::ParseError;
};

class Interpreter: AmigaComponent
{
    // The registered instruction set
    Command root;
    
    
    //
    // Initializing
    //

public:
    
    Interpreter(Amiga &ref);

    const char *getDescription() const override { return "Interpreter"; }

private:
    
    // Registers the instruction set
    void registerInstructions();

    void _reset(bool hard) override { }
    
    
    //
    // Serializing
    //

private:

    isize _size() override { return 0; }
    isize _load(const u8 *buffer) override {return 0; }
    isize _save(u8 *buffer) override { return 0; }

    
    //
    // Parsing input
    //
    
public:
    
    // Splits an input string into an argument list
    Arguments split(const string& userInput);

    // Auto-completes a command. Returns the number of auto-completed tokens
    void autoComplete(Arguments &argv);
    string autoComplete(const string& userInput);

    
    //
    // Executing commands
    //
    
public:
    
    // Executes a script file
    // void exec(std::istream &stream) throws;

    // Executes a single command
    void exec(const string& userInput, bool verbose = false) throws;
    void exec(Arguments &argv, bool verbose = false) throws;
            
    // Prints a usage string for a command
    void usage(Command &command);
    
    // Displays a help text for a (partially typed in) command
    void help(const string &userInput);
    void help(Arguments &argv);
    void help(Command &command);

};
