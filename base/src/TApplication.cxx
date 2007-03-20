// @(#)root/base:$Name:  $:$Id: TApplication.cxx,v 1.86 2007/03/02 13:34:50 rdm Exp $
// Author: Fons Rademakers   22/12/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TApplication                                                         //
//                                                                      //
// This class creates the ROOT Application Environment that interfaces  //
// to the windowing system eventloop and eventhandlers.                 //
// This class must be instantiated exactly once in any given            //
// application. Normally the specific application class inherits from   //
// TApplication (see TRint).                                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef R__HAVE_CONFIG
#include "RConfigure.h"
#endif

#include "Riostream.h"
#include "TApplication.h"
#include "TGuiFactory.h"
#include "TVirtualX.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TString.h"
#include "TError.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TTimer.h"
#include "TInterpreter.h"
#include "TStyle.h"
#include "TVirtualPad.h"
#include "TEnv.h"
#include "TColor.h"
#include "TClassTable.h"
#include "TSystemDirectory.h"
#include "TPluginManager.h"
#include "TClassTable.h"

#ifdef R__WIN32
#include "TWinNTSystem.h"
#endif

TApplication *gApplication = 0;
Bool_t TApplication::fgGraphInit = kFALSE;

//______________________________________________________________________________
class TIdleTimer : public TTimer {
public:
   TIdleTimer(Long_t ms) : TTimer(ms, kTRUE) { }
   Bool_t Notify();
};

//______________________________________________________________________________
Bool_t TIdleTimer::Notify()
{
   // Notify handler.
   gApplication->HandleIdleTimer();
   Reset();
   return kFALSE;
}


ClassImp(TApplication)

//______________________________________________________________________________
TApplication::TApplication()
{
   // Default ctor. Can be used by classes deriving from TApplication.

   fArgc          = 0;
   fArgv          = 0;
   fAppImp        = 0;
   fIsRunning     = kFALSE;
   fReturnFromRun = kFALSE;
   fNoLog         = kFALSE;
   fNoLogo        = kFALSE;
   fQuit          = kFALSE;
   fFiles         = 0;
   fIdleTimer     = 0;
   fSigHandler    = 0;
}

//______________________________________________________________________________
TApplication::TApplication(const char *appClassName,
                           Int_t *argc, char **argv, void *options,
                           Int_t numOptions)
{
   // Create an application environment. The application environment
   // provides an interface to the graphics system and eventloop
   // (be it X, Windoze, MacOS or BeOS). After creating the application
   // object start the eventloop by calling its Run() method. The command
   // line options recogized by TApplication are described in the GetOptions()
   // method. The recognized options are removed from the argument array.
   // The original list of argument options can be retrieved via the Argc()
   // and Argv() methods. The appClassName "proofserv" is reserved for the
   // PROOF system. The "options" and "numOptions" arguments are not used,
   // except if you want to by-pass the argv processing by GetOptions()
   // in which case you should specify numOptions<0. All options will
   // still be available via the Argv() method for later use.

   if (gApplication) {
      Error("TApplication", "only one instance of TApplication allowed");
      return;
   }

   if (!gROOT)
      ::Fatal("TApplication::TApplication", "ROOT system not initialized");

   if (!gSystem)
      ::Fatal("TApplication::TApplication", "gSystem not initialized");

   gApplication = this;
   gROOT->SetApplication(this);
   gROOT->SetName(appClassName);

   if (options) { }  // use unused argument

   // copy command line arguments, can be later accessed via Argc() and Argv()
   if (argc && *argc > 0) {
      fArgc = *argc;
      fArgv = (char **)new char*[fArgc];
   } else {
      fArgc = 0;
      fArgv = 0;
   }

   for (int i = 0; i < fArgc; i++)
      fArgv[i] = StrDup(argv[i]);

   fNoLog         = kFALSE;
   fNoLogo        = kFALSE;
   fQuit          = kFALSE;

   if (numOptions >= 0)
      GetOptions(argc, argv);

   if (fArgv)
      gSystem->SetProgname(fArgv[0]);

#ifdef R__WIN32
   ((TWinNTSystem*)gSystem)->NotifyApplicationCreated();
#endif

   fIdleTimer     = 0;
   fSigHandler    = 0;
   fIsRunning     = kFALSE;
   fReturnFromRun = kFALSE;
   fAppImp        = gGuiFactory->CreateApplicationImp(appClassName, argc, argv);

   // Enable autoloading
   gInterpreter->EnableAutoLoading();

   // Initialize the graphics environment
   if (gClassTable->GetDict("TPad"))
      InitializeGraphics();

   // Make sure all registered dictionaries have been initialized
   // and that all types have been loaded
   gInterpreter->InitializeDictionaries();
   gInterpreter->UpdateListOfTypes();

   // Save current interpreter context
   gInterpreter->SaveContext();
   gInterpreter->SaveGlobalsContext();

   // to allow user to interact with TCanvas's under WIN32
   gROOT->SetLineHasBeenProcessed();
}

