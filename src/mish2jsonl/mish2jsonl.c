#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mish_common.h"

typedef struct m_string {
  uint64_t len;
  char *string;
} m_string;

typedef struct worker_output {
  uint32_t status;   // 4
  uint16_t ndecoded; // 2
  uint32_t workerno; // 4
  m_string workerso; // 16
  m_string result;   // 16
} worker_output;

typedef struct cohort_results {
  uint32_t nworkers;
  m_string input;
  worker_output *outputs;
} cohort_results;

cohort_results results;
FILE *m_parsing_file;
int m_finished_parsing;

void m_cohort_print_json(FILE *f, cohort_results *r) {
  fprintf(f, "{ \"nworkers\": %u, \"input\": \"%s\", \"outputs\": [", r->nworkers, r->input.string);
  for (int i = 0; i < r->nworkers; i++) {
    if (i != 0) {
      fprintf(f, ",");
    }
    fprintf(f,
            "{ \"status\": { \"value\": %u, \"name\": \"%s\" }, \"ndecoded\": %u, \"workerno\": "
            "%u, \"worker_so\": \"%s\",\"len\": %ld, \"result\": \"%s\" }",
            r->outputs[i].status, status2str(r->outputs[i].status), r->outputs[i].ndecoded,
            r->outputs[i].workerno, r->outputs[i].workerso.string, r->outputs[i].result.len,
            r->outputs[i].status == 1 ? r->outputs[i].result.string : "bad");
  }

  fprintf(f, "]}");
  return;
}

void m_fread(void *ref, size_t size, size_t times, FILE *file) {
  size_t rd = fread(ref, size, times, file);
  if (rd == 0 &&
      size * times != 0) { // in cases of bad decompilation we want to read 0 length result string
    m_finished_parsing = 1;
  }
}

/*
    There are a few subtleties that this functions catches
    1) Not all strings we will read actually have an null byte
    even when we read the entire string, leading to extra
    junk being inserted in for example an output file
    and hence crashing the analysis tool

    2) Some disassemblers like to insert \n in their output
    this breaks values from being a valid string

    3) at the moment we alloc the memory of the string here
    using malloc will result in padded whitespaces showing
    old data that we don't want. Hence the use of calloc
    if this is only used for printing one-by-one
    we can optimize this out

    we implicitly calloc 1 extra byte to guarantee
    the string ends with a null byte (implicit calloc logic)
*/

void read_string(FILE *file, m_string *s, int len_size) {
  uint64_t string_length = 0;
  m_fread(&string_length, len_size, 1, file);

  char *input =
      calloc(1, sizeof(char) * string_length +
                    1); // calloc instead of malloc because we want to zero out the memory.
  m_fread(input, sizeof(char), string_length,
          file); // this is because we tend to reuse the same memory alot (we optimize this call out
                 // probably)

  int newsize = strcspn(input, "\n");
  input[newsize] = '\0';
  s->len = newsize; // should this be new or old size?
  s->string = input;
}

int read_next(FILE *file) {
  fread(&results.nworkers, sizeof(uint32_t), 1, file);
  results.outputs = malloc(sizeof(worker_output) * results.nworkers);
  read_string(file, &results.input, 8);

  for (int i = 0; i < results.nworkers; i++) {
    m_fread(&results.outputs[i].status, sizeof(uint32_t), 1, file);
    m_fread(&results.outputs[i].ndecoded, sizeof(uint16_t), 1, file);
    m_fread(&results.outputs[i].workerno, sizeof(uint32_t), 1, file);

    read_string(file, &results.outputs[i].workerso, 8);
    read_string(file, &results.outputs[i].result, 2);
  }

  return m_finished_parsing == 0;
}

void free_cohort_results(cohort_results *result) {
  free(results.input.string);
  for (int i = 0; i < results.nworkers; i++) {
    free(results.outputs[i].workerso.string);
    free(results.outputs[i].result.string);
  }
  free(results.outputs);
}

void m_print_results_json(FILE *output_file) {
  int is_first = 1;
  printf("[");
  while (read_next(m_parsing_file)) {
    if (!m_finished_parsing) {
      if (!is_first) {
        printf(",");
      }
      m_cohort_print_json(output_file, &results);
      free_cohort_results(&results);
      printf("\n");
      is_first = 0;
    }
  }
}

void m_print_results_jsonl(FILE *output_file) {
  while (read_next(m_parsing_file)) {
    m_cohort_print_json(output_file, &results);
    free_cohort_results(&results);
    printf("\n");
  }
}

int main(int argc, char **argv) {
  m_finished_parsing = 0;
  if (argc != 2) {
    printf("error: require path to file");
    return 0;
  }

  m_parsing_file = fopen(argv[1], "r");
  if (m_parsing_file == NULL) {
    printf("couldn't open file!");
    return 0;
  }

  m_print_results_jsonl(stdout);

  fclose(m_parsing_file);
  return 0;
}
