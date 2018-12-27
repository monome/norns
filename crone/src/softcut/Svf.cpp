//
// Created by ezra on 11/8/18.
//

#include <math.h>
#include "Svf.h"

Svf::Svf() = default;

float Svf::getNextSample(float x) {
    svf_update(&svf, x);
    return svf.lp * lpMix + svf.hp * hpMix + svf.bp * bpMix + svf.br * brMix;
}

void Svf::setSampleRate(float sr) {
    svf_set_sr(&svf, sr);
}

void Svf::setFc(float fc) {
    svf_set_fc(&svf, fc);
}

void Svf::setRq(float rq) {
    svf_set_rq(&svf, rq);
}

void Svf::setLpMix(float mix) {
    lpMix = mix;
}

void Svf::setHpMix(float mix) {
    hpMix = mix;
}

void Svf::setBpMix(float mix) {
    bpMix = mix;
}

void Svf::setBrMix(float mix) {
        brMix = mix;
}

/////////////////
// C implementation

void Svf::svf_calc_coeffs(t_svf* svf) {
    svf->g = static_cast<float>(tan(M_PI * svf->fc / svf->sr));
    svf->g1 = svf->g / (1.f + svf->g * (svf->g + svf->rq));
    svf->g2 = 2.f * (svf->g + svf->rq) * svf->g1;
    svf->g3 = svf->g * svf->g1;
    svf->g4 = 2.f * svf->g1;
}

void Svf::svf_init(t_svf* svf) {
    svf_clear_state(svf);
}

void Svf::svf_clear_state(t_svf* svf) {
    svf->v0z = 0;
    svf->v1 = 0;
    svf->v2 = 0;
}

void Svf::svf_set_sr(t_svf* svf, float sr) {
    svf->sr = sr;
    svf_calc_coeffs(svf);
}

void Svf::svf_set_fc(t_svf* svf, float fc) {
    svf->fc = fc;
    svf_calc_coeffs(svf);
}

void Svf::svf_set_rq(t_svf* svf, float rq) {
    svf->rq = rq;
    svf_calc_coeffs(svf);
}

void Svf::svf_update(t_svf* svf, float in) {
    // update
    svf->v0 = in;
    svf->v1z = svf->v1;
    svf->v2z = svf->v2;
    svf->v3 = svf->v0 + svf->v0z - 2.f * svf->v2z;
    svf->v1 += svf->g1 * svf->v3 - svf->g2 * svf->v1z;
    svf->v2 += svf->g3 * svf->v3 + svf->g4 * svf->v1z;
    svf->v0z = svf->v0;
    // output
    svf->lp = svf->v2;
    svf->bp = svf->v1;
    svf->hp = svf->v0 - svf->rq * svf->v1 - svf->v2;
    svf->br = svf->v0 - svf->rq * svf->v1;
}

float Svf::getFc() {
    return svf.fc;
}