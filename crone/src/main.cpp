//
// Created by emb on 11/18/18.
//

#include <iostream>
#include <chrono>
#include <thread>
#include <memory>

#include "MixerClient.h"
#include "SoftcutClient.h"
#include "OscInterface.h"
#include "BufDiskWorker.h"

#include "crone.h"

static inline void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static std::unique_ptr<crone::MixerClient> m;
static std::unique_ptr<crone::SoftcutClient> sc;

void crone_cleanup() { 
    std::cout << "stopping clients" << std::endl;
    m->stop();
    sc->stop();
    std::cout << "cleaning up clients..." << std::endl;
    m->cleanup();
    sc->cleanup();
    crone::OscInterface::deinit();
    std::cout << "goodbye" << std::endl;
    
}

int crone_main() {
    using namespace crone;
    using std::cout;
    using std::endl;

    m = std::make_unique<MixerClient>();
    sc = std::make_unique<SoftcutClient>();
    crone_init (m.get(), sc.get());

    cout << "initializing buffer management worker.." << endl;
    BufDiskWorker::init(48000);

    cout << "setting up jack clients.." << endl;
    m->setup();
    sc->setup();

    cout << "starting jack clients.." << endl;
    m->start();
    sc->start();

    cout << "connecting ports... " << endl;
    m->connectAdcPorts();
    m->connectDacPorts();
    m->connect<2, 2>(sc.get(), MixerClient::SinkCut, SoftcutClient::SourceAdc);
    sc->connect<6, 6>(m.get(), 0, MixerClient::SourceCut);

    cout << "starting OSC interface..." << endl;
    OscInterface::init(m.get(), sc.get());
    OscInterface::printServerMethods();

    return 0;
}

#if 0
int main() {
    crone_main();

    cout << "entering main loop..." << endl;
    while(!OscInterface::shouldQuit())  {
        sleep(100);
    }
    
    crone_cleanup();

 }
#endif