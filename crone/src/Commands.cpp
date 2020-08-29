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

Commands::Commands() : q(COMMAND_Q_CAPACITY) {}

void Commands::post(Commands::Id id, float f) {
    CommandPacket p(id, -1, f);
    post(p);
}

void Commands::post(Commands::Id id, int i, float f) {
    CommandPacket p(id, i, f);
    post(p);
}

void Commands::post(Commands::Id id, int i, int j) {
    CommandPacket p(id, i, j);
    post(p);
}

void Commands::post(Commands::Id id, int i, int j, float f) {
    CommandPacket p(id, i, j, f);
    post(p);
}

void Commands::post(CommandPacket &p) {
    bool ok = q.try_enqueue(p);
    if (!ok) {
	std::cerr << " warning: failed to post a command (queue full, most likely)" << std::endl;
    }
}

void Commands::handlePending(MixerClient *client) {
    CommandPacket p;
    while (q.try_dequeue(p)) {
        client->handleCommand(&p);
    }
}

void Commands::handlePending(SoftcutClient *client) {
    CommandPacket p;
    while (q.try_dequeue(p)) {
        client->handleCommand(&p);
    }
}
