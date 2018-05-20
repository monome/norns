#include <iostream>

#include "portaudio.h"
#include "SoftCutHeadLogic.h"

using namespace std;
//using namespace softcuthead;

PaStream *stream;
PaStreamParameters inStreamParams;
PaStreamParameters outStreamParams;

const float sampleRate = 44100;
const int ioBufSize = 512;

const float recBufDur = 10.0;
const int recBufFrames = recBufDur * sampleRate;

float recBuf[recBufFrames];

SoftCutHeadLogic softcut;


int audioCallback(const void *input, void *output,
                  unsigned long frameCount,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData)
{
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    const float **in = (const float **)input;
    float **out = (float **)output;
    
    // left channel input, both channels output
    float x;
    float y;
    float dum;
    for (int fr = 0; fr < frameCount; ++fr)
    {
        x = in[0][fr];
        softcut.nextSample(x, &dum, &dum, &y);
        out[0][fr] = y;
        out[1][fr] = y;
    }
    
    return paContinue;
}

void softcutInit()
{
    softcut.setBuffer(recBuf, recBufFrames);
    softcut.setSampleRate(sampleRate);
    softcut.setRate(2.5);
    softcut.setPre(0.5);
    softcut.setRec(1.0);
    softcut.setLoopStartSeconds(1.0);
    softcut.setLoopEndSeconds(9.0);
    softcut.setLoopFlag(true);
    softcut.setRecRun(true);
    softcut.cutToPhase(0);
}

int paInit()
{
    PaError err = paNoError;
    err = Pa_Initialize();
    if (err != paNoError)
    {
        cerr << "error initializing" << endl;
        return 1;
    }
    
    inStreamParams.device = Pa_GetDefaultInputDevice();
    inStreamParams.channelCount = 2;
    inStreamParams.sampleFormat = paFloat32 | paNonInterleaved;
    inStreamParams.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    inStreamParams.hostApiSpecificStreamInfo = nullptr;
    
    outStreamParams.device = Pa_GetDefaultOutputDevice();
    outStreamParams.channelCount = 2;
    outStreamParams.sampleFormat = paFloat32 | paNonInterleaved;
    outStreamParams.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    outStreamParams.hostApiSpecificStreamInfo = nullptr;
    
    err = Pa_OpenStream(&stream, &inStreamParams, &outStreamParams,
                        sampleRate, ioBufSize,
                        (paClipOff | paDitherOff),
                        audioCallback, nullptr);
    if (err != paNoError)
    {
        cerr << "error opening portaudio stream" << endl;
        return 1;
    }
    
    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        cerr << "error starting portaudio stream" << endl;
        return 1;
    }
    return 0;
}

int main(void)
{
    memset(recBuf, 0, sizeof(recBuf));
    softcutInit();
    if (paInit())
    {
        return 1;
    }
    
    char ch;
    cin >> ch;
    
    softcut.stop
    
    char ch;
    cin >> ch;
    
    return 0;
}
