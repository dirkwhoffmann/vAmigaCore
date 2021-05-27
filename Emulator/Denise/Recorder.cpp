// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Recorder.h"
#include "IO.h"
#include "Denise.h"
#include "MsgQueue.h"
#include "Paula.h"

Recorder::Recorder(Amiga& ref) : AmigaComponent(ref)
{
    subComponents = std::vector<HardwareComponent *> {
        
        &muxer
    };
}

bool
Recorder::hasFFmpeg() const
{
    return util::getSizeOfFile(ffmpegPath()) > 0;
}

void
Recorder::_initialize()
{
        
}

void
Recorder::_reset(bool hard)
{
    RESET_SNAPSHOT_ITEMS(hard)

    if (hard) audioClock = 0;
}

void
Recorder::_dump(dump::Category category, std::ostream& os) const
{
    using namespace util;
    
    os << tab("FFmpeg path") << ffmpegPath() << std::endl;
    os << tab("Installed") << bol(hasFFmpeg()) << std::endl;
    os << tab("Video pipe") << bol(videoPipe != -1) << std::endl;
    os << tab("Audio pipe") << bol(audioPipe != -1) << std::endl;
    os << tab("Recording") << bol(isRecording()) << std::endl;
}
    
util::Time
Recorder::getDuration() const
{
    return (isRecording() ? util::Time::now() : recStop) - recStart;
}

bool
Recorder::startRecording(int x1, int y1, int x2, int y2,
                               long bitRate,
                               long aspectX,
                               long aspectY)
{
    synchronized {
                
        debug(REC_DEBUG, "startRecording(%d,%d,%d,%d,%ld,%ld,%ld)\n",
              x1, y1, x2, y2, bitRate, aspectX, aspectY);

        if (isRecording()) return false;

        // Create pipes
        debug(REC_DEBUG, "Creating pipes...\n");
        
        unlink(videoPipePath().c_str());
        unlink(audioPipePath().c_str());
        if (mkfifo(videoPipePath().c_str(), 0666) == -1) return false;
        if (mkfifo(audioPipePath().c_str(), 0666) == -1) return false;
        
        debug(REC_DEBUG, "Pipes created\n");
        dump();
        
        debug(REC_DEBUG, "startRecording(%d,%d,%d,%d,%ld,%ld,%ld)\n",
              x1, y1, x2, y2, bitRate, aspectX, aspectY);
        
        if (isRecording()) return false;
        
        // Make sure the screen dimensions are even
        if ((x2 - x1) % 2) x2--;
        if ((y2 - y1) % 2) y2--;
        
        // Remember the cutout
        cutout.x1 = x1;
        cutout.x2 = x2;
        cutout.y1 = y1;
        cutout.y2 = y2;
        debug(REC_DEBUG, "Recorded area: (%d,%d) - (%d,%d)\n", x1, y1, x2, y2);
        
        // Set the bit rate, frame rate, and sample rate
        this->bitRate = bitRate;
        frameRate = 50;
        sampleRate = 44100;
        samplesPerFrame = sampleRate / frameRate;
        
        // Create pipes
        debug(REC_DEBUG, "Creating pipes...\n");
        
        unlink(videoPipePath().c_str());
        unlink(audioPipePath().c_str());
        if (mkfifo(videoPipePath().c_str(), 0666) == -1) return false;
        if (mkfifo(audioPipePath().c_str(), 0666) == -1) return false;
        
        debug(REC_DEBUG, "Pipes created\n");
        dump();
        
        //
        // Assemble the command line arguments for the video encoder
        //
        
        // Path to the FFmpeg executable
        string cmd1 = ffmpegPath() + " -nostdin";
        
        // Verbosity
        cmd1 += " -loglevel " + loglevel();
        
        // Input stream format
        cmd1 += " -f:v rawvideo -pixel_format rgba";
        
        // Frame rate
        cmd1 += " -r " + std::to_string(frameRate);
        
        // Frame size (width x height)
        cmd1 += " -s:v " + std::to_string(x2 - x1) + "x" + std::to_string(y2 - y1);
        
        // Input source (named pipe)
        cmd1 += " -i " + videoPipePath();
        
        // Output stream format
        cmd1 += " -f mp4 -pix_fmt yuv420p";
        
        // Bit rate
        cmd1 += " -b:v " + std::to_string(bitRate) + "k";
        
        // Aspect ratio
        cmd1 += " -bsf:v ";
        cmd1 += "\"h264_metadata=sample_aspect_ratio=";
        cmd1 += std::to_string(aspectX) + "/" + std::to_string(2*aspectY) + "\"";
        
        // Output file
        cmd1 += " -y " + videoStreamPath();
        
        
        //
        // Assemble the command line arguments for the audio encoder
        //
        
        // Path to the FFmpeg executable
        string cmd2 = ffmpegPath() + " -nostdin";
        
        // Verbosity
        cmd2 += " -loglevel " + loglevel();
        
        // Audio format and number of channels
        cmd2 += " -f:a f32le -ac 2";
        
        // Sampling rate
        cmd2 += " -sample_rate " + std::to_string(sampleRate);
        
        // Input source (named pipe)
        cmd2 += " -i " + audioPipePath();
        
        // Output stream format
        cmd2 += " -f mp4";
        
        // Output file
        cmd2 += " -y " + audioStreamPath();
        
        //
        // Launch FFmpeg instances
        //
        
        assert(videoFFmpeg == nullptr);
        assert(audioFFmpeg == nullptr);
        
        msg("\nStarting video encoder with options:\n%s\n", cmd1.c_str());
        videoFFmpeg = popen(cmd1.c_str(), "w");
        msg(videoFFmpeg ? "Success\n" : "Failed to launch\n");
        
        msg("\nStarting audio encoder with options:\n%s\n", cmd2.c_str());
        audioFFmpeg = popen(cmd2.c_str(), "w");
        msg(audioFFmpeg ? "Success\n" : "Failed to launch\n");
        
        // Open pipes
        msg("Opening video pipe\n");
        videoPipe = open(videoPipePath().c_str(), O_WRONLY);
        if (!videoPipe) {
            msg("Failed to launch the video pipe\n");
            return false;
        }
        msg("Opening audio pipe\n");
        audioPipe = open(audioPipePath().c_str(), O_WRONLY);
        if (!audioPipe) {
            msg("Failed to launch the audio pipe\n");
            return false;
        }
        
        msg("Success\n");
        state = State::prepare;
    }

    return true;
}

