// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "RetroShell.h"
#include "Amiga.h"
#include "Parser.h"

RetroShell::RetroShell(Amiga& ref) : SubComponent(ref), interpreter(ref)
{
    subComponents = std::vector<AmigaComponent *> { &remoteServer };
    
    // Initialize the text storage
    clear();

    // Initialize the input buffer
    history.push_back( { "", 0 } );
    
    // Print the startup message and the input prompt
    storage.welcome();
    *this << prompt;
}

RetroShell&
RetroShell::operator<<(char value)
{
    storage << value;
    remoteServer << value;
    return *this;
}

RetroShell&
RetroShell::operator<<(const string& value)
{
    storage << value;
    remoteServer << value;
    return *this;
}

RetroShell&
RetroShell::operator<<(int value)
{
    *this << std::to_string(value);
    return *this;
}

RetroShell&
RetroShell::operator<<(long value)
{
    *this << std::to_string(value);
    return *this;
}

RetroShell&
RetroShell::operator<<(std::stringstream &stream)
{
    string line;
    while(std::getline(stream, line)) {
        *this << line << '\n';
    }
    return *this;
}

const char *
RetroShell::text()
{
    static string all;
    
    // Add the storage contents
    storage.text(all);
        
    // Add the input line
    all += input + " ";
    
    return all.c_str();
}

void
RetroShell::tab(isize pos)
{
    auto count = pos - (isize)storage[storage.size() - 1].size();

    if (count > 0) {
        
        std::string fill(count, ' ');
        storage << fill;
        remoteServer << fill;
    }
}

void
RetroShell::clear()
{
    storage.clear();
}

void
RetroShell::printHelp()
{
    storage.printHelp();
    remoteServer.printHelp();
}

void
RetroShell::pressUp()
{
    assert(ipos >= 0 && ipos < historyLength());

    if (ipos > 0) {
        
        // Save the input line if it is currently shown
        if (ipos == historyLength() - 1) history.back() = { input, cursor };
        
        auto &item = history[--ipos];
        input = item.first;
        cursor = item.second;
        tabPressed = false;
    }
    
    assert(cursor >= 0 && cursor <= inputLength());
}

void
RetroShell::pressDown()
{
    assert(ipos >= 0 && ipos < historyLength());

    if (ipos < historyLength() - 1) {
        
        auto &item = history[++ipos];
        input = item.first;
        cursor = item.second;
        tabPressed = false;
    }
    assert(cursor >= 0 && cursor <= inputLength());
}

void
RetroShell::pressLeft()
{
    if (cursor > 0) cursor--;
    assert(cursor >= 0 && cursor <= inputLength());
    tabPressed = false;
}

void
RetroShell::pressRight()
{
    if (cursor < (isize)input.size()) cursor++;
    assert(cursor >= 0 && cursor <= inputLength());
    tabPressed = false;
}

void
RetroShell::pressHome()
{
    cursor = 0;
    tabPressed = false;
}

void
RetroShell::pressEnd()
{
    cursor = input.length();
    tabPressed = false;
}

void
RetroShell::pressTab()
{
    if (tabPressed) {
                
        // TAB was pressed twice
        help(input);

    } else {
        
        // Auto-complete the typed in command
        input = interpreter.autoComplete(input);
        cursor = input.length();
    }

    tabPressed = true;
}

void
RetroShell::pressBackspace()
{
    if (cursor > 0) {
        
        pressLeft();
        pressDelete();
    }
    tabPressed = false;
}

void
RetroShell::pressDelete()
{
    if (cursor < inputLength()) {
        input.erase(input.begin() + cursor);
    }
    tabPressed = false;
}

void
RetroShell::pressReturn()
{
    auto cmd = input;
    
    // Add the input line to the text storage
    *this << '\r' << prompt << input << '\n';

    // Clear the user input
    press('\r');
    
    // Execute the command
    execUserCommand(cmd);
}

void
RetroShell::press(char c)
{
    assert(c != '\n');
    
    switch (c) {
            
        case '\r':
            
            input = "";
            cursor = 0;
            break;
            
        default:
            
            if (isprint(c)) {
                
                if (cursor < inputLength()) {
                    input.insert(input.begin() + cursor, c);
                } else {
                    input += c;
                }
                cursor++;
            }
    }

    tabPressed = false;
}

