//
// Created by emb on 11/28/18.
//

#ifndef CRONE_MIXERCLIENT_H
#define CRONE_MIXERCLIENT_H

#include <atomic>

#include "Bus.h"
#include "Client.h"
#include "PeakMeter.h"
#include "Tape.h"
#include "Utilities.h"
#include <cmath>

#include "effects/StereoCompressor.h"
#include "effects/ZitaReverb.h"

namespace crone {
class MixerClient : public Client<6, 6> {

  public:
    struct TapeStatus {
        int play_state;   // 0 empty, 1 ready, 2 playing, 3 paused
        float play_pos_s; // seconds
        float play_len_s; // seconds
        int rec_state;    // 0 empty, 1 ready, 2 recording, 3 paused
        float rec_pos_s;  // seconds
        int loop_enabled; // 0/1
    };
    enum { MaxBufFrames = 2048 };
    typedef enum { SourceAdc = 0,
                   SourceCut = 1,
                   SourceExt = 2 } SourceId;
    typedef enum { SinkDac = 0,
                   SinkCut = 1,
                   SinkExt = 2 } SinkId;
    typedef Bus<2, MaxBufFrames> StereoBus;

  public:
    MixerClient();
    /// FIXME: the "commands" structure shouldn't really be necessary.
    /// should be able to refactor most/all parameters for atomic access.
    // called from audio thread
    void handleCommand(Commands::CommandPacket *p) override;

  private:
    void process(jack_nframes_t numFrames) override;
    void setSampleRate(jack_nframes_t) override;

  private:
    void processFx(size_t numFrames);
    void setFxDefaults();

  private:
    // processors
    StereoCompressor comp;
    ZitaReverb reverb;
    Tape<2> tape;

    // busses
    struct BusList {
        // sources
        StereoBus adc_source;
        StereoBus cut_source;
        StereoBus ext_source;
        // sinks
        StereoBus ext_sink;
        StereoBus cut_sink;
        StereoBus dac_sink;
        // fx I/O
        StereoBus ins_in;
        StereoBus ins_out;
        StereoBus aux_in;
        StereoBus aux_out;
        // monitor mix
        StereoBus adc_monitor;
        // tape playback
        StereoBus tape;
        BusList();
    };
    BusList bus;

    // levels
    struct SmoothLevelList {
      public:
        // "master" I/O levels
        LogRamp adc;
        LogRamp dac;
        LogRamp ext;
        LogRamp cut;
        LogRamp monitor;
        LogRamp tape;
        // softcut input levels
        LogRamp adc_cut;
        LogRamp ext_cut;
        LogRamp tape_cut;
        // aux send levels
        LogRamp monitor_aux;
        LogRamp cut_aux;
        LogRamp ext_aux;
        LogRamp tape_aux;
        // FX return / mix levels
        LogRamp aux;
        LogRamp ins_mix;

        SmoothLevelList();
        void setSampleRate(float sr);
    };

    SmoothLevelList smoothLevels;

    struct StaticLevelList {
        // 2x2 matrix for monitor mix
        float monitor_mix[4];
        StaticLevelList();
    };
    StaticLevelList staticLevels;

    // other state
    struct EnabledList {
        bool reverb;
        bool comp;
        EnabledList();
    };
    EnabledList enabled;

    PeakMeter inPeak[2];
    PeakMeter outPeak[2];

  public:
    float getInputPeakPos(int ch) {
        return inPeak[ch].getPos();
    }

    float getOutputPeakPos(int ch) {
        return outPeak[ch].getPos();
    }

    void openTapeRecord(const char *path) {
        tape.writer.open(path);
    }

    void startTapeRecord() {
        tape.writer.start();
    }

    void pauseTapeRecord(bool paused) {
        tape.pauseRecord(paused);
    }

    void stopTapeRecord() {
        tape.writer.stop();
    }

    void openTapePlayback(const char *path) {
        tape.reader.open(path);
    }

    void startTapePlayback() {
        tape.reader.start();
    }

    void pauseTapePlayback(bool paused) {
        tape.pausePlayback(paused);
    }

    void stopTapePlayback() {
        tape.reader.stop();
    }

    void setTapeLoop(bool loop) {
        tape.setLooping(loop);
    }

    TapeStatus getTapeStatus() {
        TapeStatus s{};

        // state constants for readability
        // these numeric states are sent verbatim over OSC
        // changing values here breaks external consumers
        constexpr int Empty = 0;
        constexpr int Ready = 1;
        constexpr int Playing = 2;
        constexpr int Paused = 3;

        // tape playback state
        const bool hasPlay = tape.playbackHasFile();
        const bool reading = tape.isReading();
        const bool primed = tape.playbackIsPrimed();
        const bool playPaused = tape.playbackIsPaused();
        const size_t playFrames = tape.playbackFramesTotal();
        const size_t processedFrames = tape.playbackFramesProcessed();
        const float playSr = tape.playbackFileSampleRate();
        const bool loop = tape.isLooping();

        // tape record state
        const bool hasRec = tape.recordHasFile();
        const bool writing = tape.isWriting();
        const bool recPaused = tape.recordIsPaused();
        const float recSr = tape.getSampleRate();
        const size_t recFrames = tape.recordFramesCaptured();

        // playback state mapping
        s.play_state = !hasPlay ? Empty : (!reading ? Ready : (playPaused ? Paused : Playing));

        // playback position/length in seconds
        if (playSr > 0.f) {
            const float invSr = 1.f / playSr;
            const float len = playFrames > 0 ? static_cast<float>(playFrames) * invSr : 0.f;
            float pos = static_cast<float>(processedFrames) * invSr;
            if (len > 0.f && pos > len) {
                const bool draining = (!reading) && primed;
                const bool wrap = (loop || reading || draining);
                // update pos for wrap (loop/reading) or drain (post-stop ringbuffer) conditions
                if (wrap && playFrames > 0) {
                    const size_t posFramesMod = processedFrames % playFrames;
                    pos = static_cast<float>(posFramesMod) * invSr;
                } else {
                    pos = len;
                }
            }
            s.play_len_s = len;
            s.play_pos_s = pos;
        } else {
            s.play_len_s = 0.f;
            s.play_pos_s = 0.f;
        }
        s.loop_enabled = loop ? 1 : 0;

        // derive record state/position
        s.rec_state = !hasRec ? Empty : (!writing ? Ready : (recPaused ? Paused : Playing));
        // when no record file is open, reflect position as 0 to avoid stale values
        if (!hasRec) {
            s.rec_pos_s = 0.f;
        } else {
            s.rec_pos_s = recSr > 0.f ? static_cast<float>(recFrames) / recSr : 0.f;
        }

        return s;
    }
};
} // namespace crone

#endif // CRONE_MIXERCLIENT_H
