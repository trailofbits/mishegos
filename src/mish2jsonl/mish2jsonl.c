#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
  input_slot input;
  worker_output *outputs;
} cohort_results;

static cohort_results results;
static int m_finished_parsing;

static const char *status2str(decode_status status) {
  switch (status) {
  case S_NONE:
    return "none";
  case S_SUCCESS:
    return "success";
  case S_FAILURE:
    return "failure";
  case S_CRASH:
    return "crash";
  case S_PARTIAL:
    return "partial";
  case S_UNKNOWN:
  default:
    return "unknown";
  }
}

// Escape a string for JSON output per RFC 7159.
// Returns a newly allocated string that must be freed by caller.
static char *json_escape_string(const char *input) {
  if (input == NULL) {
    return NULL;
  }

  size_t input_len = strlen(input);
  size_t max_output_len = input_len * 6 + 1; // Worst case: \uXXXX per char
  char *output = malloc(max_output_len);
  if (output == NULL) {
    return NULL;
  }

  char *out = output;
  for (const char *in = input; *in != '\0'; in++) {
    unsigned char c = (unsigned char)*in;
    switch (c) {
    case '"':
      *out++ = '\\';
      *out++ = '"';
      break;
    case '\\':
      *out++ = '\\';
      *out++ = '\\';
      break;
    case '\n':
      *out++ = '\\';
      *out++ = 'n';
      break;
    case '\r':
      *out++ = '\\';
      *out++ = 'r';
      break;
    case '\t':
      *out++ = '\\';
      *out++ = 't';
      break;
    case '\b':
      *out++ = '\\';
      *out++ = 'b';
      break;
    case '\f':
      *out++ = '\\';
      *out++ = 'f';
      break;
    default:
      if (c < 0x20) {
        out += sprintf(out, "\\u%04x", c);
      } else {
        *out++ = c;
      }
      break;
    }
  }
  *out = '\0';
  return output;
}

static void m_cohort_print_json(FILE *f, cohort_results *r) {
  char hexbuf[MISHEGOS_INSN_MAXLEN * 2 + 1];
  for (size_t i = 0; i < r->input.len; i++) {
    hexbuf[i * 2] = "0123456789abcdef"[r->input.raw_insn[i] / 0x10];
    hexbuf[i * 2 + 1] = "0123456789abcdef"[r->input.raw_insn[i] % 0x10];
  }
  hexbuf[r->input.len * 2] = '\0';
  fprintf(f, "{ \"nworkers\": %u, \"input\": \"%s\", \"outputs\": [", r->nworkers, hexbuf);
  for (int i = 0; i < r->nworkers; i++) {
    if (i != 0) {
      fprintf(f, ",");
    }
    char *escaped_result = json_escape_string(r->outputs[i].result.string);
    fprintf(f,
            "{ \"status\": { \"value\": %u, \"name\": \"%s\" }, \"ndecoded\": %u, \"workerno\": "
            "%u, \"worker_so\": \"%s\",\"len\": %ld, \"result\": \"%s\" }",
            r->outputs[i].status, status2str(r->outputs[i].status), r->outputs[i].ndecoded,
            r->outputs[i].workerno, r->outputs[i].workerso.string, r->outputs[i].result.len,
            escaped_result ? escaped_result : "");
    free(escaped_result);
  }

  fprintf(f, "]}");
  return;
}

static void m_fread(void *ref, size_t size, size_t times, FILE *file) {
  size_t rd = fread(ref, size, times, file);

  // reading a 0 length string can be valid if disassembeling failed
  if (rd == 0 && size * times != 0) {
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

static void read_string(FILE *file, m_string *s, int len_size) {
  uint64_t string_length = 0;
  m_fread(&string_length, len_size, 1, file);

  // calloc instead of malloc because we want to zero out the memory.
  char *input = calloc(1, sizeof(char) * string_length + 1);
  // this is because we tend to reuse the same memory alot (we optimize this out)
  m_fread(input, sizeof(char), string_length, file);

  s->len = string_length;
  s->string = input;
}

static int read_next(FILE *file) {
  fread(&results.nworkers, sizeof(uint32_t), 1, file);
  results.outputs = malloc(sizeof(worker_output) * results.nworkers);
  m_fread(&results.input, sizeof(results.input), 1, file);

  for (int i = 0; i < results.nworkers; i++) {
    read_string(file, &results.outputs[i].workerso, 8);
    m_fread(&results.outputs[i].status, sizeof(uint32_t), 1, file);
    m_fread(&results.outputs[i].ndecoded, sizeof(uint16_t), 1, file);

    read_string(file, &results.outputs[i].result, 2);
  }

  return m_finished_parsing == 0;
}

static void free_cohort_results(cohort_results *result) {
  for (int i = 0; i < results.nworkers; i++) {
    free(results.outputs[i].workerso.string);
    free(results.outputs[i].result.string);
  }
  free(results.outputs);
}

void m_print_results_json(FILE *input_file, FILE *output_file) {
  m_finished_parsing = 0;
  int is_first = 1;

  fprintf(output_file, "[");
  while (read_next(input_file)) {
    if (!m_finished_parsing) {
      if (!is_first) {
        printf(",");
      }
      m_cohort_print_json(output_file, &results);
      free_cohort_results(&results);
      fprintf(output_file, "\n");
      is_first = 0;
    }
  }
  fprintf(output_file, "]");
}

void m_print_results_jsonl(FILE *input_file, FILE *output_file) {
  m_finished_parsing = 0;

  while (read_next(input_file)) {
    m_cohort_print_json(output_file, &results);
    free_cohort_results(&results);
    fprintf(output_file, "\n");
  }
}

int main(int argc, char **argv) {
  enum { JSON, JSONL } mode = JSONL;
  int opt;
  while ((opt = getopt(argc, argv, "hn")) != -1) {
    switch (opt) {
    case 'h':
      fprintf(stdout,
              "Convert mishegos output to JSON or JSONL\n"
              "OPTIONS: -n switches output from JSONL (default) to JSON\n"
              "Usage: %s [-n] [input]\n",
              argv[0]);
      return 0;
    case 'n':
      mode = JSON;
      break;
    default:
      fprintf(stderr, "Usage: %s [-n] [input]\n", argv[0]);
      return 1;
    }
  }

  // Default to stdin.
  FILE *input;
  if (argc - optind != 1) {
    input = stdin;
  } else {
    input = fopen(argv[optind], "r");
    if (input == NULL) {
      err(errno, "fopen");
    }
  }

  if (mode == JSONL) {
    m_print_results_jsonl(input, stdout);
  } else {
    m_print_results_json(input, stdout);
  }

  fclose(input);
  return 0;
}
