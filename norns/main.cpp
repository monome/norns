#include <thread>

#include "crone_main.h"
#include "matron_main.h"

#include "sidecar.h"

#include "OscInterface.h"

int main(int argc, char **argv) {
  if (fork() == 0) {
    // parent process
    if (fork() != 0) {
      // second fork
      sidecar_client_init();
      
      crone_main();
      matron_main(argc, argv);

      matron_cleanup();
      crone_cleanup();
    } else {
      // parent
      // nothing to do..
      while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
  } else {
    // first fork
    sidecar_server_main();
  }
  return 0;
}