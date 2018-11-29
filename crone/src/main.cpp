//
// Created by emb on 11/18/18.
//

#include <iostream>
#include <chrono>
#include <thread>

#include "MixerClient.h"
#include "SoftCutClient.h"
#include "OscInterface.h"


static inline void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
    using namespace crone;
    using std::cout;
    using std::endl;

    MixerClient m;
    SoftCutClient sc;


    cout << "setting up jack clients.." << endl;
    m.setup();
    sc.setup();

    cout << "starting jack clients.." << endl;
    m.start();
    sc.start();


    cout << "connecting ports... " << endl;
    m.connectAdcPorts();
    m.connectDacPorts();
    m.connect<4, 2>(sc, MixerClient::SINK_CUT, SoftCutClient::SOURCE_ADC);

    cout << "starting OSC interface..." << endl;
    OscInterface::init();

    cout << "entering main loop..." << endl;
    while(!OscInterface::shouldQuit())  {
        sleep(100);
    }


    cout << "stopping clients" << endl;
    m.stop();
    sc.stop();
    cout << "cleaning up clients..." << endl;
    m.cleanup();
    sc.cleanup();
    cout << "goodbye" << endl;

    return 0;
}