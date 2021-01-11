#include "mish_core.h"

typedef struct {
  uint64_t islots;
  uint64_t oslots;
} counters;

uint8_t *mishegos_arena;

static bool verbose, debugging, manual;
static counters counts;
static sig_atomic_t exiting;
static sig_atomic_t worker_died;
/* TODO(ww): This should really be dynamically allocated to the size
 * of nworkers, but we use it to stash the name of each worker's shared
 * object before we finish determining the full worker count. Annoying.
 */
static worker workers[MISHEGOS_MAX_NWORKERS];
static sem_t *mishegos_isems[MISHEGOS_IN_NSLOTS];
static sem_t *mishegos_osems[MISHEGOS_OUT_NSLOTS];

static void load_worker_spec(char const *spec);
static void mishegos_shm_init();
static void mishegos_sem_init();
static void cleanup();
static void exit_sig(int signo);
static void child_sig(int signo);
static void config_init();
static void arena_init();
static void start_workers();
static void work();
static void do_inputs();
static void do_outputs();

int main(int argc, char const *argv[]) {
  if (argc != 2 || strcmp(argv[1], "-h") == 0) {
    printf("Usage: mishegos <spec|options>\n"
           "Arguments:\n"
           "\t<spec> The worker specification to load from\n"
           "Options:\n"
           "\t-Xc\tRun cleanup routines only\n");
    return 0;
  }

  if (strcmp(argv[1], "-Xc") == 0) {
    cleanup();
    return 0;
  }

  verbose = (getenv("V") != NULL);
  debugging = (getenv("D") != NULL);
  manual = (getenv("M") != NULL);

  /* Create shared memory, semaphores.
   */
  mishegos_shm_init();
  mishegos_sem_init();

  /* Exit/cleanup behavior.
   */
  atexit(cleanup);

  sigaction(SIGINT, &(struct sigaction){.sa_handler = exit_sig}, NULL);
  sigaction(SIGTERM, &(struct sigaction){.sa_handler = exit_sig}, NULL);
  sigaction(SIGABRT, &(struct sigaction){.sa_handler = exit_sig}, NULL);
  sigaction(SIGCHLD, &(struct sigaction){.sa_handler = child_sig}, NULL);

  /* Prep input slots, config, cohort collector and mutation engine.
   *
   * NOTE(ww): These methods assume that the shared memory has been initialized;
   * calling them before mishegos_shm_init would be very bad. The order is also
   * important: config_init should be called before any of the others.
   */
  config_init();
  mutator_init();

  /* Load workers from specification.
   *
   * NOTE(ww): This needs to happen after shared memory initialization, since it sets
   * the nworkers field in the config. Similarly, arena_init and cohorts_init depend on the
   * nworkers field.
   */
  load_worker_spec(argv[1]);
  arena_init();
  cohorts_init();

  /* Start workers.
   */
  start_workers();

  /* Work until stopped.
   */
  work();

  return 0;
}

static void load_worker_spec(char const *spec) {
  DLOG("loading worker specs from %s", spec);

  FILE *file = fopen(spec, "r");
  if (file == NULL) {
    err(errno, "fopen: %s", spec);
  }

  uint32_t nworkers = 0;
  while (nworkers < MISHEGOS_MAX_NWORKERS) {
    size_t size = 0;
    if (getline(&workers[nworkers].so, &size, file) < 0 || feof(file) != 0) {
      break;
    }

    /* getline retains the newline if present, so chop it off. */
    workers[nworkers].so[strcspn(workers[nworkers].so, "\n")] = '\0';

    if (workers[nworkers].so[0] == '#') {
      DLOG("skipping commented line: %s", workers[nworkers].so);
      continue;
    }

    if (access(workers[nworkers].so, R_OK) < 0) {
      err(errno, "access (read): %s", workers[nworkers].so);
    }

    DLOG("got worker %d so: %s", nworkers, workers[nworkers].so);
    nworkers++;
  }

  if (nworkers < 1) {
    errx(1, "too few workers in spec (expected > 1)");
  }

  GET_CONFIG()->nworkers = nworkers;

  fclose(file);
}

