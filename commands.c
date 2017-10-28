#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "startle/types.h"
#include "startle/macros.h"
#include "startle/support.h"
#include "startle/log.h"
#include "startle/error.h"
#include "startle/test.h"
#include "startle/map.h"
#include "commands.h"

#define COMMAND_ITEM(name, desc)                         \
  {                                                      \
    .first = (uintptr_t)#name,                           \
    .second = (uintptr_t)&command_##name                 \
  },
static pair_t commands[] = {
#include "command_list.h"
};
#undef COMMAND_ITEM

#define COMMAND_ITEM(name, desc)                         \
  {                                                      \
    .first = (uintptr_t)#name,                           \
    .second = (uintptr_t)desc                            \
  },
static pair_t command_descriptions[] = {
#include "command_list.h"
};
#undef COMMAND_ITEM

#if INTERFACE
#define COMMAND(name, desc) void command_##name(UNUSED int argc, UNUSED seg_t *argv)
#endif

bool run_command(seg_t name, int argc, seg_t *argv) {
  FOREACH(i, commands) {
    pair_t *entry = &commands[i];
    char *entry_name = (char *)entry->first;
    void (*entry_func)(int, seg_t *) = (void (*)(int, seg_t *))entry->second;
    int entry_name_size = strlen(entry_name);
    if((int)name.n <= entry_name_size &&
       strncmp(name.s, entry_name, name.n) == 0) {
      entry_func(argc, argv);
      return true;
    }
  }
  return false;
}

COMMAND(test, "run tests matching the argument") {
  run_test(argc > 0 ? argv[0] : (seg_t){"", 0});
}

COMMAND(log, "print the log") {
  log_print_all();
}
