// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _COLORIZER_H
#define _COLORIZER_H

#include "AmigaComponent.h"

class PixelEngine : public AmigaComponent {

    friend class DmaDebugger;
    
public:

    // RGBA colors used to visualize the HBLANK and VBLANK area in the debugger
    static const i32 rgbaHBlank = 0xFF444444;
    static const i32 rgbaVBlank = 0xFF444444;

private:

    //
    // Screen buffers
    //

    /* We keep four frame buffers, two for storing long frames and
     * another two for storing short frames. The short frame buffers are only
     * used in interlace mode. At each point in time, one of the two buffers
     * is the "working buffer" and the other one the "stable buffer". All
     * drawing functions write to the working buffers, only. The GPU reads from
     * the stable buffers, only. Once a frame has been completed, the working
     * buffer and the stable buffer are switched.
     */
    ScreenBuffer longFrame[2];
    ScreenBuffer shortFrame[2];

    // Pointers to the working buffers
    ScreenBuffer *workingLongFrame = &longFrame[0];
    ScreenBuffer *workingShortFrame = &shortFrame[0];

    // Pointers to the stable buffers
    ScreenBuffer *stableLongFrame = &longFrame[0];
    ScreenBuffer *stableShortFrame = &shortFrame[0];

    // Pointer to the frame buffer Denise is currently working on
    ScreenBuffer *frameBuffer = &longFrame[0];

    // Buffer storing background noise (random black and white pixels)
    u32 *noise;

    
    //
    // Color management
    //

    // The 32 Amiga color registers
    u16 colreg[32];

    // RGBA values for all possible 4096 Amiga colors
    u32 rgba[4096];

    /* The color register values translated to RGBA
     * Note that the number of elements exceeds the number of color registers:
     *  0 .. 31: RGBA values of the 32 color registers.
     * 32 .. 63: RGBA values of the 32 color registers in halfbright mode.
     * 64 .. 71: Additional colors used for debugging
     */
    static const int rgbaIndexCnt = 32 + 32 + 8;
    u32 indexedRgba[rgbaIndexCnt];

    // Color adjustment parameters
    Palette palette = COLOR_PALETTE;
    double brightness = 50.0;
    double contrast = 100.0;
    double saturation = 1.25;
    
    // Indicates whether HAM mode is switched
    bool hamMode;
    
    
    //
    // Register change history buffer
    //

public:

    // Color register history
    RegChangeRecorder<128> colChanges;


     //
    // Constructing and serializing
    //
    
public:
    
    PixelEngine(Amiga& ref);
    ~PixelEngine();

    template <class T>
    void applyToPersistentItems(T& worker)
    {
    }

    template <class T>
    void applyToResetItems(T& worker)
    {
        worker

        & colChanges
        & colreg
        & hamMode;
    }


    //
    // Methods from HardwareComponent
    //
    
private:

    void _powerOn() override;
    void _reset(bool hard) override;
    size_t _size() override { COMPUTE_SNAPSHOT_SIZE }
    size_t _load(u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    size_t _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }

    
    //
    // Configuring the color palette
    //
    
public:
    
    Palette getPalette() { return palette; }
    void setPalette(Palette p);
    
    double getBrightness() { return brightness; }
    void setBrightness(double value);
    
    double getSaturation() { return saturation; }
    void setSaturation(double value);
    
    double getContrast() { return contrast; }
    void setContrast(double value);


    //
    // Accessing color registers
    //

public:

    // Performs a consistency check for debugging.
    static bool isRgbaIndex(int nr) { return nr < rgbaIndexCnt; }
    
    // Changes one of the 32 Amiga color registers.
    void setColor(int reg, u16 value);

    // Returns a color value in Amiga format or RGBA format
    u16 getColor(int nr) { assert(nr < 32); return colreg[nr]; }
    u32 getRGBA(int nr) { assert(nr < 32); return indexedRgba[nr]; }

    // Returns sprite color in Amiga format or RGBA format
    u16 getSpriteColor(int s, int nr) { assert(s < 8); return getColor(16 + nr + 2 * (s & 6)); }
    u32 getSpriteRGBA(int s, int nr) { return rgba[getSpriteColor(s,nr)]; }


    //
    // Using the color lookup table
    //

private:

    // Updates the entire RGBA lookup table
    void updateRGBA();

    // Adjusts the RGBA value according to the selected color parameters
    void adjustRGB(u8 &r, u8 &g, u8 &b);


    //
    // Working with frame buffers
    //

private:

    // Return true if buffer points to a long frame screen buffer
    bool isLongFrame(ScreenBuffer *buf);

    // Return true if buffer points to a short frame screen buffer
    bool isShortFrame(ScreenBuffer *buf);

public:

    // Returns the stable frame buffer for long frames
    ScreenBuffer getStableLongFrame();

    // Returns the stable frame buffer for short frames
    ScreenBuffer getStableShortFrame();

    // Returns a pointer to randon noise
    u32 *getNoise();
    
    // Returns the frame buffer address of a certain pixel in the current line
    u32 *pixelAddr(int pixel);

    // Called after each line in the VBLANK area
    void endOfVBlankLine();

    // Called after each frame to switch the frame buffers
    void beginOfFrame(bool interlace);


    //
    // Working with recorded register changes
    //

public:

    // Applies a register change
    void applyRegisterChange(const RegChange &change);


    //
    // Synthesizing pixels
    //

public:
    
    /* Colorizes a rasterline.
     * This function implements the last stage in the emulator's graphics
     * pipelile. It translates a line of color register indices into a line
     * of RGBA values in GPU format.
     */
    void colorize(int line);
    
private:
    
    void colorize(u32 *dst, int from, int to);
    void colorizeHAM(u32 *dst, int from, int to, u16& ham);
    
    /* Hides some graphics layers.
     * This function is an optional stage applied after colorize(). It can
     * be used to hide some layers for debugging.
     */
    
public:
    
    void hide(int line, u16 layer, u8 alpha);
};

#endif
