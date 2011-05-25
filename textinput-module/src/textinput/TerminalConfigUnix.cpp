#ifndef WIN32

//===--- TerminalConfigUnix.cpp - termios storage -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TerminalReader and TerminalDisplay need to reset the terminal configuration
// upon destruction, to leave the terminal as before. To avoid a possible
// misunderstanding of what "before" means, centralize their storage of the
// previous termios and have them share it.
//
//  Axel Naumann <axel@cern.ch>, 2011-05-12
//===----------------------------------------------------------------------===//

#include "textinput/TerminalConfigUnix.h"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>

using namespace textinput;
using std::memcpy;

TerminalConfigUnix&
TerminalConfigUnix::Get() {
  static TerminalConfigUnix s;
  return s;
}

TerminalConfigUnix::TerminalConfigUnix():
  fIsAttached(false), fOldTIOS(), fConfTIOS() {
#ifdef TCSANOW
  fOldTIOS = new termios;
  fConfTIOS = new termios;
  tcgetattr(fileno(stdin), fOldTIOS);
  *fConfTIOS = *fOldTIOS;
#endif
}

TerminalConfigUnix::~TerminalConfigUnix() {
  Detach();
  delete fOldTIOS;
  delete fConfTIOS;
}

void
TerminalConfigUnix::Attach() {
  if (fIsAttached) return;
#ifdef TCSANOW
  tcsetattr(fileno(stdin), TCSANOW, fConfTIOS);
#endif
  fIsAttached = true;
}

void
TerminalConfigUnix::Detach() {
  // Reset the terminal configuration.
  if (!fIsAttached) return;
#ifdef TCSANOW
  tcsetattr(fileno(stdout), TCSANOW, fOldTIOS);
#endif
  fIsAttached = false;
}

#endif // ndef WIN32
