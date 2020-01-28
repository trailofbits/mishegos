#include "mish_core.h"

static output_cohort cohorts[MISHEGOS_COHORT_NSLOTS];

void cohorts_init() {
  uint32_t nworkers = GET_CONFIG()->nworkers;
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    cohorts[i].outputs = malloc(sizeof(output_slot) * nworkers);
  }
}

void cohorts_cleanup() {
  DLOG("cleaning up");

  /* NOTE(ww): Nothing here for now.
   * We could free each cohorts[i].outputs here, but we're exiting anyways.
   */
}

bool add_to_cohort(output_slot *slot) {
  DLOG("checking cohort slots");

  /* First pass: search the cohort slots to see if another worker
   * has produced an output for the same input as us.
   */
  bool inserted = false;
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    int outputno = ffs(cohorts[i].workers);
    if (outputno == 0) {
      DLOG("pass 1: no worker outputs added to cohort slot=%d yet", i);
      continue;
    }

    outputno -= 1;
    output_slot output = cohorts[i].outputs[outputno];
    if (slot->input.len != output.input.len ||
        memcmp(slot->input.raw_insn, output.input.raw_insn, output.input.len) != 0) {
      DLOG("skipping non-matching cohort slot=%d", i);
#ifdef DEBUG
      {
        char *tmp1, *tmp2;
        tmp1 = hexdump(&output.input);
        tmp2 = hexdump(&slot->input);
        DLOG("expected %s, got %s", tmp1, tmp2);
        free(tmp1);
        free(tmp2);
      }
#endif
      continue;
    }

    assert(outputno != slot->workerno && "cohort already has this output");

    /* We've found our cohort.
     */
    cohorts[i].outputs[slot->workerno] = *slot;
    cohorts[i].workers ^= (1 << slot->workerno);
    inserted = true;

    if (inserted) {
      DLOG("inserted worker output=%d into preexisting cohort slot=%d", slot->workerno, i);
      return true;
    }
  }

  /* Second pass: search the cohort slots for an empty slot.
   */
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    int outputno = ffs(cohorts[i].workers);
    if (outputno != 0) {
      continue;
    }

    DLOG("pass 2: found empty cohort slot=%d for worker output=%d", i, slot->workerno);
    cohorts[i].outputs[slot->workerno] = *slot;
    cohorts[i].workers ^= (1 << slot->workerno);

    DLOG("inserted worker output=%d into fresh cohort slot=%d", slot->workerno, i);
    return true;
  }

  DLOG("no cohort slots available...");
  return false;
}

static void dump_cohort(output_cohort *cohort) {
  DLOG("dumping cohort");
  uint32_t nworkers = GET_CONFIG()->nworkers;
  assert(cohort->workers == ~(~0 << nworkers) && "dump_cohort called on partial cohort");

  // number of workers
  write(STDOUT_FILENO, &nworkers, sizeof(nworkers));
  DLOG("Number of nworkers: %u", nworkers);
  input_slot islot = cohort->outputs[0].input;

  // input
  char *input_hex = hexdump(&islot);
  size_t input_hex_len = strlen(input_hex);
  write(STDOUT_FILENO, &input_hex_len, sizeof(input_hex_len));
  write(STDOUT_FILENO, input_hex, input_hex_len);
  free(input_hex);

  for (int i = 0; i < nworkers; ++i) {
    // status
    write(STDOUT_FILENO, &cohort->outputs[i].status, sizeof(cohort->outputs[i].status));

    // ndecoded
    write(STDOUT_FILENO, &cohort->outputs[i].ndecoded, sizeof(cohort->outputs[i].ndecoded));

    // workerno
    write(STDOUT_FILENO, &cohort->outputs[i].workerno, sizeof(cohort->outputs[i].workerno));

    // worker_so
    const char *worker_so = get_worker_so(cohort->outputs[i].workerno);
    assert(worker_so != NULL);
    size_t worker_so_len = strlen(worker_so);
    write(STDOUT_FILENO, &worker_so_len, sizeof(worker_so_len));
    write(STDOUT_FILENO, worker_so, worker_so_len);

    // len
    write(STDOUT_FILENO, &cohort->outputs[i].len, sizeof(cohort->outputs[i].len));
    if (cohort->outputs[i].len > 0) {
      // result
      write(STDOUT_FILENO, cohort->outputs[i].result, cohort->outputs[i].len);
    }
  }
}

void dump_cohorts() {
  DLOG("dumping fully populated cohorts");

  uint32_t nworkers = GET_CONFIG()->nworkers;
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    if (cohorts[i].workers != ~(~0 << nworkers)) {
      DLOG("skipping incomplete cohort (worker mask=%d, expected=%d)", cohorts[i].workers,
           ~(~0 << nworkers));
      /* Slot not fully populated.
       */
      continue;
    }

    DLOG("slot=%d fully populated, dumping and clearing the slot", i);

    output_cohort *cohort = malloc(sizeof(output_cohort));
    memcpy(cohort, &cohorts[i], sizeof(output_cohort));

    /* Clear the worker mask. We don't actually have to wipe the cohort's
     * slots, since only the mask gets checked.
     */
    cohorts[i].workers = 0;
    dump_cohort(cohort);
    free(cohort);
  }
}
