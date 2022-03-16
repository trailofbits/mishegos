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
 * https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.hh
 *
 * Modified by Eric Kilmer at Trail of Bits 2021
 * Modified to better support mishegos single-shot disassembly by removing the
 * caching of disassembly results at the same address without having to
 * reinitialize the class.
 */
#include <sleigh/sleighbase.hh>
#include <sleigh/loadimage.hh>

class SleighMishegos : public SleighBase {
  LoadImage *loader;			///< The mapped bytes in the program
  ContextDatabase *context_db;		///< Database of context values steering disassembly
  ContextCache *cache;			///< Cache of recently used context values
  ParserContext *pos;
  void clearForDelete(void);		///< Delete the context and disassembly caches
protected:
  ParserContext *obtainContext(const Address &addr,int4 state) const;
  void resolve(ParserContext &pc) const;	///< Generate a parse tree suitable for disassembly
  void resolveHandles(ParserContext &pc) const;	///< Prepare the parse tree for p-code generation
public:
  SleighMishegos(LoadImage *ld,ContextDatabase *c_db);		///< Constructor
  ~SleighMishegos(void) override;				///< Destructor
  void reset(LoadImage *ld,ContextDatabase *c_db);	///< Reset the engine for a new program
  void initialize(DocumentStorage &store) override;
  void registerContext(const string &name,int4 sbit,int4 ebit) override;
  void setContextDefault(const string &nm,uintm val) override;
  void allowContextSet(bool val) const override;
  int4 instructionLength(const Address &baseaddr) const override;
  int4 oneInstruction(PcodeEmit &emit, const Address &baseaddr) const override;
  int4 printAssembly(AssemblyEmit &emit, const Address &baseaddr) const override;
};
