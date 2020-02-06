//
// Created by ezra on 11/3/18.
//


#include <iostream>

#include "Commands.h"
#include "MixerClient.h"
#include "SoftcutClient.h"

using namespace crone;

Commands Commands::mixerCommands;
Commands Commands::softcutCommands;

Commands::Commands() = default;

void Commands::post(Commands::Id id, float f) {
    CommandPacket p(id, -1, f);
    q.push(p);
}

void Commands::post(Commands::Id id, int i, float f) {
    CommandPacket p(id, i, f);
    q.push(p);
}

void Commands::post(Commands::Id id, int i, int j) {
    CommandPacket p(id, i, j);
    q.push(p);
}

void Commands::post(Commands::Id id, int i, int j, float f) {
    CommandPacket p(id, i, j, f);
    q.push(p);
}


void Commands::handlePending(MixerClient *client) {
    CommandPacket p{};
    memset(wasHandled, 0, NUM_COMMANDS * sizeof(bool));
    while (q.pop(p)) {
        if (wasHandled[p.id]) {
            // we don't want to handle the same type of command more than once per block.
            // so if we already did one of these, push it back on the queue for next block.
            q.push(p);
        } else {
            wasHandled[p.id] = true;
            client->handleCommand(&p);
        }
    }
}

void Commands::handlePending(SoftcutClient *client) {
    CommandPacket p{};
    while (q.pop(p)) {
        client->handleCommand(&p);
    }
}