static void mishegos_shm_init() {
  int fd = shm_open(MISHEGOS_SHMNAME, O_RDWR | O_CREAT | O_EXCL, 0644);
  if (fd < 0) {
    err(errno, "shm_open: %s", MISHEGOS_SHMNAME);
  }

  if (ftruncate(fd, MISHEGOS_SHMSIZE) < 0) {
    err(errno, "ftruncate: %s (%ld)", MISHEGOS_SHMNAME, MISHEGOS_SHMSIZE);
  }

  mishegos_arena = mmap(NULL, MISHEGOS_SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mishegos_arena == MAP_FAILED) {
    err(errno, "mmap: %s (%ld)", MISHEGOS_SHMNAME, MISHEGOS_SHMSIZE);
  }

  if (close(fd) < 0) {
    err(errno, "close: %s", MISHEGOS_SHMNAME);
  }
}

static void mishegos_sem_init() {
  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_IN_SEMFMT, i);

    mishegos_isems[i] = sem_open(sem_name, O_RDWR | O_CREAT | O_EXCL, 0644, 1);
    if (mishegos_isems[i] == SEM_FAILED) {
      err(errno, "sem_open: %s", sem_name);
    }

    DLOG("mishegos_isems[%d]=%p", i, mishegos_isems[i]);
  }

  for (int i = 0; i < MISHEGOS_OUT_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_OUT_SEMFMT, i);

    mishegos_osems[i] = sem_open(sem_name, O_RDWR | O_CREAT | O_EXCL, 0644, 1);
    if (mishegos_osems[i] == SEM_FAILED) {
      err(errno, "sem_open: %s", sem_name);
    }

    DLOG("mishegos_osems[%d]=%p", i, mishegos_osems[i]);
  }
}

static mutator_mode get_mut_mode() {
  const char *mode = getenv("MODE");

  /* default to the "sliding" strategy in the mutator. */
  if (mode == NULL) {
    return M_SLIDING;
  }

  if (strcmp(mode, "sliding") == 0) {
    return M_SLIDING;
  } else if (strcmp(mode, "havoc") == 0) {
    return M_HAVOC;
  } else if (strcmp(mode, "structured") == 0) {
    return M_STRUCTURED;
  }

  errx(1, "unknown mutator mode requested: %s", mode);
}

static void config_init() {
  /* TODO(ww): Configurable RNG seed.
   */
  mish_getrandom(GET_CONFIG()->rng_seed, sizeof(GET_CONFIG()->rng_seed), 0);

  if (getenv("FAST_AND_FURIOUS") != NULL) {
    GET_CONFIG()->worker_config |= 1 << W_IGNORE_CRASHES;
  }

  if (manual) {
    GET_CONFIG()->mut_mode = M_MANUAL;
  } else if (debugging) {
    GET_CONFIG()->mut_mode = M_DUMMY;
  } else {
    /* If we're not in a manual or debugging mode, try to figure out what
     * actual fuzzing mode the user wants from us.
     */
    GET_CONFIG()->mut_mode = get_mut_mode();
  }
}

static void arena_init() {
  /* Pre-worker start, so no need for semaphores.
   */

  /* Place an initial raw instruction candidate in each input slot.
   */
  uint32_t nworkers = GET_CONFIG()->nworkers;
  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    input_slot *slot = GET_I_SLOT(i);

    if (candidate(slot)) {
      /* Set nworkers bits of the worker mask high;
       * each worker will flip their bit after consuming
       * a slot.
       */
      slot->workers = ~(~0 << nworkers);
      DLOG("slot=%d new candidate:", i);
      hexputs(slot->raw_insn, slot->len);
    }
  }

  /* Mark our output slots as having no result, so that we kick things
   * off in the right state.
   */
  for (int i = 0; i < MISHEGOS_OUT_NSLOTS; ++i) {
    output_slot *slot = GET_O_SLOT(i);
    slot->status = S_NONE;
  }

  DLOG("mishegos_arena=%p (len=%ld)", mishegos_arena, MISHEGOS_SHMSIZE);
}

