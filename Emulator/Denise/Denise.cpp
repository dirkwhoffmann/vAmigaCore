// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"
#include "SSEUtils.h"

Denise::Denise(Amiga& ref) : AmigaComponent(ref)
{
    setDescription("Denise");
    
    subComponents = vector<HardwareComponent *> {
        
        &pixelEngine,
    };

    config.emulateSprites = true;
    config.hiddenLayers = 0;
    config.hiddenLayerAlpha = 128;
    config.clxSprSpr = true;
    config.clxSprPlf = true;
    config.clxPlfPlf = true;
}

void
Denise::setRevision(DeniseRevision revision)
{
    debug("setRevision(%d)\n", revision);

    assert(isDeniseRevision(revision));
    config.revision = revision;
}

void
Denise::setHiddenLayers(u16 value)
{
    msg("Changing layer mask to %x\n", value);
    config.hiddenLayers = value;
}

void
Denise::setHiddenLayerAlpha(u8 value)
{
    // msg("Changing layer alpha to %x\n", value);
    config.hiddenLayerAlpha = value;
}

void
Denise::_powerOn()
{
}

void
Denise::_reset(bool hard)
{
    RESET_SNAPSHOT_ITEMS

    memset(bBuffer, 0, sizeof(bBuffer));
    memset(iBuffer, 0, sizeof(iBuffer));
    memset(mBuffer, 0, sizeof(mBuffer));
    memset(zBuffer, 0, sizeof(zBuffer));
}

void
Denise::_inspect()
{
    // Prevent external access to variable 'info'
    pthread_mutex_lock(&lock);
    
    // Bitplane information
    info.bplcon0 = bplcon0;
    info.bplcon1 = bplcon1;
    info.bplcon2 = bplcon2;
    info.bpu = bpu();

    info.diwstrt = agnus.diwstrt;
    info.diwstop = agnus.diwstop;
    info.diwHstrt = agnus.diwHstrt;
    info.diwHstop = agnus.diwHstop;
    info.diwVstrt = agnus.diwVstrt;
    info.diwVstop = agnus.diwVstop;

    info.joydat[0] = amiga.controlPort1.joydat();
    info.joydat[1] = amiga.controlPort2.joydat();
    info.clxdat = 0;

    for (unsigned i = 0; i < 6; i++) {
        info.bpldat[i] = bpldat[i];
    }
    for (unsigned i = 0; i < 32; i++) {
        info.colorReg[i] = pixelEngine.getColor(i);
        info.color[i] = pixelEngine.getRGBA(i);
    }
    
    pthread_mutex_unlock(&lock);
}

void
Denise::_dumpConfig()
{
    msg("  emulateSprites: %d\n", config.emulateSprites);
    msg("    hiddenLayers: %d\n", config.hiddenLayers);
    msg("hiddenLayerAlpha: %d\n", config.hiddenLayerAlpha);
    msg("       clxSprSpr: %d\n", config.clxSprSpr);
    msg("       clxSprPlf: %d\n", config.clxSprPlf);
    msg("       clxPlfPlf: %d\n", config.clxPlfPlf);
}

void
Denise::_dump()
{
}

SpriteInfo
Denise::getSpriteInfo(int nr)
{
    SpriteInfo result;
    
    pthread_mutex_lock(&lock);
    result = latchedSpriteInfo[nr];
    pthread_mutex_unlock(&lock);
    
    return result;
}

u16
Denise::peekJOY0DATR()
{
    u16 result = amiga.controlPort1.joydat();
    debug(JOYREG_DEBUG, "peekJOY0DATR() = $%04X (%d)\n", result, result);

    return result;
}

u16
Denise::peekJOY1DATR()
{
    u16 result = amiga.controlPort2.joydat();
    debug(JOYREG_DEBUG, "peekJOY1DATR() = $%04X (%d)\n", result, result);

    return result;
}

void
Denise::pokeJOYTEST(u16 value)
{
    debug(JOYREG_DEBUG, "pokeJOYTEST(%04X)\n", value);

    amiga.controlPort1.pokeJOYTEST(value);
    amiga.controlPort2.pokeJOYTEST(value);
}

u16
Denise::peekDENISEID()
{
    u16 result;

    if (config.revision == DENISE_8373) {
        result = 0xFFFC;                           // ECS
    } else {
        result = mem.peekCustomFaulty16(0xDFF07C); // OCS
    }

    debug(ECSREG_DEBUG, "peekDENISEID() = $%04X (%d)\n", result, result);
    return result;
}

void
Denise::pokeBPLCON0(u16 value)
{
    debug(BPLREG_DEBUG, "pokeBPLCON0(%X)\n", value);

    agnus.recordRegisterChange(DMA_CYCLES(1), REG_BPLCON0_DENISE, value);
}

void
Denise::setBPLCON0(u16 oldValue, u16 newValue)
{
    debug(BPLREG_DEBUG, "setBPLCON0(%X,%X)\n", oldValue, newValue);

    // Record the register change
    i64 pixel = MAX(4 * agnus.pos.h - 4, 0);
    conChanges.insert(pixel, RegChange { REG_BPLCON0_DENISE, newValue });
    
    if (ham(oldValue) ^ ham(newValue)) {
        pixelEngine.colChanges.insert(pixel, RegChange { BPLCON0, newValue } );
    }
    
    // Update value
    bplcon0 = newValue;
}

int
Denise::bpu(u16 v)
{
    // Extract the three BPU bits and check for hires mode
    int bpu = (v >> 12) & 0b111;
    bool hires = GET_BIT(v, 15);

    if (hires) {
        return bpu < 5 ? bpu : 0; // Disable all bitplanes if value is invalid
    } else {
        return bpu < 7 ? bpu : 6; // Enable six bitplanes if value is invalid
    }
}

