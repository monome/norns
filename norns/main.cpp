    #include <thread>

    #include "matron_main.h"
    #include "crone_main.h"

    #include "OscInterface.h"


    int main(int argc, char **argv) {   
        if(fork() != 0) {
            // parent process
            if(fork() != 0) {
                // second fork: matron
                matron_main(argc, argv);
                matron_cleanup();
            } else {
                // nothing to do..
                while (1) {;;}
            }
        } else { 
            // first fork: crone
            crone_main();
            while (!crone::OscInterface::shouldQuit()) { 
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            crone_cleanup();
        }
        return 0;
    }