//______________________________________________________________________________
TApplication::~TApplication()
{
   // TApplication dtor.

   for (int i = 0; i < fArgc; i++)
      if (fArgv[i]) delete [] fArgv[i];
   delete [] fArgv;
   SafeDelete(fAppImp);
}

//______________________________________________________________________________
void TApplication::InitializeGraphics()
{
   // Initialize the graphics environment.

   if (fgGraphInit)
      return;
   fgGraphInit = kTRUE;

   // Load the graphics related libraries
   LoadGraphicsLibs();

   // Try to load TrueType font renderer. Only try to load if not in batch
   // mode and Root.UseTTFonts is true and Root.TTFontPath exists. Abort silently
   // if libttf or libGX11TTF are not found in $ROOTSYS/lib or $ROOTSYS/ttf/lib.
   if (strcmp(gROOT->GetName(), "proofserv")) {
      const char *ttpath = gEnv->GetValue("Root.TTFontPath",
#ifdef TTFFONTDIR
                                          TTFFONTDIR);
#else
                                          "$(ROOTSYS)/fonts");
#endif
      char *ttfont = gSystem->Which(ttpath, "arialbd.ttf", kReadPermission);
      // Check for use of DFSG - fonts
      if (!ttfont)
         ttfont = gSystem->Which(ttpath, "FreeSansBold.ttf", kReadPermission);

#if !defined(R__WIN32)
      if (!gROOT->IsBatch() && !strcmp(gVirtualX->GetName(), "X11") &&
          ttfont && gEnv->GetValue("Root.UseTTFonts", 1)) {
         TString plugin = "x11ttf";

         TPluginHandler *h;
         if ((h = gROOT->GetPluginManager()->FindHandler("TVirtualX", plugin)))
            h->LoadPlugin();
      }
#endif
      delete [] ttfont;
   }

   // Create WM dependent application environment
   if (fAppImp)
      delete fAppImp;
   fAppImp = gGuiFactory->CreateApplicationImp(gROOT->GetName(), &fArgc, fArgv);
   if (!fAppImp) {
      MakeBatch();
      fAppImp = gGuiFactory->CreateApplicationImp(gROOT->GetName(), &fArgc, fArgv);
   }

   // Create the canvas colors early so they are allocated before
   // any color table expensive bitmaps get allocated in GUI routines (like
   // creation of XPM bitmaps).
   TColor::InitializeColors();

   // Hook for further initializing the WM dependent application environment
   Init();

   // Set default screen factor (if not disabled in rc file)
   if (gEnv->GetValue("Canvas.UseScreenFactor", 1)) {
      Int_t  x, y;
      UInt_t w, h;
      if (gVirtualX) {
         gVirtualX->GetGeometry(-1, x, y, w, h);
         if (h > 0 && h < 1000) gStyle->SetScreenFactor(0.0011*h);
      }
   }
}

