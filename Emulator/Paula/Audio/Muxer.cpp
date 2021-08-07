// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Muxer.h"
#include "CIA.h"
#include "IO.h"
#include "MsgQueue.h"
#include "Oscillator.h"
#include <cmath>

Muxer::Muxer(Amiga& ref) : SubComponent(ref)
{
    subComponents = std::vector<AmigaComponent *> {

        &filterL,
        &filterR
    };
    
    sampler[0] = new Sampler();
    sampler[1] = new Sampler();
    sampler[2] = new Sampler();
    sampler[3] = new Sampler();

    setSampleRate(44100);
}
 
Muxer::~Muxer()
{
    delete sampler[0];
    delete sampler[1];
    delete sampler[2];
    delete sampler[3];
}

void
Muxer::_reset(bool hard)
{
    RESET_SNAPSHOT_ITEMS(hard)
    
    stats.bufferUnderflows = 0;
    stats.bufferOverflows = 0;
    stats.producedSamples = 0;
    stats.consumedSamples = 0;
    
    for (isize i = 0; i < 4; i++) sampler[i]->reset();
    stream.clear();
}

void
Muxer::clear()
{
    trace(AUDBUF_DEBUG, "clear()\n");
    
    // Wipe out the ringbuffer
    stream.wipeOut();
    stream.alignWritePtr();
    
    // Wipe out the filter buffers
    filterL.clear();
    filterR.clear();
}

MuxerConfig
Muxer::getDefaultConfig()
{
    MuxerConfig defaults;

    defaults.samplingMethod = SMP_NONE;
    defaults.filterType = FILTER_BUTTERWORTH;
    defaults.filterAlwaysOn = false;
    defaults.volL = 50;
    defaults.volR = 50;
    defaults.vol[0] = 100;
    defaults.vol[1] = 100;
    defaults.vol[2] = 100;
    defaults.vol[3] = 100;
    defaults.pan[0] = 170;
    defaults.pan[1] = 30;
    defaults.pan[2] = 30;
    defaults.pan[3] = 170;

    return defaults;
}

void
Muxer::resetConfig()
{
    MuxerConfig defaults = getDefaultConfig();
    
    setConfigItem(OPT_SAMPLING_METHOD, defaults.samplingMethod);
    setConfigItem(OPT_FILTER_TYPE, defaults.filterType);
    setConfigItem(OPT_FILTER_ALWAYS_ON, defaults.filterAlwaysOn);
    setConfigItem(OPT_AUDVOLL, defaults.volL);
    setConfigItem(OPT_AUDVOLR, defaults.volR);

    for (isize i = 0; i < 4; i++) {

        setConfigItem(OPT_AUDVOL, defaults.vol[i]);
        setConfigItem(OPT_AUDPAN, defaults.pan[i]);
    }
}

i64
Muxer::getConfigItem(Option option) const
{
    switch (option) {
            
        case OPT_SAMPLING_METHOD:
            return config.samplingMethod;
            
        case OPT_FILTER_TYPE:
            assert(filterL.getFilterType() == config.filterType);
            assert(filterR.getFilterType() == config.filterType);
            return config.filterType;
                        
        case OPT_FILTER_ALWAYS_ON:
            return config.filterAlwaysOn;

        case OPT_AUDVOLL:
            return config.volL;

        case OPT_AUDVOLR:
            return config.volR;

        default:
            assert(false);
            return 0;
    }
}

i64
Muxer::getConfigItem(Option option, long id) const
{
    switch (option) {
            
        case OPT_AUDVOL:
            return config.vol[id];

        case OPT_AUDPAN:
            return config.pan[id];
            
        default:
            assert(false);
            return 0;
    }
}

void
Muxer::setConfigItem(Option option, i64 value)
{
    bool wasMuted = isMuted();
    
    switch (option) {
            
        case OPT_SAMPLING_METHOD:
            
            if (!SamplingMethodEnum::isValid(value)) {
                throw VAError(ERROR_OPT_INVALID_ARG, SamplingMethodEnum::keyList());
            }
            
            config.samplingMethod = (SamplingMethod)value;
            return;
            
        case OPT_FILTER_TYPE:
            
            if (!FilterTypeEnum::isValid(value)) {
                throw VAError(ERROR_OPT_INVALID_ARG, FilterTypeEnum::keyList());
            }

            config.filterType = (FilterType)value;
            filterL.setFilterType((FilterType)value);
            filterR.setFilterType((FilterType)value);
            return;
                        
        case OPT_FILTER_ALWAYS_ON:
                        
            config.filterAlwaysOn = value;
            return;

        case OPT_AUDVOLL:
            
            if (value < 0) value = 0;
            if (value > 100) value = 100;

            config.volL = value;
            volL = pow((double)value / 50, 1.4);
                        
            if (wasMuted != isMuted())
                messageQueue.put(isMuted() ? MSG_MUTE_ON : MSG_MUTE_OFF);
            return;
            
        case OPT_AUDVOLR:

            if (value < 0) value = 0;
            if (value > 100) value = 100;

            config.volR = value;
            volR = pow((double)value / 50, 1.4);

            if (wasMuted != isMuted())
                messageQueue.put(isMuted() ? MSG_MUTE_ON : MSG_MUTE_OFF);
            return;
            
        default:
            return;
    }
}

