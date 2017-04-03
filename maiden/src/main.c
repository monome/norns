#include "io.h"
#include "ui.h"

//-------------------------
//---- function definitions

int main(int argc, char **argv) {
  ui_init();
  // io_init launches child thread for ui_loop()
  if(io_init(argc, argv) == 0) {
    io_loop(); // waits for threads and children to exit
  }
}