static void cleanup() {
  DLOG("cleaning up");

  if (GET_CONFIG() != NULL) {
    uint32_t nworkers = GET_CONFIG()->nworkers;
    for (int i = 0; i < nworkers; ++i) {
      if (workers[i].running) {
        kill(workers[i].pid, SIGINT);
        waitpid(workers[i].pid, NULL, 0);
      }

      if (workers[i].so != NULL) {
        free(workers[i].so);
      }
    }
  }

  // NOTE(ww): We don't care if these functions fail.
  shm_unlink(MISHEGOS_SHMNAME);

  if (mishegos_arena != NULL) {
    munmap(mishegos_arena, MISHEGOS_SHMSIZE);
  }

  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_IN_SEMFMT, i);

    sem_unlink(sem_name);
    sem_close(mishegos_isems[i]);
  }

  for (int i = 0; i < MISHEGOS_OUT_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_OUT_SEMFMT, i);

    sem_unlink(sem_name);
    sem_close(mishegos_osems[i]);
  }

  cohorts_cleanup();
}

static void exit_sig(int signo) {
  exiting = true;
}

static void child_sig(int signo) {
  /* No point in handling a crashed worker if we're exiting.
   */
  if (!exiting) {
    /* See the NOTE in work(). This doesn't solve the problem, just makes
     * it easier to confirm when debugging.
     */
    assert(!worker_died);
    worker_died = true;
  }
}

static void start_worker(int workerno) {
  assert(workerno <= MISHEGOS_MAX_NWORKERS && "workerno out of bounds");
  DLOG("starting worker=%d with so=%s", workerno, workers[workerno].so);

  pid_t pid;
  switch (pid = fork()) {
  case 0: { // Child.
    char workerno_s[32] = {};
    snprintf(workerno_s, sizeof(workerno_s), "%d", workerno);
    // TODO(ww): Should be configurable.
    if (execl("./src/worker/worker", "worker", workerno_s, workers[workerno].so, NULL) < 0) {
      err(errno, "execl");
    }
    break;
  }
  case -1: { // Error.
    err(errno, "fork");
    break;
  }
  default: { // Parent.
    workers[workerno].pid = pid;
    workers[workerno].running = true;
    break;
  }
  }
}

static void start_workers() {
  DLOG("starting workers");
  uint32_t nworkers = GET_CONFIG()->nworkers;
  for (int i = 0; i < nworkers; ++i) {
    start_worker(i);
  }
}

static void find_and_restart_dead_worker() {
  int status = 0;

  pid_t wpid = waitpid((pid_t)-1, &status, WNOHANG);
  assert(wpid > 0 && "handling a dead worker but waitpid didn't get one?");

  /* If worker exits on us without signaling abnormal termination,
   * that *probably* means it failed during initialization. If that's the case,
   * we should just exit, since it probably won't restart correctly.
   */
  if (!WIFSIGNALED(status)) {
    errx(1, "treating unsignaled dead worker as an init failure and exiting...");
  }

  uint32_t nworkers = GET_CONFIG()->nworkers;
  int workerno = -1;
  for (int i = 0; i < nworkers; ++i) {
    if (workers[i].pid == wpid) {
      workerno = i;
      break;
    }
  }
  assert(workerno >= 0 && "reaped a worker that's not in our worker table?");

  /* Mark our crashed worker as not running, just in case we get
   * signaled for cleanup between here and actually restarting it.
   */
  workers[workerno].running = false;
  start_worker(workerno);
}

