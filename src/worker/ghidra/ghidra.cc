#include <sleigh/loadimage.hh>

#include <iostream>
#include <exception>
#include <typeinfo>
#include <stdexcept>

#include "sleighMishegos.hh"
#include "../worker.h"
#include "mish_common.h"

extern "C" {
char *worker_name = (char *)"ghidra";

void worker_ctor();

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length);
}

// This is a tiny LoadImage class which feeds the executable bytes to the translator
class MyLoadImage : public LoadImage {
  uintb baseaddr;
  int4 length;
  uint1 *data;

public:
  MyLoadImage(uintb ad, uint1 *ptr, int4 sz) : LoadImage("nofile") {
    baseaddr = ad;
    data = ptr;
    length = sz;
  }
  virtual void loadFill(uint1 *ptr, int4 size, const Address &addr);
  virtual string getArchType(void) const {
    return "x86:LE:64:default";
  }
  virtual void adjustVma(long adjust) {
  }
  virtual void setData(uint1 *ptr, int4 sz) {
    this->data = ptr;
    this->length = sz;
  }
};

// This is the only important method for the LoadImage. It returns bytes from the static array
// depending on the address range requested
void MyLoadImage::loadFill(uint1 *ptr, int4 size, const Address &addr) {
  uintb start = addr.getOffset();
  uintb max = baseaddr + (length - 1);
  for (int4 i = 0; i < size; ++i) {              // For every byte requestes
    uintb curoff = start + i;                    // Calculate offset of byte
    if ((curoff < baseaddr) || (curoff > max)) { // If byte does not fall in window
      ptr[i] = 0;                                // return 0
      continue;
    }
    uintb diff = curoff - baseaddr;
    ptr[i] = data[(int4)diff]; // Otherwise return data from our window
  }
}

// Here is a simple class for emitting assembly.  In this case, we send the strings straight
// to the result.
class AssemblyMishegos : public AssemblyEmit {
  decode_result *result;

public:
  AssemblyMishegos(decode_result *dr) : result(dr){};
  virtual void dump(const Address &addr, const string &mnem, const string &body) {
    // std::cout << ": " << mnem << ' ' << body << std::endl;
    result->status = S_SUCCESS;
    result->len =
        snprintf(result->result, MISHEGOS_DEC_MAXLEN, "%s %s\n", mnem.c_str(), body.c_str());
  }
};

static const uintb START_ADDRESS = 0x0;

// Storing data files
static DocumentStorage docstorage;

// Context for disassembly
static ContextInternal context;

// Loader for reading instruction bytes
static MyLoadImage loader(START_ADDRESS, nullptr, 0);

// Translator for doing disassembly
static SleighMishegos trans(&loader, &context);

void worker_ctor() {
  // Set up the assembler/pcode-translator
  string sleighfilename = "src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/"
                          "data/languages/x86-64.sla";
  // Need this for correctly setting up the 64 bit x86 mode
  string pspecfilename = "src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/"
                         "data/languages/x86-64.pspec";

  // Read sleigh and spec file into DOM
  Element *sleighroot = docstorage.openDocument(sleighfilename)->getRoot();
  docstorage.registerTag(sleighroot);
  Element *specroot = docstorage.openDocument(pspecfilename)->getRoot();
  docstorage.registerTag(specroot);

  trans.initialize(docstorage); // Initialize the translator
  // Single instruction disasm. Prevent instructions from messing up future
  // instruction disassembly
  trans.allowContextSet(false);

  // Now that context symbol names are loaded by the translator
  // we can set the default context
  // This imitates what is done in
  //   void Architecture::parseProcessorConfig(DocumentStorage &store)
  const Element *el = docstorage.getTag("processor_spec");
  const List &list(el->getChildren());
  for (List::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
    const string &elname((*iter)->getName());
    if (elname == "context_data") {
      context.restoreFromSpec(*iter, &trans);
      break;
    }
  }
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  loader.setData(raw_insn, length);

  // Set up the disassembly dumper
  AssemblyMishegos assememit(result);

  // Starting disassembly address
  Address addr(trans.getDefaultCodeSpace(), START_ADDRESS);

  try {
    result->ndecoded = trans.printAssembly(assememit, addr);
  } catch (...) {
    // Uncomment for debugging exception info
    // std::exception_ptr p = std::current_exception();
    // std::cout << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;

    result->status = S_FAILURE;
  }
}
