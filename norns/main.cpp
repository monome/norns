#include <thread>

#include "lachesis_main.h"
#include "atropos_main.h"

int main(int argc, char **argv) {
    std::thread atropos_thread(atropos_main);

    std::thread lachesis_thread([argc, argv]() { lachesis_main(argc, argv); });


    lachesis_thread.join();
    atropos_cleanup();

    return 0;
}