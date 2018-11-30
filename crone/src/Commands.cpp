//
// Created by ezra on 11/3/18.
//


#include <iostream>

#include "Commands.h"
#include "MixerClient.h"
#include "SoftCutClient.h"

using namespace crone;

Commands Commands::mixerCommands;
Commands Commands::softcutCommands;

Commands::Commands() = default;

void Commands::post(Commands::Id id, float value) {
    CommandPacket p(id, -1, value);
    q.push(p);
}

void Commands::post(Commands::Id id, int voice, float value) {
    CommandPacket p(id, voice, value);
    q.push(p);
}


void Commands::handlePending(MixerClient *client) {
    CommandPacket p;
    while (q.pop(p)) {
        client->handleCommand(&p);
    }
}

void Commands::handlePending(SoftCutClient *client) {
    CommandPacket p;
    while (q.pop(p)) {
        client->handleCommand(&p);
    }
}

/// FIXME: audio Client subclasses should also define a Command subclass?
//void Commands::handlePending(AudioMain* a) {
//    CommandPacket p;
//    while (q.pop(p)) {
//        a->handleCommand(&p);
//    }
//}
