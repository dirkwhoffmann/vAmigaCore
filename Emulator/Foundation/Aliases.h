// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _ALIASES_H
#define _ALIASES_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

//
// Basic types
//

typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

static_assert(sizeof(i8) == 1,  "i8 size mismatch");
static_assert(sizeof(i16) == 2, "i16 size mismatch");
static_assert(sizeof(i32) == 4, "i32 size mismatch");
static_assert(sizeof(i64) == 8, "i64 size mismatch");
static_assert(sizeof(u8) == 1,  "u8 size mismatch");
static_assert(sizeof(u16) == 2, "u16 size mismatch");
static_assert(sizeof(u32) == 4, "u32 size mismatch");
static_assert(sizeof(u64) == 8, "u64 size mismatch");


//
// Syntactic sugar
//

#define fallthrough


//
// Cycles
//

typedef i64 Cycle;            // Master cycle units
typedef i64 CPUCycle;         // CPU cycle units
typedef i64 CIACycle;         // CIA cycle units
typedef i64 DMACycle;         // DMA cycle units

// Converts a certain unit to master cycles
#define USEC(delay)           ((delay) * 28)
#define MSEC(delay)           ((delay) * 28000)
#define SEC(delay)            ((delay) * 28000000)

#define CPU_CYCLES(cycles)    ((cycles) << 2)
#define CIA_CYCLES(cycles)    ((cycles) * 40)
#define DMA_CYCLES(cycles)    ((cycles) << 3)

// Converts master cycles to a certain unit
#define AS_USEC(delay)        ((delay) / 28)
#define AS_MSEC(delay)        ((delay) / 28000)
#define AS_SEC(delay)         ((delay) / 28000000)

#define AS_CPU_CYCLES(cycles) ((cycles) >> 2)
#define AS_CIA_CYCLES(cycles) ((cycles) / 40)
#define AS_DMA_CYCLES(cycles) ((cycles) >> 3)

#define IS_CPU_CYCLE(cycles)  ((cycles) & 3 == 0)
#define IS_CIA_CYCLE(cycles)  ((cycles) % 40 == 0)
#define IS_DMA_CYCLE(cycles)  ((cycles) & 7 == 0)


//
// Positions
//

typedef i16 PixelPos;


//
// Floppy disks
//

typedef i16 Side;
typedef i16 Cylinder;
typedef i16 Track;
typedef i16 Sector;

/* All enumeration types are declared via VAMIGA_ENUM. We don't use the
 * standard C enum style to make enumerations easily accessible in Swift.
 */

// Definition for Swift
#ifdef VA_ENUM
#define VAMIGA_ENUM(_type, _name) \
typedef VA_ENUM(_type, _name)

// Definition for clang
#elif defined(__clang__)
#define VAMIGA_ENUM(_type, _name) \
typedef enum __attribute__((enum_extensibility(open))) _name : _type _name; \
enum _name : _type

// Definition for gcc
#else
#define VAMIGA_ENUM(_type, _name) \
enum _name : _type
#endif

#endif
