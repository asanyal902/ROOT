//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Lukasz Janyst <ljanyst@cern.ch>
//------------------------------------------------------------------------------

#include "cling/Interpreter/Interpreter.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Version.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Sema/SemaConsumer.h"
//#include "../../clang/lib/Sema/Sema.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Constants.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/GlobalVariable.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/System/DynamicLibrary.h"
#include "llvm/System/Path.h"
#include "llvm/Target/TargetSelect.h"

#include "Visitors.h"
#include "ClangUtils.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

static const char* fake_argv[] = { "clang", "-x", "c++", "-D__CLING__", 0 };
static const int fake_argc = (sizeof(fake_argv) / sizeof(const char*)) - 1;

namespace {

//-------------------------------------------------------------------------
// Copy the execution engine memory mappings for the global
// variables in the source module to the destination module.
//-------------------------------------------------------------------------
static
void
copyGlobalMappings(llvm::ExecutionEngine* ee, llvm::Module* src,
                   llvm::Module* dst)
{
   // Loop over all the global variables in the destination module.
   std::string new_global_name;
   llvm::Module::global_iterator dst_iter = dst->global_begin();
   llvm::Module::global_iterator dst_end = dst->global_end();
   for (; dst_iter != dst_end; ++dst_iter) {
      new_global_name = dst_iter->getName();
      if (new_global_name.size() > 1) {
         if (new_global_name.substr(0, 5) == "llvm.") {
            continue;
         }
         if (new_global_name[0] == '_') {
            if (
               (new_global_name[1] == '_') ||
               std::isupper(new_global_name[1])
            ) {
               continue;
            }
         }
         if (new_global_name[0] == '.') {
            continue;
         }
      }
      //fprintf(stderr, "Destination module has global: %s\n",
      //        new_global_name.c_str());
      //fprintf(stderr, "Search source module for global var: %s\n",
      //        dst_iter->getName().data());
      // Find the same variable (by name) in the source module.
      llvm::GlobalVariable* src_gv =
         src->getGlobalVariable(dst_iter->getName());
      if (!src_gv) { // no such global in prev module
         continue; // skip it
      }
      // Get the mapping from the execution engine for the source
      // global variable and create a new mapping to the same
      // address for the destination global variable.  Now they
      // share the same allocated memory (and so have the same value).
      // FIXME: We should compare src var and dst var types here!
      void* p = ee->getPointerToGlobal(src_gv);
      //fprintf(stderr, "Setting mapping for: %s to %lx\n",
      //   dst_iter->getName().data(), (unsigned long) p);
      // And duplicate it for the destination module.
      ee->addGlobalMapping(&*dst_iter, p);
   }
   // This example block copies the global variable and the mapping.
   //GlobalVariable* src_gv = &*src_global_iter;
   //void* p = ee->getPointerToGlobal(src_gv);
   //string name = src_gv->getName();
   //// New global variable is owned by destination module.
   //GlobalVariable* dst_gv = new GlobalVariable(
   //  *dest_module, // Module&
   //  src_gv->getType(), // const Type*
   //  src_gv->isConstant(), // bool, isConstant
   //  src_gv->getLinkage(), // LinkageTypes
   //  src_gv->getInitializer(), // Constant*, Initializer
   //  "" // const Twine&, Name
   //);
   //dst_gv->copyAttributesFrom(src_gv);
   //++src_global_iter;
   //src_gv->eraseFromParent();
   //dst_gv->setName(name);
   //ee->addGlobalMapping(dst_gv, p);
}

#if 0
static
llvm::sys::Path
GetExecutablePath(const char* Argv0, bool CanonicalPrefixes)
{
   if (!CanonicalPrefixes) {
      return llvm::sys::Path(Argv0);
   }
   // This just needs to be some symbol in the binary; C++ doesn't
   // allow taking the address of ::main however.
   void* P = (void*)(intptr_t) GetExecutablePath;
   return llvm::sys::Path::GetMainExecutable(Argv0, P);
}
#endif // 0

} // unnamed namespace

namespace cling {

//
//  Dummy function so we can use dladdr to find the executable path.
//
void locate_cling_executable()
{
}

#if 0
//
// MacroDetector
//

class MacroDetector : public clang::PPCallbacks {
private:
   const clang::CompilerInstance& m_CI;
   unsigned int m_minpos;
   std::vector<std::string> m_macros;
   clang::FileID m_InterpreterFile; // file included by the stub, identified by MacroDefined(), FileChanged()
   unsigned int m_inclLevel; // level of inclusions below m_InterpreterFile

   // Whether the CPP interpreter file tag was found,
   // the file is parsed, or has been parsed
   enum {
      kNotSeenYet,
      kIsComingUp,
      kIsActive,
      kHasPassed
   } m_InterpreterFileStatus;

public:

   MacroDetector(const clang::CompilerInstance& CI, unsigned int minpos)
      : clang::PPCallbacks(), m_CI(CI), m_minpos(minpos),
        m_inclLevel(0),
        m_InterpreterFileStatus(kNotSeenYet) {
   }

   virtual ~MacroDetector();

   std::vector<std::string>&
   getMacrosVector() {
      return m_macros;
   }

   void
   MacroDefined(const clang::IdentifierInfo* II, const clang::MacroInfo* MI);

   void
   MacroUndefined(const clang::IdentifierInfo* II, const clang::MacroInfo* MI);