//______________________________________________________________________________
void TApplication::ClearInputFiles()
{
   // Clear list containing macro files passed as program arguments.
   // This method is called from TRint::Run() to ensure that the macro
   // files are only executed the first time Run() is called.

   if (fFiles) {
      fFiles->Delete();
      SafeDelete(fFiles);
   }
}

//______________________________________________________________________________
void TApplication::GetOptions(Int_t *argc, char **argv)
{
   // Get and handle command line options. Arguments handled are removed
   // from the argument array. The following arguments are handled:
   //    -b : run in batch mode without graphics
   //    -n : do not execute logon and logoff macros as specified in .rootrc
   //    -q : exit after processing command line macro files
   //    -l : do not show splash screen
   // The last three options are only relevant in conjunction with TRint.
   // The following help and info arguments are supported:
   //    -?      : print usage
   //    -h      : print usage
   //    --help  : print usage
   //    -config : print ./configure options

   fNoLog = kFALSE;
   fQuit  = kFALSE;
   fFiles = 0;

   if (!argc)
      return;

   int i, j;

   for (i = 1; i < *argc; i++) {
      if (!strcmp(argv[i], "-?") || !strncmp(argv[i], "-h", 2) ||
          !strncmp(argv[i], "--help", 6)) {
         fprintf(stderr, "Usage: %s [-l] [-b] [-n] [-q] [dir] [file1.C ... fileN.C]\n", argv[0]);
         fprintf(stderr, "Options:\n");
         fprintf(stderr, "  -b : run in batch mode without graphics\n");
         fprintf(stderr, "  -n : do not execute logon and logoff macros as specified in .rootrc\n");
         fprintf(stderr, "  -q : exit after processing command line macro files\n");
         fprintf(stderr, "  -l : do not show splash screen\n");
         fprintf(stderr, " dir : if dir is a valid directory cd to it before executing\n");
         fprintf(stderr, "\n");
         fprintf(stderr, "  -?      : print usage\n");
         fprintf(stderr, "  -h      : print usage\n");
         fprintf(stderr, "  --help  : print usage\n");
         fprintf(stderr, "  -config : print ./configure options\n");
         fprintf(stderr, "\n");
         Terminate(0);
      } else if (!strcmp(argv[i], "-config")) {
         fprintf(stderr, "ROOT ./configure options:\n%s\n", gROOT->GetConfigOptions());
         Terminate(0);
      } else if (!strcmp(argv[i], "-b")) {
         MakeBatch();
         argv[i] = "";
#if 0
      } else if (!strcmp(argv[i], "-x")) {
         // remote ROOT display server not operational yet
#ifndef R__WIN32
         if (gVirtualX != gGXBatch) delete gVirtualX;
#endif
         gVirtualX = new TGXClient("Root_Client");
         if (gVirtualX->IsZombie()) {
            delete gVirtualX;
            gROOT->SetBatch();
            if (gGuiFactory != gBatchGuiFactory) delete gGuiFactory;
            gGuiFactory = gBatchGuiFactory;
            gVirtualX = gGXBatch;
         }
         argv[i] = "";
#endif
      } else if (!strcmp(argv[i], "-n")) {
         fNoLog = kTRUE;
         argv[i] = "";
      } else if (!strcmp(argv[i], "-q")) {
         fQuit = kTRUE;
         argv[i] = "";
      } else if (!strcmp(argv[i], "-l")) {
         // used by front-end program to not display splash screen
         fNoLogo = kTRUE;
         argv[i] = "";
      } else if (!strcmp(argv[i], "-splash")) {
         // used when started by front-end program to signal that
         // splash screen can be popped down (TRint::PrintLogo())
         argv[i] = "";
      } else if (argv[i][0] != '-' && argv[i][0] != '+') {
         Long64_t size;
         Long_t id, flags, modtime;
         char *arg = strchr(argv[i], '(');
         if (arg) *arg = '\0';
         char *dir = gSystem->ExpandPathName(argv[i]);
         if (arg) *arg = '(';
         if (!gSystem->GetPathInfo(dir, &id, &size, &flags, &modtime)) {
            if ((flags & 2)) {
               // if directory make it working directory
               gSystem->ChangeDirectory(dir);
               TSystemDirectory *workdir = new TSystemDirectory("workdir", gSystem->WorkingDirectory());
               TObject *w = gROOT->GetListOfBrowsables()->FindObject("workdir");
               TObjLink *lnk = gROOT->GetListOfBrowsables()->FirstLink();
               while (lnk) {
                  if (lnk->GetObject() == w) {
                     lnk->SetObject(workdir);
                     lnk->SetOption(gSystem->WorkingDirectory());
                     break;
                  }
                  lnk = lnk->Next();
               }
               delete w;
            } else if (flags == 0 || flags == 1) {
               // if file add to list of files to be processed
               if (!fFiles) fFiles = new TObjArray;
               fFiles->Add(new TObjString(argv[i]));
            }
            argv[i] = "";
         } else {
            char *mac, *s = strtok(dir, "+(");
            if ((mac = gSystem->Which(TROOT::GetMacroPath(), s,
                                      kReadPermission))) {
               // if file add to list of files to be processed
               if (!fFiles) fFiles = new TObjArray;
               fFiles->Add(new TObjString(argv[i]));
               argv[i] = "";
               delete [] mac;
            } else
               // only warn if we're plain root,
               // other progs might have their own params
               if (!strcmp(GetName(),"Rint"))
                  Warning("GetOptions", "Macro %s not found.", s);
         }
         delete [] dir;
      }
      // ignore unknown options
   }

   // remove handled arguments from argument array
   j = 0;
   for (i = 0; i < *argc; i++) {
      if (strcmp(argv[i],"")) {
         argv[j] = argv[i];
         j++;
      }
   }

   *argc = j;
}