void
RetroShell::press(const string &s)
{
    for (auto c : s) press(c);
}

isize
RetroShell::cursorRel()
{
    assert(cursor >= 0 && cursor <= inputLength());
    return cursor - input.length();
}

void
RetroShell::execUserCommand(const string &command)
{
    if (!command.empty()) {
        
        // Add the command to the history buffer
        history.back() = { command, (isize)command.size() };
        history.push_back( { "", 0 } );
        ipos = (isize)history.size() - 1;
        
        // Execute the command
        try { exec(command); } catch (...) { };
        
    } else {
        
        printHelp();
    }
    
    printPrompt();
}

void
RetroShell::exec(const string &command)
{
    // Skip comments
    if (command.substr(0,1) == "#") return;

    // Check if the command marked with 'try'
    bool ignoreError = command.rfind("try", 0) == 0;
    
    // Call the interpreter
    try {
        
        interpreter.exec(command);
    
    } catch (std::exception &err) {
        
        // Print error message
        describe(err);
        
        // Rethrow the exception
        if (!ignoreError) throw;
    }
}

void
RetroShell::execScript(std::ifstream &fs)
{
    script.str("");
    script << fs.rdbuf();
    scriptLine = 1;
    continueScript();
}

void
RetroShell::execScript(const string &contents)
{
    script.str("");
    script << contents;
    scriptLine = 1;
    continueScript();
}

void
RetroShell::continueScript()
{
    string command;
    while(std::getline(script, command)) {
            
        // Print the command
        *this << command << '\n';
        
        // Execute the command
        try {
            exec(command);
            
        } catch (ScriptInterruption &) {
            
            msgQueue.put(MSG_SCRIPT_PAUSE, scriptLine);
            return;
        
        } catch (std::exception &) {
            
            *this << "Aborted in line " << scriptLine << '\n';
            msgQueue.put(MSG_SCRIPT_ABORT, scriptLine);
            return;
        }

        scriptLine++;
    }
    
    msgQueue.put(MSG_SCRIPT_DONE, scriptLine);
}

void
RetroShell::describe(const std::exception &e)
{
    if (auto err = dynamic_cast<const TooFewArgumentsError *>(&e)) {
        
        *this << err->what() << ": Too few arguments";
        *this << '\n';
        return;
    }
        
    if (auto err = dynamic_cast<const TooManyArgumentsError *>(&e)) {
        
        *this << err->what() << ": Too many arguments";
        *this << '\n';
        return;
    }
    
    if (auto err = dynamic_cast<const util::EnumParseError *>(&e)) {
        
        *this << err->token << " is not a valid key" << '\n';
        *this << "Expected: " << err->expected << '\n';
        return;
    }
    
    if (auto err = dynamic_cast<const util::ParseNumError *>(&e)) {
        
        *this << err->token << " is not a number";
        *this << '\n';
        return;
    }
    
    if (auto err = dynamic_cast<const util::ParseBoolError *>(&e)) {

        *this << err->token << " must be true or false";
        *this << '\n';
        return;
    }
    
    if (auto err = dynamic_cast<const util::ParseError *>(&e)) {

        *this << err->what() << ": Syntax error";
        *this << '\n';
        return;
    }
    
    if (auto err = dynamic_cast<const VAError *>(&e)) {

        *this << err->what();
        *this << '\n';
        return;
    }
}

void
RetroShell::help(const string &command)
{
    interpreter.help(command);
}

void
RetroShell::dump(AmigaComponent &component, dump::Category category)
{
    std::stringstream ss; string line;
    
    suspended {
        component.dump(category, ss);
    }
    while(std::getline(ss, line)) *this << line << '\n';
}

void
RetroShell::vsyncHandler()
{
    if (agnus.clock >= wakeUp) {
        
        // Ask the external thread (GUI) to continue the script
        msgQueue.put(MSG_SCRIPT_WAKEUP);
        wakeUp = INT64_MAX;
    }
}
