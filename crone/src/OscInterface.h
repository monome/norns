#include <utility>

//
// Created by ezra on 11/4/18.
//

#ifndef CRONE_OSCINTERFACE_H
#define CRONE_OSCINTERFACE_H

#include <iostream>
#include <string>
#include <vector>

#include <lo/lo.h>
#include <array>

#include "MixerClient.h"
#include "SoftcutClient.h"
#include "Poll.h"

// FIXME: didn't realize that liblo has a perfectly ok-looking cpp interface already. this could be cleaner.
// having a custom method wrapper is probably fine, easier to refactor if we move to different IPC.

namespace crone {
    using std::string;

    class OscInterface {

    private:
        static lo_server_thread st;
        static lo_address matronAddress;

        static bool quitFlag;
        static string port;
        static unsigned int numMethods;
        enum { MaxNumMethods = 256 };

        // OscMethod: thin wrapper for passing lambdas to OSC server thread
        class OscMethod {
            typedef void(*Handler)(lo_arg **argv, int argc);
        public:
            string path;
            string format;
            OscMethod() = default;
            OscMethod(string p, string f, Handler h);
            Handler handler;
        };

        static std::array<OscMethod, MaxNumMethods> methods;
        static std::unique_ptr<Poll> vuPoll;
        static std::unique_ptr<Poll> phasePoll;
        static MixerClient *mixerClient;
        static SoftcutClient *softCutClient;

    private:
        typedef void(*Handler)(lo_arg **argv, int argc);
        static void handleLoError(int num, const char *m, const char *path) {
            std::cerr << "liblo error: " << num << "; " << m << "; " << path << std::endl;
        }

        static void addServerMethod(const char* path, const char* format, Handler handler);

        static void addServerMethods();


    public:
        static void init(MixerClient *m, SoftcutClient *sc);
        static void deinit();
        static void printServerMethods();

        static bool shouldQuit() { return quitFlag; }
        static std::string getPortNumber() { return port; }
    };
}

#endif //CRONE_OSCINTERFACE_H
