// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _DENISE_INC
#define _DENISE_INC

#include "HardwareComponent.h"
#include "Colorizer.h"

//
// THIS CLASS IS A STUB TO GET THE VISUAL PROTOTYPE WORKING
//

class Denise : public HardwareComponent {
    
public:
    
    // Color synthesizer
    Colorizer colorizer;
    
    // Denise has been executed up to this clock cycle.
    Cycle clock = 0;
    
    // Frame counter (records the number of frames drawn since power on)
    uint64_t frame = 0;
    
    
    //
    // Registers
    //
    
    // The three bitplane control registers
    uint16_t bplcon0 = 0;
    uint16_t bplcon1 = 0;
    uint16_t bplcon2 = 0;

    // The 6 bitplane data registers
    uint16_t bpldat[6]; 
    
    // Sprite control word 1 (SPRxPOS)
    uint16_t sprpos[8];
    
    // Sprite control word 2 (SPRxCTL)
    uint16_t sprctl[8];

    // Counter for digital (mouse) input (port 1 and 2)
    uint16_t joydat[2]; 


    
    /* The 6 bitplane parallel-to-serial shift registers
     * Denise transfers the current values of the bpldat registers into
     * the shift registers after BPLDAT1 is written to. This is emulated
     * in function fillShiftRegisters().
     */
    uint32_t shiftReg[6];
    
    // Scroll values. The variables are set in pokeBPLCON1()
    int8_t scrollLowEven;
    int8_t scrollLowOdd;
    int8_t scrollHiEven;
    int8_t scrollHiOdd;
    
    /*
    // Vertical screen buffer size
    static const long VPIXELS = 288;
    
    // Horizontal screen buffer size
    static const long HPIXELS = 768;

    // Screen buffer size
    static const long BUFSIZE = VPIXELS * HPIXELS * 4;
    */
    

    /* Screen buffer for long and short frames
     *
     *   - Long frames consist of the odd rasterlines 1, 3, 5, ..., 625.
     *   - Short frames consist of the even rasterlines 2, 4, 6, ..., 624.
     *
     * Paula writes the graphics output into this buffer. At a later stage,
     * both buffers are converted into textures. After that, the GPU merges
     * them into the final image and draws it on the screen.
     */
    int *longFrame = new int[HPIXELS * VPIXELS];
    int *shortFrame = new int[HPIXELS * VPIXELS];

    /* Currently active frame buffer
     * This variable points either to longFrame or shortFrame
     */
    int *frameBuffer = longFrame;

    /* Pointer to the beginning of the current rasterline
     * This pointer is used by all rendering methods to write pixels. It always
     * points to the beginning of a rasterline, either in longFrame or
     * shortFrame. It is reset at the beginning of each frame and incremented
     * at the beginning of each rasterline.
     */
    int *pixelBuffer = longFrame;

    /* Offset into pixelBuffer
     */
    short bufferoffset;

    //
    // Constructing and destructing
    //
    
public:
    
    Denise();
    ~Denise();
    
    //
    // Methods from HardwareComponent
    //
    
private:
    
    void _powerOn() override;
    void _powerOff() override;
    void _reset() override;
    void _ping() override;
    void _dump() override;

    void didLoadFromBuffer(uint8_t **buffer) override;
  
    //
    // Collecting information
    //
    
public:
    
    // Collects the data shown in the GUI's debug panel.
    DeniseInfo getInfo();
    
    //
    // Accessing registers
    //
    
public:
   
    void pokeBPLCON0(uint16_t value);
    void pokeBPLCON1(uint16_t value);
    void pokeBPLCON2(uint16_t value);
    void pokeBPLxDAT(int x, uint16_t value);

    void pokeSPRxPOS(int x, uint16_t value);
    void pokeSPRxCTL(int x, uint16_t value);

    uint16_t peekJOYxDAT(int x);

    
    // Returns true if we're running in HIRES mode
    inline bool hires() { return (bplcon0 & 0x8000); }

    // Returns true if we're running in LORES mode
    inline bool lores() { return !(bplcon0 & 0x8000); }
    
    
    //
    // Processing events
    //
    
    // Processes an overdue event
    void serviceEvent(EventID id, int64_t data);


    
    //
    // Handling DMA
    //
    
    void fillShiftRegisters();
    
    
    //
    // Synthesizing pixels
    //
    
    // Synthesizes 16 pixels from the current shift register data
    void draw16();
    
    
    //
    // Debugging the component
    //
    
    void debugSetActivePlanes(int count);

    void debugSetBPLCON0Bit(unsigned bit, bool value);

    
    //
    // FAKE METHODS FOR THE VISUAL PROTOTYPE (TEMPORARY)
    //
    
public:
    
    /* Returns true if the long frame / short frame is ready for display
     * The long frame is ready for display, if Denise is currently working on
     * on the short frame and vice verse.
     */
    bool longFrameIsReady() { return (frameBuffer == shortFrame); }
    bool shortFrameIsReady() { return (frameBuffer == longFrame); }

    /* Returns the currently stabel screen buffer.
     * If Denise is working on the long frame, a pointer to the short frame is
     * returned and vice versa.
     */
    void *screenBuffer() { return (frameBuffer == longFrame) ? shortFrame : longFrame; }
   
    // Fake some video output
    void endOfFrame();
};

#endif
