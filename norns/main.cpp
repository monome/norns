#include <thread>

#include "matron_main.h"
#include "atropos_main.h"

int main(int argc, char **argv) {
    std::thread atropos_thread(atropos_main);

    std::thread matron_thread([argc, argv]() { matron_main(argc, argv); });


    matron_thread.join();
    atropos_cleanup();

    return 0;
}