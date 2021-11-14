/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This class was copied from upstream
 * https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc
 *
 * Modified by Eric Kilmer at Trail of Bits 2021
 * Modified to better support mishegos single-shot disassembly by removing the
 * caching of disassembly results at the same address without having to
 * reinitialize the class.
 */
#include "sleighMishegos.hh"

#include <sleigh/sleighbase.hh>
#include <sleigh/loadimage.hh>

/// \param ld is the LoadImage to draw program bytes from
/// \param c_db is the context database
SleighMishegos::SleighMishegos(LoadImage *ld, ContextDatabase *c_db)
    : SleighBase()

{
  loader = ld;
  context_db = c_db;
  cache = new ContextCache(c_db);
  pos = new ParserContext(cache);

  // Values taken from DisassemblyCache::initialize
  pos->initialize(75, 20, getConstantSpace());
}

void SleighMishegos::clearForDelete(void)

{
  delete cache;
  delete pos;
}

SleighMishegos::~SleighMishegos(void)

{
  clearForDelete();
}

/// Completely clear everything except the base and reconstruct
/// with a new LoadImage and ContextDatabase
/// \param ld is the new LoadImage
/// \param c_db is the new ContextDatabase
void SleighMishegos::reset(LoadImage *ld, ContextDatabase *c_db)

{
  clearForDelete();
  loader = ld;
  context_db = c_db;
  cache = new ContextCache(c_db);
}

/// The .sla file from the document store is loaded and cache objects are prepared
/// \param store is the document store containing the main \<sleigh> tag.
void SleighMishegos::initialize(DocumentStorage &store)

{
  if (!isInitialized()) { // Initialize the base if not already
    const Element *el = store.getTag("sleigh");
    if (el == (const Element *)0)
      throw LowlevelError("Could not find sleigh tag");
    restoreXml(el);
  } else
    reregisterContext();
}

/// \brief Obtain a parse tree for the instruction at the given address
///
/// The tree may be cached from a previous access.  If the address
/// has not been parsed, disassembly is performed, and a new parse tree
/// is prepared.  Depending on the desired \e state, the parse tree
/// can be prepared either for disassembly or for p-code generation.
/// \param addr is the given address of the instruction
/// \param state is the desired parse state.
/// \return the parse tree object (ParseContext)
ParserContext *SleighMishegos::obtainContext(const Address &addr, int4 state) const

{
  pos->setAddr(addr);
  pos->setParserState(ParserContext::uninitialized);

  resolve(*pos);
  if (state == ParserContext::disassembly)
    return pos;

  // If we reach here,  state must be ParserContext::pcode
  resolveHandles(*pos);
  return pos;
}

/// Resolve \e all the constructors involved in the instruction at the indicated address
/// \param pos is the parse object that will hold the resulting tree
void SleighMishegos::resolve(ParserContext &pos) const

{
  loader->loadFill(pos.getBuffer(), 16, pos.getAddr());
  ParserWalkerChange walker(&pos);
  pos.deallocateState(walker); // Clear the previous resolve and initialize the walker
  Constructor *ct, *subct;
  uint4 off;
  int4 oper, numoper;

  pos.setDelaySlot(0);
  walker.setOffset(0);        // Initial offset
  pos.clearCommits();         // Clear any old context commits
  pos.loadContext();          // Get context for current address
  ct = root->resolve(walker); // Base constructor
  walker.setConstructor(ct);
  ct->applyContext(walker);
  while (walker.isState()) {
    ct = walker.getConstructor();
    oper = walker.getOperand();
    numoper = ct->getNumOperands();
    while (oper < numoper) {
      OperandSymbol *sym = ct->getOperand(oper);
      off = walker.getOffset(sym->getOffsetBase()) + sym->getRelativeOffset();
      pos.allocateOperand(oper, walker); // Descend into new operand and reserve space
      walker.setOffset(off);
      TripleSymbol *tsym = sym->getDefiningSymbol();
      if (tsym != (TripleSymbol *)0) {
        subct = tsym->resolve(walker);
        if (subct != (Constructor *)0) {
          walker.setConstructor(subct);
          subct->applyContext(walker);
          break;
        }
      }
      walker.setCurrentLength(sym->getMinimumLength());
      walker.popOperand();
      oper += 1;
    }
    if (oper >= numoper) { // Finished processing constructor
      walker.calcCurrentLength(ct->getMinimumLength(), numoper);
      walker.popOperand();
      // Check for use of delayslot
      ConstructTpl *templ = ct->getTempl();
      if ((templ != (ConstructTpl *)0) && (templ->delaySlot() > 0))
        pos.setDelaySlot(templ->delaySlot());
    }
  }
  pos.setNaddr(pos.getAddr() + pos.getLength()); // Update Naddr to pointer after instruction
  pos.setParserState(ParserContext::disassembly);
}