//______________________________________________________________________________
void TApplication::HandleIdleTimer()
{
   // Handle idle timeout. When this timer expires the registered idle command
   // will be executed by this routine and a signal will be emitted.

   if (!fIdleCommand.IsNull())
      ProcessLine(GetIdleCommand());

   Emit("HandleIdleTimer()");
}

//______________________________________________________________________________
void TApplication::Help(const char *line)
{
   // Print help on interpreter.

   gInterpreter->ProcessLine(line);

   Printf("\nROOT special commands.");
   Printf("===========================================================================");
   Printf("             pwd          : show current directory, pad and style");
   Printf("             ls           : list contents of current directory");
   Printf("             which [file] : shows path of macro file");
}

//______________________________________________________________________________
void TApplication::LoadGraphicsLibs()
{
   // Load shared libs neccesary for graphics. These libraries are only
   // loaded when gROOT->IsBatch() is kFALSE.

   if (gROOT->IsBatch()) return;

   TPluginHandler *h;
   if ((h = gROOT->GetPluginManager()->FindHandler("TVirtualPad")))
      h->LoadPlugin();

   TString name;
   TString title1 = "ROOT interface to ";
   TString nativex, title;
   TString nativeg = "root";
#ifndef R__WIN32
   nativex = "x11";
   name    = "X11";
   title   = title1 + "X11";
#else
   nativex = "win32gdk";
   name    = "Win32gdk";
   title   = title1 + "Win32gdk";
#endif

   TString guiBackend(gEnv->GetValue("Gui.Backend", "native"));
   guiBackend.ToLower();
   if (guiBackend == "native") {
      guiBackend = nativex;
   } else {
      name  = guiBackend;
      title = title1 + guiBackend;
   }
   TString guiFactory(gEnv->GetValue("Gui.Factory", "native"));
   guiFactory.ToLower();
   if (guiFactory == "native")
      guiFactory = nativeg;

   if ((h = gROOT->GetPluginManager()->FindHandler("TVirtualX", guiBackend))) {
      if (h->LoadPlugin() == -1)
         return;
      gVirtualX = (TVirtualX *) h->ExecPlugin(2, name.Data(), title.Data());
   }
   if ((h = gROOT->GetPluginManager()->FindHandler("TGuiFactory", guiFactory))) {
      if (h->LoadPlugin() == -1)
         return;
      gGuiFactory = (TGuiFactory *) h->ExecPlugin(0);
   }
}

