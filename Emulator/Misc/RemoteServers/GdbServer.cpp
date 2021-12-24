// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "GdbServer.h"
#include "Amiga.h"
#include "CPU.h"
#include "IOUtils.h"
#include "Memory.h"
#include "MemUtils.h"
#include "MsgQueue.h"
#include "OSDebugger.h"
#include "RetroShell.h"

void
GdbServer::_dump(dump::Category category, std::ostream& os) const
{
    using namespace util;
     
    RemoteServer::_dump(category, os);

    if (category & dump::Segments) {
        
        os << tab("Code segment") << hex(codeSeg()) << std::endl;
        os << tab("Data segment") << hex(dataSeg()) << std::endl;
        os << tab("BSS segment") << hex(bssSeg()) << std::endl;
    }
}

bool
GdbServer::_launchable()
{
    // If the seglist is present, we are ready to go
    if (!segList.empty()) return true;
    
    // If not, try to located the process
    if (!args.empty()) osDebugger.read(args[0], segList);
    return !segList.empty();
}

string
GdbServer::_receive()
{
    latestCmd = connection.recv();
    retroShell << "R: " << latestCmd << "\n";
    return latestCmd;
}

void
GdbServer::_send(const string &payload)
{
    retroShell << "T: " << payload << "\n";
    connection.send(payload);
}

void
GdbServer::_process(const string &payload)
{
    try {
        
        process(latestCmd);
        
    } catch (VAError &err) {
        
        auto msg = "GDB server error: " + string(err.what());
        debug(SRV_DEBUG, "%s\n", msg.c_str());

        // Display the error message in RetroShell
        retroShell << msg << '\n';

        // Disconnect the client
        disconnect();
    }
}

void
GdbServer::didConnect()
{
    ackMode = true;
}

void
GdbServer::didSwitch(SrvState from, SrvState to)
{
    if (from == SRV_STATE_OFF && to == SRV_STATE_LAUNCHING) {
        
        retroShell << "Waiting for process '" << args[0] << "' to launch.\n";
        retroShell.flush();
    }
    
    if ((from == SRV_STATE_OFF && to == SRV_STATE_LISTENING) ||
        (from == SRV_STATE_LAUNCHING && to == SRV_STATE_LISTENING)) {
        
        retroShell << "Successfully attached to process '" << args[0] << "'\n\n";
        retroShell << "    Data segment: " << util::hexstr <8> (dataSeg()) << "\n";
        retroShell << "    Code segment: " << util::hexstr <8> (codeSeg()) << "\n";
        retroShell << "     BSS segment: " << util::hexstr <8> (bssSeg()) << "\n\n";
        retroShell.flush();

        if (amiga.isRunning()) {

            amiga.pause();
            retroShell << "Pausing emulation.\n\n";
        }
    }
}

void
GdbServer::reply(const string &payload)
{
    string packet = "$";
    
    packet += payload;
    packet += "#";
    packet += computeChecksum(payload);
    
    send(packet);
}

u32
GdbServer::codeSeg() const
{
   return segList.size() > 0 ? segList[0].first : 0;
}

u32
GdbServer::dataSeg() const
{
    return segList.size() > 1 ? segList[1].first : 0;
}

u32
GdbServer::bssSeg() const
{
    return segList.size() > 2 ? segList[2].first : dataSeg();
}

string
GdbServer::computeChecksum(const string &s)
{
    uint8_t chk = 0;
    for(auto &c : s) chk += (uint8_t)c;

    return util::hexstr <2> (chk);
}

bool
GdbServer::verifyChecksum(const string &s, const string &chk)
{
    return chk == computeChecksum(s);
}

string
GdbServer::readRegister(isize nr)
{
    if (nr >= 0 && nr <= 7) {
        return util::hexstr <8> ((u32)cpu.getD((int)(nr)));
    }
    if (nr >= 8 && nr <= 15) {
        return util::hexstr <8> ((u32)cpu.getA((int)(nr - 8)));
    }
    if (nr == 16) {
        return util::hexstr <8> ((u32)cpu.getSR());
    }
    if (nr == 17) {
        return util::hexstr <8> ((u32)cpu.getPC());
    }

    return "xxxxxxxx";
}

string
GdbServer::readMemory(isize addr)
{
    auto byte = mem.spypeek8 <ACCESSOR_CPU> ((u32)addr);
    return util::hexstr <2> (byte);
}

void
GdbServer::breakpointReached()
{
    debug(SRV_DEBUG, "Breakpoint reached\n");
    process <'?'> ("");
}