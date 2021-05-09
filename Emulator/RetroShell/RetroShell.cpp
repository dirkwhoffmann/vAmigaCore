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

RetroShell::RetroShell(Amiga& ref) : AmigaComponent(ref), interpreter(ref)
{
    // Initialize the text storage
    storage.push_back("");

    // Initialize the input buffer
    input.push_back("");
    
    // Print a startup message
    *this << "vAmiga " << V_MAJOR << '.' << V_MINOR << '.' << V_SUBMINOR;
    *this << " (" << __DATE__ << " " << __TIME__ << ")" << '\n';
    *this << '\n';
    *this << "Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de" << '\n';
    *this << "Licensed under the GNU General Public License v3" << '\n';
    *this << '\n';

    printHelp();
    *this << '\n';
    printPrompt();
}

void
RetroShell::_initialize()
{

}

isize
RetroShell::cposRel()
{
    isize lineLength = (isize)lastLine().size();
    
    return cpos >= lineLength ? 0 : lineLength - cpos;
}

RetroShell&
RetroShell::operator<<(char value)
{
    if (value == '\n') {

        // Newline (appends an empty line)
        storage.push_back("");
        cpos = cposMin = 0;
        shorten();

    } else if (value == '\r') {

        // Carriage return (clears the current line)
        storage.back() = "";
        
    } else {
        
        // Add a single character
        storage.back() += value;
    }
    
    shorten();
    isDirty = true;
    return *this;
}

RetroShell&
RetroShell::operator<<(const string& text)
{
    storage.back() += text;
    isDirty = true;
    return *this;
}

RetroShell&
RetroShell::operator<<(int value)
{
    *this << std::to_string(value);
    isDirty = true;
    return *this;
}

RetroShell&
RetroShell::operator<<(long value)
{
    *this << std::to_string(value);
    isDirty = true;
    return *this;
}

void
RetroShell::tab(isize hpos)
{
    isize delta = hpos - (int)storage.back().length();
    for (isize i = 0; i < delta; i++) {
        *this << ' ';
    }
    isDirty = true;
}

void
RetroShell::printPrompt()
{
    // Finish the current line (if neccessary)
    if (!lastLine().empty()) *this << '\n';

    // Print the prompt
    *this << prompt;
    cpos = cposMin = prompt.size();
}

void
RetroShell::clear()
{
    storage.clear();
    printPrompt();
}

void
RetroShell::printHelp()
{
    *this << "Press 'TAB' twice for help." << '\n';
}

void
RetroShell::shorten()
{
    while (storage.size() > 600) {
        
        storage.erase(storage.begin());
    }
}

void
RetroShell::pressUp()
{
    if (ipos == (isize)input.size() - 1) {
        lastInput() = lastLine().substr(cposMin);
    }
    
    if (ipos > 0) ipos--;
    if (ipos < (isize)input.size()) lastLine() = prompt + input[ipos];
    pressEnd();
    tabPressed = false;
}

void
RetroShell::pressDown()
{
    if (ipos + 1 < (isize)input.size()) ipos++;
    if (ipos < (isize)input.size()) lastLine() = prompt + input[ipos];
    tabPressed = false;
}

void
RetroShell::pressLeft()
{
    cpos = std::max(cpos - 1, cposMin);
    tabPressed = false;
}

void
RetroShell::pressRight()
{
    cpos = std::min(cpos + 1, (isize)lastLine().size());
    tabPressed = false;
}

void
RetroShell::pressHome()
{
    cpos = cposMin;
    tabPressed = false;
}

void
RetroShell::pressEnd()
{
    cpos = (isize)lastLine().size();
    tabPressed = false;
}

void
RetroShell::pressTab()
{
    if (tabPressed) {
        
        // TAB was pressed twice
        string currentInput = lastLine();
        isize cposMinOld = cposMin;
        
        // *this << '\n';
        
        // Print the instructions for this command
        interpreter.help(lastLine().substr(cposMin));
        
        // Repeat the old input string
        *this << currentInput;
        cposMin = cposMinOld;
        cpos = lastLine().length();
        
    } else {
        
        // Auto-complete the typed in command
        string stripped = storage.back().substr(cposMin);
        lastLine() = prompt + interpreter.autoComplete(stripped);
        cpos = (isize)lastLine().length();
    }
    
    tabPressed = true;
}

void
RetroShell::pressBackspace()
{
    if (cpos > cposMin) {
        lastLine().erase(lastLine().begin() + --cpos);
    }
    tabPressed = false;
}

void
RetroShell::pressDelete()
{
    if (cpos < (isize)lastLine().size()) {
        lastLine().erase(lastLine().begin() + cpos);
    }
    tabPressed = false;
}

void
RetroShell::pressReturn()
{
    // Get the last line without the prompt
    string command = lastLine().substr(cposMin);
    
    *this << '\n';
    
    // Print help message if there was no input
    if (command.empty()) {
        printHelp();
        printPrompt();
        return;
    }
    
    // Add command to the command history buffer
    input[input.size() - 1] = command;
    input.push_back("");
    ipos = (isize)input.size() - 1;
    
    // Execute the command
    try { exec(command); } catch (...) { };
    printPrompt();
    tabPressed = false;
}

void
RetroShell::pressKey(char c)
{    
    if (isprint(c)) {
                
        if (cpos < (isize)lastLine().size()) {
            lastLine().insert(lastLine().begin() + cpos, c);
        } else {
            lastLine() += c;
        }
        cpos++;
        
        isDirty = true;        
        tabPressed = false;
    }
}