//______________________________________________________________________________
void TApplication::MakeBatch()
{
   // Switch to batch mode.

   gROOT->SetBatch();
   if (gGuiFactory != gBatchGuiFactory) delete gGuiFactory;
   gGuiFactory = gBatchGuiFactory;
#ifndef R__WIN32
   if (gVirtualX != gGXBatch) delete gVirtualX;
#endif
   gVirtualX = gGXBatch;
}

//______________________________________________________________________________
Long_t TApplication::ProcessLine(const char *line, Bool_t sync, Int_t *err)
{
   // Process a single command line, either a C++ statement or an interpreter
   // command starting with a ".".
   // Return the return value of the command casted to a long.

   if (!line || !*line) return 0;

   if (!strncasecmp(line, ".qqqqqqq", 7)) {
      gSystem->Abort();
   } else if (!strncasecmp(line, ".qqqqq", 5)) {
      Info("ProcessLine", "Bye... (try '.qqqqqqq' if still running)");
      gSystem->Exit(1);
   } else if (!strncasecmp(line, ".exit", 4) || !strncasecmp(line, ".quit", 2)) {
      gInterpreter->ResetGlobals();
      Terminate(0);
      return 0;
   }

   if (!strncmp(line, "?", 1)) {
      Help(line);
      return 1;
   }

   if (!strncmp(line, ".pwd", 4)) {
      if (gDirectory)
         Printf("Current directory: %s", gDirectory->GetPath());
      if (gPad)
         Printf("Current pad:       %s", gPad->GetName());
      if (gStyle)
         Printf("Current style:     %s", gStyle->GetName());
      return 1;
   }

   if (!strncmp(line, ".ls", 3)) {
      const char *opt = 0;
      if (line[3]) opt = &line[3];
      if (gDirectory) gDirectory->ls(opt);
      return 1;
   }

   if (!strncmp(line, ".which", 6)) {
      char *fn  = Strip(line+7);
      char *s   = strtok(fn, "+(");
      char *mac = gSystem->Which(TROOT::GetMacroPath(), s, kReadPermission);
      if (!mac)
         Printf("No macro %s in path %s", s, TROOT::GetMacroPath());
      else
         Printf("%s", mac);
      delete [] fn;
      delete [] mac;
      return mac ? 1 : 0;
   }

   if (!strncmp(line, ".L", 2) || !strncmp(line, ".U", 2)) {
      TString aclicMode;
      TString arguments;
      TString io;
      TString fname = gSystem->SplitAclicMode(line+3, aclicMode, arguments, io);

      char *mac = gSystem->Which(TROOT::GetMacroPath(), fname, kReadPermission);
      if (arguments.Length()) {
         Warning("ProcessLine", "argument(s) \"%s\" ignored with .%c", arguments.Data(),
                 line[1],TROOT::GetMacroPath());
      }
      Long_t retval = 0;
      if (!mac)
         Error("ProcessLine", "macro %s not found in path %s", fname.Data(),
               TROOT::GetMacroPath());
      else {
         char cmd = line[1];
         if (sync)
            retval = gInterpreter->ProcessLineSynch(Form(".%c %s%s%s", cmd, mac, aclicMode.Data(),io.Data()),
                                                   (TInterpreter::EErrorCode*)err);
         else {
            retval = gInterpreter->ProcessLine(Form(".%c %s%s%s", cmd, mac, aclicMode.Data(),io.Data()),
                                              (TInterpreter::EErrorCode*)err);
         }
      }

      delete [] mac;

      return retval;
   }

   if (!strncmp(line, ".X", 2) || !strncmp(line, ".x", 2)) {
      return ProcessFile(line+3, err);
   }

   if (!strcmp(line, ".reset")) {
      // Do nothing, .reset disabled in CINT because too many side effects
      Printf("*** .reset not allowed, please use gROOT->Reset() ***");
      return 0;

#if 0
      // delete the ROOT dictionary since CINT will destroy all objects
      // referenced by the dictionary classes (TClass et. al.)
      gROOT->GetListOfClasses()->Delete();
      // fall through
#endif
   }

   if (sync)
      return gInterpreter->ProcessLineSynch(line, (TInterpreter::EErrorCode*)err);
   else
      return gInterpreter->ProcessLine(line, (TInterpreter::EErrorCode*)err);
}