void
Denise::pokeBPLCON1(u16 value)
{
    debug(BPLREG_DEBUG, "pokeBPLCON1(%X)\n", value);

    // Record the register change
    agnus.recordRegisterChange(DMA_CYCLES(1), REG_BPLCON1_DENISE, value);
}

void
Denise::setBPLCON1(u16 value)
{
    debug(BPLREG_DEBUG, "setBPLCON1(%X)\n", value);

    bplcon1 = value & 0xFF;

    pixelOffsetOdd  = (bplcon1 & 0b00000001) << 1;
    pixelOffsetEven = (bplcon1 & 0b00010000) >> 3;
}

void
Denise::pokeBPLCON2(u16 value)
{
    debug(BPLREG_DEBUG, "pokeBPLCON2(%X)\n", value);

    agnus.recordRegisterChange(DMA_CYCLES(1), REG_BPLCON2, value);
}

void
Denise::setBPLCON2(u16 value)
{
    debug(BPLREG_DEBUG, "setBPLCON2(%X)\n", value);

    bplcon2 = value;

    // Record the pixel coordinate where the change takes place
    conChanges.insert(4 * agnus.pos.h + 4, RegChange { REG_BPLCON2, value });
}

u16
Denise::zPF(u16 priorityBits)
{
    switch (priorityBits) {

        case 0: return Z_0;
        case 1: return Z_1;
        case 2: return Z_2;
        case 3: return Z_3;
        case 4: return Z_4;
    }

    return 0;
}

u16
Denise::peekCLXDAT()
{
    u16 result = clxdat | 0x8000;
    clxdat = 0;
    
    debug(CLX_DEBUG, "peekCLXDAT() = %x\n", result);
    return result;
}

void
Denise::pokeCLXCON(u16 value)
{
    debug(CLX_DEBUG, "pokeCLXCON(%x)\n", value);
    clxcon = value;
}

template <int x> void
Denise::pokeBPLxDAT(u16 value)
{
    assert(x < 6);
    debug(BPLREG_DEBUG, "pokeBPL%dDAT(%X)\n", x + 1, value);
    
    bpldat[x] = value;
}

template <int x> void
Denise::pokeSPRxPOS(u16 value)
{
    assert(x < 8);
    debug(SPRREG_DEBUG, "pokeSPR%dPOS(%X)\n", x, value);

    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0  (Ex = VSTART)
    // E7 E6 E5 E4 E3 E2 E1 E0 H8 H7 H6 H5 H4 H3 H2 H1  (Hx = HSTART)

    // Determine the sprite pair this sprite belongs to
    const int pair = x / 2;

    // sprpos[x] = value;

    // Record the register change
    i64 pos = 4 * (agnus.pos.h + 1);
    sprChanges[pair].insert(pos, RegChange { REG_SPR0POS + x, value } );
}

template <int x> void
Denise::pokeSPRxCTL(u16 value)
{
    assert(x < 8);
    debug(SPRREG_DEBUG, "pokeSPR%dCTL(%X)\n", x, value);

    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // L7 L6 L5 L4 L3 L2 L1 L0 AT  -  -  -  - E8 L8 H0  (Lx = VSTOP)

    // Determine the sprite pair this sprite belongs to
    const int pair = x / 2;

    // sprctl[x] = value;

    // Save the attach bit
    // REPLACE_BIT(attach, x, GET_BIT(value, 7));
    
    // Disarm the sprite
    CLR_BIT(armed, x);

    // Record the register change
    i64 pos = 4 * (agnus.pos.h + 1);
    sprChanges[pair].insert(pos, RegChange { REG_SPR0CTL + x, value } );
}

template <int x> void
Denise::pokeSPRxDATA(u16 value)
{
    assert(x < 8);
    debug(SPRREG_DEBUG, "pokeSPR%dDATA(%X)\n", x, value);
    
    // Determine the sprite pair this sprite belongs to
     const int pair = x / 2;
    
    // sprdata[x] = value;

    // Arm the sprite
    SET_BIT(armed, x);
    SET_BIT(wasArmed, x);

    // Record the register change
    i64 pos = 4 * (agnus.pos.h + 1);
    sprChanges[pair].insert(pos, RegChange { REG_SPR0DATA + x, value } );
}

template <int x> void
Denise::pokeSPRxDATB(u16 value)
{
    assert(x < 8);
    debug(SPRREG_DEBUG, "pokeSPR%dDATB(%X)\n", x, value);
    
    // Determine the sprite pair this sprite belongs to
    const int pair = x / 2;
    
    // sprdatb[x] = value;

    // Record the register change
    i64 pos = 4 * (agnus.pos.h + 1);
    sprChanges[pair].insert(pos, RegChange { REG_SPR0DATB + x, value });
}

template <PokeSource s, int xx> void
Denise::pokeCOLORxx(u16 value)
{
    debug(COLREG_DEBUG, "pokeCOLOR%02d(%X)\n", xx, value);

    u32 reg = 0x180 + 2*xx;
    i16 pos = agnus.pos.h;

    // If the CPU modifies color, the change takes effect one DMA cycle earlier
    if (s != POKE_COPPER && agnus.pos.h != 0) pos--;
    
    // Record the color change
    pixelEngine.colChanges.insert(4 * pos, RegChange { reg, value } );
}

bool
Denise::attached(int x) {

    assert(x >= 1 && x <= 7);
    assert(IS_ODD(x));

    return GET_BIT(attach, x);
}

bool
Denise::spritePixelIsVisible(int hpos)
{
    u16 z = zBuffer[hpos];
    return (z & Z_SP01234567) > (z & ~Z_SP01234567);
}

