//
// Created by emb on 11/18/18.
//

#include "JackClient.h"
#include "OscInterface.h"


int main() {
    using namespace crone;

    JackClient::setup();
    JackClient::start();

    OscInterface::init();

    bool quit = false;

    while(!quit)  {
        ;;
    }

    JackClient::stop();
    JackClient::cleanup();

}