//______________________________________________________________________________
Long_t TApplication::ProcessFile(const char *name, int *error)
{
   // Process a file containing a C++ macro.

   const Int_t kBufSize = 1024;

   if (!name || !*name) return 0;

   TString aclicMode;
   TString arguments;
   TString io;
   TString fname = gSystem->SplitAclicMode(name, aclicMode, arguments, io);

   char *exnam = gSystem->Which(TROOT::GetMacroPath(), fname, kReadPermission);
   if (!exnam) {
      Error("ProcessFile", "macro %s not found in path %s", fname.Data(),
            TROOT::GetMacroPath());
      delete [] exnam;
      return 0;
   }

   ::ifstream file(exnam, ios::in);
   if (!file.good()) {
      Error("ProcessFile", "%s no such file", exnam);
      delete [] exnam;
      return 0;
   }

   char currentline[kBufSize];
   int tempfile = 0;
   int comment  = 0;
   int ifndefc  = 0;
   int ifdef    = 0;
   char *s      = 0;
   Bool_t execute = kFALSE;
   Long_t retval = 0;

   while (1) {
      file.getline(currentline, kBufSize);
      if (file.eof()) break;
      s = currentline;
      while (s && (*s == ' ' || *s == '\t')) s++;   // strip-off leading blanks

      // very simple minded pre-processor parsing, only works in case macro file
      // starts with "#ifndef __CINT__". In that case everything till next
      // "#else" or "#endif" will be skipped.
      if (*s == '#') {
         char *cs = Compress(currentline);
         if (strstr(cs, "#ifndef__CINT__") ||
             strstr(cs, "#if!defined(__CINT__)"))
            ifndefc = 1;
         else if (ifndefc && (strstr(cs, "#ifdef") || strstr(cs, "#ifndef") ||
                  strstr(cs, "#ifdefined") || strstr(cs, "#if!defined")))
            ifdef++;
         else if (ifndefc && strstr(cs, "#endif")) {
            if (ifdef)
               ifdef--;
            else
               ifndefc = 0;
         } else if (ifndefc && !ifdef && strstr(cs, "#else"))
            ifndefc = 0;
         delete [] cs;
      }
      if (!*s || *s == '#' || ifndefc || !strncmp(s, "//", 2)) continue;

      if (!strncmp(s, ".X", 2) || !strncmp(s, ".x", 2)) {
         retval = ProcessFile(s+3);
         execute = kTRUE;
         continue;
      }

      if (!strncmp(s, "/*", 2)) comment = 1;
      if (comment) {
         // handle slightly more complex cases like: /*  */  /*
again:
         s = strstr(s, "*/");
         if (s) {
            comment = 0;
            s += 2;

            while (s && (*s == ' ' || *s == '\t')) s++; // strip-off leading blanks
            if (!*s) continue;
            if (!strncmp(s, "//", 2)) continue;
            if (!strncmp(s, "/*", 2)) {
               comment = 1;
               goto again;
            }
         }
      }
      if (!comment && *s == '{') tempfile = 1;
      if (!comment) break;
   }
   file.close();

   if (!execute) {
      TString exname = exnam;
      if (!tempfile) {
         // We have a script that does NOT contain an unamed macro,
         // so we can call the script compiler on it.
         exname += aclicMode;
      }
      exname += arguments;
      exname += io;

      if (tempfile) {
         retval = gInterpreter->ProcessLineSynch(Form(".x %s", exname.Data()),
                                        (TInterpreter::EErrorCode*)error);
      } else
         retval = gInterpreter->ProcessLineSynch(Form(".X %s", exname.Data()),
                                        (TInterpreter::EErrorCode*)error);
   }

   delete [] exnam;
   return retval;
}