void
Muxer::setConfigItem(Option option, long id, i64 value)
{
    switch (option) {
                        
        case OPT_AUDVOL:
    
            assert(id >= 0 && id <= 3);
            if (value < 0) value = 0;
            if (value > 100) value = 100;
            
            config.vol[id] = value;
            vol[id] = pow((double)value / 100, 1.4);
            
            return;
            
        case OPT_AUDPAN:
                        
            assert(id >= 0 && id <= 3);
            if (value < 0 || value > 200) {
                warn(" Invalid pan: %lld\n", value);
                warn("Valid values: 0 ... 200\n");
                return;
            }

            config.pan[id] = value;
            
            if (value <= 50) pan[id] = (50 + value) / 100.0;
            else if (value <= 150) pan[id] = (150 - value) / 100.0;
            else if (value <= 200) pan[id] = (value - 150) / 100.0;
            return;

        default:
            return;
    }
}

void
Muxer::_dump(dump::Category category, std::ostream& os) const
{
    using namespace util;
    
    if (category & dump::Config) {
        
        os << tab("Sampling method");
        os << SamplingMethodEnum::key(config.samplingMethod) << std::endl;
        os << tab("Filter type");
        os << FilterTypeEnum::key(config.filterType) << std::endl;
        os << tab("Filter always on");
        os << bol(config.filterAlwaysOn) << std::endl;
        os << tab("Channel 1 pan");
        os << dec(config.pan[0]) << std::endl;
        os << tab("Channel 2 pan");
        os << dec(config.pan[1]) << std::endl;
        os << tab("Channel 3 pan");
        os << dec(config.pan[2]) << std::endl;
        os << tab("Channel 4 pan");
        os << dec(config.pan[3]) << std::endl;
        os << tab("Channel 1 volume");
        os << dec(config.vol[0]) << std::endl;
        os << tab("Channel 2 volume");
        os << dec(config.vol[1]) << std::endl;
        os << tab("Channel 3 volume");
        os << dec(config.vol[2]) << std::endl;
        os << tab("Channel 4 volume");
        os << dec(config.vol[3]) << std::endl;
        os << tab("Left master volume");
        os << dec(config.volL) << std::endl;
        os << tab("Right master volume");
        os << dec(config.volR) << std::endl;
    }
}

void
Muxer::setSampleRate(double hz)
{
    trace(AUD_DEBUG, "setSampleRate(%f)\n", hz);

    sampleRate = hz;
    cyclesPerSample = MHz(Oscillator::masterClockFrequency) / hz;

    filterL.setSampleRate(hz);
    filterR.setSampleRate(hz);
}

isize
Muxer::didLoadFromBuffer(const u8 *buffer)
{
    for (isize i = 0; i < 4; i++) sampler[i]->reset();
    return 0;
}

void
Muxer::rampUp()
{
    // Only proceed if the emulator is not running in warp mode
    // if (warpMode) return;
    
    volume.target = 1.0;
    volume.delta = 3;
    
    ignoreNextUnderOrOverflow();
}

void
Muxer::rampUpFromZero()
{
    volume.current = 0.0;
    
    rampUp();
}
 
void
Muxer::rampDown()
{
    volume.target = 0.0;
    volume.delta = 50;
    
    ignoreNextUnderOrOverflow();
}

void
Muxer::synthesize(Cycle clock, Cycle target, long count)
{
    assert(target > clock);
    assert(count > 0);

    // Determine the number of elapsed cycles per audio sample
    double cyclesPerSample = (double)(target - clock) / (double)count;
                
    switch (config.samplingMethod) {
            
        case SMP_NONE:    synthesize<SMP_NONE>   (clock, count, cyclesPerSample); break;
        case SMP_NEAREST: synthesize<SMP_NEAREST>(clock, count, cyclesPerSample); break;
        case SMP_LINEAR:  synthesize<SMP_LINEAR> (clock, count, cyclesPerSample); break;
        default:          assert(false);
    }
}

void
Muxer::synthesize(Cycle clock, Cycle target)
{
    assert(target > clock);
    assert(cyclesPerSample > 0);
    
    // Determine how many samples we need to produce
    double exact = (double)(target - clock) / cyclesPerSample + fraction;
    long count = (long)exact;
    fraction = exact - (double)count;
             
    switch (config.samplingMethod) {
        case SMP_NONE:    synthesize<SMP_NONE>   (clock, count, cyclesPerSample); break;
        case SMP_NEAREST: synthesize<SMP_NEAREST>(clock, count, cyclesPerSample); break;
        case SMP_LINEAR:  synthesize<SMP_LINEAR> (clock, count, cyclesPerSample); break;
        default:          assert(false);

    }
}