void
Denise::fillShiftRegisters(bool odd, bool even)
{
    if (odd) armedOdd = true;
    if (even) armedEven = true;
    
    spriteClipBegin = MIN(spriteClipBegin, agnus.ppos() + 2);
    
    switch (bpu()) {
        case 6: shiftReg[5] = bpldat[5];
        case 5: shiftReg[4] = bpldat[4];
        case 4: shiftReg[3] = bpldat[3];
        case 3: shiftReg[2] = bpldat[2];
        case 2: shiftReg[1] = bpldat[1];
        case 1: shiftReg[0] = bpldat[0];
    }
    
    if (!NO_SSE) {
        
        transposeSSE(shiftReg, slice);
        
    } else {
        
        assert(false);
        
        u32 mask = 0x8000;
        for (int i = 0; i < 16; i++, mask >>= 1) {
            
            slice[i] =
            (!!(shiftReg[0] & mask) << 0) |
            (!!(shiftReg[1] & mask) << 1) |
            (!!(shiftReg[2] & mask) << 2) |
            (!!(shiftReg[3] & mask) << 3) |
            (!!(shiftReg[4] & mask) << 4) |
            (!!(shiftReg[5] & mask) << 5);
        }
    }
}

template <bool hiresMode> void
Denise::drawOdd(int offset)
{
    assert(!hiresMode || (agnus.pos.h & 0x3) == agnus.scrollHiresOdd);
    assert( hiresMode || (agnus.pos.h & 0x7) == agnus.scrollLoresOdd);

    static const u16 masks[7] = {
       0b000000,         // 0 bitplanes
       0b000001,         // 1 bitplanes
       0b000001,         // 2 bitplanes
       0b000101,         // 3 bitplanes
       0b000101,         // 4 bitplanes
       0b010101,         // 5 bitplanes
       0b010101          // 6 bitplanes
    };
    
    u16 mask = masks[bpu()];
    i16 currentPixel = agnus.ppos() + offset;
    
    for (int i = 0; i < 16; i++) {
        
        u8 index = slice[i] & mask;
        
        if (hiresMode) {
            
            // Synthesize one hires pixel
            assert(currentPixel < sizeof(bBuffer));
            bBuffer[currentPixel] = (bBuffer[currentPixel] & 0b101010) | index;
            currentPixel++;
            
        } else {
            
            // Synthesize two lores pixels
            assert(currentPixel + 1 < sizeof(bBuffer));
            bBuffer[currentPixel] = (bBuffer[currentPixel] & 0b101010) | index;
            currentPixel++;
            bBuffer[currentPixel] = (bBuffer[currentPixel] & 0b101010) | index;
            currentPixel++;
        }
    }
 
    // Disarm and clear the shift registers
    armedOdd = false;
    shiftReg[0] = shiftReg[2] = shiftReg[4] = 0;
}

template <bool hiresMode> void
Denise::drawEven(int offset)
{
    assert(!hiresMode || (agnus.pos.h & 0x3) == agnus.scrollHiresEven);
    assert( hiresMode || (agnus.pos.h & 0x7) == agnus.scrollLoresEven);
    
    static const u16 masks[7] = {
       0b000000,         // 0 bitplanes
       0b000000,         // 1 bitplanes
       0b000010,         // 2 bitplanes
       0b000010,         // 3 bitplanes
       0b001010,         // 4 bitplanes
       0b001010,         // 5 bitplanes
       0b101010          // 6 bitplanes
    };
    
    u16 mask = masks[bpu()];
    i16 currentPixel = agnus.ppos() + offset;
    
    for (int i = 0; i < 16; i++) {

        u8 index = slice[i] & mask;

        if (hiresMode) {
            
            // Synthesize one hires pixel
            assert(currentPixel < sizeof(bBuffer));
            bBuffer[currentPixel] = (bBuffer[currentPixel] & 0b010101) | index;
            currentPixel++;

        } else {
            
            // Synthesize two lores pixels
            assert(currentPixel + 1 < sizeof(bBuffer));
            bBuffer[currentPixel] = (bBuffer[currentPixel] & 0b010101) | index;
            currentPixel++;
            bBuffer[currentPixel] = (bBuffer[currentPixel] & 0b010101) | index;
            currentPixel++;
        }
    }
 
    // Disarm and clear the shift registers
    armedEven = false;
    shiftReg[1] = shiftReg[3] = shiftReg[5] = 0;
}

template <bool hiresMode> void
Denise::drawBoth(int offset)
{
    static const u16 masks[7] = {
        0b000000,         // 0 bitplanes
        0b000001,         // 1 bitplanes
        0b000011,         // 2 bitplanes
        0b000111,         // 3 bitplanes
        0b001111,         // 4 bitplanes
        0b011111,         // 5 bitplanes
        0b111111          // 6 bitplanes
    };
    
    u16 mask = masks[bpu()];
    i16 currentPixel = agnus.ppos() + offset;
    
    for (int i = 0; i < 16; i++) {
        
        u8 index = slice[i] & mask;
        
        if (hiresMode) {
            
            // Synthesize one hires pixel
            assert(currentPixel < sizeof(bBuffer));
            bBuffer[currentPixel++] = index;
            
        } else {
            
            // Synthesize two lores pixels
            assert(currentPixel + 1 < sizeof(bBuffer));
            bBuffer[currentPixel++] = index;
            bBuffer[currentPixel++] = index;
        }
    }
    
    // Disarm and clear the shift registers
    armedEven = armedOdd = false;
    for (int i = 0; i < 6; i++) shiftReg[i] = 0;
}

void
Denise::drawHiresBoth()
{
    if (armedOdd && armedEven && pixelOffsetOdd == pixelOffsetEven) {

        assert((agnus.pos.h & 0x3) == agnus.scrollHiresOdd);
        assert((agnus.pos.h & 0x3) == agnus.scrollHiresEven);
        drawBoth<true>(pixelOffsetOdd);

    } else {
    
        drawHiresOdd();
        drawHiresEven();
    }
}

