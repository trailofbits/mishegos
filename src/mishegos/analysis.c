#include "mish_common.h"

static output_cohort cohorts[MISHEGOS_COHORT_NSLOTS];
static sem_t *mishegos_csems[MISHEGOS_COHORT_NSLOTS];

void analysis_init() {
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_COHORT_SEMFMT, i);

    mishegos_csems[i] = sem_open(sem_name, O_RDWR | O_CREAT | O_EXCL, 0644, 1);
    if (mishegos_csems[i] == SEM_FAILED) {
      err(errno, "sem_open: %s", sem_name);
    }

    DLOG("mishegos_csems[%d]=%p", i, mishegos_csems[i]);
  }
}

void analysis_cleanup() {
  DLOG("cleaning up");

  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    char sem_name[NAME_MAX + 1] = {};
    snprintf(sem_name, sizeof(sem_name), MISHEGOS_COHORT_SEMFMT, i);

    sem_unlink(sem_name);
    sem_close(mishegos_csems[i]);
  }
}

bool add_to_cohort(output_slot *slot) {
  DLOG("checking cohort slots");
  sleep(1);

  /* First pass: search the cohort slots to see if another worker
   * has produced an output for the same input as us.
   */
  bool inserted = false;
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    sem_wait(mishegos_csems[i]);

    int outputno = ffs(cohorts[i].workers);
    if (outputno == 0) {
      DLOG("pass 1: no worker outputs added to cohort slot=%d yet", i);
      goto done_p1;
    }

    outputno -= 1;
    output_slot output = cohorts[i].outputs[outputno];
    if (slot->input.len != output.input.len ||
        memcmp(slot->input.raw_insn, output.input.raw_insn, output.input.len) != 0) {
      DLOG("skipping non-matching cohort slot=%d", i);
      goto done_p1;
    }

    assert(outputno != slot->workerno && "cohort already has this output");

    /* We've found our cohort.
     */
    cohorts[i].outputs[slot->workerno] = *slot;
    cohorts[i].workers ^= (1 << slot->workerno);
    inserted = true;

  done_p1:
    sem_post(mishegos_csems[i]);

    if (inserted) {
      DLOG("inserted worker output=%d into preexisting cohort slot=%d", slot->workerno, i);
      hexputs(slot->input.raw_insn, slot->input.len);
      return true;
    }
  }

  /* Second pass: search the cohort slots for an empty slot.
   */
  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    sem_wait(mishegos_csems[i]);

    int outputno = ffs(cohorts[i].workers);
    if (outputno == 0) {
      DLOG("pass 2: found empty cohort slot=%d for worker output=%d", i, slot->workerno);
      cohorts[i].outputs[slot->workerno] = *slot;
      cohorts[i].workers ^= (1 << slot->workerno);
      DLOG("pass 2: slot=%d cohorts[i].workers=%d", i, cohorts[i].workers);
      inserted = true;
      goto done_p2;
    }

  done_p2:
    sem_post(mishegos_csems[i]);

    if (inserted) {
      DLOG("inserted worker output=%d into fresh cohort slot=%d", slot->workerno, i);
      hexputs(slot->input.raw_insn, slot->input.len);
      return true;
    }
  }

  DLOG("no cohort slots available...");
  assert(!inserted && "should not have inserted anything into a cohort slot");
  return false;
}

void pump_cohorts() {
}
