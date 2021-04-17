// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "DmaDebugger.h"
#include "Agnus.h"
#include "Bus.h"
#include "Denise.h"
#include "MsgQueue.h"
#include "PixelEngine.h"

DmaDebugger::DmaDebugger(Amiga &ref) : AmigaComponent(ref)
{
}

void
DmaDebugger::_initialize()
{
    config.enabled = false;

    // Visualize all channels by default except the CPU channel
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_CPU,      false);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_REFRESH,  true);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_DISK,     true);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_AUDIO,    true);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_BITPLANE, true);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_SPRITE,   true);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_COPPER,   true);
    setConfigItem(OPT_DMA_DEBUG_ENABLE, DMA_CHANNEL_BLITTER,  true);
    
    // Assign default colors
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_CPU,      0xFFFFFF00);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_REFRESH,  0xFF000000);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_DISK,     0x00FF0000);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_AUDIO,    0xFF00FF00);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_BITPLANE, 0x00FFFF00);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_SPRITE,   0x0088FF00);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_COPPER,   0xFFFF0000);
    setConfigItem(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_BLITTER,  0xFFCC0000);
 
    config.displayMode = DMA_DISPLAY_MODE_FG_LAYER;
    config.opacity = 50;
}

i64
DmaDebugger::getConfigItem(Option option) const
{
    switch (option) {
            
        case OPT_DMA_DEBUG_ENABLE:  return config.enabled;
        case OPT_DMA_DEBUG_MODE:    return config.displayMode;
        case OPT_DMA_DEBUG_OPACITY: return config.opacity;
                        
        default:
            assert(false);
            return 0;
    }
}

i64
DmaDebugger::getConfigItem(Option option, long id) const
{
    assert(id >= 0 && id < BUS_COUNT);
    
    switch (option) {
            
        case OPT_DMA_DEBUG_ENABLE: return config.visualize[id];
        case OPT_DMA_DEBUG_COLOR:  return config.debugColor[id];
                        
        default:
            assert(false);
            return 0;
    }
}

bool
DmaDebugger::setConfigItem(Option option, i64 value)
{
    switch (option) {
                                    
        case OPT_DMA_DEBUG_ENABLE:
            
            if (config.enabled == value) {
                return false;
            }
            
            config.enabled = value;
            messageQueue.put(value ? MSG_DMA_DEBUG_ON : MSG_DMA_DEBUG_OFF);
            return true;
            
        case OPT_DMA_DEBUG_MODE:
            
            if (!DmaDisplayModeEnum::isValid(value)) {
                throw ConfigArgError(DmaDisplayModeEnum::keyList());
            }
            if (config.displayMode == value) {
                return false;
            }
            
            config.displayMode = (DmaDisplayMode)value;
            return true;

        case OPT_DMA_DEBUG_OPACITY:
            
            if (config.opacity == value) {
                return false;
            }
            
            config.opacity = value;
            return true;

        default:
            return false;
    }
}

