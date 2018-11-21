//
// Created by ezra on 11/3/18.
//

#include <iostream>
#include "Commands.h"
#include "AudioMain.h"

using namespace crone;

// FIXME: can throw, shouldn't be storage level init
boost::lockfree::spsc_queue <Commands::CommandPacket> Commands::q(100);

void Commands::post(Commands::Id id, float value) {
    CommandPacket p(id, -1, value);
    q.push(p);
}

void Commands::post(Commands::Id id, int voice, float value) {
    CommandPacket p(id, voice, value);
    q.push(p);
}

void Commands::handlePending(AudioMain* a) {
    CommandPacket p;
    while (q.pop(p)) {
        a->handleCommand(&p);
    }
}
