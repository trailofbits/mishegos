#include "mish_common.h"
#include "parson.h"

/* TODO(ww): Probably don't need these semaphores, since
   our cohort slots are static and there's only
   one main mishegos process.
 */

static output_cohort cohorts[MISHEGOS_COHORT_NSLOTS];
static sem_t *mishegos_csems[MISHEGOS_COHORT_NSLOTS];

void cohorts_init() {
  // NOTE(ww): We don't need to call this with each cohort dump,
  // so just put it here.
  json_set_escape_slashes(0);

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

void cohorts_cleanup() {
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

    DLOG("!!! cohorts[%d].workers=%d", i, cohorts[i].workers);
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
      {
        char *tmp1, *tmp2;
        tmp1 = hexdump(&output.input);
        tmp2 = hexdump(&slot->input);
        DLOG("expected %s, got %s", tmp1, tmp2);
        free(tmp1);
        free(tmp2);
      }
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

static void dump_cohort(output_cohort *cohort) {
  DLOG("dumping cohort");
  assert(cohort->workers == ~(~0 << MISHEGOS_NWORKERS) && "dump_cohort called on partial cohort");

  JSON_Value *root_value = json_value_init_object();
  JSON_Object *cohort_obj = json_value_get_object(root_value);

  input_slot islot = cohort->outputs[0].input;
  char *input_hex = hexdump(&islot);
  json_object_set_string(cohort_obj, "input", input_hex);
  free(input_hex);

  JSON_Value *outputs_value = json_value_init_array();
  JSON_Array *outputs_arr = json_value_get_array(outputs_value);
  for (int i = 0; i < MISHEGOS_NWORKERS; ++i) {
    JSON_Value *output_value = json_value_init_object();
    JSON_Object *output = json_value_get_object(output_value);

    json_object_set_string(output, "result", cohort->outputs[i].result);
    json_object_set_number(output, "len", cohort->outputs[i].len);
    json_object_set_number(output, "workerno", cohort->outputs[i].workerno);
    json_object_set_number(output, "status", cohort->outputs[i].status);
    json_object_set_string(output, "status_name", status2str(cohort->outputs[i].status));

    const char *worker_so = get_worker_so(cohort->outputs[i].workerno);
    assert(worker_so != NULL);
    json_object_set_string(output, "worker_so", worker_so);

    json_array_append_value(outputs_arr, output_value);
  }
  json_object_set_value(cohort_obj, "outputs", outputs_value);

  char *dump = json_serialize_to_string(root_value);
  puts(dump);
  json_free_serialized_string(dump);

  json_value_free(root_value);
}

void dump_cohorts() {
  DLOG("dumping fully populated cohorts");

  for (int i = 0; i < MISHEGOS_COHORT_NSLOTS; ++i) {
    sem_wait(mishegos_csems[i]);

    if (cohorts[i].workers != ~(~0 << MISHEGOS_NWORKERS)) {
      DLOG("skipping incomplete cohort (workers=%d, expected=%d)", cohorts[i].workers,
           ~(~0 << MISHEGOS_NWORKERS));
      /* Slot not fully populated.
       */
      sem_post(mishegos_csems[i]);
      continue;
    }

    DLOG("slot=%d fully populated, dumping and clearing the slot", i);

    output_cohort *cohort = malloc(sizeof(output_cohort));
    memcpy(cohort, &cohorts[i], sizeof(output_cohort));

    /* Clear the worker mask. We don't actually have to wipe the cohort's
     * slots, since only the mask gets checked.
     */
    cohorts[i].workers = 0;
    memset(&cohorts[i], 0, sizeof(output_cohort));

    /* We're using our copy, so we can remove the lock
     * before actually dumping the cohort.
     */
    sem_post(mishegos_csems[i]);

    dump_cohort(cohort);
    free(cohort);
  }
}