bool
DmaDebugger::setConfigItem(Option option, long id, i64 value)
{
    if (!DmaChannelEnum::isValid(id)) { return false; }
    DmaChannel channel = (DmaChannel)id;
    
    switch (option) {
                                    
        case OPT_DMA_DEBUG_ENABLE:
            
            config.visualize[channel] = value;

            switch(channel) {
                                        
                case DMA_CHANNEL_CPU:
                    
                    visualize[BUS_CPU] = value;
                    return true;
                    
                case DMA_CHANNEL_REFRESH:
                    
                    visualize[BUS_REFRESH] = value;
                    return true;

                case DMA_CHANNEL_DISK:
                    
                    visualize[BUS_DISK] = value;
                    return true;

                case DMA_CHANNEL_AUDIO:

                    visualize[BUS_AUDIO] = value;
                    return true;

                case DMA_CHANNEL_COPPER:

                    visualize[BUS_COPPER] = value;
                    return true;

                case DMA_CHANNEL_BLITTER:

                    visualize[BUS_BLITTER] = value;
                    return true;
        
                case DMA_CHANNEL_BITPLANE:

                    visualize[BUS_BPL1] = value;
                    visualize[BUS_BPL2] = value;
                    visualize[BUS_BPL3] = value;
                    visualize[BUS_BPL4] = value;
                    visualize[BUS_BPL5] = value;
                    visualize[BUS_BPL6] = value;
                    return true;

                case DMA_CHANNEL_SPRITE:

                    visualize[BUS_SPRITE0] = value;
                    visualize[BUS_SPRITE1] = value;
                    visualize[BUS_SPRITE2] = value;
                    visualize[BUS_SPRITE3] = value;
                    visualize[BUS_SPRITE4] = value;
                    visualize[BUS_SPRITE5] = value;
                    visualize[BUS_SPRITE6] = value;
                    visualize[BUS_SPRITE7] = value;
                    return true;
                                        
                default:
                    return false;
            }

        case OPT_DMA_DEBUG_COLOR:
            
            config.debugColor[channel] = (u32)value;

            switch(channel) {
                                        
                case DMA_CHANNEL_CPU:
                    
                    setColor(BUS_CPU, (u32)value);
                    return true;
                    
                case DMA_CHANNEL_REFRESH:

                    setColor(BUS_REFRESH, (u32)value);
                    return true;

                case DMA_CHANNEL_DISK:
                    
                    setColor(BUS_DISK, (u32)value);
                    return true;

                case DMA_CHANNEL_AUDIO:

                    setColor(BUS_AUDIO, (u32)value);
                    return true;

                case DMA_CHANNEL_COPPER:

                    setColor(BUS_COPPER, (u32)value);
                    return true;

                case DMA_CHANNEL_BLITTER:

                    setColor(BUS_BLITTER, (u32)value);
                    return true;
        
                case DMA_CHANNEL_BITPLANE:

                    setColor(BUS_BPL1, (u32)value);
                    setColor(BUS_BPL2, (u32)value);
                    setColor(BUS_BPL3, (u32)value);
                    setColor(BUS_BPL4, (u32)value);
                    setColor(BUS_BPL5, (u32)value);
                    setColor(BUS_BPL6, (u32)value);
                    return true;

                case DMA_CHANNEL_SPRITE:

                    setColor(BUS_SPRITE0, (u32)value);
                    setColor(BUS_SPRITE1, (u32)value);
                    setColor(BUS_SPRITE2, (u32)value);
                    setColor(BUS_SPRITE3, (u32)value);
                    setColor(BUS_SPRITE4, (u32)value);
                    setColor(BUS_SPRITE5, (u32)value);
                    setColor(BUS_SPRITE6, (u32)value);
                    setColor(BUS_SPRITE7, (u32)value);
                    return true;
                                        
                default:
                    return false;
            }
        default:
            return false;
    }
}

DMADebuggerInfo
DmaDebugger::getInfo()
{
    DMADebuggerInfo result;
    
    synchronized {
                
        result.visualizeCopper = config.visualize[DMA_CHANNEL_COPPER];
        result.visualizeBlitter = config.visualize[DMA_CHANNEL_BLITTER];
        result.visualizeDisk = config.visualize[DMA_CHANNEL_DISK];
        result.visualizeAudio = config.visualize[DMA_CHANNEL_AUDIO];
        result.visualizeSprites = config.visualize[DMA_CHANNEL_SPRITE];
        result.visualizeBitplanes = config.visualize[DMA_CHANNEL_BITPLANE];
        result.visualizeCpu = config.visualize[DMA_CHANNEL_CPU];
        result.visualizeRefresh = config.visualize[DMA_CHANNEL_REFRESH];
        
        getColor(DMA_CHANNEL_COPPER, result.copperColor);
        getColor(DMA_CHANNEL_BLITTER, result.blitterColor);
        getColor(DMA_CHANNEL_DISK, result.diskColor);
        getColor(DMA_CHANNEL_AUDIO, result.audioColor);
        getColor(DMA_CHANNEL_SPRITE, result.spriteColor);
        getColor(DMA_CHANNEL_BITPLANE, result.bitplaneColor);
        getColor(DMA_CHANNEL_CPU, result.cpuColor);
        getColor(DMA_CHANNEL_REFRESH, result.refreshColor);
    }

    return result;
}

