#include "mish_common.h"
#include "worker.h"

static bool ignore_crashes;
static sig_atomic_t exiting;
static uint32_t workerno;
static char *worker_name;
static void (*worker_ctor)();
static void (*worker_dtor)();
static try_decode_t try_decode;
static sem_t *mishegos_isems[MISHEGOS_IN_NSLOTS];
static sem_t *mishegos_osems[MISHEGOS_OUT_NSLOTS];
static uint8_t *mishegos_arena;
static jmp_buf fault_buf;
static input_slot input;
static output_slot output;

static void init_sems();
static void init_shm();
static void cleanup();
static void exit_sig(int signo);
static void fault_sig(int signo);
static void work();

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    errx(1, "Usage: worker <no> <so>");
  }

  DLOG("new worker started: pid=%d", getpid());

  workerno = atoi(argv[1]);
  if (workerno > MISHEGOS_MAX_NWORKERS) {
    errx(1, "workerno > %d", MISHEGOS_MAX_NWORKERS);
  }

  void *so = dlopen(argv[2], RTLD_LAZY);
  if (so == NULL) {
    errx(1, "dlopen: %s", dlerror());
  }

  worker_ctor = dlsym(so, "worker_ctor");
  worker_dtor = dlsym(so, "worker_dtor");

  try_decode = (try_decode_t)dlsym(so, "try_decode");
  if (try_decode == NULL) {
    errx(1, "dlsym: %s", dlerror());
  }

  worker_name = *((char **)dlsym(so, "worker_name"));
  if (worker_name == NULL) {
    errx(1, "dlsym: %s", dlerror());
  }

  DLOG("worker loaded: name=%s", worker_name);

  if (worker_ctor != NULL) {
    worker_ctor();
  }

  init_sems();
  init_shm();

  ignore_crashes = (GET_CONFIG()->worker_config >> W_IGNORE_CRASHES) & 1;

  atexit(cleanup);

  sigaction(SIGINT, &(struct sigaction){.sa_handler = exit_sig}, NULL);
  sigaction(SIGTERM, &(struct sigaction){.sa_handler = exit_sig}, NULL);
  sigaction(SIGABRT, &(struct sigaction){.sa_handler = exit_sig}, NULL);

  if (ignore_crashes) {
    DLOG("instructed not to handle crashes in worker=%s, beware!", worker_name);
  } else {
    sigaction(SIGSEGV, &(struct sigaction){.sa_handler = fault_sig}, NULL);
    sigaction(SIGBUS, &(struct sigaction){.sa_handler = fault_sig}, NULL);
    sigaction(SIGILL, &(struct sigaction){.sa_handler = fault_sig}, NULL);
  }

  work();

  return 0;
}

static void init_sems() {
  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_IN_SEMFMT, i);

    mishegos_isems[i] = sem_open(sem_name, O_RDWR, 0644, 1);
    if (mishegos_isems[i] == SEM_FAILED) {
      err(errno, "sem_open: %s", sem_name);
    }

    DLOG("mishegos_isems[%d]=%p", i, mishegos_isems[i]);
  }

  for (int i = 0; i < MISHEGOS_OUT_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_OUT_SEMFMT, i);

    mishegos_osems[i] = sem_open(sem_name, O_RDWR, 0644, 1);
    if (mishegos_osems[i] == SEM_FAILED) {
      err(errno, "sem_open: %s", sem_name);
    }

    DLOG("mishegos_osems[%d]=%p", i, mishegos_osems[i]);
  }
}

static void init_shm() {
  int fd = shm_open(MISHEGOS_SHMNAME, O_RDWR, 0644);
  if (fd < 0) {
    err(errno, "shm_open: %s", MISHEGOS_SHMNAME);
  }

  mishegos_arena = mmap(NULL, MISHEGOS_SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mishegos_arena == MAP_FAILED) {
    err(errno, "mmap: %s (%ld)", MISHEGOS_SHMNAME, MISHEGOS_SHMSIZE);
  }

  if (close(fd) < 0) {
    err(errno, "close: %s", MISHEGOS_SHMNAME);
  }

  DLOG("mishegos_arena=%p (len=%ld)", mishegos_arena, MISHEGOS_SHMSIZE);
}

