//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id: Interpreter.cpp 27029 2008-12-19 13:16:34Z axel $
// author:  Axel Naumann <axel@cern.ch>
//------------------------------------------------------------------------------

#include <cling/UserInterface/UserInterface.h>

#include <cling/Interpreter/Interpreter.h>

#include <llvm/System/DynamicLibrary.h>
#include <llvm/System/Path.h>
#include <llvm/Support/MemoryBuffer.h>

#include <iostream>

#include <sys/stat.h>
#include <stdio.h>
#include <cling/EditLine/EditLine.h>

namespace llvm {
   class Module;
}

namespace {

   //------------------------------------------------------------------------------
   // String constants - MOVE SOMEWHERE REASONABLE!
   //------------------------------------------------------------------------------
   static std::string code_prefix = "#include <stdio.h>\nint imain(int argc, char** argv) {\n";
   static std::string code_suffix = ";\nreturn 0; } ";

} // unnamed namespace


//---------------------------------------------------------------------------
// Construct an interface for an interpreter
//---------------------------------------------------------------------------
cling::UserInterface::UserInterface(Interpreter& interp, const char* prompt /*= "[cling] $"*/):
   m_Interp(&interp)
{
}


//---------------------------------------------------------------------------
// Destruct the interface
//---------------------------------------------------------------------------
cling::UserInterface::~UserInterface()
{
}

//---------------------------------------------------------------------------
// Interact with the user using a prompt
//---------------------------------------------------------------------------
void cling::UserInterface::runInteractively()
{
   std::cerr << std::endl;
   std::cerr << "**** Welcome to the cling prototype! ****" << std::endl;
   std::cerr << "* Type C code and press enter to run it *" << std::endl;
   std::cerr << "* Type .q, exit or ctrl+D to quit       *" << std::endl;
   std::cerr << "*****************************************" << std::endl;
   struct stat buf;
   static const char* histfile = ".cling_history";
   using_history();
   history_max_entries = 100;
   if (stat(histfile, &buf) == 0) {
      read_history(histfile);
   }
   m_QuitRequested = false;
   while (!m_QuitRequested) {
      char* line = readline("[cling]$ ");
      if (line) {
         NextInteractiveLine(line);
         add_history(line);
         free(line);
      } else break;
   }
   write_history(histfile);
}


//---------------------------------------------------------------------------
// Process an interactive line, return whether processing was successful
//---------------------------------------------------------------------------
bool cling::UserInterface::NextInteractiveLine(const std::string& line)
{
   if (ProcessMeta(line)) return true;
   
   //----------------------------------------------------------------------
   // Wrap the code
   //----------------------------------------------------------------------
   std::string wrapped = code_prefix + line + code_suffix;
   llvm::MemoryBuffer* buff;
   buff  = llvm::MemoryBuffer::getMemBufferCopy( &*wrapped.begin(),
                                                 &*wrapped.end(),
                                                 "CLING" );

   //----------------------------------------------------------------------
   // Parse and run it
   //----------------------------------------------------------------------
   std::string errMsg;
   llvm::Module* module = m_Interp->link( buff, &errMsg );

   if(!module) {
      std::cerr << std::endl;
      std::cerr << "[!] Errors occured while parsing your code!" << std::endl;
      if (!errMsg.empty())
         std::cerr << "[!] " << errMsg << std::endl;
      std::cerr << std::endl;
      return false;
   }
   m_Interp->executeModuleMain( module, "imain" );
   return true;
}


//---------------------------------------------------------------------------
// Process possible meta commands (.L,...)
//---------------------------------------------------------------------------
bool cling::UserInterface::ProcessMeta(const std::string& input)
{
   if (input[0] != '.') return false;
   switch (input[1]) {
   case 'L':
      {
         size_t first = 3;
         while (isspace(input[first])) ++first;
         size_t last = input.length();
         while (last && isspace(input[last - 1])) --last;
         if (!last) {
            std::cerr << "[i] Failure: no file name given!" << std::endl;
         } else {
            std::string filename = input.substr(first, last - first);
            llvm::sys::Path path(filename);
            if (path.isDynamicLibrary()) {
               std::string errMsg;
               if (!llvm::sys::DynamicLibrary::LoadLibraryPermanently(filename.c_str(), &errMsg))
                  std::cerr << "[i] Success!" << std::endl;
               else
                  std::cerr << "[i] Failure: " << errMsg << std::endl;
            } else {
               if( m_Interp->addUnit( filename ) )
                  std::cerr << "[i] Success!" << std::endl;
               else
                  std::cerr << "[i] Failure" << std::endl;
            }
         }
         break;
      }
   case 'x':
      {
         size_t first = 3;
         while (isspace(input[first])) ++first;
         size_t last = input.length();
         while (last && isspace(input[last - 1])) --last;
         if (!last) {
            std::cerr << "[i] Failure: no file name given!" << std::endl;
         } else {
            m_Interp->executeFile(input.substr(first, last - first));
         }
         break;
      }
   case 'U':
      {
         llvm::sys::Path path(input.substr(3));
         if (path.isDynamicLibrary()) {
            std::cerr << "[i] Failure: cannot unload shared libraries yet!" << std::endl;
         }
         m_Interp->removeUnit( input.substr( 3 ) );
         break;
      }
   case 'q':
      m_QuitRequested = true;
      break;
   default:
      return false;
   }
   return true;
}

