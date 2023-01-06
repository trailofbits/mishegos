
#include "mish_common.h"
#include "mutator.h"

#include <assert.h>
#include <dlfcn.h>
#include <err.h>
#include <pthread.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#define WITH_FUTEX

typedef struct {
  _Atomic uint32_t val;
#ifdef WITH_FUTEX
  _Atomic uint32_t waiters;
#endif
} mish_atomic_uint;

static void mish_atomic_wait_for(mish_atomic_uint *var, uint32_t target) {
  uint32_t old;
  size_t cnt = 0;
  while ((old = atomic_load(&var->val)) != target) {
#ifdef __x86_64__
    __asm__ volatile("pause");
#endif
    (void)cnt;
#ifdef WITH_FUTEX
    if (++cnt > 10000) {
      atomic_fetch_add_explicit(&var->waiters, 1, memory_order_relaxed);
      syscall(SYS_futex, &var->val, FUTEX_WAIT, old, NULL);
      atomic_fetch_sub_explicit(&var->waiters, 1, memory_order_relaxed);
    }
#endif
  }
}

static uint32_t mish_atomic_fetch_add(mish_atomic_uint *var, uint32_t val) {
  return atomic_fetch_add(&var->val, val);
}

static void mish_atomic_store(mish_atomic_uint *var, uint32_t val) {
  atomic_store(&var->val, val);
}

static void mish_atomic_notify(mish_atomic_uint *var) {
#ifdef WITH_FUTEX
  if (atomic_load_explicit(&var->waiters, memory_order_relaxed))
    syscall(SYS_futex, &var->val, FUTEX_WAKE, INT_MAX);
#endif
}

#define MISHEGOS_NUM_SLOTS_PER_CHUNK 4096
#define MISHEGOS_NUM_CHUNKS 16

typedef struct {
  mish_atomic_uint generation;
  mish_atomic_uint remaining_workers;
  uint32_t input_count;
  input_slot inputs[MISHEGOS_NUM_SLOTS_PER_CHUNK];
} input_chunk;

typedef struct {
  output_slot outputs[MISHEGOS_NUM_SLOTS_PER_CHUNK];
} output_chunk;

struct worker_config {
  size_t soname_len;
  const char *soname;
  int workerno;
  input_chunk *input_chunks;
  output_chunk *output_chunks;
  pthread_t thread;
};

static void *alloc_shared(size_t size) {
  void *res = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON | MAP_POPULATE, -1, 0);
  if (res == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  return res;
}

static void *worker(void *wc_vp) {
  const struct worker_config *wc = wc_vp;
  void *so = dlopen(wc->soname, RTLD_LAZY);
  if (!so) {
    perror(wc->soname);
    return NULL;
  }

  void (*worker_ctor)() = (void (*)())dlsym(so, "worker_ctor");
  void (*worker_dtor)() = (void (*)())dlsym(so, "worker_dtor");
  typedef void (*try_decode_t)(output_slot * result, uint8_t * raw_insn, uint8_t length);
  try_decode_t try_decode = (try_decode_t)dlsym(so, "try_decode");
  char *worker_name = *((char **)dlsym(so, "worker_name"));

  if (worker_ctor != NULL) {
    worker_ctor();
  }

  uint32_t gen = 1;
  size_t idx = 0;

  input_chunk *input_chunks = wc->input_chunks;
  output_chunk *output_chunks = wc->output_chunks;
  while (1) {
    mish_atomic_wait_for(&input_chunks[idx].generation, gen);

    for (size_t i = 0; i < input_chunks[idx].input_count; i++) {
      output_chunks[idx].outputs[i].len = 0;
      output_chunks[idx].outputs[i].ndecoded = 0;
      try_decode(&output_chunks[idx].outputs[i], input_chunks[idx].inputs[i].raw_insn,
                 input_chunks[idx].inputs[i].len);
    }

    if (mish_atomic_fetch_add(&input_chunks[idx].remaining_workers, -1) == 1)
      mish_atomic_notify(&input_chunks[idx].remaining_workers);

    idx++;
    if (idx == MISHEGOS_NUM_CHUNKS) {
      idx = 0;
      gen++;
    }
  }
}

/* By default, filter all inputs which all decoders identify as invalid. */
static int filter_min_success = 1;
static int filter_max_success = MISHEGOS_MAX_NWORKERS;
static bool filter_ndecoded_same = false;

static void process(size_t slot, size_t idx, input_chunk *input_chunks, int nworkers,
                    struct worker_config *workers) {
  int num_success = 0;
  bool ndecoded_same = true;
  int last_ndecoded = -1;
  for (int j = 0; j < nworkers; j++) {
    output_slot *output = &workers[j].output_chunks[slot].outputs[idx];
    num_success += output->status == S_SUCCESS;
    if (output->status == S_SUCCESS) {
      if (last_ndecoded == -1)
        last_ndecoded = output->ndecoded;
      else if (last_ndecoded != output->ndecoded)
        ndecoded_same = false;
    }
  }
  if (num_success >= filter_min_success && num_success <= filter_max_success)
    goto keep;
  if (filter_ndecoded_same && !ndecoded_same)
    goto keep;
  return;

keep:;
  input_slot *input = &input_chunks[slot].inputs[idx];
  fwrite(&nworkers, sizeof(nworkers), 1, stdout);
  uint64_t len = input->len * 2;
  fwrite(&len, sizeof(len), 1, stdout);
  for (size_t i = 0; i < input->len; i++)
    fprintf(stdout, "%02x", input->raw_insn[i]);
  for (int j = 0; j < nworkers; j++) {
    output_slot *output = &workers[j].output_chunks[slot].outputs[idx];
    fwrite(&output->status, sizeof(output->status), 1, stdout);
    fwrite(&output->ndecoded, sizeof(output->ndecoded), 1, stdout);
    fwrite(&j, sizeof(j), 1, stdout);
    fwrite(&workers[j].soname_len, sizeof(workers[j].soname_len), 1, stdout);
    fwrite(workers[j].soname, 1, workers[j].soname_len, stdout);
    fwrite(&output->len, sizeof(output->len), 1, stdout);
    fwrite(output->result, 1, output->len, stdout);
  }
}