void
Denise::drawLoresBoth()
{
    if (armedOdd && armedEven && pixelOffsetOdd == pixelOffsetEven) {

        assert((agnus.pos.h & 0x7) == agnus.scrollLoresOdd);
        assert((agnus.pos.h & 0x7) == agnus.scrollLoresEven);
        drawBoth<false>(pixelOffsetOdd);

    } else {
    
        drawLoresOdd();
        drawLoresEven();
    }
}

void
Denise::translate()
{
    int pixel = 0;

    u16 bplcon0 = initialBplcon0;
    bool dual = dbplf(bplcon0);

    u16 bplcon2 = initialBplcon2;
    bool pri = PF2PRI(bplcon2);
    prio1 = zPF1(bplcon2);
    prio2 = zPF2(bplcon2);

    // Add a dummy register change to ensure we draw until the line ends
    conChanges.insert(sizeof(bBuffer), RegChange { REG_NONE, 0 });

    // Iterate over all recorded register changes
    for (int i = conChanges.begin(); i != conChanges.end(); i = conChanges.next(i)) {

        Cycle trigger = conChanges.keys[i];
        RegChange &change = conChanges.elements[i];

        // Translate a chunk of bitplane data
        if (dual) {
            translateDPF(pri, pixel, trigger);
        } else {
            translateSPF(pixel, trigger);
        }
        pixel = trigger;

        // Apply the register change
        switch (change.addr) {

            case REG_BPLCON0_DENISE:
                bplcon0 = change.value;
                dual = dbplf(bplcon0);
                break;

            case REG_BPLCON2:
                bplcon2 = change.value;
                pri = PF2PRI(bplcon2);
                prio1 = zPF1(bplcon2);
                prio2 = zPF2(bplcon2);
                break;

            default:
                assert(change.addr == REG_NONE);
                break;
        }
    }

    // Clear the history cache
    conChanges.clear();
}

void
Denise::translateSPF(int from, int to)
{
    // The usual case: prio2 is a valid value
    if (prio2) {
        for (int i = from; i < to; i++) {

            u8 s = bBuffer[i];

            assert(PixelEngine::isRgbaIndex(s));
            iBuffer[i] = mBuffer[i] = s;
            zBuffer[i] = s ? prio2 : 0;
        }

    // The unusual case: prio2 is ivalid
    } else {

        for (int i = from; i < to; i++) {

             u8 s = bBuffer[i];

             assert(PixelEngine::isRgbaIndex(s));
             iBuffer[i] = mBuffer[i] = (s & 16) ? 16 : s;
             zBuffer[i] = 0;
         }
    }
}

void
Denise::translateDPF(bool pf2pri, int from, int to)
{
    pf2pri ? translateDPF<true>(from, to) : translateDPF<false>(from, to);
}

template <bool pf2pri> void
Denise::translateDPF(int from, int to)
{
    /* If the priority of a playfield is set to an illegal value (prio1 or
     * prio2 will be 0 in that case), all pixels are drawn transparent.
     */
    u8 mask1 = prio1 ? 0b1111 : 0b0000;
    u8 mask2 = prio2 ? 0b1111 : 0b0000;

    for (int i = from; i < to; i++) {

        u8 s = bBuffer[i];

        // Determine color indices for both playfields
        u8 index1 = (((s & 1) >> 0) | ((s & 4) >> 1) | ((s & 16) >> 2));
        u8 index2 = (((s & 2) >> 1) | ((s & 8) >> 2) | ((s & 32) >> 3));

        if (index1) {
            if (index2) {

                // PF1 is solid, PF2 is solid
                if (pf2pri) {
                    iBuffer[i] = mBuffer[i] = (index2 | 0b1000) & mask2;
                    zBuffer[i] = prio2 | Z_DPF21;
                } else {
                    iBuffer[i] = mBuffer[i] = index1 & mask1;
                    zBuffer[i] = prio1 | Z_DPF12;
                }

            } else {

                // PF1 is solid, PF2 is transparent
                iBuffer[i] = mBuffer[i] = index1 & mask1;
                zBuffer[i] = prio1 | Z_DPF1;
            }

        } else {
            if (index2) {

                // PF1 is transparent, PF2 is solid
                iBuffer[i] = mBuffer[i] = (index2 | 0b1000) & mask2;
                zBuffer[i] = prio2 | Z_DPF2;

            } else {

                // PF1 is transparent, PF2 is transparent
                iBuffer[i] = mBuffer[i] = 0;
                zBuffer[i] = Z_DPF;
            }
        }
    }
}

void
Denise::drawSprites()
{
    if (wasArmed) {
        
        if (wasArmed & 0b11000000) drawSpritePair<3>();
        if (wasArmed & 0b00110000) drawSpritePair<2>();
        if (wasArmed & 0b00001100) drawSpritePair<1>();
        if (wasArmed & 0b00000011) drawSpritePair<0>();
        
        // Record sprite data in debug mode
        if (amiga.getDebugMode()) {
            for (int i = 0; i < 8; i++) {
                if (GET_BIT(wasArmed, i)) recordSpriteData(i);
            }
        }
    }
    
    /* If a sprite was armed, the code above has been executed which means
     * that all recorded register changes have been applied and the relevant
     * sprite registers are all up to date at this time. For unarmed sprites,
     * however, the register change buffers may contain unprocessed entried.
     * We replay those to get the sprite registers up to date.
     */
    if (!sprChanges[3].isEmpty()) replaySpriteRegChanges<3>();
    if (!sprChanges[2].isEmpty()) replaySpriteRegChanges<2>();
    if (!sprChanges[1].isEmpty()) replaySpriteRegChanges<1>();
    if (!sprChanges[0].isEmpty()) replaySpriteRegChanges<0>();
}