void
Recorder::stopRecording()
{
    debug(REC_DEBUG, "stopRecording()\n");

    synchronized {
        
        if (isRecording()) {
            state = State::finalize;
        }
    }
}

bool
Recorder::exportAs(const string &path)
{
    if (isRecording()) return false;
    
    //
    // Assemble the command line arguments for the video encoder
    //
    
    string cmd;
    
    // Path to the FFmpeg executable
    cmd += ffmpegPath();

    // Verbosity
    cmd += " -loglevel " + loglevel();

    // Input streams
    cmd += " -i " + videoStreamPath();
    cmd += " -i " + audioStreamPath();

    // Don't reencode
    cmd += " -c:v copy -c:a copy";

    // Output file
    cmd += " -y " + path;

    //
    // Launch FFmpeg
    //
    
    msg("\nMerging video and audio stream with options:\n%s\n", cmd.c_str());

    if (system(cmd.c_str()) == -1) {
        warn("Failed: %s\n", cmd.c_str());
    }
    
    msg("Done\n");
    return true;
}

void
Recorder::vsyncHandler(Cycle target)
{
    // Quick-exit if the recorder is not active
    if (state == State::wait) return;
    
    synchronized {
        
        switch (state) {
                
            case State::wait: break;
            case State::prepare: prepare(); break;
            case State::record: record(target); break;
            case State::finalize: finalize(); break;
        }
    }
}

void
Recorder::prepare()
{
    state = State::record;
    audioClock = 0;
    recStart = util::Time::now();
    messageQueue.put(MSG_RECORDING_STARTED);
}

void
Recorder::record(Cycle target)
{
    assert(videoFFmpeg != nullptr);
    assert(audioFFmpeg != nullptr);
    assert(videoPipe != -1);
    assert(audioPipe != -1);
    
    recordVideo(target);
    recordAudio(target);
}

void
Recorder::recordVideo(Cycle target)
{
    ScreenBuffer buffer = denise.pixelEngine.getStableBuffer();
    
    isize width = sizeof(u32) * (cutout.x2 - cutout.x1);
    isize height = cutout.y2 - cutout.y1;
    isize offset = cutout.y1 * HPIXELS + cutout.x1 + HBLANK_MIN * 4;
    u8 *data = new u8[width * height];
    u8 *src = (u8 *)(buffer.data + offset);
    u8 *dst = data;
    for (isize y = 0; y < height; y++, src += 4 * HPIXELS, dst += width) {
        memcpy(dst, src, width);
    }
    
    // Feed the video pipe
    assert(videoPipe != -1);
    (void)write(videoPipe, data, width * height);
}

void
Recorder::recordAudio(Cycle target)
{
    
    // Clone Paula's muxer contents
    muxer.sampler[0]->clone(*paula.muxer.sampler[0]);
    muxer.sampler[1]->clone(*paula.muxer.sampler[1]);
    muxer.sampler[2]->clone(*paula.muxer.sampler[2]);
    muxer.sampler[3]->clone(*paula.muxer.sampler[3]);
    assert(muxer.sampler[0]->r == paula.muxer.sampler[0]->r);
    assert(muxer.sampler[0]->w == paula.muxer.sampler[0]->w);
    
    // If this is the first frame to record, adjust the audio clock
    if (audioClock == 0) audioClock = target-1;
    
    // Synthesize audio samples
    muxer.synthesize(audioClock, target, samplesPerFrame);
    audioClock = target;
    
    // Copy samples to buffer
    float *samples = new float[2 * samplesPerFrame];
    muxer.copy(samples, samplesPerFrame);
    
    // Feed the audio pipe
    assert(audioPipe != -1);
    (void)write(audioPipe, (u8 *)samples, 2 * sizeof(float) * samplesPerFrame);
}

void
Recorder::finalize()
{    
    // Close pipes
    close(videoPipe);
    close(audioPipe);
    videoPipe = -1;
    audioPipe = -1;
    
    // Shut down encoders
    pclose(videoFFmpeg);
    pclose(audioFFmpeg);
    videoFFmpeg = nullptr;
    audioFFmpeg = nullptr;
    
    // Switch state and inform the GUI
    state = State::wait;
    recStop = util::Time::now();
    messageQueue.put(MSG_RECORDING_STOPPED);
}
