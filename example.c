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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "startle/types.h"
#include "startle/macros.h"
#include "startle/support.h"
#include "startle/log.h"
#include "startle/error.h"
#include "startle/test.h"
#include "startle/map.h"
#include "startle/stats_types.h"
#include "startle/stats.h"
#include "commands.h"

int main(int argc, char **argv) {
  seg_t command_name = {NULL, 0};
  seg_t command_argv[4];
  unsigned int command_argc = 0;
  error_t error;
  if(catch_error(&error)) {
    log_print_all();
    return -error.type;
  } else {
    COUNTUP(i, argc) {
      char *arg = argv[i];
      int len = strlen(arg);
      if(len == 0) continue;
      if(arg[0] == '-') {
        // run previous command
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
    // run previous command
    if(command_name.s) {
      run_command(command_name, command_argc, command_argv);
    }
    return 0;
  }
}

COMMAND(test, "run tests matching the argument") {
  run_test(argc > 0 ? argv[0] : (seg_t){"", 0});
}

COMMAND(log, "print the log") {
  log_print_all();
}

/* How to use this:
 * 1. Find an interesting tag (last four characters) in the log
 * 2. Load the program in a debugger
 * 3. Set a breakpoint on the function 'breakpoint'
 * 4. Run again passing the log tag with this flag
 * 5. Finish the 'breakpoint' function
 * 6. Now you are at the log message
 *
 * With LLDB:
 * $ lldb example
 * (lldb) b breakpoint
 * (lldb) process launch -- -watch mhdc -fib 5
 * (lldb) finish
 */
COMMAND(watch, "watch for a log tag") {
  if(argc > 0) {
    seg_t tag = argv[0];
    if(tag.n == sizeof(tag_t) &&
       tag.s[0] >= 'a' &&
       tag.s[0] <= 'z') {
      set_log_watch(tag.s, false);
    } else {
      printf("invalid log tag\n");
    }
  }
}

// Naive recursive calculation of the nth Fibonacci number
int fib(int n) {
  CONTEXT("Calculating fib(%d)", n);
  assert_error(n >= 0, "must be a positive number");
  if(n < 2) {
    LOG("fib(%d) is obviously 1", n);
    return 1;
  } else {
    assert_counter(10000000);
    int a = fib(n - 1);
    int b = fib(n - 2);
    int r = a + b;
    assert_error(r >= 0, "overflow");
    LOG("%d + %d = %d", a, b, r);
    return r;
  }
}

COMMAND(fib, "calculate the Nth Fibonacci number") {
  if(argc > 0) {
    int x = atoi(argv[0].s);
    int r = fib(x);
    printf("fib(%d) = %d\n", x, r);
  }
}

// Memoized recursive calculation of the nth Fibonacci number
MAP(fib_result_map, 50);
int fib_map(int n) {
  CONTEXT("Calculating fib_map(%d)", n);
  assert_error(n >= 0, "must be a positive number");
  if(n < 2) {
    LOG("fib(%d) is obviously 1", n);
    return 1;
  } else {
    assert_counter(10000000);
    pair_t *x = map_find(fib_result_map, n);
    if(x) {
      int r = x->second;
      LOG("found fib(%d) = %d", n, r);
      return r;
    } else {
      int a = fib_map(n - 1);
      int b = fib_map(n - 2);
      int r = a + b;
      assert_error(r >= 0, "overflow");
      LOG_UNLESS(map_insert(fib_result_map, (pair_t) {n, r}),
                 NOTE("insertion failed"));
      LOG("insert fib(%d) = %d + %d = %d", n, a, b, r);
      return r;
    }
  }
}

COMMAND(fib_map, "calculate the Nth Fibonacci number, with memoization") {
  if(argc > 0) {
    int x = atoi(argv[0].s);
    int r = fib_map(x);
    printf("fib_map(%d) = %d\n", x, r);
  }
}

int64_t nanos() {
  struct timespec ts;
  int err = clock_gettime(CLOCK_MONOTONIC, &ts);
  return err ? 0 : ts.tv_sec * 1000000000 + ts.tv_nsec;
}

COMMAND(time_map_insertion, "time map insertion") {
  if(argc < 2) return;
  int x = atoi(argv[0].s);
  if(!INRANGE(x, 1, 30)) return;
  int reps = atoi(argv[1].s);
  if(!INRANGE(reps, 1, 1000)) return;

  stats_reset_counters();

  size_t n = (1l << x);

  pair_t *data = (pair_t *)malloc(sizeof(pair_t) * n);
  COUNTUP(i, n) {
    //long k = REVI(i);
    //long k = (i << 1) & (n - 1) + (i >> (x - 1));
    long k = random();
    //long k = (i * 1337) % (n - 1);
    //long k = i & 1 ? REVI(i) : i;
    data[i] = (pair_t) {k, k};
  }

  map_t map = alloc_map(n);

  int64_t start = nanos();
  uintptr_t mask = x > 12 ? (1<<(x - 5)) - 1 : 0;
  LOOP(reps) {
    map_clear(map);
    COUNTUP(i, n) {
      map_insert(map, data[i]);
      if(mask && (i & mask) == 0) {
        putchar('.');
        fflush(stdout);
      }
    }
  }
  if(mask) putchar('\n');
  int64_t stop = nanos();
  double time = stop - start; // ns
  printf("reps = %d, log2(n) = %d, time = %g ns\n", reps, x, time / ((double)(n * reps)));

  start = nanos();
  COUNTUP(i, n) {
    pair_t *x = map_find(map, data[i].first);
    assert_error(x);
    assert_eq(x->second, data[i].first);
    if(mask && (i & mask) == 0) {
      putchar('.');
      fflush(stdout);
    }
  }
  if(mask) putchar('\n');
  stop = nanos();
  time = stop - start; // ns
  printf("lookup time = %g ns\n", time / ((double)(n)));

  start = nanos();
  map_sort_full(map);
  stop = nanos();
  time = stop - start; // ns
  printf("full sort time = %g ns\n", time / ((double)(n)));

  start = nanos();
  COUNTUP(i, n) {
    pair_t *x = map_find_sorted(map, data[i].first);
    assert_error(x);
    assert_eq(x->second, data[i].first);
    if(mask && (i & mask) == 0) {
      putchar('.');
      fflush(stdout);
    }
  }
  if(mask) putchar('\n');
  stop = nanos();
  time = stop - start; // ns
  printf("lookup time with full sort = %g ns\n", time / ((double)(n)));

#if 0 // STATS
  double avg_swaps = 
    (double)GET_COUNTER(swap) /
    (double)GET_COUNTER(merge);
  double swaps_per_insert =
    (double)GET_COUNTER(swap) /
    (double)(n * reps);
  double merge_per_insert =
    (double)GET_COUNTER(merge) /
    (double)(n * reps);

  printf("avg swaps/n= %g\n", avg_swaps);
  printf("avg swaps/insert= %g\n", swaps_per_insert);
  printf("avg merge n/insert= %g\n", merge_per_insert);
  printf("max depth = %lld\n", GET_COUNTER(merge_depth_max));
  printf("ratio = %g\n", swaps_per_insert / ((double)x * x));
  printf("search finds = %g\n", (double)GET_COUNTER(find_last) / ((double)n*x*x));
#endif

  free(map);
  free(data);
}

#define N 256
TEST(random_merge) {
  pair_t arr[N];
  COUNTUP(i, 10000) {
    int b = random() % N;
    //printf("b = %d, s = %d\n", b, i);
    test_pairs(arr, LENGTH(arr), b, i);
    merge_huang88(arr, arr + b, LENGTH(arr), key_cmp);
    FOREACH(i, arr) {
      if(arr[i].first != i) {
        //print_pairs(arr, n*n);
        assert_eq(arr[i].first, i);
      }
    }
  }
  return 0;
}
