// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _COLORIZER_INC
#define _COLORIZER_INC

#include "HardwareComponent.h"
#include "RegisterChange.h"

class PixelEngine : public HardwareComponent {

    friend class DmaDebugger;
    
public:

    // Quick-access references
    class Agnus *agnus;
    class Denise *denise;

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

    // zBuffer to implement the sprite / playfield display hierarchy
    int8_t zBuffer[HPOS_CNT];


    //
    // Color management
    //

    // The 32 Amiga color registers
    uint16_t colreg[32];

    // RGBA values for all possible 4096 Amiga colors
    uint32_t rgba[4096];

    /* The color register values translated to RGBA
     * Note that the number of elements exceeds the number of color registers:
     *  0 .. 31: RGBA values of the 32 color registers.
     * 32 .. 63: RGBA values of the 32 color registers in halfbright mode.
     * 64 .. 71: Additional colors used for debugging
     */
    static const int rgbaIndexCnt = 32 + 32 + 8;
    uint32_t indexedRgba[rgbaIndexCnt];

    // Color adjustment parameters
    Palette palette = COLOR_PALETTE;
    double brightness = 50.0;
    double contrast = 100.0;
    double saturation = 1.25;


    // The current drawing mode
    DrawingMode mode;
    

    //
    // Register change history
    //

    ChangeHistory colorRegHistory;

    /* Recorded register changes
     * The change history is recorded by Denise and reset at the end of each
     * scanline.
     * DEPRECATED
     */
    RegisterChange changeHistory[128];

    // Number of recorded register changes
    // DEPRECATED
    int changeCount = 0;

    //
    // Playfield priorities
    //

    // Priority of playfield 1 (derived from bit PF1P2 - PF1P0 in BPLCON2)
    uint8_t prio1;

    // Priority of playfield 2 (derived from bit PF2P2 - PF2P0 in BPLCON2)
    uint8_t prio2;

    // Minimum of pf1pri and pf2pri
    uint8_t prioMin;


    //
    // Constructing and destructing
    //
    
public:
    
    PixelEngine();


    //
    // Iterating over snapshot items
    //

    template <class T>
    void applyToPersistentItems(T& worker)
    {
    }

    template <class T>
    void applyToResetItems(T& worker)
    {
        worker

        & colorRegHistory
        & colreg
        & mode
        & changeHistory
        & changeCount
        & prio1
        & prio2
        & prioMin;
    }


    //
    // Methods from HardwareComponent
    //
    
private:

    void _initialize() override;
    void _powerOn() override;
    void _reset() override { RESET_SNAPSHOT_ITEMS }
    size_t _size() override { COMPUTE_SNAPSHOT_SIZE }
    size_t _load(uint8_t *buffer) override { LOAD_SNAPSHOT_ITEMS }
    size_t _save(uint8_t *buffer) override { SAVE_SNAPSHOT_ITEMS }

    
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
    bool isRgbaIndex(int nr) { return nr < rgbaIndexCnt; }
    
    // Changes one of the 32 Amiga color registers.
    void setColor(int reg, uint16_t value);

    // Returns a color value in Amiga format or RGBA format
    uint16_t getColor(int nr) { assert(nr < 32); return colreg[nr]; }
    uint32_t getRGBA(int nr) { assert(nr < 32); return indexedRgba[nr]; }

    // Returns sprite color in Amiga format or RGBA format
    uint16_t getSpriteColor(int s, int nr) { assert(s < 8); return getColor(16 + nr + 2 * (s & 6)); }
    uint32_t getSpriteRGBA(int s, int nr) { return rgba[getSpriteColor(s,nr)]; }


    //
    // Using the color lookup table
    //

private:

    // Updates the entire RGBA lookup table
    void updateRGBA();

    // Adjusts the RGBA value according to the selected color parameters
    void adjustRGB(uint8_t &r, uint8_t &g, uint8_t &b);


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

    // Returns the frame buffer address of a certain pixel in the current line
    int *pixelAddr(int pixel);

    // Called after each frame to switch the frame buffers
    // void prepareForNextFrame(bool lf, bool interlace);
    void prepareForNextFrame(bool interlace);


    //
    // Working with recorded register changes
    //

public:

    // Records a color register change to be processed in translateToRGBA()
    void recordRegisterChange(uint32_t addr, uint16_t value, int16_t pixel);

    // Applies a register change
    void applyRegisterChange(const RegisterChange &change);


    //
    // Synthesizing pixels
    //

public:

    // Translates bitplane data to RGBA values
    void translateToRGBA(uint8_t *src, int line);

    /* Draws a chunk of pixels.
     * There are three variants of this function:
     *
     *     drawSP:  Draws in single-playfield mode
     *     drawDP:  Draws in dual-playfield mode
     *     drawHAM: Draws in HAM mode
     *
     * Parameters:
     *
     *     src:     Pointer to a buffer storing color indices
     *     dst:     Pointer to a buffer storing RGBA values
     *     from:    First pixel to draw
     *     to:      Last pixel to draw + 1
     *
     * Side effects:
     *
     *     The source buffer is cleared (0 is written)
     */
    void drawSPF(uint8_t *src, int *dst, int from, int to);
    void drawDPF(uint8_t *src, int *dst, int from, int to);
    template <bool pf2pri> void drawDPF(uint8_t *src, int *dst, int from, int to);
    void drawHAM(uint8_t *src, int *dst, int from, int to, uint16_t& ham);

};

#endif
