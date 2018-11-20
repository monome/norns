//
// Created by emb on 11/18/18.
//

#ifndef CRONE_JACKCLIENT_H
#define CRONE_JACKCLIENT_H

namespace  crone {
    class JackClient {
    public:
        static void setup();

        static void cleanup();

        static void start();

        static void stop();


    private:
        class Imp;

        static Imp imp;
    };
}

#endif //CRONE_JACKCLIENT_H