template <SamplingMethod method> void
Muxer::synthesize(Cycle clock, long count, double cyclesPerSample)
{
    assert(count > 0);

    stream.lock();
    
    // Check for a buffer overflow
    if (stream.count() + count >= stream.cap()) handleBufferOverflow();

    double cycle = clock;
    bool filter = ciaa.powerLED() || config.filterAlwaysOn;

    for (long i = 0; i < count; i++) {

        double ch0 = sampler[0]->interpolate<method>((Cycle)cycle) * vol[0];
        double ch1 = sampler[1]->interpolate<method>((Cycle)cycle) * vol[1];
        double ch2 = sampler[2]->interpolate<method>((Cycle)cycle) * vol[2];
        double ch3 = sampler[3]->interpolate<method>((Cycle)cycle) * vol[3];
        
        // Compute left channel output
        float l =
        ch0 * (1 - pan[0]) + ch1 * (1 - pan[1]) +
        ch2 * (1 - pan[2]) + ch3 * (1 - pan[3]);

        // Compute right channel output
        float r =
        ch0 * pan[0] + ch1 * pan[1] +
        ch2 * pan[2] + ch3 * pan[3];

        // Apply audio filter
        if (filter) { l = filterL.apply(l); r = filterR.apply(r); }
        
        // Apply master volume
        l *= volL;
        r *= volR;
        
        // Write sample into ringbuffer
        stream.add(l, r);
        stats.producedSamples++;
        
        cycle += cyclesPerSample;
    }
    
    stream.unlock();
}

void
Muxer::handleBufferUnderflow()
{
    // There are two common scenarios in which buffer underflows occur:
    //
    // (1) The consumer runs slightly faster than the producer
    // (2) The producer is halted or not startet yet
    
    trace(AUDBUF_DEBUG, "UNDERFLOW (r: %zd w: %zd)\n", stream.r, stream.w);
    
    // Reset the write pointer
    stream.alignWritePtr();

    // Determine the elapsed seconds since the last pointer adjustment
    auto elapsedTime = util::Time::now() - lastAlignment;
    lastAlignment = util::Time::now();
    
    // Adjust the sample rate, if condition (1) holds
    if (elapsedTime.asSeconds() > 10.0) {

        stats.bufferUnderflows++;
        
        // Increase the sample rate based on what we've measured
        auto offPerSec = (stream.cap() / 2) / elapsedTime.asSeconds();
        setSampleRate(getSampleRate() + (isize)offPerSec);
    }
}

void
Muxer::handleBufferOverflow()
{
    // There are two common scenarios in which buffer overflows occur:
    //
    // (1) The consumer runs slightly slower than the producer
    // (2) The consumer is halted or not startet yet
    
    trace(AUDBUF_DEBUG, "OVERFLOW (r: %zd w: %zd)\n", stream.r, stream.w);
    
    // Reset the write pointer
    stream.alignWritePtr();

    // Determine the number of elapsed seconds since the last adjustment
    auto elapsedTime = util::Time::now() - lastAlignment;
    lastAlignment = util::Time::now();
    trace(AUDBUF_DEBUG, "elapsedTime: %f\n", elapsedTime.asSeconds());
    
    // Adjust the sample rate, if condition (1) holds
    if (elapsedTime.asSeconds() > 10.0) {
        
        stats.bufferOverflows++;
        
        // Decrease the sample rate based on what we've measured
        auto offPerSec = (stream.cap() / 2) / elapsedTime.asSeconds();
        double newSampleRate = getSampleRate() - (isize)offPerSec;

        trace(AUDBUF_DEBUG, "Changing sample rate to %f\n", newSampleRate);
        setSampleRate(newSampleRate);
    }
}

void
Muxer::ignoreNextUnderOrOverflow()
{
    lastAlignment = util::Time::now();
}

void
Muxer::copy(void *buffer, isize n)
{
    stream.lock();
    
    // Check for a buffer underflow
    if (stream.count() < n) handleBufferUnderflow();
    
    // Copy sound samples
    stream.copy(buffer, n, volume);
    stats.consumedSamples += n;
    
    stream.unlock();
}

void
Muxer::copy(void *buffer1, void *buffer2, isize n)
{
    stream.lock();
    
    // Check for a buffer underflow
    if (stream.count() < n) handleBufferUnderflow();
    
    // Copy sound samples
    stream.copy(buffer1, buffer2, n, volume);
    stats.consumedSamples += n;

    stream.unlock();
}

SampleType *
Muxer::nocopy(isize n)
{
    SampleType *addr;
    stream.lock();
    
    if (stream.count() < n) handleBufferUnderflow();
    addr = stream.currentAddr();
    stream.skip(n);
    stats.consumedSamples += n;

    stream.unlock();
    return addr;
}