/// Resolve handle templates for the given parse tree, assuming Constructors
/// are already resolved.
/// \param pos is the given parse tree
void SleighMishegos::resolveHandles(ParserContext &pos) const

{
  TripleSymbol *triple;
  Constructor *ct;
  int4 oper, numoper;

  ParserWalker walker(&pos);
  walker.baseState();
  while (walker.isState()) {
    ct = walker.getConstructor();
    oper = walker.getOperand();
    numoper = ct->getNumOperands();
    while (oper < numoper) {
      OperandSymbol *sym = ct->getOperand(oper);
      walker.pushOperand(oper); // Descend into node
      triple = sym->getDefiningSymbol();
      if (triple != (TripleSymbol *)0) {
        if (triple->getType() == SleighSymbol::subtable_symbol)
          break;
        else // Some other kind of symbol as an operand
          triple->getFixedHandle(walker.getParentHandle(), walker);
      } else { // Must be an expression
        PatternExpression *patexp = sym->getDefiningExpression();
        intb res = patexp->getValue(walker);
        FixedHandle &hand(walker.getParentHandle());
        hand.space = pos.getConstSpace(); // Result of expression is a constant
        hand.offset_space = (AddrSpace *)0;
        hand.offset_offset = (uintb)res;
        hand.size = 0; // This size should not get used
      }
      walker.popOperand();
      oper += 1;
    }
    if (oper >= numoper) { // Finished processing constructor
      ConstructTpl *templ = ct->getTempl();
      if (templ != (ConstructTpl *)0) {
        HandleTpl *res = templ->getResult();
        if (res != (HandleTpl *)0) // Pop up handle to containing operand
          res->fix(walker.getParentHandle(), walker);
        // If we need an indicator that the constructor exports nothing try
        // else
        //   walker.getParentHandle().setInvalid();
      }
      walker.popOperand();
    }
  }
  pos.setParserState(ParserContext::pcode);
}

int4 SleighMishegos::instructionLength(const Address &baseaddr) const

{
  ParserContext *pos = obtainContext(baseaddr, ParserContext::disassembly);
  return pos->getLength();
}

int4 SleighMishegos::printAssembly(AssemblyEmit &emit, const Address &baseaddr) const

{
  int4 sz;

  ParserContext *pos = obtainContext(baseaddr, ParserContext::disassembly);
  ParserWalker walker(pos);
  walker.baseState();

  Constructor *ct = walker.getConstructor();
  ostringstream mons;
  ct->printMnemonic(mons, walker);
  ostringstream body;
  ct->printBody(body, walker);
  emit.dump(baseaddr, mons.str(), body.str());
  sz = pos->getLength();
  return sz;
}

int4 SleighMishegos::oneInstruction(PcodeEmit &emit, const Address &baseaddr) const

{
  throw UnimplError("Unimplemented oneInstruction", 0);
}

void SleighMishegos::registerContext(const string &name, int4 sbit, int4 ebit)

{
  context_db->registerVariable(name, sbit, ebit);
}

void SleighMishegos::setContextDefault(const string &name, uintm val)

{
  context_db->setVariableDefault(name, val);
}

void SleighMishegos::allowContextSet(bool val) const

{
  cache->allowSet(val);
}