   void
   FileChanged(clang::SourceLocation Loc, clang::PPCallbacks::FileChangeReason Reason,
               clang::SrcMgr::CharacteristicKind FileType);
};

MacroDetector::~MacroDetector()
{
}

void
MacroDetector::MacroDefined(const clang::IdentifierInfo* II,
                            const clang::MacroInfo* MI)
{
   if (MI->isBuiltinMacro()) {
      return;
   }
   clang::SourceManager& SM = m_CI.getSourceManager();
   clang::FileID mainFileID = SM.getMainFileID();
   if (SM.getFileID(MI->getDefinitionLoc()) == mainFileID) {
      if (II && II->getLength() == 35
          && II->getName() == "__CLING__MAIN_FILE_INCLUSION_MARKER") {
         assert(m_InterpreterFileStatus == kNotSeenYet && "Inclusion-buffer already found the interpreter file!");
         m_InterpreterFileStatus = kIsComingUp;
      } else if (II && II->getLength() == 25
          && II->getName() == "__CLING__MAIN_FILE_MARKER") {
         assert(m_InterpreterFileStatus == kNotSeenYet && "Inclusion-buffer already found the interpreter file!");
         m_InterpreterFileStatus = kIsActive;
      }
      return;
   }

   if (m_InterpreterFileStatus != kIsActive || m_InterpreterFile.isInvalid()
       || SM.getFileID(MI->getDefinitionLoc()) != m_InterpreterFile)
      return;

   clang::SourceLocation SLoc = SM.getInstantiationLoc(
                                   MI->getDefinitionLoc());
   clang::SourceLocation ELoc = SM.getInstantiationLoc(
                                   MI->getDefinitionEndLoc());
   unsigned start = SM.getFileOffset(SLoc);
   if (start < m_minpos) {
      return;
   }
   unsigned end = SM.getFileOffset(ELoc);
   const clang::LangOptions& LO = m_CI.getLangOpts();
   end += clang::Lexer::MeasureTokenLength(ELoc, SM, LO);
   std::pair<const char*, const char*> buf = std::make_pair(
      SM.getBuffer(m_InterpreterFile)->getBufferStart(),
      SM.getBuffer(m_InterpreterFile)->getBufferEnd());
   std::string str(buf.first + start, end - start);
   m_macros.push_back("#define " + str);
}

void
MacroDetector::MacroUndefined(const clang::IdentifierInfo* II,
                              const clang::MacroInfo* MI)
{
   if (MI->isBuiltinMacro()) {
      return;
   }
   clang::SourceManager& SM = m_CI.getSourceManager();
   clang::FileID mainFileID = SM.getMainFileID();
   if (SM.getFileID(MI->getDefinitionLoc()) == mainFileID) {
      if (II
          && ((II->getLength() == 35
               && II->getName() == "__CLING__MAIN_FILE_INCLUSION_MARKER")
              || (II->getLength() == 25
                  && II->getName() == "__CLING__MAIN_FILE_MARKER"))) {
         assert(m_InterpreterFileStatus == kIsActive && "Interpreter file is not active!");
         m_InterpreterFileStatus = kHasPassed;
      }
      return;
   }

   if (m_InterpreterFileStatus != kIsActive || m_InterpreterFile.isInvalid()
       || SM.getFileID(MI->getDefinitionLoc()) != m_InterpreterFile)
      return;

   clang::SourceLocation SLoc = SM.getInstantiationLoc(
                                   MI->getDefinitionLoc());
   clang::SourceLocation ELoc = SM.getInstantiationLoc(
                                   MI->getDefinitionEndLoc());
   unsigned start = SM.getFileOffset(SLoc);
   if (start < m_minpos) {
      return;
   }
   unsigned end = SM.getFileOffset(ELoc);
   const clang::LangOptions& LO = m_CI.getLangOpts();
   end += clang::Lexer::MeasureTokenLength(ELoc, SM, LO);
   std::pair<const char*, const char*> buf = std::make_pair(
      SM.getBuffer(m_InterpreterFile)->getBufferStart(),
      SM.getBuffer(m_InterpreterFile)->getBufferEnd());
   std::string str(buf.first + start, end - start);
   std::string::size_type pos = str.find(' ');
   if (pos != std::string::npos) {
      str = str.substr(0, pos);
   }
   m_macros.push_back("#undef " + str);
}

void
MacroDetector::FileChanged(clang::SourceLocation Loc, clang::PPCallbacks::FileChangeReason Reason,
                           clang::SrcMgr::CharacteristicKind /*FileType*/)
{
   // This callback is invoked whenever a source file is entered or exited.
   // The SourceLocation indicates the new location, and EnteringFile indicates
   // whether this is because we are entering a new include'd file (when true)
   // or whether we're exiting one because we ran off the end (when false).
   // It extracts the main file ID - the one requested by the interpreter
   // and not the string buffer used to prepent the previous global declarations
   // and to append the execution suffix.

   clang::SourceManager& SM = m_CI.getSourceManager();
   if (m_InterpreterFileStatus == kIsComingUp) {
      m_InterpreterFile = SM.getFileID(Loc);
      m_InterpreterFileStatus = kIsActive;
   } else if (m_InterpreterFileStatus == kIsActive) {
      if (Reason == EnterFile) {
         ++m_inclLevel;
         if (m_inclLevel == 1) {
            clang::FileID inclFileID = SM.getFileID(Loc);
            if (!inclFileID.isInvalid()) {
               const clang::FileEntry* FE = SM.getFileEntryForID(inclFileID);
               if (FE) {
                  std::string filename = FE->getName();
                  m_macros.push_back("#include \"" + filename + "\"");
               }
            }
         }
      } else if (Reason == ExitFile) {
         --m_inclLevel;
      }
   }
}

#endif // 0

//
//  Interpreter
//

//---------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------
Interpreter::Interpreter(const char* llvmdir /*= 0*/):
   m_llvm_context(0),
   m_CI(0),
   m_ASTCI(0),
   m_engine(0),
   m_prev_module(0),
   m_numCallWrappers(0),
   m_printAST(false)
{
   //
   //  Initialize the llvm library.
   //
   llvm::InitializeAllTargets();
   llvm::InitializeAllAsmPrinters();
   //
   //  Create an execution engine to use.
   //
   //m_llvm_context = &llvm::getGlobalContext();
   m_llvm_context = new llvm::LLVMContext;
   m_CI = createCI(llvmdir);
   m_ASTCI = createCI(llvmdir);
   m_prev_module = new llvm::Module("_Clang_first", *m_llvm_context);
   // Note: Engine takes ownership of the module.
   llvm::EngineBuilder builder(m_prev_module);
   std::string errMsg;
   builder.setErrorStr(&errMsg);
   builder.setEngineKind(llvm::EngineKind::JIT);
   m_engine = builder.create();
   if (!m_engine) {
      std::cerr << "Error: Unable to create the execution engine!\n";
      std::cerr << errMsg << '\n';
   } else {
      compileString("#include <stdio.h>\n");
   }

}

//---------------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------------
Interpreter::~Interpreter()
{
   //delete m_prev_module;
   //m_prev_module = 0; // Don't do this, the engine does it.
   delete m_engine;
   m_engine = 0;

   m_CI->takeLLVMContext(); // Don't take down the context with the CI.
   delete m_CI;
   m_CI = 0;

   m_ASTCI->takeLLVMContext(); // Don't take down the context with the CI.
   delete m_ASTCI;
   m_ASTCI = 0;

   delete m_llvm_context;
   m_llvm_context = 0;
   // Shutdown the llvm library.
   llvm::llvm_shutdown();
}

//---------------------------------------------------------------------------
// Note: Used by MetaProcessor.
Interpreter::InputType
Interpreter::analyzeInput(const std::string& contextSource,
                          const std::string& line, int& indentLevel,
                          std::vector<clang::FunctionDecl*>* fds)
{
   // Check if there is an explicitation continuation character.
   if (line.length() > 1 && line[line.length() - 2] == '\\') {
      indentLevel = 1;
      return Incomplete;
   }
   //
   //  Setup a compiler instance to work with.
   //
   clang::CompilerInstance* CI = getCI();
   if (!CI) {
      return Incomplete;
   }
   CI->createPreprocessor();
   llvm::MemoryBuffer* buffer =
      llvm::MemoryBuffer::getMemBufferCopy(line, "CLING");
   CI->getSourceManager().clearIDTables();
   CI->getSourceManager().createMainFileIDForMemBuffer(buffer);
   if (CI->getSourceManager().getMainFileID().isInvalid()) {
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return Incomplete;
   }
   clang::Token lastTok;
   bool tokWasDo = false;
   int stackSize = analyzeTokens(CI->getPreprocessor(), lastTok,
                                 indentLevel, tokWasDo);
   ///*reuseCI*/CI->takeLLVMContext();
   ///*reuseCI*/delete CI;
   ///*reuseCI*/CI = 0;
   if (stackSize < 0) {
      return TopLevel;
   }
   // tokWasDo is used for do { ... } while (...); loops
   if (
      !lastTok.is(clang::tok::semi) &&
      (
         !lastTok.is(clang::tok::r_brace) ||
         tokWasDo
      )
   ) {
      return Incomplete;
   }
   if (stackSize > 0) {
      return Incomplete;
   }
   CI = getCI();
   if (!CI) {
      return TopLevel;
   }
   CI->createPreprocessor();
   clang::Preprocessor& PP = CI->getPreprocessor();
   // Setting this ensures "foo();" is not a valid top-level declaration.
   //diag.setDiagnosticMapping(clang::diag::ext_missing_type_specifier,
   //                          clang::diag::MAP_ERROR);
   CI->getDiagnosticClient().BeginSourceFile(CI->getLangOpts(), &PP);
   //CI->createASTContext();
   CI->setASTContext(new clang::ASTContext(CI->getLangOpts(),
      PP.getSourceManager(), CI->getTarget(), PP.getIdentifierTable(),
      PP.getSelectorTable(), PP.getBuiltinInfo(), 0));

   CI->setASTConsumer(maybeGenerateASTPrinter());
   PP.getBuiltinInfo().InitializeBuiltins(PP.getIdentifierTable(),
                                          PP.getLangOptions().NoBuiltin);
   //std::string src = contextSource + buffer->getBuffer().str();
   struct : public clang::ASTConsumer {
      bool hadIncludedDecls;
      unsigned pos;
      unsigned maxPos;
      clang::SourceManager* sm;
      std::vector<clang::FunctionDecl*> fds;
      void HandleTopLevelDecl(clang::DeclGroupRef D) {
         for (
            clang::DeclGroupRef::iterator I = D.begin(), E = D.end();
            I != E;
            ++I
         ) {
            clang::FunctionDecl* FD = dyn_cast<clang::FunctionDecl>(*I);
            if (FD) {
               clang::SourceLocation Loc = FD->getTypeSpecStartLoc();
               if (!Loc.isValid()) {
                  continue;
               }
               if (sm->isFromMainFile(Loc)) {
                  unsigned offset =
                     sm->getFileOffset(sm->getInstantiationLoc(Loc));
                  if (offset >= pos) {
                     fds.push_back(FD);
                  }
               }
               else {
                  while (!sm->isFromMainFile(Loc)) {
                     const clang::SrcMgr::SLocEntry& Entry =
                        sm->getSLocEntry(
                           sm->getFileID(sm->getSpellingLoc(Loc)));
                     if (!Entry.isFile()) {
                        break;
                     }
                     Loc = Entry.getFile().getIncludeLoc();
                  }
                  unsigned offset = sm->getFileOffset(Loc);
                  if (offset >= pos) {
                     hadIncludedDecls = true;
                  }
               }
            }
         }
      }
   } consumer;
   consumer.hadIncludedDecls = false;
   consumer.pos = contextSource.length();
   consumer.maxPos = consumer.pos + buffer->getBuffer().size();
   consumer.sm = &CI->getSourceManager();
   buffer = llvm::MemoryBuffer::getMemBufferCopy(line, "CLING");
   if (!buffer) {
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return TopLevel;
   }
   CI->getSourceManager().clearIDTables();
   CI->getSourceManager().createMainFileIDForMemBuffer(buffer);
   if (CI->getSourceManager().getMainFileID().isInvalid()) {
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return TopLevel;
   }
   clang::ParseAST(PP, &CI->getASTConsumer(), CI->getASTContext());
   //CI->setASTConsumer(0);  // We may use the consumer below.
   //CI->setASTContext(0);  // We may use the consumer below.
   if (CI->hasPreprocessor()) {
      CI->getPreprocessor().EndSourceFile();
   }
   CI->clearOutputFiles(/*EraseFiles=*/CI->getDiagnostics().getNumErrors());
   CI->getDiagnosticClient().EndSourceFile();
#if 0
   if (
      CI->getDiagnostics().hadError(
         clang::diag::err_unterminated_block_comment)
   ) {
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return Incomplete;
   }
#endif // 0
   if (
      !CI->getDiagnostics().getNumErrors() &&
      (
         !consumer.fds.empty() ||
         consumer.hadIncludedDecls
      )
   ) {
      if (!consumer.fds.empty()) {
         fds->swap(consumer.fds);
      }
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return TopLevel;
   }
   ///*reuseCI*/CI->takeLLVMContext();
   ///*reuseCI*/delete CI;
   ///*reuseCI*/CI = 0;
   return Stmt;
}

//---------------------------------------------------------------------------
// Note: Used only by analyzeInput().
int Interpreter::analyzeTokens(clang::Preprocessor& PP,
                               clang::Token& lastTok, int& indentLevel,
                               bool& tokWasDo)
{
   std::stack<std::pair<clang::Token, clang::Token> > S; // Tok, PrevTok
   indentLevel = 0;
   PP.EnterMainSourceFile();
   clang::Token Tok;
   PP.Lex(Tok);
   while (Tok.isNot(clang::tok::eof)) {
      if (Tok.is(clang::tok::l_square)) {
         S.push(std::make_pair(Tok, lastTok));
      }
      else if (Tok.is(clang::tok::l_paren)) {
         S.push(std::make_pair(Tok, lastTok));
      }
      else if (Tok.is(clang::tok::l_brace)) {
         S.push(std::make_pair(Tok, lastTok));
         indentLevel++;
      }
      else if (Tok.is(clang::tok::r_square)) {
         if (S.empty() || S.top().first.isNot(clang::tok::l_square)) {
            std::cout << "Unmatched [\n";
            return -1;
         }
         tokWasDo = false;
         S.pop();
      }
      else if (Tok.is(clang::tok::r_paren)) {
         if (S.empty() || S.top().first.isNot(clang::tok::l_paren)) {
            std::cout << "Unmatched (\n";
            return -1;
         }
         tokWasDo = false;
         S.pop();
      }
      else if (Tok.is(clang::tok::r_brace)) {
         if (S.empty() || S.top().first.isNot(clang::tok::l_brace)) {
            std::cout << "Unmatched {\n";
            return -1;
         }
         tokWasDo = S.top().second.is(clang::tok::kw_do);
         S.pop();
         indentLevel--;
      }
      lastTok = Tok;
      PP.Lex(Tok);
   }
   int result = S.size();
   // TODO: We need to properly account for indent-level for blocks that do not
   //       have braces... such as:
   //
   //       if (X)
   //         Y;
   //
   // TODO: Do-while without braces doesn't work, e.g.:
   //
   //       do
   //         foo();
   //       while (bar());
   //
   // Both of the above could be solved by some kind of rewriter-pass that would
   // insert implicit braces (or simply a more involved analysis).
   // Also try to match preprocessor conditionals...
   if (result == 0) {
      clang::Lexer Lexer(PP.getSourceManager().getMainFileID(),
                         PP.getSourceManager().getBuffer(
                            PP.getSourceManager().getMainFileID()),
                         PP.getSourceManager(), PP.getLangOptions());
      Lexer.LexFromRawLexer(Tok);
      while (Tok.isNot(clang::tok::eof)) {
         if (Tok.is(clang::tok::hash)) {
            Lexer.LexFromRawLexer(Tok);
            if (clang::IdentifierInfo *II = PP.LookUpIdentifierInfo(Tok)) {
               switch (II->getPPKeywordID()) {
                  case clang::tok::pp_if:
                  case clang::tok::pp_ifdef:
                  case clang::tok::pp_ifndef:
                     ++result;
                     break;
                  case clang::tok::pp_endif:
                     if (result == 0)
                        return -1; // Nesting error.
                     --result;
                     break;
                  default:
                     break;
               }
            }
         }
         Lexer.LexFromRawLexer(Tok);
      }
   }
   return result;
}

void
Interpreter::processLine(const std::string& input_line)
{
   //
   //  Transform the input line to implement cint
   //  command line semantics (declarations are global),
   //  and compile to produce a module.
   //
   bool withStatements = true;
   llvm::Module* m = makeModuleFromCommandLine(input_line, withStatements);
   if (!m) {
      return;
   }
   //
   //  Transfer global mappings from previous module.
   //
   copyGlobalMappings(m_engine, m_prev_module, m);
   //
   //  All done with previous module, delete it.
   //
   {
      bool ok = m_engine->removeModule(m_prev_module);
      if (!ok) {
         //fprintf(stderr, "Previous module not found in execution engine!\n");
      }
      delete m_prev_module;
      m_prev_module = 0;
   }
   //
   //  Give new module to the execution engine.
   //
   m_engine->addModule(m); // Note: The engine takes ownership of the module.
   //
   //  Run it using the JIT.
   //
   if (withStatements)
      executeCommandLine();
   //
   //  All done, save module to transfer mappings
   //  on the next run.
   //
   m_prev_module = m;
}

llvm::Module*
Interpreter::makeModuleFromCommandLine(const std::string& input_line, bool& haveStatements)
{
   clang::CompilerInstance* CI = 0;
   //
   //  Wrap input into a function along with
   //  the saved global declarations.
   //
   std::string src;
   src += "void __cling_internal() {\n";
   src += "#define __CLING__MAIN_FILE_MARKER __cling__prompt\n";
   src += input_line;
   src += "\n#undef __CLING__MAIN_FILE_MARKER\n";
   src += "\n} // end __cling_internal()\n";
   //fprintf(stderr, "input_line:\n%s\n", src.c_str());
   std::string wrapped;
   createWrappedSrc(src, wrapped, haveStatements);
   if (!wrapped.size()) {
      return 0;
   }
   //
   //  Send the wrapped code through the
   //  frontend to produce a translation unit.
   //
   CI = compileString(wrapped);
   if (!CI) {
      return 0;
   }
   ///**/   ///*reuseCI*/CI->takeLLVMContext();
   ///**/   ///*reuseCI*/delete CI;
   ///**/   ///*reuseCI*/CI = 0;
   ///**/   return 0;
   // Note: We have a valid compiler instance at this point.
   clang::TranslationUnitDecl* tu =
      CI->getASTContext().getTranslationUnitDecl();
   if (!tu) { // Parse failed, return.
      fprintf(stderr, "Wrapped parse failed, no translation unit!\n");
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 0;
   }
   //
   //  Send the translation unit through the
   //  llvm code generator to make a module.
   //
   llvm::Module* m = doCodegen(CI, "CLING");
   if (!m) {
      fprintf(stderr, "Module creation failed!\n");
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 0;
   }
   //
   //  All done with the compiler instance,
   //  get rid of it.
   //
   ///*reuseCI*/CI->takeLLVMContext();
   ///*reuseCI*/delete CI;
   ///*reuseCI*/CI = 0;
   //printModule(m);
   return m;
}

void
Interpreter::createWrappedSrc(const std::string& src, std::string& wrapped, bool& haveStatements)
{
   haveStatements = false;
   std::vector<clang::Stmt*> stmts;
   clang::CompilerInstance* CI = createStatementList(src, stmts);
   if (!CI) {
      wrapped.clear();
      return;
   }
   //
   //  Rewrite the source code to support cint command
   //  line semantics.  We must move variable declarations
   //  to the global namespace and change the code so that
   //  the new global variables are used.
   //
   std::string held_globals;
   std::string wrapped_globals;
   std::string wrapped_stmts;
   {
      clang::SourceManager& SM = CI->getSourceManager();
      const clang::LangOptions& LO = CI->getLangOpts();
      std::vector<clang::Stmt*>::iterator stmt_iter = stmts.begin();
      std::vector<clang::Stmt*>::iterator stmt_end = stmts.end();
      for (; stmt_iter != stmt_end; ++stmt_iter) {
         clang::Stmt* cur_stmt = *stmt_iter;
         std::string stmt_string;
         {
            std::pair<unsigned, unsigned> r =
               getStmtRangeWithSemicolon(cur_stmt, SM, LO);
            stmt_string = src.substr(r.first, r.second - r.first);
            //fprintf(stderr, "stmt: %s\n", stmt_string.c_str());
         }
         //
         //  Handle expression statements.
         //
         {
            const clang::Expr* expr = dyn_cast<clang::Expr>(cur_stmt);
            if (expr) {
               //fprintf(stderr, "have expr stmt.\n");
               wrapped_stmts.append(stmt_string + '\n');
               continue;
            }
         }
         //
         //  Handle everything that is not a declaration statement.
         //
         const clang::DeclStmt* DS = dyn_cast<clang::DeclStmt>(cur_stmt);
         if (!DS) {
            //fprintf(stderr, "not expr, not declaration.\n");
            wrapped_stmts.append(stmt_string + '\n');
            continue;
         }
         //
         //  Loop over each declarator in the declaration statement.
         //
         clang::DeclStmt::const_decl_iterator D = DS->decl_begin();
         clang::DeclStmt::const_decl_iterator E = DS->decl_end();
         for (; D != E; ++D) {
            //
            //  Handle everything that is not a variable declarator.
            //
            const clang::VarDecl* VD = dyn_cast<clang::VarDecl>(*D);
            if (!VD) {
               if (DS->isSingleDecl()) {
                  //fprintf(stderr, "decl, not var decl, single decl.\n");
                  wrapped_globals.append(stmt_string + '\n');
                  held_globals.append(stmt_string + '\n');
                  continue;
               }
               //fprintf(stderr, "decl, not var decl, not single decl.\n");
               clang::SourceLocation SLoc =
                  SM.getInstantiationLoc((*D)->getLocStart());
               clang::SourceLocation ELoc =
                  SM.getInstantiationLoc((*D)->getLocEnd());
               std::pair<unsigned, unsigned> r =
                  getRangeWithSemicolon(SLoc, ELoc, SM, LO);
               std::string decl = src.substr(r.first, r.second - r.first);
               wrapped_globals.append(decl + ";\n");
               held_globals.append(decl + ";\n");
               continue;
            }
            //
            //  Handle a variable declarator.
            //
            std::string decl = VD->getNameAsString();
            // FIXME: Probably should not remove the qualifiers!
            VD->getType().getUnqualifiedType().
            getAsStringInternal(decl, clang::PrintingPolicy(LO));
            const clang::Expr* I = VD->getInit();
            //
            //  Handle variable declarators with no initializer
            //  or with an initializer that is a constructor call.
            //
            if (!I || dyn_cast<clang::CXXConstructExpr>(I)) {
               if (!I) {
                  //fprintf(stderr, "var decl, no init.\n");
               }
               else {
                  //fprintf(stderr, "var decl, init is constructor.\n");
               }
               wrapped_globals.append(decl + ";\n"); // FIXME: wrong for constructor
               held_globals.append(decl + ";\n");
               continue;
            }
            //
            //  Handle variable declarators with a constant initializer.
            //
            if (I->isConstantInitializer(CI->getASTContext(), false)) {
               //fprintf(stderr, "var decl, init is const.\n");
               std::pair<unsigned, unsigned> r = getStmtRange(I, SM, LO);
               wrapped_globals.append(decl + " = " +
                                      src.substr(r.first, r.second - r.first) + ";\n");
               held_globals.append(decl + ";\n");
               continue;
            }
            //
            //  Handle variable declarators whose initializer is not a list.
            //
            const clang::InitListExpr* ILE = dyn_cast<clang::InitListExpr>(I);
            if (!ILE) {
               //fprintf(stderr, "var decl, init is not list.\n");
               std::pair<unsigned, unsigned> r = getStmtRange(I, SM, LO);
               wrapped_stmts.append(std::string(VD->getName())  + " = " +
                                    src.substr(r.first, r.second - r.first) + ";\n");
               wrapped_globals.append(decl + ";\n");
               held_globals.append(decl + ";\n");
               continue;
            }
            //
            //  Handle variable declarators with an initializer list.
            //
            //fprintf(stderr, "var decl, init is list.\n");
            unsigned numInits = ILE->getNumInits();
            for (unsigned j = 0; j < numInits; ++j) {
               std::string stmt;
               llvm::raw_string_ostream stm(stmt);
               stm << VD->getNameAsString() << "[" << j << "] = ";
               std::pair<unsigned, unsigned> r =
                  getStmtRange(ILE->getInit(j), SM, LO);
               stm << src.substr(r.first, r.second - r.first) << ";\n";
               wrapped_stmts.append(stm.str());
            }
            wrapped_globals.append(decl + ";\n");
            held_globals.append(decl + ";\n");
         }
      }
      haveStatements = !wrapped_stmts.empty();
      if (haveStatements) {
         wrapped_stmts = "void __cling_internal() {\n" + wrapped_stmts;
         wrapped_stmts += "\n} // end __cling_internal()\n";
      }
   }
   //
   //fprintf(stderr, "m_globalDeclarations:\n%s\n",
   //   m_globalDeclarations.c_str());
   //fprintf(stderr, "held_globals:\n%s\n", held_globals.c_str());
   //fprintf(stderr, "---\n");
   //fprintf(stderr, "wrapped_globals:\n%s\n", wrapped_globals.c_str());
   //fprintf(stderr, "wrapped_stmts:\n%s\n", wrapped_stmts.c_str());
   wrapped += wrapped_globals + wrapped_stmts;
   //
   //  Shutdown parse.
   //
   CI->setASTConsumer(0);
   CI->setASTContext(0);
   //if (CI->hasPreprocessor()) {
   //   CI->getPreprocessor().EndSourceFile();
   //}
   //CI->clearOutputFiles(/*EraseFiles=*/CI->getDiagnostics().getNumErrors());
   //CI->getDiagnosticClient().EndSourceFile();
   unsigned err_count = CI->getDiagnostics().getNumErrors();
   if (err_count) {
      wrapped.clear();
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return;
   }
   ///*reuseCI*/CI->takeLLVMContext();
   ///*reuseCI*/delete CI;
   ///*reuseCI*/CI = 0;
}

clang::CompilerInstance*
Interpreter::createStatementList(const std::string& srcCode,
                                 std::vector<clang::Stmt*>& stmts)
{
   clang::CompilerInstance* CI = getCI();
   if (!CI) {
      return 0;
   }
   CI->createPreprocessor();
   clang::Preprocessor& PP = CI->getPreprocessor();
   //PP.addPPCallbacks(new MacroDetector(*CI, m_globalDeclarations.size()));
   CI->getDiagnosticClient().BeginSourceFile(CI->getLangOpts(), &PP);
   //CI->createASTContext();
   CI->setASTContext(new clang::ASTContext(CI->getLangOpts(),
      PP.getSourceManager(), CI->getTarget(), PP.getIdentifierTable(),
      PP.getSelectorTable(), PP.getBuiltinInfo(), 0));
   // Create an ASTConsumer for this frontend run which
   // will produce a list of statements seen.
   StmtSplitter splitter(stmts);
   FunctionBodyConsumer* consumer =
      new FunctionBodyConsumer(splitter, "__cling_internal");
   CI->setASTConsumer(consumer);
   PP.getBuiltinInfo().InitializeBuiltins(PP.getIdentifierTable(),
                                          PP.getLangOptions().NoBuiltin);
   llvm::MemoryBuffer* SB =
      llvm::MemoryBuffer::getMemBufferCopy(srcCode, "CLING");
   if (!SB) {
      fprintf(stderr, "Interpreter::createStatementList: Failed to create "
                      "memory buffer!\n");
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 0;
   }
   CI->getSourceManager().clearIDTables();
   CI->getSourceManager().createMainFileIDForMemBuffer(SB);
   if (CI->getSourceManager().getMainFileID().isInvalid()) {
      fprintf(stderr, "Interpreter::createStatementList: Failed to create "
                      "main file id!\n");
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 0;
   }
   clang::ParseAST(PP, &CI->getASTConsumer(), CI->getASTContext());
   //CI->setASTConsumer(0); // We still need these later.
   //CI->setASTContext(0); // We still need these later.
   if (CI->hasPreprocessor()) {
      CI->getPreprocessor().EndSourceFile();
   }
   CI->clearOutputFiles(/*EraseFiles=*/CI->getDiagnostics().getNumErrors());
   CI->getDiagnosticClient().EndSourceFile();
   unsigned err_count = CI->getDiagnostics().getNumErrors();
   if (err_count) {
      fprintf(stderr, "Interpreter::createStatementList: Parse failed!\n");
      CI->setASTConsumer(0);
      CI->setASTContext(0);
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 0;
   }
   return CI;
}

clang::CompilerInstance*
Interpreter::createCI(const char* llvmdir /*=0*/)
{
   //
   //  Create and setup a compiler instance.
   //
   clang::CompilerInstance* CI = new clang::CompilerInstance();
   //bool first_time = true;
   CI->setLLVMContext(m_llvm_context);
   {
      //
      //  Buffer the error messages while we process
      //  the compiler options.
      //
      clang::TextDiagnosticBuffer *DiagsBuffer = new clang::TextDiagnosticBuffer();
      // Diags takes ownership of DiagsBuffer
      clang::Diagnostic Diags(DiagsBuffer);
      clang::CompilerInvocation::CreateFromArgs(CI->getInvocation(),
            fake_argv + 1, fake_argv + fake_argc, Diags);
      if (
         CI->getHeaderSearchOpts().UseBuiltinIncludes &&
         CI->getHeaderSearchOpts().ResourceDir.empty()
      ) {
         if (llvmdir) {
            llvm::sys::Path P(llvmdir);
            P.appendComponent("lib");
            P.appendComponent("clang");
            P.appendComponent(CLANG_VERSION_STRING);
            
            CI->getHeaderSearchOpts().ResourceDir = P.str();
         } else {
            // FIXME: The first arg really does need to be argv[0] on FreeBSD.
            //
            // Note: The second arg is not used for Apple, FreeBSD, Linux,
            //       or cygwin, and can only be used on systems which support
            //       the use of dladdr().
            //
            // Note: On linux and cygwin this uses /proc/self/exe to find the path.
            //
            // Note: On Apple it uses _NSGetExecutablePath().
            //
            // Note: On FreeBSD it uses getprogpath().
            //
            // Note: Otherwise it uses dladdr().
            //
            CI->getHeaderSearchOpts().ResourceDir =
               clang::CompilerInvocation::GetResourcesPath("cling",
                                                           (void*)(intptr_t) locate_cling_executable);
         }
      }
      CI->createDiagnostics(fake_argc - 1, const_cast<char**>(fake_argv + 1));
      if (!CI->hasDiagnostics()) {
         CI->takeLLVMContext();
         delete CI;
         CI = 0;
         return 0;
      }
      // Output the buffered error messages now.
      DiagsBuffer->FlushDiagnostics(CI->getDiagnostics());
      if (CI->getDiagnostics().getNumErrors()) {
         CI->takeLLVMContext();
         delete CI;
         CI = 0;
         return 0;
      }
   }
   CI->setTarget(clang::TargetInfo::CreateTargetInfo(CI->getDiagnostics(),
                 CI->getTargetOpts()));
   if (!CI->hasTarget()) {
      CI->takeLLVMContext();
      delete CI;
      CI = 0;
      return 0;
   }
   CI->getTarget().setForcedLangOptions(CI->getLangOpts());
   CI->createFileManager();
   //
   //  If we are managing a permanent CI,
   //  the code looks like this:
   //
   //if (first_time) {
   //   CI->createSourceManager();
   //   first_time = false;
   //}
   //else {
   //   CI->getSourceManager().clearIDTables();
   //}
   CI->createSourceManager();
   //CI->createPreprocessor(); // Note: This single line takes almost all the time!
   return CI;
}

clang::CompilerInstance*
Interpreter::getCI()
{
   if (!m_CI) {
      return 0;
   }
   m_CI->createDiagnostics(fake_argc - 1, const_cast<char**>(fake_argv + 1));
   if (!m_CI->hasDiagnostics()) {
      m_CI->takeLLVMContext();
      delete m_CI;
      m_CI = 0;
      return 0;
   }
   return m_CI;
}

clang::CompilerInstance*
Interpreter::getASTCI()
{
   if (!m_ASTCI) {
      return 0;
   }
   m_ASTCI->createDiagnostics(fake_argc - 1, const_cast<char**>(fake_argv + 1));
   if (!m_ASTCI->hasDiagnostics()) {
      m_ASTCI->takeLLVMContext();
      delete m_ASTCI;
      m_ASTCI = 0;
      return 0;
   }
   return m_ASTCI;
}

clang::ASTConsumer*
Interpreter::maybeGenerateASTPrinter() const
{
   if (m_printAST) {
      return clang::CreateASTPrinter(&llvm::outs());
   }
   return new clang::ASTConsumer();
}

namespace {
   class MutableMemoryBuffer: public llvm::MemoryBuffer {
      std::string m_FileID;
      size_t m_Alloc;
   protected:
      void maybeRealloc(llvm::StringRef code, size_t oldlen) {
         size_t applen = code.size();
         char* B = 0;
         if (oldlen) {
            B = const_cast<char*>(getBufferStart());
            assert(!B[oldlen] && "old buffer is not 0 terminated!");
            // remove trailing '\0'
            --oldlen;
         }
         size_t newlen = oldlen + applen + 1;
         if (newlen > m_Alloc) {
            m_Alloc += 64*1024;
            B = (char*)realloc(B, m_Alloc);
         }
         memcpy(B + oldlen, code.data(), applen);
         B[newlen - 1] = 0;
         init(B, B + newlen - 1);
      }
      
