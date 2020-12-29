// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// This file must conform to standard ANSI-C to be compatible with Swift.

#ifndef _MESSAGE_QUEUE_TYPES_H
#define _MESSAGE_QUEUE_TYPES_H

#include "Aliases.h"

enum_long(MessageType)
{
    MSG_NONE = 0,
    
    // Message queue
    MSG_REGISTER,
    MSG_UNREGISTER,
    
    // Emulator state
    MSG_CONFIG,
    MSG_POWER_ON,
    MSG_POWER_OFF,
    MSG_RUN,
    MSG_PAUSE,
    MSG_RESET,
    MSG_WARP_ON,
    MSG_WARP_OFF,
    MSG_MUTE_ON,
    MSG_MUTE_OFF,
    MSG_POWER_LED_ON,
    MSG_POWER_LED_DIM,
    MSG_POWER_LED_OFF,
        
    // CPU
    MSG_BREAKPOINT_CONFIG,
    MSG_BREAKPOINT_REACHED,
    MSG_WATCHPOINT_REACHED,
    MSG_CPU_HALT,

    // Memory
    MSG_MEM_LAYOUT,
        
    // Floppy drives
    MSG_DRIVE_CONNECT,
    MSG_DRIVE_DISCONNECT,
    MSG_DRIVE_SELECT,
    MSG_DRIVE_READ,
    MSG_DRIVE_WRITE,
    MSG_DRIVE_LED_ON,
    MSG_DRIVE_LED_OFF,
    MSG_DRIVE_MOTOR_ON,
    MSG_DRIVE_MOTOR_OFF,
    MSG_DRIVE_HEAD,
    MSG_DRIVE_HEAD_POLL,
    MSG_DISK_INSERT,
    MSG_DISK_EJECT,
    MSG_DISK_SAVED,
    MSG_DISK_UNSAVED,
    MSG_DISK_PROTECT,
    MSG_DISK_UNPROTECT,

    // Keyboard
    MSG_CTRL_AMIGA_AMIGA,
    
    // Ports
    MSG_SER_IN,
    MSG_SER_OUT,

    // Snapshots
    MSG_AUTO_SNAPSHOT_TAKEN,
    MSG_USER_SNAPSHOT_TAKEN,
    MSG_SNAPSHOT_RESTORED,

    // Screen recording
    MSG_RECORDING_STARTED,
    MSG_RECORDING_STOPPED,
    
    // Debugging
    MSG_DMA_DEBUG_ON,
    MSG_DMA_DEBUG_OFF
};

inline bool isMessageType(long value)
{
    return value >= 0 && value <= MSG_DMA_DEBUG_OFF;
}

inline const char *sMessageType(MessageType type)
{
    assert(isMessageType(type));

    switch (type) {
            
        case MSG_NONE:                return "MSG_NONE";
        case MSG_REGISTER:            return "MSG_REGISTER";
        case MSG_UNREGISTER:          return "MSG_UNREGISTER";
            
        case MSG_CONFIG:              return "MSG_CONFIG";
        case MSG_POWER_ON:            return "MSG_POWER_ON";
        case MSG_POWER_OFF:           return "MSG_POWER_OFF";
        case MSG_RUN:                 return "MSG_RUN";
        case MSG_PAUSE:               return "MSG_PAUSE";
        case MSG_RESET:               return "MSG_RESET";
        case MSG_WARP_ON:             return "MSG_WARP_ON";
        case MSG_WARP_OFF:            return "MSG_WARP_OFF";
        case MSG_MUTE_ON:             return "MSG_MUTE_ON";
        case MSG_MUTE_OFF:            return "MSG_MUTE_OFF";
        case MSG_POWER_LED_ON:        return "MSG_POWER_LED_ON";
        case MSG_POWER_LED_DIM:       return "MSG_POWER_LED_DIM";
        case MSG_POWER_LED_OFF:       return "MSG_POWER_LED_OFF";
                
        case MSG_BREAKPOINT_CONFIG:   return "MSG_BREAKPOINT_CONFIG";
        case MSG_BREAKPOINT_REACHED:  return "MSG_BREAKPOINT_REACHED";
        case MSG_WATCHPOINT_REACHED:  return "MSG_WATCHPOINT_REACHED";
        case MSG_CPU_HALT:            return "MSG_CPU_HALT";

        case MSG_MEM_LAYOUT:          return "MSG_MEM_LAYOUT";
                
        case MSG_DRIVE_CONNECT:       return "MSG_DRIVE_CONNECT";
        case MSG_DRIVE_DISCONNECT:    return "MSG_DRIVE_DISCONNECT";
        case MSG_DRIVE_SELECT:        return "MSG_DRIVE_SELECT";
        case MSG_DRIVE_READ:          return "MSG_DRIVE_READ";
        case MSG_DRIVE_WRITE:         return "MSG_DRIVE_WRITE";
        case MSG_DRIVE_LED_ON:        return "MSG_DRIVE_LED_ON";
        case MSG_DRIVE_LED_OFF:       return "MSG_DRIVE_LED_OFF";
        case MSG_DRIVE_MOTOR_ON:      return "MSG_DRIVE_MOTOR_ON";
        case MSG_DRIVE_MOTOR_OFF:     return "MSG_DRIVE_MOTOR_OFF";
        case MSG_DRIVE_HEAD:          return "MSG_DRIVE_HEAD";
        case MSG_DRIVE_HEAD_POLL:     return "MSG_DRIVE_HEAD_POLL";
        case MSG_DISK_INSERT:         return "MSG_DISK_INSERT";
        case MSG_DISK_EJECT:          return "MSG_DISK_EJECT";
        case MSG_DISK_SAVED:          return "MSG_DISK_SAVED";
        case MSG_DISK_UNSAVED:        return "MSG_DISK_UNSAVED";
        case MSG_DISK_PROTECT:        return "MSG_DISK_PROTECT";
        case MSG_DISK_UNPROTECT:      return "MSG_DISK_UNPROTECT";

        case MSG_CTRL_AMIGA_AMIGA:    return "MSG_CTRL_AMIGA_AMIGA";
            
        case MSG_SER_IN:              return "MSG_SER_IN";
        case MSG_SER_OUT:             return "MSG_SER_OUT";

        case MSG_AUTO_SNAPSHOT_TAKEN: return "MSG_AUTO_SNAPSHOT_TAKEN";
        case MSG_USER_SNAPSHOT_TAKEN: return "MSG_USER_SNAPSHOT_TAKEN";
        case MSG_SNAPSHOT_RESTORED:   return "MSG_SNAPSHOT_RESTORED";

        case MSG_RECORDING_STARTED:   return "MSG_RECORDING_STARTED";
        case MSG_RECORDING_STOPPED:   return "MSG_RECORDING_STOPPED";
            
        case MSG_DMA_DEBUG_ON:        return "MSG_DMA_DEBUG_ON";
        case MSG_DMA_DEBUG_OFF:       return "MSG_DMA_DEBUG_OFF";
        default:                      return "???";
    }
}

typedef struct
{
    MessageType type;
    long data;
}
Message;

// Callback function signature
typedef void Callback(const void *, long, long);

#endif