template <unsigned pair> void
Denise::drawSpritePair()
{
    assert(pair < 4);

    const unsigned sprite1 = 2 * pair;
    const unsigned sprite2 = 2 * pair + 1;

    int strt1 = 2 * (sprhpos(sprpos[sprite1], sprctl[sprite1]) + 1);
    int strt2 = 2 * (sprhpos(sprpos[sprite2], sprctl[sprite2]) + 1);
    bool armed1 = GET_BIT(initialArmed, sprite1);
    bool armed2 = GET_BIT(initialArmed, sprite2);
    int strt = 0;
    
    // Iterate over all recorded register changes
    if (!sprChanges[pair].isEmpty()) {
        
        int begin = sprChanges[pair].begin();
        int end = sprChanges[pair].end();
        
        for (int i = begin; i != end; i = sprChanges[pair].next(i)) {
            
            Cycle trigger = sprChanges[pair].keys[i];
            RegChange &change = sprChanges[pair].elements[i];
            
            // Draw a chunk of pixels
            drawSpritePair<pair>(strt, trigger, strt1, strt2, armed1, armed2);
            strt = trigger;
            
            // Apply the recorded register change
            switch (change.addr) {
                    
                case REG_SPR0DATA + sprite1:
                    sprdata[sprite1] = change.value;
                    armed1 = true;
                    break;
                    
                case REG_SPR0DATA + sprite2:
                    sprdata[sprite2] = change.value;
                    armed2 = true;
                    break;
                    
                case REG_SPR0DATB + sprite1:
                    sprdatb[sprite1] = change.value;
                    break;
                    
                case REG_SPR0DATB + sprite2:
                    sprdatb[sprite2] = change.value;
                    break;
                    
                case REG_SPR0POS + sprite1:
                    sprpos[sprite1] = change.value;
                    strt1 = 2 * (sprhpos(sprpos[sprite1], sprctl[sprite1]) + 1);
                    break;
                    
                case REG_SPR0POS + sprite2:
                    sprpos[sprite2] = change.value;
                    strt2 = 2 * (sprhpos(sprpos[sprite2], sprctl[sprite2]) + 1);
                    break;
                    
                case REG_SPR0CTL + sprite1:
                    sprctl[sprite1] = change.value;
                    strt1 = 2 * (sprhpos(sprpos[sprite1], sprctl[sprite1]) + 1);
                    armed1 = false;
                    break;
                    
                case REG_SPR0CTL + sprite2:
                    sprctl[sprite2] = change.value;
                    strt2 = 2 * (sprhpos(sprpos[sprite2], sprctl[sprite2]) + 1);
                    armed2 = false;
                    REPLACE_BIT(attach, sprite2, GET_BIT(change.value, 7));
                    break;

                default:
                    assert(false);
            }
        }
    }
    
    // Draw until the end of the line
    drawSpritePair<pair>(strt, sizeof(mBuffer) - 1,
                         strt1, strt2,
                         armed1, armed2);
    
    sprChanges[pair].clear();
}

template <unsigned pair> void
Denise::replaySpriteRegChanges()
{
    assert(pair < 4);
    
    const unsigned sprite1 = 2 * pair;
    const unsigned sprite2 = 2 * pair + 1;
    
    int begin = sprChanges[pair].begin();
    int end = sprChanges[pair].end();
    
    for (int i = begin; i != end; i = sprChanges[pair].next(i)) {
        
        RegChange &change = sprChanges[pair].elements[i];
        
        // Apply the recorded register change
        switch (change.addr) {
                
            case REG_SPR0DATA + sprite1:
                sprdata[sprite1] = change.value;
                break;
                
            case REG_SPR0DATA + sprite2:
                sprdata[sprite2] = change.value;
                break;
                
            case REG_SPR0DATB + sprite1:
                sprdatb[sprite1] = change.value;
                break;
                
            case REG_SPR0DATB + sprite2:
                sprdatb[sprite2] = change.value;
                break;
                
            case REG_SPR0POS + sprite1:
                sprpos[sprite1] = change.value;
                break;
                
            case REG_SPR0POS + sprite2:
                sprpos[sprite2] = change.value;
                break;
                
            case REG_SPR0CTL + sprite1:
                sprctl[sprite1] = change.value;
                break;
                
            case REG_SPR0CTL + sprite2:
                sprctl[sprite2] = change.value;
                REPLACE_BIT(attach, sprite2, GET_BIT(change.value, 7));
                break;
                
            default:
                assert(false);
        }
    }
    
    sprChanges[pair].clear();
}

template <unsigned pair> void
Denise::drawSpritePair(int hstrt, int hstop, int strt1, int strt2, bool armed1, bool armed2)
{
    assert(pair < 4);
    
    const unsigned sprite1 = 2 * pair;
    const unsigned sprite2 = 2 * pair + 1;

    assert(hstrt >= 0 && hstrt <= sizeof(mBuffer));
    assert(hstop >= 0 && hstop <= sizeof(mBuffer));

    for (int hpos = hstrt; hpos < hstop; hpos += 2) {

        if (hpos == strt1 && armed1) {
            ssra[sprite1] = sprdata[sprite1];
            ssrb[sprite1] = sprdatb[sprite1];
        }
        if (hpos == strt2 && armed2) {
            ssra[sprite2] = sprdata[sprite2];
            ssrb[sprite2] = sprdatb[sprite2];
        }

        if (ssra[sprite1] | ssrb[sprite1] | ssra[sprite2] | ssrb[sprite2]) {
            
            if (hpos >= spriteClipBegin && hpos < spriteClipEnd) {
                
                if (attached(sprite2)) {
                    drawAttachedSpritePixelPair<sprite2>(hpos);
                } else {
                    drawSpritePixel<sprite1>(hpos);
                    drawSpritePixel<sprite2>(hpos);
                }
            }
            
            ssra[sprite1] <<= 1;
            ssrb[sprite1] <<= 1;
            ssra[sprite2] <<= 1;
            ssrb[sprite2] <<= 1;
        }
    }

    // Perform collision checks (if enabled)
    if (config.clxSprSpr) {
        checkS2SCollisions<2 * pair>(strt1, strt1 + 31);
        checkS2SCollisions<2 * pair + 1>(strt2, strt2 + 31);
    }
    if (config.clxSprPlf) {
        checkS2PCollisions<2 * pair>(strt1, strt1 + 31);
        checkS2PCollisions<2 * pair + 1>(strt2, strt2 + 31);
    }
}

