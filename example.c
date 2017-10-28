#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "startle/types.h"
#include "startle/macros.h"
#include "startle/support.h"
#include "startle/log.h"
#include "startle/error.h"
#include "startle/test.h"
#include "startle/map.h"
#include "commands.h"

int main(int argc, char **argv) {
  seg_t command_name = {NULL, 0};
  seg_t command_argv[4];
  unsigned int command_argc = 0;
  COUNTUP(i, argc) {
    char *arg = argv[i];
    int len = strlen(arg);
    if(len == 0) continue;
    if(arg[0] == '-') {
      if(command_name.s) {
        run_command(command_name, command_argc, command_argv);
      }
      command_argc = 0;
      command_name = (seg_t) {.s = arg + 1,
                              .n = len - 1};
    } else if(command_argc < LENGTH(command_argv)) {
      command_argv[command_argc++] = (seg_t) { .s = arg, .n = len };
    }
  }
  if(command_name.s) {
    run_command(command_name, command_argc, command_argv);
  }
}
