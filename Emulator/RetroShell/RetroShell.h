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

namespace va {

class RetroShell : public AmigaComponent {

    // Interpreter for commands typed into the debug console
    // Interpreter interpreter;
    
    //
    // Text storage
    //
    
    // The text storage
    std::vector<string> storage;
    string all;
    
    // The input history buffer
    std::vector<string> input;

    // Input prompt
    string prompt = "vAmiga\% ";
    
    // The current cursor position
    isize cpos = 0;

    // The minimum cursor position in this row
    isize cposMin = 0;
    
    // The currently active input string
    isize ipos = 0;

    // Indicates if TAB was the most recently pressed key
    bool tabPressed = false;
    
    bool isDirty = false;
    
    
    //
    // Initializing
    //
    
public:
    
    RetroShell(Amiga& ref);
        
    const char *getDescription() const override { return "RetroShell"; }

    void _reset(bool hard) override { }
    
    
    //
    // Serializing
    //

private:

    isize _size() override { return 0; }
    isize _load(const u8 *buffer) override {return 0; }
    isize _save(u8 *buffer) override { return 0; }

    
    //
    // Managing user input
    //

public:
    
    void pressUp();
    void pressDown();
    void pressLeft();
    void pressRight();
    void pressTab();
    void pressReturn();
    void pressKey(char c);


    //
    // Working with the text storage
    //

public:
    
    const char *text();
        
    // Prints a message
    RetroShell &operator<<(char value);
    RetroShell &operator<<(const string &value);
    RetroShell &operator<<(int value);
    RetroShell &operator<<(long value);

    // Returns the cursor position relative to the line end
    isize cposRel();

private:

    // Returns a reference to the last line in the text storage
    string &lastLine() { return storage.back(); }
                
    // Clears the console window
    void clear();
    
    // Prints a help line
    void printHelp();

    // Prints the input prompt
    void printPrompt();
    
    // Shortens the text storage if it grows too large
    void shorten();
    
    // Clears the current line
    void clearLine() { *this << '\r'; }

    // Moves the cursor forward to a certain column
    void tab(int hpos);

    // Replaces the last line
    void replace(const string &text, const string &prefix);
    void replace(const string &text) { replace(text, prompt); }    
};

}