void
DmaDebugger::getColor(DmaChannel channel, double *rgb)
{
    assert_enum(DmaChannel, channel);
    
    RgbColor color = RgbColor(config.debugColor[channel]);
    rgb[0] = color.r;
    rgb[1] = color.g;
    rgb[2] = color.b;
}

void
DmaDebugger::setColor(BusOwner owner, u32 rgba)
{
    assert_enum(BusOwner, owner);
                          
    // Compute the color variants used for drawing
    RgbColor color = RgbColor(rgba);
    debugColor[owner][0] = color.shade(0.3);
    debugColor[owner][1] = color.shade(0.1);
    debugColor[owner][2] = color.tint(0.1);
    debugColor[owner][3] = color.tint(0.3);
}

void
DmaDebugger::computeOverlay()
{
    // Only proceed if DMA debugging has been turned on
    if (!config.enabled) return;

    BusOwner *owners = agnus.busOwner;
    u16 *values = agnus.busValue;
    u32 *ptr = denise.pixelEngine.pixelAddr(0);

    double opacity = config.opacity / 100.0;
    double bgWeight = 0;
    double fgWeight = 0;

    switch (config.displayMode) {

        case DMA_DISPLAY_MODE_FG_LAYER:

            bgWeight = 0.0;
            fgWeight = 1.0 - opacity;
            break;

        case DMA_DISPLAY_MODE_BG_LAYER:

            bgWeight = 1.0 - opacity;
            fgWeight = 0.0;
            break;

        case DMA_DISPLAY_MODE_ODD_EVEN_LAYERS:

            bgWeight = opacity;
            fgWeight = 1.0 - opacity;
            break;

        default: assert(false);

    }

    for (isize i = 0; i < HPOS_CNT; i++, ptr += 4) {

        BusOwner owner = owners[i];

        // Handle the easy case first: No foreground pixels
        if (!visualize[owner]) {

            if (bgWeight != 0.0) {
                ptr[0] = GpuColor(ptr[0]).shade(bgWeight).rawValue;
                ptr[1] = GpuColor(ptr[1]).shade(bgWeight).rawValue;
                ptr[2] = GpuColor(ptr[2]).shade(bgWeight).rawValue;
                ptr[3] = GpuColor(ptr[3]).shade(bgWeight).rawValue;
            }
            continue;
        }

        // Get RGBA values of foreground pixels
        GpuColor col0 = debugColor[owner][(values[i] & 0xC000) >> 14];
        GpuColor col1 = debugColor[owner][(values[i] & 0x0C00) >> 10];
        GpuColor col2 = debugColor[owner][(values[i] & 0x00C0) >> 6];
        GpuColor col3 = debugColor[owner][(values[i] & 0x000C) >> 2];

        if (fgWeight != 0.0) {
            col0 = col0.mix(GpuColor(ptr[0]), fgWeight);
            col1 = col1.mix(GpuColor(ptr[1]), fgWeight);
            col2 = col2.mix(GpuColor(ptr[2]), fgWeight);
            col3 = col3.mix(GpuColor(ptr[3]), fgWeight);
        }

        ptr[0] = col0.rawValue;
        ptr[1] = col1.rawValue;
        ptr[2] = col2.rawValue;
        ptr[3] = col3.rawValue;
    }
}

void
DmaDebugger::vSyncHandler()
{
    // Only proceed if the debugger is enabled
    if (!config.enabled) return;

    // Clear old data in the next frame's VBLANK area
    u32 *ptr = denise.pixelEngine.frameBuffer->data;
    for (isize row = 0; row < VBLANK_CNT; row++) {
        for (isize col = 0; col <= LAST_PIXEL; col++) {
            ptr[row * HPIXELS + col] = PixelEngine::rgbaVBlank;
        }
    }
}
