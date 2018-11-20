//
// Created by ezra on 11/4/18.
//

#include "OscInterface.h"

using namespace crone;

bool OscInterface::quitFlag;
std::string OscInterface::port;
lo_server_thread OscInterface::st;
std::vector<OscInterface::OscMethod> OscInterface::methods;

void OscInterface::addServerMethods() {
    addServerMethod("/hello", "", [](lo_arg **argv, int argc) {
        (void)argv; (void)argc;
        std::cout << "hello" << std::endl;
    });

    addServerMethod("/goodbye", "", [](lo_arg **argv, int argc) {
        (void)argv; (void)argc;
        std::cout << "goodbye" << std::endl;
    });

}
