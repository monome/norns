//
// Created by emb on 11/18/18.
//

#include "JackClient.h"



int main() {
    using namespace crone;

    JackClient::setup();
    JackClient::start();

    bool quit = false;

    while(!quit)  {
        ;;
    }

    JackClient::stop();
    JackClient::cleanup();

}