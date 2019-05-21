#include "mish_common.h"
#include "mutator.h"

static volatile bool exiting;
static worker workers[MISHEGOS_NWORKERS];
// static pid_t worker_pids[MISHEGOS_NWORKERS];
static sem_t *mishegos_isems[MISHEGOS_IN_NSLOTS];
static sem_t *mishegos_osem;
static uint8_t *mishegos_arena;

static void test();
static void load_worker_spec(char const *spec);
static void mishegos_shm_init();
static void mishegos_sem_init();
static void cleanup();
static void exit_sig(int signo);
static void arena_init();
static void start_workers();
static void work();
static void do_inputs();
static void do_output();

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    errx(1, "Usage: mishegos <spec|-t>");
  }

  if (strcmp(argv[1], "-t") == 0) {
    DLOG("mutation testing mode");
    test();
    return 0;
  }

  load_worker_spec(argv[1]);

  // Create shared memory, semaphores.
  mishegos_shm_init();
  mishegos_sem_init();

  // Exit/cleanup behavior.
  atexit(cleanup);
  signal(SIGINT, exit_sig);
  signal(SIGTERM, exit_sig);
  signal(SIGABRT, exit_sig);

  // Configure the mutation engine.
  set_mutator_mode(M_SLIDING);

  // Prep the input slots.
  arena_init();

  // Start workers.
  start_workers();

  work();
  return 0;
}

static void test() {
  input_slot slot;

  puts("test 1: havoc mode");
  set_mutator_mode(M_HAVOC);
  for (int i = 0; i < 5; ++i) {
    candidate(&slot);
    hexputs(slot.raw_insn, slot.len);
  }

  puts("test 2: sliding mode");
  set_mutator_mode(M_SLIDING);
  for (int i = 0; i < 5; ++i) {
    candidate(&slot);
    hexputs(slot.raw_insn, slot.len);
  }
}

static void load_worker_spec(char const *spec) {
  DLOG("loading worker specs from %s", spec);

  FILE *file = fopen(spec, "r");
  if (file == NULL) {
    err(errno, "fopen: %s", spec);
  }

  int i = 0;
  while (i < MISHEGOS_NWORKERS) {
    size_t size = 0;
    if (getline(&workers[i].so, &size, file) < 0 && feof(file) == 0) {
      break;
    }

    /* getline retains the newline, so chop it off. */
    workers[i].so[strlen(workers[i].so) - 1] = '\0';

    if (workers[i].so[0] == '#') {
      DLOG("skipping commented line: %s", workers[i].so);
      continue;
    }

    workers[i].no = i;
    DLOG("got worker %d so: %s", workers[1].no, workers[i].so);
    i++;
  }

  if (i < MISHEGOS_NWORKERS) {
    errx(1, "too few workers in spec");
  }

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

  mishegos_osem = sem_open(MISHEGOS_OUT_SEMNAME, O_RDWR | O_CREAT | O_EXCL, 0644, 1);
  if (mishegos_osem == SEM_FAILED) {
    err(errno, "sem_open: %s", MISHEGOS_OUT_SEMNAME);
  }

  DLOG("mishegos_osem=%p", mishegos_osem);
}

static void arena_init() {
  /* Pre-worker start, so no need for semaphores.
   */

  /* Place an initial raw instruction candidate in each input slot.
   */
  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    input_slot *slot = GET_I_SLOT(i);
    /* Set NWORKERS bits of the worker mask high;
     * each worker will flip their bit after consuming
     * a slot.
     */
    slot->workers = ~(~0 << MISHEGOS_NWORKERS);
    candidate(slot);
    DLOG("slot=%d new candidate:", i);
    hexputs(slot->raw_insn, slot->len);
  }

  /* Mark our output slot as having no result so that we kick things
   * off in the right state.
   */
  output_slot *slot = GET_O_SLOT(0);
  slot->status = S_NONE;

  DLOG("mishegos_arena=%p (len=%ld)", mishegos_arena, MISHEGOS_SHMSIZE);
}

static void cleanup() {
  DLOG("cleaning up");
  // TODO(ww): kill(2) and wait for each pid in worker_pids.
  for (int i = 0; i < MISHEGOS_NWORKERS; ++i) {
    kill(workers[i].pid, SIGINT);
    waitpid(workers[i].pid, NULL, 0);

    if (workers[i].so != NULL) {
      free(workers[i].so);
    }
  }

  // NOTE(ww): We don't care if these functions fail.
  shm_unlink(MISHEGOS_SHMNAME);
  munmap(mishegos_arena, MISHEGOS_SHMSIZE);

  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_IN_SEMFMT, i);

    sem_unlink(sem_name);
    sem_close(mishegos_isems[i]);
  }

  sem_unlink(MISHEGOS_OUT_SEMNAME);
  sem_close(mishegos_osem);
}

static void exit_sig(int signo) {
  exiting = true;
}

static void start_workers() {
  for (int i = 0; i < MISHEGOS_NWORKERS; ++i) {
    DLOG("starting worker=%d with so=%s", i, workers[i].so);

    pid_t pid;
    switch (pid = fork()) {
    case 0: { // Child.
      char workerno_s[32] = {};
      snprintf(workerno_s, sizeof(workerno_s), "%d", workers[i].no);
      // TODO(ww): Should be configurable.
      if (execl("./src/worker/worker", "worker", workerno_s, workers[i].so, NULL) < 0) {
        // TODO(ww): Signal to the parent that we failed to spawn.
        err(errno, "execl");
      }
      break;
    }
    case -1: { // Error.
      err(errno, "fork");
      break;
    }
    default: { // Parent.
      workers[i].pid = pid;
      break;
    }
    }
  }
}

static void work() {
  while (!exiting) {
    DLOG("working...");
    do_inputs();
    do_output();
    sleep(1);
  }

  DLOG("exiting...");
}

static void do_inputs() {
  DLOG("checking input slots");
  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    sem_wait(mishegos_isems[i]);

    input_slot *slot = GET_I_SLOT(i);
    if (slot->workers != 0) {
      DLOG("input slot#%d still waiting on worker(s)", i);
      goto done;
    }

    /* If our worker mask is empty, then we can put a new sample
     * in the slot and reset the mask.
     */
    slot->workers = ~(~0 << MISHEGOS_NWORKERS);
    candidate(slot);
    DLOG("slot=%d new candidate:", i);
    hexputs(slot->raw_insn, slot->len);

  done:
    sem_post(mishegos_isems[i]);
  }
}

static void do_output() {
  DLOG("checking output slot");

  sem_wait(mishegos_osem);

  output_slot *slot = GET_O_SLOT(0);
  if (slot->status == S_NONE) {
    DLOG("output slot still waiting on a result");
    goto done;
  }

  if (slot->status == S_FAILURE) {
    DLOG("result: failure");
  } else {
    DLOG("result: %s\n%.*s", status2str(slot->status), (int)slot->len, slot->result);
  }
  slot->status = S_NONE;

done:
  sem_post(mishegos_osem);
}
