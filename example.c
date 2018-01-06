/* Copyright 2012-2018 Dustin M. DeWeese

   This file is part of the Startle example program.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