   public:
      MutableMemoryBuffer(llvm::StringRef Code, llvm::StringRef Name)
         : m_FileID(Name), m_Alloc(0) {
         maybeRealloc(Code, 0);
      }

      virtual ~MutableMemoryBuffer() {
         free((void*)getBufferStart());
      }

      void append(llvm::StringRef code) {
         assert(getBufferSize() && "buffer is empty!");
         maybeRealloc(code, getBufferSize());
      }
      virtual const char *getBufferIdentifier() const {
         return m_FileID.c_str();
      }
   };
}


clang::CompilerInstance*
Interpreter::compileString(const std::string& srcCode)
{
   clang::CompilerInstance* CI = getASTCI();
   if (!CI) {
      return 0;
   }

   static clang::Sema* S = 0;
   static clang::Parser* P = 0;
   static MutableMemoryBuffer* MMB = 0;
   if (!S) {
      CI->createPreprocessor();
   }

   clang::Preprocessor& PP = CI->getPreprocessor();
   clang::ASTConsumer *Consumer = 0;

   if (!S) {
      CI->getDiagnosticClient().BeginSourceFile(CI->getLangOpts(), &PP);
      clang::ASTContext *Ctx = new clang::ASTContext(CI->getLangOpts(),
         PP.getSourceManager(), CI->getTarget(), PP.getIdentifierTable(),
         PP.getSelectorTable(), PP.getBuiltinInfo(), 0);
      CI->setASTContext(Ctx);
      CI->setASTConsumer(maybeGenerateASTPrinter());
      Consumer = &CI->getASTConsumer();
      PP.getBuiltinInfo().InitializeBuiltins(PP.getIdentifierTable(),
                                             PP.getLangOptions().NoBuiltin);
      //llvm::MemoryBuffer* SB =
      //   llvm::MemoryBuffer::getMemBufferCopy(srcCode, "CLING");
      MMB = new MutableMemoryBuffer(srcCode, "CLING");
      if (!MMB) {
         fprintf(stderr, "Interpreter::compileString: Failed to create memory "
                 "buffer!\n");
         ///*reuseCI*/CI->takeLLVMContext();
         ///*reuseCI*/delete CI;
         ///*reuseCI*/CI = 0;
         return 0;
      }

      CI->getSourceManager().clearIDTables();
      CI->getSourceManager().createMainFileIDForMemBuffer(MMB);
      if (CI->getSourceManager().getMainFileID().isInvalid()) {
         fprintf(stderr, "Interpreter::compileString: Failed to create main "
                 "file id!\n");
         ///*reuseCI*/CI->takeLLVMContext();
         ///*reuseCI*/delete CI;
         ///*reuseCI*/CI = 0;
         return 0;
      }

      bool CompleteTranslationUnit = false;
      clang::CodeCompleteConsumer *CompletionConsumer = 0;
      S = new clang::Sema(PP, *Ctx, *Consumer, CompleteTranslationUnit, CompletionConsumer);
      P = new clang::Parser(PP, *S);
      PP.EnterMainSourceFile();

      // Initialize the parser.
      P->Initialize();

      Consumer->Initialize(*Ctx);

      if (clang::SemaConsumer *SC = dyn_cast<clang::SemaConsumer>(Consumer))
         SC->InitializeSema(*S);
   } else {
      MMB->append(srcCode);
   }

   // BEGIN REPLACEMENT clang::ParseAST(PP, &CI->getASTConsumer(), CI->getASTContext());

   if (!Consumer) Consumer = &CI->getASTConsumer();
   clang::Parser::DeclGroupPtrTy ADecl;

   while (!P->ParseTopLevelDecl(ADecl)) {  // Not end of file.
      // If we got a null return and something *was* parsed, ignore it.  This
      // is due to a top-level semicolon, an action override, or a parse error
      // skipping something.
      if (ADecl)
         Consumer->HandleTopLevelDecl(ADecl.getAsVal<clang::DeclGroupRef>());
   };

   // Process any TopLevelDecls generated by #pragma weak.
   for (llvm::SmallVector<clang::Decl*,2>::iterator
           I = S->WeakTopLevelDecls().begin(),
           E = S->WeakTopLevelDecls().end(); I != E; ++I)
      Consumer->HandleTopLevelDecl(clang::DeclGroupRef(*I));

   clang::ASTContext *Ctx = &CI->getASTContext();

   Consumer->HandleTranslationUnit(*Ctx);

   //if (SemaConsumer *SC = dyn_cast<SemaConsumer>(Consumer))
   //   SC->ForgetSema();

   // END REPLACEMENT clang::ParseAST(PP, &CI->getASTConsumer(), CI->getASTContext());


   //CI->setASTConsumer(0);
   //if (CI->hasPreprocessor()) {
   //   CI->getPreprocessor().EndSourceFile();
   //}
   //CI->clearOutputFiles(/*EraseFiles=*/CI->getDiagnostics().getNumErrors());
   CI->getDiagnosticClient().EndSourceFile();
   unsigned err_count = CI->getDiagnostics().getNumErrors();
   if (err_count) {
      fprintf(stderr, "Interpreter::compileString: Parse failed!\n");
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 0;
   }
   return CI;
}

clang::CompilerInstance*
Interpreter::compileFile(const std::string& filename, const std::string* trailcode /*=0*/)
{
   std::string code;
   code += "#define __CLING__MAIN_FILE_INCLUSION_MARKER \"" + filename + "\"\n";
   code += "#include \"" + filename + "\"\n";
   code += "#undef __CLING__MAIN_FILE_INCLUSION_MARKER\n";
   if (trailcode) code += *trailcode;
   return compileString(code);
}

llvm::Module*
Interpreter::doCodegen(clang::CompilerInstance* CI, const std::string& filename)
{
   clang::TranslationUnitDecl* tu =
      CI->getASTContext().getTranslationUnitDecl();
   if (!tu) {
      fprintf(
           stderr
         , "Interpreter::doCodegen: No translation unit decl in passed "
           "ASTContext!\n"
      );
      return 0;
   }
   llvm::OwningPtr<clang::CodeGenerator> codeGen(
      CreateLLVMCodeGen(CI->getDiagnostics(), filename, CI->getCodeGenOpts(),
                        CI->getLLVMContext()));
   codeGen->Initialize(CI->getASTContext());
   clang::TranslationUnitDecl::decl_iterator iter = tu->decls_begin();
   clang::TranslationUnitDecl::decl_iterator iter_end = tu->decls_end();
   //fprintf(stderr, "Running code generation.\n");
   for (; iter != iter_end; ++iter) {
      codeGen->HandleTopLevelDecl(clang::DeclGroupRef(*iter));
   }
   codeGen->HandleTranslationUnit(CI->getASTContext());
   //fprintf(stderr, "Finished code generation.\n");
   llvm::Module* m = codeGen->ReleaseModule();
   if (!m) {
      fprintf(
           stderr
         , "Interpreter::doCodegen: Code generation did not create a module!\n"
      );
      return 0;
   }
   return m;
}

int
Interpreter::verifyModule(llvm::Module* m)
{
   //
   //  Verify generated module.
   //
   bool mod_has_errs = llvm::verifyModule(*m, llvm::PrintMessageAction);
   if (mod_has_errs) {
      return 1;
   }
   return 0;
}

void
Interpreter::printModule(llvm::Module* m)
{
   //
   //  Print module LLVM code in human-readable form.
   //
   llvm::PassManager PM;
   PM.add(llvm::createPrintModulePass(&llvm::outs()));
   PM.run(*m);
}

void
Interpreter::executeCommandLine()
{
   //fprintf(stderr, "Running generated code with JIT.\n");
   //
   //  Run global initialization.
   //
   
   // incremental version of m_engine->runStaticConstructorsDestructors(false);

   for (unsigned m = m_posInitGlobals.first, e = m_engine->modules_size(); m != e; ++m) {
      const llvm::Module* module = m_engine->modules_at(m);
      llvm::GlobalVariable *GV = module->getNamedGlobal("llvm.global_ctors");
      llvm::ConstantArray *InitList = dyn_cast<llvm::ConstantArray>(GV->getInitializer());
      m_posInitGlobals.first = m;
      if (!InitList) continue;
      for (unsigned i = m_posInitGlobals.second, e = InitList->getNumOperands(); i != e; ++i) {
         m_posInitGlobals.second = 0;
         if (llvm::ConstantStruct *CS = 
             dyn_cast<llvm::ConstantStruct>(InitList->getOperand(i))) {
            if (CS->getNumOperands() != 2) return; // Not array of 2-element structs.

            llvm::Constant *FP = CS->getOperand(1);
            if (FP->isNullValue())
               break;  // Found a null terminator, exit.
   
            if (llvm::ConstantExpr *CE = dyn_cast<llvm::ConstantExpr>(FP))
               if (CE->isCast())
                  FP = CE->getOperand(0);
            if (llvm::Function *F = dyn_cast<llvm::Function>(FP)) {
               // Execute the ctor/dtor function!
               m_engine->runFunction(F, std::vector<llvm::GenericValue>());
            }
         }
         m_posInitGlobals.second = i + 1;
      }
   }
   

   //
   //  Run the function __cling_internal().
   //
   // Create argument list for function.
   std::vector<llvm::GenericValue> args;
   //llvm::GenericValue arg1;
   //arg1.IntVal = llvm::APInt(32, 5);
   //args.push_back(arg1);
   llvm::Function* f = m_engine->FindFunctionNamed("_Z16__cling_internalv");
   if (!f) {
      fprintf(
           stderr
         , "Interpreter::executeCommandLine: Could not find the "
           "__cling_internal() function!\n"
      );
      return;
   }
   llvm::GenericValue ret = m_engine->runFunction(f, args);
   //
   //fprintf(stderr, "Finished running generated code with JIT.\n");
   //
   // Print the result.
   //llvm::outs() << "Result: " << ret.IntVal << "\n";
   // Run global destruction.
   //m_engine->runStaticConstructorsDestructors(true);
   m_engine->freeMachineCodeForFunction(f);
}

llvm::sys::Path
Interpreter::findDynamicLibrary(const std::string& filename,
                                bool addPrefix /* = true */,
                                bool addSuffix /* = true */) const
{
   // Check wether filename is a dynamic library, either through absolute path
   // or in one of the system library paths.
   {
      llvm::sys::Path FullPath(filename);
      if (FullPath.isDynamicLibrary())
         return FullPath;
   }

   std::vector<llvm::sys::Path> LibPaths;
   llvm::sys::Path::GetSystemLibraryPaths(LibPaths);
   for (unsigned i = 0; i < LibPaths.size(); ++i) {
      llvm::sys::Path FullPath(LibPaths[i]);
      FullPath.appendComponent(filename);
      if (FullPath.isDynamicLibrary())
         return FullPath;
   }

   if (addPrefix) {
      static const std::string prefix("lib");
      llvm::sys::Path found = findDynamicLibrary(prefix + filename, false, addSuffix);
      if (found.isDynamicLibrary())
         return found;
   }

   if (addSuffix) {
      llvm::sys::Path found = findDynamicLibrary(filename + LTDL_SHLIB_EXT, false, false);
      if (found.isDynamicLibrary())
         return found;
   }

   return llvm::sys::Path();
}

int
Interpreter::loadFile(const std::string& filename, const std::string* trailcode /*=0*/)
{
   llvm::sys::Path DynLib = findDynamicLibrary(filename);
   if (DynLib.isDynamicLibrary()) {
      std::string errMsg;
      bool err =
         llvm::sys::DynamicLibrary::LoadLibraryPermanently(DynLib.str().c_str(), &errMsg);
      if (err) {
         //llvm::errs() << "Could not load shared library: " << errMsg << '\n';
         fprintf(stderr
                 , "Interpreter::loadFile: Could not load shared library!\n"
                 );
         fprintf(stderr, "%s\n", errMsg.c_str());
         return 1;
      }
      return 0;
   }
   clang::CompilerInstance* CI = compileFile(filename, trailcode);
   if (!CI) {
      return 1;
   }
   clang::TranslationUnitDecl* tu =
      CI->getASTContext().getTranslationUnitDecl();
   if (!tu) { // Parse failed, return.
      fprintf(
           stderr
         , "Interpreter::loadFile: No translation unit decl found!\n"
      );
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 1;
   }
   llvm::Module* m = doCodegen(CI, filename);
   if (!m) {
      //fprintf(stderr, "Error: Backend did not create a module!\n");
      ///*reuseCI*/CI->takeLLVMContext();
      ///*reuseCI*/delete CI;
      ///*reuseCI*/CI = 0;
      return 1;
   }
   //--
   //llvm::Linker linker("executable", llvm::CloneModule(m_prev_module));
   //if (linker.LinkInModule(m, errMsg)) {
   //   m = linker.releaseModule();
   //   delete m;
   //   m = 0;
   //   return 0;
   //}
   //m = linker.releaseModule();
   //
   //  Transfer global mappings from previous module.
   //
   //copyGlobalMappings(m_engine, m_prev_module, m);
   //
   //  All done with previous module, delete it.
   //
   //{
   //   bool ok = m_engine->removeModule(m_prev_module);
   //   if (!ok) {
   //      //fprintf(stderr, "Previous module not found in execution engine!\n");
   //   }
   //   delete m_prev_module;
   //   m_prev_module = 0;
   //}
   //--
   //
   //  Give new module to the execution engine.
   //
   m_engine->addModule(m); // Note: The engine takes ownership of the module.
   ///*reuseCI*/CI->takeLLVMContext();
   ///*reuseCI*/delete CI;
   ///*reuseCI*/CI = 0;
   return 0;
}

int
Interpreter::executeFile(const std::string& filename)
{
   std::string::size_type pos = filename.find_last_of('/');
   if (pos == std::string::npos) {
      pos = 0;
   }
   else {
      ++pos;
   }

   // Note: We are assuming the filename does not end in slash here.
   std::string funcname(filename, pos);
   std::string::size_type endFileName = std::string::npos;

   std::string args;
   pos = funcname.find_first_of('(');
   if (pos != std::string::npos) {
      std::string::size_type posParamsEnd = funcname.find_last_of(')');
      if (posParamsEnd != std::string::npos) {
         args = funcname.substr(pos, posParamsEnd - pos + 1);
         endFileName = filename.find_first_of('(');
      }
   }

   //fprintf(stderr, "funcname: %s\n", funcname.c_str());
   pos = funcname.find_last_of('.');
   if (pos != std::string::npos) {
      funcname.erase(pos);
      //fprintf(stderr, "funcname: %s\n", funcname.c_str());
   }

   std::ostringstream swrappername;
   swrappername << "__cling__internal_wrapper" << m_numCallWrappers++;
   std::string wrapper = "extern \"C\" void ";
   wrapper += swrappername.str() + "() {\n  " + funcname + "(" + args + ");\n}";
   int err = loadFile(filename.substr(0, endFileName), &wrapper);
   if (err) {
      return err;
   }
   executeFunction(swrappername.str());
   return 0;
}

void
Interpreter::executeFunction(const std::string& funcname)
{
   // Call an extern C function without arguments
   llvm::Function* f = m_engine->FindFunctionNamed(funcname.c_str());
   if (!f) {
      fprintf(
           stderr
         , "Interpreter::executeFunction: Could not find function named: %s\n"
         , funcname.c_str()
      );
      return;
   }
   std::vector<llvm::GenericValue> args;
   llvm::GenericValue ret = m_engine->runFunction(f, args);
   //
   //fprintf(stderr, "Finished running generated code with JIT.\n");
   //
   // Print the result.
   //llvm::outs() << "Result: " << ret.IntVal << "\n";
   // Run global destruction.
   //m_engine->runStaticConstructorsDestructors(true);
   m_engine->freeMachineCodeForFunction(f);
}

} // namespace cling

