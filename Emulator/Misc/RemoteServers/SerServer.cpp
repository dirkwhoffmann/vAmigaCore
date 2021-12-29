// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "SerServer.h"
#include "Agnus.h"
#include "IOUtils.h"
#include "Scheduler.h"
#include "SuspendableThread.h"
#include "UART.h"

SerServer::SerServer(Amiga& ref) : RemoteServer(ref)
{
}

void
SerServer::_dump(dump::Category category, std::ostream& os) const
{
    using namespace util;
        
    RemoteServer::_dump(category, os);
    
    if (category & dump::State) {
        
        os << tab("Received bytes");
        os << dec(receivedBytes) << std::endl;
        os << tab("Transmitted bytes");
        os << dec(transmittedBytes) << std::endl;
        os << tab("Processed bytes");
        os << dec(processedBytes) << std::endl;
        os << tab("Lost bytes");
        os << dec(lostBytes) << std::endl;
        os << tab("Buffered bytes");
        os << dec(buffer.count()) << std::endl;
    }
}

ServerConfig
SerServer::getDefaultConfig()
{
    ServerConfig defaults;
    
    defaults.port = 8080;
    defaults.protocol = SRVPROT_DEFAULT;
    defaults.verbose = false;

    return defaults;
}

string
SerServer::doReceive()
{
    auto result = connection.recv();
    receivedBytes += result.size();
    return result;
}

void
SerServer::doSend(const string &packet)
{
    transmittedBytes += packet.size();
    connection.send(packet);
}

void
SerServer::doProcess(const string &packet)
{
    for (auto c : packet) { processIncomingByte((u8)c); }
}

void
SerServer::processIncomingByte(u8 byte)
{
    if (!buffer.isFull()) {

        buffer.write(byte);
        
        // When enough bytes have been received, leave buffering mode
        if (buffer.count() >= 8) buffering = false;

    } else {

        lostBytes++;
        debug(SRV_DEBUG, "Buffer overflow\n");
    }
}

void
SerServer::didSwitch(SrvState from, SrvState to)
{
    if (to == SRV_STATE_CONNECTED) {

        // Start a new sessing
        skippedTransmissions = 0;
        receivedBytes = 0;
        transmittedBytes = 0;
        processedBytes = 0;
        lostBytes = 0;
        
        // Start scheduling messages
        assert(scheduler.id[SLOT_SER] == EVENT_NONE);
        scheduler.scheduleImm <SLOT_SER> (SER_RECEIVE);
    }
    
    if (from == SRV_STATE_CONNECTED) {
        
        // Stop scheduling messages
        scheduler.cancel <SLOT_SER> ();
    }
}

void
SerServer::serviceSerEvent()
{
    assert(scheduler.id[SLOT_SER] == SER_RECEIVE);
    
    if (buffer.isEmpty()) {
        
        // Enter buffering mode if we run dry
        buffering = true;

    } else if (buffering) {
        
        // Exit buffering mode if now new symbols came in for quite a while
        if (++skippedTransmissions > 8) buffering = false;

    } else {
    
        // Hand the oldest buffer element over to the UART
        uart.receiveShiftReg = buffer.read();
        uart.copyFromReceiveShiftRegister();
        processedBytes++;
        skippedTransmissions = 0;
    }
    
    scheduleNextEvent();
}

void
SerServer::scheduleNextEvent()
{
    assert(scheduler.id[SLOT_SER] == SER_RECEIVE);
    
    // Otherwise, emulate proper timing based on the current baud rate
    auto pulseWidth = uart.pulseWidth();
    
    // If the pulseWidth is extremely low, fallback to a default value
    if (pulseWidth < 40) {
        
        debug(SRV_DEBUG, "Very low SERPER value\n");
        pulseWidth = 12000;
    }
    
    agnus.scheduleRel<SLOT_SER>(8 * pulseWidth, SER_RECEIVE);
}