template <int x> void
Denise::drawSpritePixel(int hpos)
{
    assert(hpos >= spriteClipBegin);
    assert(hpos < spriteClipEnd);

    u8 a = (ssra[x] >> 15);
    u8 b = (ssrb[x] >> 14) & 2;
    u8 col = a | b;

    if (col) {

        u16 z = Z_SP[x];
        int base = 16 + 2 * (x & 6);

        if (z > zBuffer[hpos]) mBuffer[hpos] = base | col;
        if (z > zBuffer[hpos + 1]) mBuffer[hpos + 1] = base | col;
        zBuffer[hpos] |= z;
        zBuffer[hpos + 1] |= z;
    }
}

template <int x> void
Denise::drawAttachedSpritePixelPair(int hpos)
{
    assert(IS_ODD(x));
    assert(hpos >= spriteClipBegin);
    assert(hpos < spriteClipEnd);

    u8 a1 = !!GET_BIT(ssra[x-1], 15);
    u8 b1 = !!GET_BIT(ssrb[x-1], 15) << 1;
    u8 a2 = !!GET_BIT(ssra[x], 15) << 2;
    u8 b2 = !!GET_BIT(ssrb[x], 15) << 3;
    assert(a1 == ((ssra[x-1] >> 15)));
    assert(b1 == ((ssrb[x-1] >> 14) & 0b0010));
    assert(a2 == ((ssra[x] >> 13) & 0b0100));
    assert(b2 == ((ssrb[x] >> 12) & 0b1000));

    u8 col = a1 | b1 | a2 | b2;

    if (col) {

        u16 z = Z_SP[x];

        if (z > zBuffer[hpos]) {
            mBuffer[hpos] = 0b10000 | col;
            zBuffer[hpos] |= z;
        }
        if (z > zBuffer[hpos+1]) {
            mBuffer[hpos+1] = 0b10000 | col;
            zBuffer[hpos+1] |= z;
        }
    }
}

void
Denise::drawBorder()
{
    int borderL = 0;
    int borderR = 0;
    int borderV = 0;

#ifdef BORDER_DEBUG
    borderL = 64;
    borderR = 65;
    borderV = 66;
#endif

    // Check if the horizontal flipflop was set somewhere in this rasterline
    bool hFlopWasSet = agnus.diwHFlop || agnus.diwHFlopOn != -1;

    // Check if the whole line is blank (drawn in background color)
    bool lineIsBlank = !agnus.diwVFlop || !hFlopWasSet;

    // Draw the border
    if (lineIsBlank) {

        for (int i = 0; i <= LAST_PIXEL; i++) {
            iBuffer[i] = mBuffer[i] = borderV;
        }

    } else {

        // Draw left border
        if (!agnus.diwHFlop && agnus.diwHFlopOn != -1) {
            for (int i = 0; i < 2 * agnus.diwHFlopOn; i++) {
                assert(i < sizeof(iBuffer));
                iBuffer[i] = mBuffer[i] = borderL;
            }
        }

        // Draw right border
        if (agnus.diwHFlopOff != -1) {
            for (int i = 2 * agnus.diwHFlopOff; i <= LAST_PIXEL; i++) {
                assert(i < sizeof(iBuffer));
                iBuffer[i] = mBuffer[i] = borderR;
            }
        }
    }

#ifdef LINE_DEBUG
    if (LINE_DEBUG) {
        for (int i = 0; i <= LAST_PIXEL / 2; i++) {
            iBuffer[i] = mBuffer[i] = 64;
        }
    }
#endif
}

template <int x> void
Denise::checkS2SCollisions(int start, int end)
{
    // For the odd sprites, only proceed if collision detection is enabled
    if (IS_ODD(x) && !GET_BIT(clxcon, 12 + (x/2))) return;

    // Set up the sprite comparison masks
    u16 comp01 = Z_SP0 | (GET_BIT(clxcon, 12) ? Z_SP1 : 0);
    u16 comp23 = Z_SP2 | (GET_BIT(clxcon, 13) ? Z_SP3 : 0);
    u16 comp45 = Z_SP4 | (GET_BIT(clxcon, 14) ? Z_SP5 : 0);
    u16 comp67 = Z_SP6 | (GET_BIT(clxcon, 15) ? Z_SP7 : 0);

    // Iterate over all sprite pixels
    for (int pos = end; pos >= start; pos -= 2) {

        u16 z = zBuffer[pos];
        
        // Skip if there are no other sprites at this pixel coordinate
        if (!(z & (Z_SP01234567 ^ Z_SP[x]))) continue;

        // Skip if the sprite is transparent at this pixel coordinate
        if (!(z & Z_SP[x])) continue;

        // Set sprite collision bits
        if ((z & comp45) && (z & comp67)) SET_BIT(clxdat, 14);
        if ((z & comp23) && (z & comp67)) SET_BIT(clxdat, 13);
        if ((z & comp23) && (z & comp45)) SET_BIT(clxdat, 12);
        if ((z & comp01) && (z & comp67)) SET_BIT(clxdat, 11);
        if ((z & comp01) && (z & comp45)) SET_BIT(clxdat, 10);
        if ((z & comp01) && (z & comp23)) SET_BIT(clxdat, 9);

        if (CLX_DEBUG) {
            if ((z & comp45) && (z & comp67)) debug("Collision between 45 and 67\n");
            if ((z & comp23) && (z & comp67)) debug("Collision between 23 and 67\n");
            if ((z & comp23) && (z & comp45)) debug("Collision between 23 and 45\n");
            if ((z & comp01) && (z & comp67)) debug("Collision between 01 and 67\n");
            if ((z & comp01) && (z & comp45)) debug("Collision between 01 and 45\n");
            if ((z & comp01) && (z & comp23)) debug("Collision between 01 and 23\n");
        }
    }
}

