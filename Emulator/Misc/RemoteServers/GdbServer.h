// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "RemoteServer.h"
#include "OSDebugger.h"

enum class GdbCmd
{
    Attached,
    C,
    ContQ,
    Cont,
    MustReplyEmpty,
    CtrlC,
    Offset,
    StartNoAckMode,
    sThreadInfo,
    Supported,
    Symbol,
    TfV,
    TfP,
    TStatus,
    fThreadInfo,
};

class GdbServer : public RemoteServer {

    // The name of the process to be debugged
    string processName;
    
    // The segment list of the debug process
    os::SegList segList;
    
    // The most recently processed command string
    string latestCmd;
    
    // Indicates whether received packets should be acknowledged
    bool ackMode = true;

    
    //
    // Initializing
    //
    
public:

    using RemoteServer::RemoteServer;

    
    //
    // Methods from AmigaObject
    //
    
private:
    
    const char *getDescription() const override { return "GdbServer"; }
    void _dump(dump::Category category, std::ostream& os) const override;
    
    
    //
    // Methods from RemoteServer
    //
    
public:
    
    isize _defaultPort() const override { return 8082; }
    bool _launchable() override;
    string _receive() override throws;
    void _send(const string &payload) override throws;
    void _process(const string &payload) override throws;

    void didConnect() override throws;
    void didSwitch(SrvState from, SrvState to) override;

private:
    
    // Sends a packet with control characters and a checksum attached
    void reply(const string &payload);
    
    // 
        
    //
    // Analyzing the attached process
    //
    
    u32 codeSeg() const;
    u32 dataSeg() const;
    u32 bssSeg() const;

    
    //
    // Managing checksums
    //

    // Computes a checksum for a given string
    string computeChecksum(const string &s);

    // Verifies the checksum for a given string
    bool verifyChecksum(const string &s, const string &chk);

      
    //
    // Handling packets
    //

public:
        
    // Processes a packet in the format used by GDB
    void process(string packet) throws;

    // Processes a checksum-free packet with the first letter stripped off
    void process(char letter, string packet) throws;

private:
        
    // Processes a single command (GdbServerCmds.cpp)
    template <char letter> void process(string arg) throws;
    template <char letter, GdbCmd cmd> void process(string arg) throws;
    
    
    //
    // Reading the emulator state
    //
    
    // Reads a register value
    string readRegister(isize nr);

    // Reads a byte from memory
    string readMemory(isize addr);
    
    
    //
    // Delegation methods
    //
    
public:
    
    void breakpointReached();
};