const char *
RetroShell::text()
{
    all = "";
    
    if (auto numRows = storage.size()) {
        
        // Add all rows except the last one
        for (usize i = 0; i < numRows - 1; i++) all += storage[i] + "\n";
        
        // Add the last row
        all += storage[numRows - 1] + " ";        
    }
    
    return all.c_str();
}

void
RetroShell::exec(const string &command)
{
    // Skip empty lines
    if (command == "") return;

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
    msg("execScript(ifstream)\n");
    
    script.str("");
    script << fs.rdbuf();
    scriptLine = 1;
    printPrompt();
    continueScript();
}

void
RetroShell::execScript(const string &contents)
{
    // msg("execScript(string)\n");

    script.str("");
    script << contents;
    scriptLine = 1;
    printPrompt();
    continueScript();
}

void
RetroShell::continueScript()
{
    msg("continueScript()\n");
    
    string command;
    while(std::getline(script, command)) {
            
        msg("%s\n", command.c_str());
        
        // Print the command
        printPrompt();
        *this << command << '\n';
        
        // Execute the command
        try {
            exec(command);
            
        } catch (ScriptInterruption &e) {
            
            messageQueue.put(MSG_SCRIPT_PAUSE, scriptLine);
            printPrompt();
            return;
        
        } catch (std::exception &e) {
            
            *this << "Aborted in line " << scriptLine << '\n';
            messageQueue.put(MSG_SCRIPT_ABORT, scriptLine);
            printPrompt();
            return;
        }

        scriptLine++;
    }
    
    printPrompt();
    messageQueue.put(MSG_SCRIPT_DONE, scriptLine);
}

void
RetroShell::describe(const std::exception &e)
{
    if (auto err = dynamic_cast<const TooFewArgumentsError *>(&e)) {
        
        *this << err->what() << ": Too few arguments";
        *this << '\n';
        
    } else if (auto err = dynamic_cast<const TooManyArgumentsError *>(&e)) {
        
        *this << err->what() << ": Too many arguments";
        *this << '\n';
    
    } else if (auto err = dynamic_cast<const util::EnumParseError *>(&e)) {
        
        *this << err->token << " is not a valid key" << '\n';
        *this << "Expected: " << err->expected << '\n';

    } else if (auto err = dynamic_cast<const util::ParseNumError *>(&e)) {
        
        *this << err->token << " is not a number";
        *this << '\n';

    } else if (auto err = dynamic_cast<const util::ParseBoolError *>(&e)) {

        *this << err->token << " must be true or false";
        *this << '\n';

    } else if (auto err = dynamic_cast<const util::ParseError *>(&e)) {

        *this << err->what() << ": Syntax error";
        *this << '\n';

    } else if (auto err = dynamic_cast<const ConfigUnsupportedError *>(&e)) {

        *this << "This option is not yet supported.";
        *this << '\n';

    } else if (auto err = dynamic_cast<const ConfigLockedError *>(&e)) {

        *this << "This option is locked because the Amiga is powered on.";
        *this << '\n';

    } else if (auto err = dynamic_cast<const ConfigArgError *>(&e)) {

        *this << "Error: Invalid argument. Expected: " << err->what();
        *this << '\n';

    /*
    } else if (auto err = dynamic_cast<const ConfigFileNotFoundError *>(&e)) {

        *this << err->what() << ": File not found";
        *this << '\n';
    */
    } else if (auto err = dynamic_cast<const ConfigFileReadError *>(&e)) {

        *this << "Error: Unable to read file " << err->what();
        *this << '\n';
    
    } else if (auto err = dynamic_cast<const VAError *>(&e)) {

        describe(*err);
    }
}

void
RetroShell::describe(const struct VAError &err)
{
    switch ((ErrorCode)err.data) {
            
        case ERROR_FILE_NOT_FOUND:
            *this << err.description << ": File not found" << '\n';
            return;
            
        case ERROR_ROM_MISSING:
            *this << "No Boot or Kickstart Rom found" << '\n';
            return;
            
        case ERROR_CHIP_RAM_MISSING:
            *this << "No Chip Ram found" << '\n';
            return;

        case ERROR_AROS_NO_EXTROM:
            *this << "The Aros Kickstart requires an extension Rom" << '\n';
            return;

        case ERROR_AROS_RAM_LIMIT:
            *this << "Aros requires at least 1 MB of memory" << '\n';
            return;

        case ERROR_CHIP_RAM_LIMIT:
            *this << "The selected Agnus can only handle ";
            *this << agnus.chipRamLimit() << " MB of Chip Ram" << '\n';
            return;
            
        default:
            
            *this << "Command failed with error code " << (isize)err.data;
            *this << " (" << err.what() << ")" << '\n';
    }
}

/*
void
RetroShell::exec(std::istream &stream)
{
    isize line = 0;
    string command;
        
    // *this << '\n';
    
    while(std::getline(stream, command)) {

        line++;
        printf("Line %zd: %s\n", line, command.c_str());

        // Execute the command
        *this << command << '\n';
        bool result = exec(command, line);
        
        if (!result) {
            
            printf("Aborted in line %zd\n", line);
            *this << "Aborted in line " << line << '\n';
            printPrompt();
            
            throw util::Exception(command, line);
        }
        printPrompt();
    }
}
*/

void
RetroShell::dump(HardwareComponent &component, dump::Category category)
{
    std::stringstream ss; string line;
    
    amiga.suspend();
    component.dump(category, ss);
    amiga.resume();
    
    while(std::getline(ss, line)) *this << line << '\n';
}

void
RetroShell::vsyncHandler()
{
    if (agnus.clock >= wakeUp) {
        
        // Ask the external thread (GUI) to continue the script
        messageQueue.put(MSG_SCRIPT_WAKEUP);
        wakeUp = INT64_MAX;
    }
}