template <int x> void
Denise::checkS2PCollisions(int start, int end)
{
    debug(CLX_DEBUG, "checkS2PCollisions<%d>(%d, %d)\n", x, start, end);
    
    // For the odd sprites, only proceed if collision detection is enabled
    if (IS_ODD(x) && !getENSP<x>()) return;

    // Set up the sprite comparison mask
    u16 sprMask;
    switch(x) {
        case 0:
        case 1: sprMask = Z_SP0 | (getENSP<1>() ? Z_SP1 : 0); break;
        case 2:
        case 3: sprMask = Z_SP2 | (getENSP<3>() ? Z_SP3 : 0); break;
        case 4:
        case 5: sprMask = Z_SP4 | (getENSP<5>() ? Z_SP5 : 0); break;
        case 6:
        case 7: sprMask = Z_SP6 | (getENSP<7>() ? Z_SP7 : 0); break;

        default: sprMask = 0; assert(false);
    }

    u8 enabled1 = getENBP1();
    u8 enabled2 = getENBP2();
    u8 compare1 = getMVBP1() & enabled1;
    u8 compare2 = getMVBP2() & enabled2;

    // Check for sprite-playfield collisions
    for (int pos = end; pos >= start; pos -= 2) {

        u16 z = zBuffer[pos];

        // Skip if the sprite is transparent at this pixel coordinate
        if (!(z & Z_SP[x])) continue;

        // debug(CLX_DEBUG, "<%d> b[%d] = %X e1 = %X e2 = %X, c1 = %X c2 = %X\n",
        //     x, pos, bBuffer[pos], enabled1, enabled2, compare1, compare2);

        // Check for a collision with playfield 2
        if ((bBuffer[pos] & enabled2) == compare2) {
            debug(CLX_DEBUG, "S%d collides with PF2\n", x);
            SET_BIT(clxdat, 5 + (x / 2));
            SET_BIT(clxdat, 1 + (x / 2));

        } else {
            // There is a hardware oddity in single-playfield mode. If PF2
            // doesn't match, PF1 doesn't match either. No matter what.
            // See http://eab.abime.net/showpost.php?p=965074&postcount=2
            if (zBuffer[pos] & Z_DUAL) continue;
        }

        // Check for a collision with playfield 1
        if ((bBuffer[pos] & enabled1) == compare1) {
            debug(CLX_DEBUG, "S%d collides with PF1\n", x);
            SET_BIT(clxdat, 1 + (x / 2));
        }
    }
}

void
Denise::checkP2PCollisions()
{
    // Quick-exit if the collision bit already set
    if (GET_BIT(clxdat, 0)) return;

    // Set up comparison masks
    u8 enabled1 = getENBP1();
    u8 enabled2 = getENBP2();
    u8 compare1 = getMVBP1() & enabled1;
    u8 compare2 = getMVBP2() & enabled2;

    // Check all pixels one by one
    for (int pos = 0; pos < HPIXELS; pos++) {

        u16 b = bBuffer[pos];
        // debug(CLX_DEBUG, "b[%d] = %X e1 = %X e2 = %X c1 = %X c2 = %X\n",
        //       pos, b, enabled1, enabled2, compare1, compare2);

        // Check if there is a hit with playfield 1
        if ((b & enabled1) != compare1) continue;

        // Check if there is a hit with playfield 2
        if ((b & enabled2) != compare2) continue;

        // Set collision bit
        SET_BIT(clxdat, 0);
        return;
    }
}

void
Denise::beginOfFrame(bool interlace)
{
    pixelEngine.beginOfFrame(interlace);
    
    if (amiga.getDebugMode()) {
        
        for (int i = 0; i < 8; i++) {
            latchedSpriteInfo[i] = spriteInfo[i];
            spriteInfo[i].height = 0;
            spriteInfo[i].vstrt  = 0;
            spriteInfo[i].vstop  = 0;
            spriteInfo[i].hstrt  = 0;
            spriteInfo[i].attach = false;
        }
    }
}

void
Denise::beginOfLine(int vpos)
{
    // Reset the register change recorders
    conChanges.clear();
    pixelEngine.colChanges.clear();
    
    // Save the current values of various Denise registers
    initialBplcon0 = bplcon0;
    initialBplcon1 = bplcon1;
    initialBplcon2 = bplcon2;
    initialArmed = armed;
    wasArmed = armed;

    // Prepare the bitplane shift registers
    for (int i = 0; i < 6; i++) shiftReg[i] = 0;

    // Clear the bBuffer
    memset(bBuffer, 0, sizeof(bBuffer));

    // Reset the sprite clipping range
    spriteClipBegin = HPIXELS;
    spriteClipEnd = HPIXELS;
}