static void work() {
  while (!exiting) {
    DLOG("working...");

    if (verbose) {
      if (counts.islots % 1000 == 0) {
        VERBOSE("inputs processed: %lu", counts.islots);
      }
      if (counts.oslots % 1000 == 0) {
        VERBOSE("outputs processed: %lu", counts.oslots);
      }
    }

    /* NOTE(ww): I'm pretty confident we could check for worker
     * failure anywhere within the event loop, but it makes sense
     * (to me) to have it right at the beginning.
     *
     * NOTE(ww): There's probably a pretty rare case getting missed here:
     * if two workers happen to die at the same time (maybe even on the same input?),
     * worker_died will be set to true twice and waitpid(-1, ...)
     * will choose just one.
     */
    if (worker_died) {
      DLOG("worker died! restarting...");
      worker_died = false;
      /* We expect our worker to clean up after itself, i.e. catch its own
       * crash, put S_CRASH in its status, and re-raise the signal with default
       * behavior to ensure that it gets propagated to us correctly. As a result,
       * we don't need to do anything other than finding and restarting the
       * appropriate worker.
       */
      find_and_restart_dead_worker();
    }

    do_inputs();
    do_outputs();
    dump_cohorts();
  }

  /* If we're in manual mode, there's a good chance that a decent chunk of our inputs are
   * still making their way through the workers + output pipeline at the time exhaustion
   * is signaled. We time-loop here a bit to give output and cohort matching a chance
   * to catch up.
   */
  if (manual) {
    DLOG("manual mode: making sure to flush results");
    for (int i = 0; i < 10; ++i) {
      do_outputs();
      dump_cohorts();
      millisleep(150);
    }
  }

  DLOG("exiting...");
}

static void do_inputs() {
  DLOG("checking input slots");

  /* This reverse loop is a silly optimization: our workers each contend
     for slot semaphores in ascending order, so we balance things out a bit
     by contending in descending order. Same for output slots.
   */
  uint32_t nworkers = GET_CONFIG()->nworkers;
  for (int i = MISHEGOS_IN_NSLOTS - 1; i >= 0; i--) {
    /* NOTE(ww): Using sem_trywait results in a pretty nice performance
     * boost within the workers, but degrades performance horrendously here.
     * Why? Don't know. Maybe because syscall overhead exceeds waiting/lock
     * evaluation here?
     */
    sem_wait(mishegos_isems[i]);

    input_slot *slot = GET_I_SLOT(i);
    if (slot->workers != 0) {
      DLOG("input slot=%d still waiting on worker(s)", i);
      goto done;
    }

    /* If our worker mask is empty, then we can put a new sample
     * in the slot and reset the mask.
     */
    if (candidate(slot)) {
      slot->workers = ~(~0 << nworkers);
      counts.islots++;
      DLOG("slot=%d new candidate:", i);
      hexputs(slot->raw_insn, slot->len);
    } else {
      VERBOSE("mutation engine exhausted! signaling exit...");
      exiting = true;
    }

  done:
    sem_post(mishegos_isems[i]);
  }
}

static void do_outputs() {
  DLOG("checking output slots");

  for (int i = MISHEGOS_OUT_NSLOTS - 1; i >= 0; i--) {
    sem_wait(mishegos_osems[i]);

    output_slot *slot = GET_O_SLOT(i);
    if (slot->status == S_NONE) {
      DLOG("output slot %d still waiting on a result", i);
      goto done;
    }

    if (!add_to_cohort(slot)) {
      DLOG("output slot %d still waiting on a cohort slot", i);
      goto done;
    }

    /* Mark the output slot as available.
     */
    slot->status = S_NONE;
    counts.oslots++;
    DLOG("output slot %d is available again", i);

  done:
    sem_post(mishegos_osems[i]);
  }
}

const char *get_worker_so(uint32_t workerno) {
  assert(workerno < MISHEGOS_MAX_NWORKERS);
  return workers[workerno].so;
}

/* TODO(ww): Move these to a separate file.
 */
char *hexdump(input_slot *slot) {
  assert(slot->len <= 15);
  char *buf = malloc((slot->len * 2) + 1);

  for (int i = 0; i < slot->len; ++i) {
    sprintf(buf + (i * 2), "%02x", slot->raw_insn[i]);
  }

  return buf;
}

void hex2bytes(uint8_t *outbuf, const char *const input, size_t input_len) {
  for (size_t i = 0, j = 0; j < input_len / 2; i += 2, ++j) {
    outbuf[j] = (input[i] % 32 + 9) % 25 * 16 + (input[i + 1] % 32 + 9) % 25;
  }
}

void millisleep(uint64_t millis) {
  nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = millis * 1000000}, NULL);
}
