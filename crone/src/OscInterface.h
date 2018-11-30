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


namespace crone {
    using std::string;

    class OscInterface {
    private:
        static lo_server_thread st;
        static bool quitFlag;
        static string port;
        static unsigned int numMethods;
        enum { MAX_NUM_METHODS = 256 };

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

        static std::array<OscMethod, MAX_NUM_METHODS> methods;

    private:

        typedef void(*Handler)(lo_arg **argv, int argc);
        static void handleLoError(int num, const char *m, const char *path) {
            std::cerr << "liblo error: " << num << "; " << m << "; " << path << std::endl;
        }

        static void addServerMethod(const char* path, const char* format, Handler handler) {
            OscMethod m(path, format, handler);
            methods[numMethods] = m;
            lo_server_thread_add_method(st, path, format,
            [] (const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data) -> int
            {
                (void) path;
                (void) types;
                (void) msg;
                auto pm = static_cast<OscMethod*>(data);
                std::cerr << "osc rx: " << path << std::endl;
                pm->handler(argv, argc);
                return 0;
            }, &(methods[numMethods]));
            numMethods++;
        }

        static void addServerMethods();
        static void printServerMethods();


    public:
        static void init() {
            quitFlag = false;
            port = "9999";
            st = lo_server_thread_new(port.c_str(), handleLoError);
            addServerMethods();
            // printServerMethods();
            lo_server_thread_start(st);
        }

        static bool shouldQuit() { return quitFlag; }

        static std::string getPortNumber() { return port; }
    };
}

#endif //CRONE_OSCINTERFACE_H