void
Denise::endOfLine(int vpos)
{
    // debug("endOfLine pixel = %d HPIXELS = %d\n", pixel, HPIXELS);

    // Check if we are below the VBLANK area
    if (vpos >= 26) {

        // Translate bitplane data to color register indices
        translate();

        // Draw sprites if at least one is armed
        drawSprites();

        // Draw border pixels
        drawBorder();

        // Perform playfield-playfield collision check (if enabled)
        if (config.clxPlfPlf) checkP2PCollisions();
        
        // Synthesize RGBA values and write the result into the frame buffer
        pixelEngine.colorize(vpos);

        // Remove certain graphics layers if requested
        if (config.hiddenLayers) {
            pixelEngine.hide(vpos, config.hiddenLayers, config.hiddenLayerAlpha);
        }
    } else {
        pixelEngine.endOfVBlankLine();
    }

    // Invoke the DMA debugger
    dmaDebugger.computeOverlay();
    
    // Encode a HIRES / LORES marker in the first HBLANK pixel
    *denise.pixelEngine.pixelAddr(HBLANK_MIN * 4) = hires() ? 0 : -1;
}

void
Denise::pokeDMACON(u16 oldValue, u16 newValue)
{
    if (Agnus::bpldma(newValue)) {

        // Bitplane DMA on
        debug(DMA_DEBUG, "Bitplane DMA switched on\n");

    } else {

        // Bitplane DMA off
        debug(DMA_DEBUG, "Bitplane DMA switched off\n");
    }
}

void
Denise::recordSpriteData(unsigned nr)
{
    assert(nr < 8);

    u16 line = spriteInfo[nr].height;

    // Record data registers
    spriteInfo[nr].data[line] = HI_W_LO_W(sprdatb[nr], sprdata[nr]);

    // Record additional information in sprite line 0
    if (line == 0) {
        
        spriteInfo[nr].hstrt = sprhpos(sprpos[nr], sprctl[nr]);
        spriteInfo[nr].vstrt = agnus.sprVStrt[nr];
        spriteInfo[nr].vstop = agnus.sprVStop[nr];
        spriteInfo[nr].attach = IS_ODD(nr) ? attached(nr) : 0;
        
        for (int i = 0; i < 16; i++) {
            spriteInfo[nr].colors[i] = pixelEngine.getColor(i + 16);
        }
    }
    
    spriteInfo[nr].height = (line + 1) % VPOS_CNT;
}

void
Denise::dumpBuffer(u8 *buffer, size_t length)
{
    const size_t cols = 16;

    for (int i = 0; i < (length + cols - 1) / cols; i++) {
        for (int j =0; j < cols; j++) msg("%2d ", buffer[i * cols + j]);
        msg("\n");
    }
}

template void Denise::pokeBPLxDAT<0>(u16 value);
template void Denise::pokeBPLxDAT<1>(u16 value);
template void Denise::pokeBPLxDAT<2>(u16 value);
template void Denise::pokeBPLxDAT<3>(u16 value);
template void Denise::pokeBPLxDAT<4>(u16 value);
template void Denise::pokeBPLxDAT<5>(u16 value);

template void Denise::pokeSPRxPOS<0>(u16 value);
template void Denise::pokeSPRxPOS<1>(u16 value);
template void Denise::pokeSPRxPOS<2>(u16 value);
template void Denise::pokeSPRxPOS<3>(u16 value);
template void Denise::pokeSPRxPOS<4>(u16 value);
template void Denise::pokeSPRxPOS<5>(u16 value);
template void Denise::pokeSPRxPOS<6>(u16 value);
template void Denise::pokeSPRxPOS<7>(u16 value);

template void Denise::pokeSPRxCTL<0>(u16 value);
template void Denise::pokeSPRxCTL<1>(u16 value);
template void Denise::pokeSPRxCTL<2>(u16 value);
template void Denise::pokeSPRxCTL<3>(u16 value);
template void Denise::pokeSPRxCTL<4>(u16 value);
template void Denise::pokeSPRxCTL<5>(u16 value);
template void Denise::pokeSPRxCTL<6>(u16 value);
template void Denise::pokeSPRxCTL<7>(u16 value);

template void Denise::pokeSPRxDATA<0>(u16 value);
template void Denise::pokeSPRxDATA<1>(u16 value);
template void Denise::pokeSPRxDATA<2>(u16 value);
template void Denise::pokeSPRxDATA<3>(u16 value);
template void Denise::pokeSPRxDATA<4>(u16 value);
template void Denise::pokeSPRxDATA<5>(u16 value);
template void Denise::pokeSPRxDATA<6>(u16 value);
template void Denise::pokeSPRxDATA<7>(u16 value);

template void Denise::pokeSPRxDATB<0>(u16 value);
template void Denise::pokeSPRxDATB<1>(u16 value);
template void Denise::pokeSPRxDATB<2>(u16 value);
template void Denise::pokeSPRxDATB<3>(u16 value);
template void Denise::pokeSPRxDATB<4>(u16 value);
template void Denise::pokeSPRxDATB<5>(u16 value);
template void Denise::pokeSPRxDATB<6>(u16 value);
template void Denise::pokeSPRxDATB<7>(u16 value);

template void Denise::pokeCOLORxx<POKE_CPU, 0>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 0>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 1>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 1>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 2>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 2>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 3>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 3>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 4>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 4>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 5>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 5>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 6>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 6>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 7>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 7>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 8>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 8>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 9>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 9>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 10>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 10>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 11>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 11>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 12>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 12>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 13>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 13>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 14>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 14>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 15>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 15>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 16>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 16>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 17>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 17>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 18>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 18>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 19>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 19>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 20>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 20>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 21>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 21>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 22>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 22>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 23>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 23>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 24>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 24>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 25>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 25>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 26>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 26>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 27>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 27>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 28>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 28>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 29>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 29>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 30>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 30>(u16 value);
template void Denise::pokeCOLORxx<POKE_CPU, 31>(u16 value);
template void Denise::pokeCOLORxx<POKE_COPPER, 31>(u16 value);

template void Denise::drawOdd<false>(int offset);
template void Denise::drawOdd<true>(int offset);
template void Denise::drawEven<false>(int offset);
template void Denise::drawEven<true>(int offset);

template void Denise::translateDPF<true>(int from, int to);
template void Denise::translateDPF<false>(int from, int to);