//______________________________________________________________________________
void TApplication::Run(Bool_t retrn)
{
   // Main application eventloop. Calls system dependent eventloop via gSystem.

   SetReturnFromRun(retrn);

   fIsRunning = kTRUE;

   gSystem->Run();

   fIsRunning = kFALSE;
}

//______________________________________________________________________________
void TApplication::SetIdleTimer(UInt_t idleTimeInSec, const char *command)
{
   // Set the command to be executed after the system has been idle for
   // idleTimeInSec seconds. Normally called via TROOT::Idle(...).

   if (fIdleTimer) RemoveIdleTimer();
   fIdleCommand = command;
   fIdleTimer = new TIdleTimer(idleTimeInSec*1000);
   gSystem->AddTimer(fIdleTimer);
}

//______________________________________________________________________________
void TApplication::RemoveIdleTimer()
{
   // Remove idle timer. Normally called via TROOT::Idle(0).

   if (fIdleTimer) {
      // timers are removed from the gSystem timer list by their dtor
      SafeDelete(fIdleTimer);
   }
}

//______________________________________________________________________________
void TApplication::StartIdleing()
{
   // Called when system starts idleing.

   if (fIdleTimer) {
      fIdleTimer->Reset();
      gSystem->AddTimer(fIdleTimer);
   }
}

//______________________________________________________________________________
void TApplication::StopIdleing()
{
   // Called when system stops idleing.

   if (fIdleTimer)
      gSystem->RemoveTimer(fIdleTimer);
}

//______________________________________________________________________________
void TApplication::Terminate(Int_t status)
{
   // Terminate the application by call TSystem::Exit() unless application has
   // been told to return from Run(), by a call to SetReturnFromRun().

   Emit("Terminate(Int_t)", status);

   if (fReturnFromRun)
      gSystem->ExitLoop();
   else
      gSystem->Exit(status);
}

//______________________________________________________________________________
void TApplication::KeyPressed(Int_t key)
{
   // Emit signal when console keyboard key was pressed.

   Emit("KeyPressed(Int_t)", key);
}

//______________________________________________________________________________
void TApplication::ReturnPressed(char *text )
{
   // Emit signal when return key was pressed.

   Emit("ReturnPressed(char*)", text);
}

//______________________________________________________________________________
void TApplication::SetEchoMode(Bool_t)
{
   // Set console echo mode:
   //
   //  mode = kTRUE  - echo input symbols
   //  mode = kFALSE - noecho input symbols
}

//______________________________________________________________________________
void TApplication::CreateApplication()
{
   // Static function used to create a default application environment.

   if (!gApplication) {
      char *a = StrDup("RootApp");
      char *b = StrDup("-b");
      char *argv[2];
      Int_t argc = 2;
      argv[0] = a;
      argv[1] = b;
      new TApplication("RootApp", &argc, argv, 0, 0);
      if (gDebug > 0)
         Printf("<TApplication::CreateApplication>: "
                "created default TApplication");
      delete [] a; delete [] b;
   }
}
