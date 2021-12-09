#include <thread>

#include "matron_main.h"
#include "crone_main.h"

int main(int argc, char **argv) {
    std::thread crone_thread(crone_main);

    std::thread matron_thread([argc, argv]() { matron_main(argc, argv); });


    matron_thread.join();
    crone_cleanup();

    return 0;
}