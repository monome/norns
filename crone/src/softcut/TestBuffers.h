//
// Created by ezra on 11/16/18.
//

#ifndef SOFTCUT_TESTBUFFERS_H
#define SOFTCUT_TESTBUFFERS_H

#include <iostream>
#include <fstream>

namespace softcut {
class TestBuffers {
public:
    typedef enum { Read, Write, Fade, State, Pre, Rec, numChannels } Channel;
    enum { numFrames = 131072, frameMask = 131071 };

    float buf[numChannels][numFrames]{};
    unsigned int idx = 0;

    void init() {
        for (int ch=0; ch<numChannels; ++ch) {
            for (int fr=0; fr<numFrames; ++fr) {
                buf[ch][fr] = 0.f;
            }
        }
    }

    void update(float readPhase, float writePhase, float fade, float state, float pre, float rec) {
        buf[Read][idx] = readPhase;
        buf[Write][idx] = writePhase;
        buf[Fade][idx] = fade;
        buf[State][idx] = state;
        buf[Pre][idx] = pre;
        buf[Rec][idx] = rec;
        idx = (idx+1)&frameMask;
    }

    // print buffer contents in matlab format
    void print() {
        using std::endl;
        std::ofstream ofs ("softcut_test_buffers.m", std::ofstream::out);
        ofs << "function y = softcut_test_buffers() " << endl;
        ofs << "  y = [" << endl;
        for (int ch=0; ch<numChannels; ++ch) {
            ofs << "    [ ";
            for (int fr=0; fr<numFrames; ++fr) {
                ofs << buf[ch][fr] << " ";
            }
            ofs << "    ]," << endl << endl;
        }
        ofs << "  ];" << endl;
        ofs << "end" << endl;

        ofs.close();
    }
};
}


#endif //SOFTCUT_TESTBUFFERS_H