int main(int argc, char **argv) {
  bool fork_mode = false;
  const char *mutator_name = NULL;

  int opt;
  while ((opt = getopt(argc, argv, "hfm:s:n")) != -1) {
    switch (opt) {
    case 'f':
      fork_mode = true;
      break;
    case 'm':
      mutator_name = optarg;
      break;
    case 's': {
      char *next;
      /* Both values are capped to nworkers below, s.t. -1 => nworkers - 1. */
      filter_min_success = strtol(optarg, &next, 0);
      if (*next == ':')
        filter_max_success = strtol(next + 1, &next, 0);
      if (*next != '\0')
        errx(1, "-s needs format <min> or <min>:<max>");
      break;
    }
    case 'n':
      filter_ndecoded_same = true;
      break;
    case 'h':
    default:
      fprintf(stderr, "usage: %s [-f] [-m mutator] [-s min[:max]] [-n]\n", argv[0]);
      fprintf(stderr, "  -f: use fork mode\n");
      fprintf(stderr, "  -m: specify mutator\n");
      fprintf(stderr, "  -s: keep samples where success count is in range; default is 1:-1\n");
      fprintf(stderr, "      (0 = all; 1 = #success >= 1; -1 = #success = nworkers - 1;\n");
      fprintf(stderr, "       1:-2 = #success >= 1 && <= nworkers - 1;\n");
      fprintf(stderr, "       1:0 = filter all (e.g., for use with -n); etc.)\n");
      fprintf(stderr, "  -n: keep samples where successful ndecoded differs\n");
      return 1;
    }
  }

  if (optind + 1 != argc) {
    fprintf(stderr, "expected worker file as positional argument\n");
    return 1;
  }

  mutator_t mutator = mutator_create(mutator_name);

  FILE *file = fopen(argv[optind], "r");
  if (file == NULL) {
    perror(argv[optind]);
    return 1;
  }

  input_chunk *input_chunks = alloc_shared(sizeof(input_chunk) * MISHEGOS_NUM_CHUNKS);

  struct worker_config workers[MISHEGOS_MAX_NWORKERS];
  int nworkers = 0;

  while (nworkers < MISHEGOS_MAX_NWORKERS) {
    size_t size = 0;
    char *line = NULL;
    if (getline(&line, &size, file) < 0 || feof(file) != 0) {
      break;
    }
    if (line[0] == '#') {
      continue;
    }

    /* getline retains the newline if present, so chop it off. */
    line[strcspn(line, "\n")] = '\0';
    if (access(line, R_OK) < 0) {
      perror(line);
      return 1;
    }

    workers[nworkers].soname_len = strlen(line);
    workers[nworkers].soname = line;
    workers[nworkers].workerno = nworkers;
    workers[nworkers].input_chunks = input_chunks;
    workers[nworkers].output_chunks = alloc_shared(sizeof(output_chunk) * MISHEGOS_NUM_CHUNKS);
    if (fork_mode) {
      pid_t child = fork();
      if (child < 0) {
        perror("fork");
        exit(1);
      } else if (child == 0) {
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        worker(&workers[nworkers]);
        exit(0);
      }
    } else {
      pthread_create(&workers[nworkers].thread, NULL, worker, &workers[nworkers]);
    }
    nworkers++;
  }

  if (filter_min_success < 0) {
    filter_min_success += nworkers + 1;
  }
  if (filter_max_success < 0) {
    filter_max_success += nworkers + 1;
  }
  fprintf(stderr, "filter min=%d max=%d\n", filter_min_success, filter_max_success);

  uint64_t gen = 1;
  uint64_t idx = 0;
  uint64_t exit_idx = MISHEGOS_NUM_CHUNKS;
  while (true) {
    mish_atomic_wait_for(&input_chunks[idx].remaining_workers, 0);

    if (gen > 1) {
      for (size_t i = 0; i < input_chunks[idx].input_count; i++) {
        process(idx, i, input_chunks, nworkers, workers);
      }
    }

    if (idx == exit_idx) {
      break;
    }

    // Not yet exiting, so fill another chunk.
    if (exit_idx == MISHEGOS_NUM_CHUNKS) {
      size_t count = 0;
      for (size_t i = 0; i < MISHEGOS_NUM_SLOTS_PER_CHUNK; i++) {
        bool filled = mutator(&input_chunks[idx].inputs[i]);
        if (filled) {
          count++;
        } else { // no more mutations
          exit_idx = idx;
          break;
        }
      }

      input_chunks[idx].input_count = count;
      mish_atomic_store(&input_chunks[idx].remaining_workers, nworkers);
      mish_atomic_store(&input_chunks[idx].generation, gen);
      mish_atomic_notify(&input_chunks[idx].generation);
    }

    idx++;
    if (idx == MISHEGOS_NUM_CHUNKS) {
      idx = 0;
      gen++;
    }
  }
}
