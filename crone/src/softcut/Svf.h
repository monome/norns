//
// Created by ezra on 11/8/18.
//
// state variable filter
// after Hal Chamberlin, Andy Simper

#ifndef SOFTCUT_SVF_H
#define SOFTCUT_SVF_H

#include <memory>

class Svf {
public:
    //---------------
    //-- C++ wrapper
    Svf();
    float getNextSample(float x);
    void setSampleRate(float sr);
    void setFc(float fc);
    void setRq(float rq);
    void setLpMix(float mix);
    void setHpMix(float mix);
    void setBpMix(float mix);
    void setBrMix(float mix);

    float getFc();

private:
    float lpMix;
    float hpMix;
    float bpMix;
    float brMix;

    //------------------
    //-- C implementation
    typedef struct _svf {
        // sample rate
        float sr;
        // corner frequency in hz
        float fc;
        // reciprocal of Q in [0,1]
        float rq;
        // intermediate coefficients
        float g;
        float g1;
        float g2;
        float g3;
        float g4;
        // state variables
        float v0;
        float v1;
        float v2;
        float v0z;
        float v1z;
        float v2z;
        float v3;
        // outputs
        float lp; // lowpass
        float hp; // highpass
        float bp; // bandpass
        float br; // bandreject
    } t_svf;

    t_svf svf;

    static void svf_calc_coeffs(t_svf* svf);
    static void svf_init(t_svf* svf);
    static void svf_clear_state(t_svf* svf);
    static void svf_set_sr(t_svf* svf, float sr);
    static void svf_set_fc(t_svf* svf, float fc);
    static void svf_set_rq(t_svf* svf, float rq);
    static void svf_update(t_svf* svf, float in);

};


#endif //SOFTCUT_SVF_H