static void cleanup() {
  DLOG("cleaning up");

  if (worker_dtor != NULL) {
    worker_dtor();
  }

  munmap(mishegos_arena, MISHEGOS_SHMSIZE);

  for (int i = 0; i < MISHEGOS_IN_NSLOTS; ++i) {
    sem_close(mishegos_isems[i]);
  }

  for (int i = 0; i < MISHEGOS_OUT_NSLOTS; ++i) {
    sem_close(mishegos_osems[i]);
  }
}

static void exit_sig(int signo) {
  exiting = true;
}

static void fault_sig(int signo) {
  longjmp(fault_buf, 1);
}

static bool get_first_new_input_slot() {
  bool retrieved = false;
  for (int i = 0; i < MISHEGOS_IN_NSLOTS && !retrieved; ++i) {
    if (sem_trywait(mishegos_isems[i]) < 0) {
      assert(errno == EAGAIN);
      DLOG("input slot=%d being processed elsewhere", i);
      continue;
    }

    input_slot *slot = GET_I_SLOT(i);
    if (!(slot->workers & (1 << workerno))) {
      /* If our bit in the worker mask is low, then we've already
       * processed this slot.
       */
      DLOG("input slot=%d already processed", i);
      goto done;
    }

    memcpy(&input, slot, sizeof(input_slot));
    slot->workers = slot->workers ^ (1 << workerno);
    retrieved = true;

  done:
    sem_post(mishegos_isems[i]);
  }

  return retrieved;
}

static void put_first_available_output_slot() {
  bool available = false;

  while (!available && !exiting) {
    for (int i = 0; i < MISHEGOS_OUT_NSLOTS; ++i) {
      if (sem_trywait(mishegos_osems[i]) < 0) {
        assert(errno == EAGAIN);
        DLOG("%s: output slot=%d being processed elsewhere", worker_name, i);
        continue;
      }

      output_slot *dest = GET_O_SLOT(i);
      if (dest->status != S_NONE) {
        DLOG("%s: output slot=%d occupied", worker_name, i);
        goto done;
      }

      DLOG("%s: output slot=%d is free, using", worker_name, i);
      memcpy(dest, &output, sizeof(output_slot));
      available = true;

    done:
      sem_post(mishegos_osems[i]);

      /* NOTE(ww): This is really unpleasant: We have to explicitly break out
       * of our inner loop here, to avoid filling *every* available slot
       * with the same output (which would subsequently break the cohort collector
       * when it tries to uniq the outputs). I'm sure there's a cleaner way to
       * express this.
       */
      if (available) {
        break;
      }
    }
  }
}

static void internal_work() {
  try_decode(&output, input.raw_insn, input.len);

  /* Copy our input slot into our output slot, so that we can identify
   * individual runs.
   */
  memcpy(&output.input, &input, sizeof(input_slot));

  /* Also put our worker number into the output slot, so we can index
   * our worker cohort correctly.
   */
  output.workerno = workerno;

  put_first_available_output_slot();
}

static void work() {
  while (!exiting) {
    DLOG("%s working...", worker_name);
    if (get_first_new_input_slot()) {
      memset(&output, 0, sizeof(output_slot));

      if (ignore_crashes) {
        internal_work();
      } else {
        if (sigsetjmp(fault_buf, 0) == 0) {
          internal_work();
        } else {
          /* Our worker has faulted. We need to create an S_CRASH output
           * and get out of dodge ASAP.
           */
          output.input = input;
          output.status = S_CRASH;
          output.workerno = workerno;
          put_first_available_output_slot();

          /* Doesn't actually matter which signal we raise here as long as it
           * causes termination (which it will, since it's registered with SA_RESETHAND).
           */
          raise(SIGSEGV);
        }
      }
    }
  }

  DLOG("exiting...");
}
