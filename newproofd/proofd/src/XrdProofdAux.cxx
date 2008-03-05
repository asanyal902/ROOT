// @(#)root/proofd:$Id$
// Author: G. Ganis  June 2007

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdProofdAux                                                          //
//                                                                      //
// Authors: G. Ganis, CERN, 2007                                        //
//                                                                      //
// Small auxilliary classes used in XrdProof                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
#include "XrdProofdPlatform.h"

#include "XrdOuc/XrdOucStream.hh"
#include "XrdSys/XrdSysPriv.hh"

#include "XrdProofdAux.h"
#include "XrdProofdConfig.h"
#include "XrdProofdProtocol.h"

// Tracing
#include "XrdProofdTrace.h"
static const char *gTraceID = "";
extern XrdOucTrace *XrdProofdTrace;
#define TRACEID gTraceID

// Local definitions
#ifdef XPD_LONG_MAX
#undefine XPD_LONG_MAX
#endif
#define XPD_LONG_MAX 2147483647

XrdSysRecMutex XrdProofdAux::fgFormMutex;

//______________________________________________________________________________
char *XrdProofdAux::Expand(char *p)
{
   // Expand path 'p' relative to:
   //     $HOME               if begins with ~/
   //     <user>'s $HOME      if begins with ~<user>/
   //     $PWD                if does not begin with '/' or '~'
   //   getenv(<ENVVAR>)      if it begins with $<ENVVAR>)
   // The returned array of chars is the result of reallocation
   // of the input one.
   // If something is inconsistent, for example <ENVVAR> does not
   // exists, the original string is untouched

   // Make sure there soething to expand
   if (!p || strlen(p) <= 0 || p[0] == '/')
      return p;

   char *po = p;

   // Relative to the environment variable
   if (p[0] == '$') {
      // Resolve env
      XrdOucString env(&p[1]);
      int isl = env.find('/');
      env.erase(isl);
      char *p1 = (isl > 0) ? (char *)(p + isl + 2) : 0;
      if (getenv(env.c_str())) {
         int lenv = strlen(getenv(env.c_str()));
         int lp1 = p1 ? strlen(p1) : 0;
         po = (char *) malloc(lp1 + lenv + 2);
         if (po) {
            memcpy(po, getenv(env.c_str()), lenv);
            if (p1) {
               memcpy(po+lenv+1, p1, lp1);
               po[lenv] = '/';
            }
            po[lp1 + lenv + 1] = 0;
            free(p);
         } else
            po = p;
      }
      return po;
   }

   // Relative to the local location
   if (p[0] != '~') {
      if (getenv("PWD")) {
         int lpwd = strlen(getenv("PWD"));
         int lp = strlen(p);
         po = (char *) malloc(lp + lpwd + 2);
         if (po) {
            memcpy(po, getenv("PWD"), lpwd);
            memcpy(po+lpwd+1, p, lp);
            po[lpwd] = '/';
            po[lpwd+lp+1] = 0;
            free(p);
         } else
            po = p;
      }
      return po;
   }

   // Relative to $HOME or <user>'s $HOME
   if (p[0] == '~') {
      char *pu = p+1;
      char *pd = strchr(pu,'/');
      *pd++ = '\0';
      // Get the correct user structure
      XrdProofUI ui;
      int rc = 0;
      if (strlen(pu) > 0) {
         rc = XrdProofdAux::GetUserInfo(pu, ui);
      } else {
         rc = XrdProofdAux::GetUserInfo(getuid(), ui);
      }
      if (rc == 0) {
         int ldir = ui.fHomeDir.length();
         int lpd = strlen(pd);
         po = (char *) malloc(lpd + ldir + 2);
         if (po) {
            memcpy(po, ui.fHomeDir.c_str(), ldir);
            memcpy(po+ldir+1, pd, lpd);
            po[ldir] = '/';
            po[lpd + ldir + 1] = 0;
            free(p);
         } else
            po = p;
      }
      return po;
   }

   // We are done
   return po;
}

//______________________________________________________________________________
void XrdProofdAux::Expand(XrdOucString &p)
{
   // Expand path 'p' relative to:
   //     $HOME               if begins with ~/
   //     <user>'s $HOME      if begins with ~<user>/
   //     $PWD                if does not begin with '/' or '~'
   //   getenv(<ENVVAR>)      if it begins with $<ENVVAR>)
   // The input string is updated with the result.
   // If something is inconsistent, for example <ENVVAR> does not
   // exists, the original string is untouched

   char *po = strdup((char *)p.c_str());
   po = Expand(po);
   p = po;
   SafeFree(po);
}

//__________________________________________________________________________
long int XrdProofdAux::GetLong(char *str)
{
   // Extract first integer from string at 'str', if any

   // Reposition on first digit
   char *p = str;
   while ((*p < 48 || *p > 57) && (*p) != '\0')
      p++;
   if (*p == '\0')
      return XPD_LONG_MAX;

   // Find the last digit
   int j = 0;
   while (*(p+j) >= 48 && *(p+j) <= 57)
      j++;
   *(p+j) = '\0';

   // Convert now
   return strtol(p, 0, 10);
}

//__________________________________________________________________________
int XrdProofdAux::GetUserInfo(const char *usr, XrdProofUI &ui)
{
   // Get information about user 'usr' in a thread safe way.
   // Return 0 on success, -errno on error

   // Make sure input is defined
   if (!usr || strlen(usr) <= 0)
      return -EINVAL;

   // Call getpwnam_r ...
   struct passwd pw;
   struct passwd *ppw = 0;
   char buf[2048];
#if defined(__sun) && !defined(__GNUC__)
   ppw = getpwnam_r(usr, &pw, buf, sizeof(buf));
#else
   getpwnam_r(usr, &pw, buf, sizeof(buf), &ppw);
#endif
   if (ppw) {
      // Fill output
      ui.fUid = (int) pw.pw_uid;
      ui.fGid = (int) pw.pw_gid;
      ui.fHomeDir = pw.pw_dir;
      ui.fUser = usr;
      // Done
      return 0;
   }

   // Failure
   if (errno != 0)
      return ((int) -errno);
   else
      return -ENOENT;
}

//__________________________________________________________________________
int XrdProofdAux::GetUserInfo(int uid, XrdProofUI &ui)
{
   // Get information about user with 'uid' in a thread safe way.
   // Retur 0 on success, -errno on error

   // Make sure input make sense
   if (uid <= 0)
      return -EINVAL;

   // Call getpwuid_r ...
   struct passwd pw;
   struct passwd *ppw = 0;
   char buf[2048];
#if defined(__sun) && !defined(__GNUC__)
   ppw = getpwuid_r((uid_t)uid, &pw, buf, sizeof(buf));
#else
   getpwuid_r((uid_t)uid, &pw, buf, sizeof(buf), &ppw);
#endif
   if (ppw) {
      // Fill output
      ui.fUid = uid;
      ui.fGid = (int) pw.pw_gid;
      ui.fHomeDir = pw.pw_dir;
      ui.fUser = pw.pw_name;
      // Done
      return 0;
   }

   // Failure
   if (errno != 0)
      return ((int) -errno);
   else
      return -ENOENT;
}

//__________________________________________________________________________
int XrdProofdAux::Write(int fd, const void *buf, size_t nb)
{
   // Write nb bytes at buf to descriptor 'fd' ignoring interrupts
   // Return the number of bytes written or -1 in case of error

   if (fd < 0)
      return -1;

   const char *pw = (const char *)buf;
   int lw = nb;
   int nw = 0, written = 0;
   while (lw) {
      if ((nw = write(fd, pw + written, lw)) < 0) {
         if (errno == EINTR) {
            errno = 0;
            continue;
         } else {
            break;
         }
      }
      // Count
      written += nw;
      lw -= nw;
   }

   // Done
   return written;
}

//_____________________________________________________________________________
int XrdProofdAux::AssertDir(const char *path, XrdProofUI ui, bool changeown)
{
   // Make sure that 'path' exists and is owned by the entity
   // described by 'ui'.
   // If changeown is TRUE it tries to acquire the privileges before.
   // Return 0 in case of success, -1 in case of error

   TRACE(ACT, "AssertDir: enter: "<<path);

   if (!path || strlen(path) <= 0)
      return -1;

   struct stat st;
   if (stat(path,&st) != 0) {
      if (errno == ENOENT) {

         {  XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
            if (XpdBadPGuard(pGuard, ui.fUid) && changeown) {
               TRACE(XERR, "AsserDir: could not get privileges to create dir");
               return -1;
            }

            if (mkdir(path, 0755) != 0) {
               TRACE(XERR, "AssertDir: unable to create dir: "<<path<<
                             " (errno: "<<errno<<")");
               return -1;
            }
         }
         if (stat(path,&st) != 0) {
            TRACE(XERR, "AssertDir: unable to stat dir: "<<path<<
                          " (errno: "<<errno<<")");
            return -1;
         }
      } else {
         // Failure: stop
         TRACE(XERR, "AssertDir: unable to stat dir: "<<path<<
                       " (errno: "<<errno<<")");
         return -1;
      }
   }

   // Make sure the ownership is right
   if (changeown &&
      ((int) st.st_uid != ui.fUid || (int) st.st_gid != ui.fGid)) {

      XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
      if (XpdBadPGuard(pGuard, ui.fUid)) {
         TRACE(XERR, "AsserDir: could not get privileges to change ownership");
         return -1;
      }

      // Set ownership of the path to the client
      if (chown(path, ui.fUid, ui.fGid) == -1) {
         TRACE(XERR, "AssertDir: cannot set user ownership"
                       " on path (errno: "<<errno<<")");
         return -1;
      }
   }

   // We are done
   return 0;
}

//_____________________________________________________________________________
int XrdProofdAux::ChangeToDir(const char *dir, XrdProofUI ui, bool changeown)
{
   // Change current directory to 'dir'.
   // If changeown is TRUE it tries to acquire the privileges before.
   // Return 0 in case of success, -1 in case of error

   TRACE(ACT, "ChangeToDir: enter: changing to " << ((dir) ? dir : "**undef***"));

   if (!dir || strlen(dir) <= 0)
      return -1;

   if (changeown && (int) geteuid() != ui.fUid) {

      XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
      if (XpdBadPGuard(pGuard, ui.fUid)) {
         MTRACE(XERR, "xpd:child: ", "ChangeToDir: could not get privileges");
         return -1;
      }
      if (chdir(dir) == -1) {
         MTRACE(XERR, "xpd:child: ", "ChangeToDir: can't change directory to "<< dir);
         return -1;
      }
   } else {
      if (chdir(dir) == -1) {
         MTRACE(XERR, "xpd:child: ", "ChangeToDir: can't change directory to "<< dir);
         return -1;
      }
   }

   // We are done
   return 0;
}

//_____________________________________________________________________________
int XrdProofdAux::SymLink(const char *path, const char *link)
{
   // Create a symlink 'link' to 'path'
   // Return 0 in case of success, -1 in case of error

   TRACE(ACT, "SymLink: enter");

   if (!path || strlen(path) <= 0 || !link || strlen(link) <= 0)
      return -1;

   // Remove existing link, if any
   if (unlink(link) != 0 && errno != ENOENT) {
      TRACE(XERR, "SymLink: problems unlinking existing symlink "<< link<<
                    " (errno: "<<errno<<")");
      return -1;
   }
   if (symlink(path, link) != 0) {
      TRACE(XERR, "SymLink: problems creating symlink " << link<<
                    " (errno: "<<errno<<")");
      return -1;
   }

   // We are done
   return 0;
}

//______________________________________________________________________________
int XrdProofdAux::CheckIf(XrdOucStream *s, const char *host)
{
   // Check existence and match condition of an 'if' directive
   // If none (valid) is found, return -1.
   // Else, return number of chars matching.

   // There must be an 'if'
   char *val = s ? s->GetToken() : 0;
   if (!val || strncmp(val,"if",2)) {
      if (val)
         // allow the analysis of the token
         s->RetToken();
      return -1;
   }

   // check value if any
   val = s->GetToken();
   if (!val)
      return -1;

   // Deprecate
   XPDPRT( ">>> Warning: 'if' conditions at the end of the directive are deprecated ");
   XPDPRT( ">>> Please use standard Scalla/Xrootd 'if-else-fi' constructs");
   XPDPRT( ">>> (see http://xrootd.slac.stanford.edu/doc/xrd_config/xrd_config.htm)");

   // Notify
   TRACE(DBG, "CheckIf: <pattern>: " <<val);

   // Return number of chars matching
   XrdOucString h(host);
   return h.matches((const char *)val);
}

//______________________________________________________________________________
int XrdProofdAux::GetNumCPUs()
{
   // Find out and return the number of CPUs in the local machine.
   // Return -1 in case of failure.

   static int ncpu = -1;

   // Use cached value, if any
   if (ncpu > 0)
      return ncpu;
   ncpu = 0;

#if defined(linux)
   // Look for in the /proc/cpuinfo file
   XrdOucString fcpu("/proc/cpuinfo");
   FILE *fc = fopen(fcpu.c_str(), "r");
   if (!fc) {
      if (errno == ENOENT) {
         XPDPRT( "GetNumCPUs: /proc/cpuinfo missing!!! Something very bad going on");
      } else {
         XrdOucString emsg("GetNumCPUs: cannot open ");
         emsg += fcpu;
         emsg += ": errno: ";
         emsg += errno;
         XPDPRT( emsg.c_str());
      }
      return -1;
   }
   // Read lines and count those starting with "processor"
   char line[2048] = { 0 };
   while (fgets(line, sizeof(line), fc)) {
      if (!strncmp(line, "processor", strlen("processor")))
         ncpu++;
   }
   // Close the file
   fclose(fc);

#elif defined(__sun)

   // Run "psrinfo" in popen and count lines
   FILE *fp = popen("psrinfo", "r");
   if (fp != 0) {
      char line[2048] = { 0 };
      while (fgets(line, sizeof(line), fp))
         ncpu++;
      pclose(fp);
   }

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)

   // Run "sysctl -n hw.ncpu" in popen and decode the output
   FILE *fp = popen("sysctl -n hw.ncpu", "r");
   if (fp != 0) {
      char line[2048] = { 0 };
      while (fgets(line, sizeof(line), fp))
         ncpu = XrdProofdAux::GetLong(&line[0]);
      pclose(fp);
   }
#endif

   XPDPRT( "GetNumCPUs: # of cores found: "<<ncpu);

   // Done
   return (ncpu <= 0) ? (int)(-1) : ncpu ;
}

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
//__________________________________________________________________________
int XrdProofdAux::GetMacProcList(kinfo_proc **plist, int &nproc)
{
   // Returns a list of all processes on the system.  This routine
   // allocates the list and puts it in *plist and counts the
   // number of entries in 'nproc'. Caller is responsible for 'freeing'
   // the list.
   // On success, the function returns 0.
   // On error, the function returns an errno value.
   //
   // Adapted from: reply to Technical Q&A 1123,
   //               http://developer.apple.com/qa/qa2001/qa1123.html
   //

   int rc = 0;
   kinfo_proc *res;
   bool done = 0;
   static const int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

   TRACE(ACT, "GetMacProcList: enter ");

   // Declaring name as const requires us to cast it when passing it to
   // sysctl because the prototype doesn't include the const modifier.
   size_t len = 0;

   if (!plist || (*plist))
      return EINVAL;
   nproc = 0;

   // We start by calling sysctl with res == 0 and len == 0.
   // That will succeed, and set len to the appropriate length.
   // We then allocate a buffer of that size and call sysctl again
   // with that buffer.  If that succeeds, we're done.  If that fails
   // with ENOMEM, we have to throw away our buffer and loop.  Note
   // that the loop causes use to call sysctl with 0 again; this
   // is necessary because the ENOMEM failure case sets length to
   // the amount of data returned, not the amount of data that
   // could have been returned.

   res = 0;
   do {
      // Call sysctl with a 0 buffer.
      len = 0;
      if ((rc = sysctl((int *)name, (sizeof(name)/sizeof(*name)) - 1,
                       0, &len, 0, 0)) == -1) {
         rc = errno;
      }

      // Allocate an appropriately sized buffer based on the results
      // from the previous call.
      if (rc == 0) {
         res = (kinfo_proc *) malloc(len);
         if (!res)
            rc = ENOMEM;
      }

      // Call sysctl again with the new buffer.  If we get an ENOMEM
      // error, toss away our buffer and start again.
      if (rc == 0) {
         if ((rc = sysctl((int *)name, (sizeof(name)/sizeof(*name)) - 1,
                          res, &len, 0, 0)) == -1) {
            rc = errno;
         }
         if (rc == 0) {
            done = 1;
         } else if (rc == ENOMEM) {
            if (res)
               free(res);
            res = 0;
            rc = 0;
         }
      }
   } while (rc == 0 && !done);

   // Clean up and establish post conditions.
   if (rc != 0 && !res) {
      free(res);
      res = 0;
   }
   *plist = res;
   if (rc == 0)
      nproc = len / sizeof(kinfo_proc);

   // Done
   return rc;
}
#endif

//______________________________________________________________________________
int XrdProofdAux::VerifyProcessByID(int pid, const char *pname)
{
   // Check if a process named 'pname' and process 'pid' is still
   // in the process table.
   // For {linux, sun, macosx} it uses the system info; for other systems it
   // invokes the command shell 'ps ax' via popen.
   // Return 1 if running, 0 if not running, -1 if the check could not be run.

   int rc = 0;

   TRACE(ACT, "VerifyProcessByID: enter: pid: "<<pid);

   // Check input consistency
   if (pid < 0) {
      TRACE(XERR, "VerifyProcessByID: invalid pid");
      return -1;
   }

   // Name
   const char *pn = (pname && strlen(pname) > 0) ? pname : "proofserv";

#if defined(linux)
   // Look for the relevant /proc dir
   XrdOucString fn("/proc/");
   fn += pid;
   fn += "/stat";
   FILE *ffn = fopen(fn.c_str(), "r");
   if (!ffn) {
      if (errno == ENOENT) {
         TRACE(DBG, "VerifyProcessByID: process does not exists anymore");
         return 0;
      } else {
         XrdOucString emsg("VerifyProcessByID: cannot open ");
         emsg += fn;
         emsg += ": errno: ";
         emsg += errno;
         TRACE(XERR, emsg.c_str());
         return -1;
      }
   }
   // Read status line
   char line[2048] = { 0 };
   if (fgets(line, sizeof(line), ffn)) {
      if (strstr(line, pn))
         // Still there
         rc = 1;
   } else {
      XrdOucString emsg("VerifyProcessByID: cannot read ");
      emsg += fn;
      emsg += ": errno: ";
      emsg += errno;
      TRACE(XERR, emsg.c_str());
      fclose(ffn);
      return -1;
   }
   // Close the file
   fclose(ffn);

#elif defined(__sun)

   // Look for the relevant /proc dir
   XrdOucString fn("/proc/");
   fn += pid;
   fn += "/psinfo";
   int ffd = open(fn.c_str(), O_RDONLY);
   if (ffd <= 0) {
      if (errno == ENOENT) {
         TRACE(DBG, "VerifyProcessByID: process does not exists anymore");
         return 0;
      } else {
         XrdOucString emsg("VerifyProcessByID: cannot open ");
         emsg += fn;
         emsg += ": errno: ";
         emsg += errno;
         TRACE(XERR, emsg.c_str());
         return -1;
      }
   }
   // Get the information
   psinfo_t psi;
   if (read(ffd, &psi, sizeof(psinfo_t)) != sizeof(psinfo_t)) {
      XrdOucString emsg("VerifyProcessByID: cannot read ");
      emsg += fn;
      emsg += ": errno: ";
      emsg += errno;
      TRACE(XERR, emsg.c_str());
      close(ffd);
      return -1;
   }

   // Verify now
   if (strstr(psi.pr_fname, pn))
      // The process is still there
      rc = 1;

   // Close the file
   close(ffd);

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)

   // Get the proclist
   kinfo_proc *pl = 0;
   int np;
   int ern = 0;
   if ((ern = XrdProofdAux::GetMacProcList(&pl, np)) != 0) {
      XrdOucString emsg("VerifyProcessByID: cannot get the process list: errno: ");
      emsg += ern;
      TRACE(XERR, emsg.c_str());
      return -1;
   }

   // Loop over the list
   while (np--) {
      if (pl[np].kp_proc.p_pid == pid &&
          strstr(pl[np].kp_proc.p_comm, pn)) {
         // Process still exists
         rc = 1;
         break;
      }
   }
   // Cleanup
   free(pl);
#else
   // Use the output of 'ps ax' as a backup solution
   XrdOucString cmd = "ps ax | grep proofserv 2>/dev/null";
   if (pname && strlen(pname))
      cmd.replace("proofserv", pname);
   FILE *fp = popen(cmd.c_str(), "r");
   if (fp != 0) {
      char line[2048] = { 0 };
      while (fgets(line, sizeof(line), fp)) {
         if (pid == XrdProofdAux::GetLong(line)) {
            // Process still running
            rc = 1;
            break;
         }
      }
      pclose(fp);
   } else {
      // Error executing the command
      return -1;
   }
#endif
   // Done
   return rc;
}

//______________________________________________________________________________
int XrdProofdAux::KillProcess(int pid, bool forcekill, XrdProofUI ui, bool changeown)
{
   // Kill the process 'pid'.
   // A SIGTERM is sent, unless 'kill' is TRUE, in which case a SIGKILL is used.
   // If add is TRUE (default) the pid is added to the list of processes
   // requested to terminate.
   // Return 0 on success, -1 if not allowed or other errors occured.

   TRACE(ACT, "KillProcess: enter: pid: "<<pid<< ", forcekill: "<< forcekill);

   if (pid > 0) {
      // We need the right privileges to do this
      XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
      if (XpdBadPGuard(pGuard, ui.fUid) && changeown) {
         XrdOucString msg = "KillProcess: could not get privileges";
         TRACE(XERR, msg.c_str());
         return -1;
      } else {
         bool signalled = 1;
         if (forcekill)
            // Hard shutdown via SIGKILL
            if (kill(pid, SIGKILL) != 0) {
               if (errno != ESRCH) {
                  XrdOucString msg = "KillProcess: kill(pid,SIGKILL) failed for process: ";
                  msg += pid;
                  msg += " - errno: ";
                  msg += errno;
                  TRACE(XERR, msg.c_str());
                  return -1;
               }
               signalled = 0;
            }
         else
            // Softer shutdown via SIGTERM
            if (kill(pid, SIGTERM) != 0) {
               if (errno != ESRCH) {
                  XrdOucString msg = "KillProcess: kill(pid,SIGTERM) failed for process: ";
                  msg += pid;
                  msg += " - errno: ";
                  msg += errno;
                  TRACE(XERR, msg.c_str());
                  return -1;
               }
               signalled = 0;
            }
         // Add to the list of termination attempts
         if (!signalled) {
            TRACE(DBG, "KillProcess: process ID "<<pid<<" not found in the process table");
         }
      }
   } else {
      return -1;
   }

   // Done
   return 0;
}

//______________________________________________________________________________
char *XrdProofdAux::ReadMsg(int fd)
{
   // Read a meassage from descriptor 'fd'

   char *buf = 0;

   // Read message length
   int len = 0;
   if (read(fd, &len, sizeof(len)) != sizeof(len)) {
      TRACE(XERR, "readMsg: problems receiving message length");
      return buf;
   }

   // Read message
   buf = new char[len + 1];
   int n, nr = 0;
   for (n = 0; n < len; n += nr) {
      while ((nr = read(fd, buf + n, len - n)) == -1 && errno == EINTR)
         errno = 0;   // probably a SIGCLD that was caught
      if (nr <= 0)
         break;       // EOF or failure
   }
   if (nr < 0)
      SafeDelArray(buf);
   // Done
   return buf;
}

//______________________________________________________________________________
int XrdProofdAux::Form(XrdOucString &str, const char *fmt, ...)
{
   // Format a string in 'str' according to 'fmt' and the arguments

   static int buf_size = 2048;
   static char *buf = 0;

   XrdSysMutexHelper mh(fgFormMutex);

   va_list ap;
   va_start(ap, fmt);

again:
   if (!buf)
      buf = new char[buf_size];

   int n = vsnprintf(buf, buf_size, fmt, ap);
   // old vsnprintf's return -1 if string is truncated new ones return
   // total number of characters that would have been written
   if (n == -1 || n >= buf_size) {
      if (n == -1)
         buf_size *= 2;
      else
         buf_size = n+1;
      delete [] buf;
      buf = 0;
      va_end(ap);
      va_start(ap, fmt);
      goto again;
   }
   va_end(ap);

   str = buf;

   // Done
   return n;
}

//______________________________________________________________________________
int XrdProofdAux::RmDir(const char *path)
{
   // Remove directory at path and its content.
   // Returns 0 on success, -errno of the last error on failure

   int rc = 0;

   TRACE(DBG, "RmDir: dir: "<<path);

   // Open dir
   DIR *dir = opendir(path);
   if (!dir) {
      TRACE(XERR, "RmDir: cannot open dir "<<path<<" ; error: "<<errno);
      return -errno;
   }

   // Scan the directory
   XrdOucString entry;
   struct stat st;
   struct dirent *ent = 0;
   while ((ent = (struct dirent *)readdir(dir))) {
      // Skip the basic entries
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
      // Get info about the entry
      XrdProofdAux::Form(entry, "%s/%s", path, ent->d_name);
      if (stat(entry.c_str(), &st) != 0) {
         TRACE(XERR, "RmDir: cannot stat entry "<<entry<<" ; error: "<<errno);
         rc = -errno;
         break;
      }
      // Remove directories recursively
      if (S_ISDIR(st.st_mode)) {
         rc = XrdProofdAux::RmDir(entry.c_str());
         if (rc != 0) {
            TRACE(XERR, "RmDir: problems removing"<<entry<<" ; error: "<<-rc);
            break;
         }
      } else {
         // Remove the entry
         if (unlink(entry.c_str()) != 0) {
            rc = -errno;
            TRACE(XERR, "RmDir: problems removing"<<entry<<" ; error: "<<-rc);
            break;
         }
      }
   }
   // Close the directory
   closedir(dir);

   // If successful, remove the directory
   if (!rc && rmdir(path) != 0) {
      rc = -errno;
      TRACE(XERR, "RmDir: problems removing"<<path<<" ; error: "<<-rc);
   }

   // Done
   return rc;
}

//______________________________________________________________________________
int XrdProofdAux::MvDir(const char *oldpath, const char *newpath)
{
   // Move content of directory at oldpath to newpath.
   // The destination path 'newpath' must exist.
   // Returns 0 on success, -errno of the last error on failure

   int rc = 0;

   TRACE(DBG, "MvDir: oldpath "<<oldpath<<", newpath: "<<newpath);

   // Open existing dir
   DIR *dir = opendir(oldpath);
   if (!dir) {
      TRACE(XERR, "MvDir: cannot open dir "<<oldpath<<" ; error: "<<errno);
      return -errno;
   }

   // Assert destination dir
   struct stat st;
   if (stat(newpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
      TRACE(XERR, "MvDir: destination dir "<<newpath<<
                  " does nto exists of is nto a directory; errno: "<<errno);
      return -ENOENT;
   }

   // Scan the source directory
   XrdOucString srcentry, dstentry;
   struct dirent *ent = 0;
   while ((ent = (struct dirent *)readdir(dir))) {
      // Skip the basic entries
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
      // Get info about the entry
      XrdProofdAux::Form(srcentry, "%s/%s", oldpath, ent->d_name);
      if (stat(srcentry.c_str(), &st) != 0) {
         TRACE(XERR, "MvDir: cannot stat entry "<<srcentry<<" ; error: "<<errno);
         rc = -errno;
         break;
      }
      // Destination entry
      XrdProofdAux::Form(dstentry, "%s/%s", newpath, ent->d_name);
      // Mv directories recursively
      if (S_ISDIR(st.st_mode)) {
         mode_t srcmode = st.st_mode;
         // Create dest sub-dir
         if (stat(dstentry.c_str(), &st) == 0) {
            if (!S_ISDIR(st.st_mode)) {
               TRACE(XERR, "MvDir: destination path already exists and is not a directory: "<<dstentry);
               rc = -ENOTDIR;
               break;
            }
         } else {
            if (mkdir(dstentry.c_str(), srcmode) != 0) {
               TRACE(XERR, "MvDir: cannot create entry "<<dstentry<<" ; error: "<<errno);
               rc = -errno;
               break;
            }
         }
         if ((rc = XrdProofdAux::MvDir(srcentry.c_str(), dstentry.c_str())) != 0) {
            TRACE(XERR, "MvDir: problems moving "<<srcentry<<" to "<<dstentry<<"; error: "<<-rc);
            break;
         }
         if ((rc = XrdProofdAux::RmDir(srcentry.c_str())) != 0) {
            TRACE(XERR, "MvDir: problems removing "<<srcentry<<"; error: "<<-rc);
            break;
         }
      } else {
         // Move the entry
         if (rename(srcentry.c_str(), dstentry.c_str()) != 0) {
            rc = -errno;
            TRACE(XERR, "MvDir: problems moving "<<srcentry<<" to "<<dstentry<<"; error: "<<-rc);
            break;
         }
      }
   }
   // Close the directory
   closedir(dir);

   // Done
   return rc;
}

//______________________________________________________________________________
int XrdProofdAux::Touch(const char *path, int opt)
{
   // Set access (opt == 1), modify (opt =2 ) or access&modify (opt = 0, default)
   // times of path to current time.
   // Returns 0 on success, -errno on failure

   if (opt == 0) {
      if (utime(path, 0) != 0)
         return -errno;
   } else if (opt <= 2) {
      struct stat st;
      if (stat(path, &st) != 0)
         return -errno;
      struct utimbuf ut;
      if (opt == 1) {
         ut.actime = time(0);
         ut.modtime = st.st_mtime;
      } else if (opt == 2) {
         ut.modtime = time(0);
         ut.actime = st.st_atime;
      }
      if (utime(path, &ut) != 0)
         return -errno;
   } else {
      // Unknown option
      return -1;
   }
   // Done
   return 0;
}

//
// Functions to process directives for integer and strings
//

//______________________________________________________________________________
int DoDirectiveClass(XrdProofdDirective *d, char *val, XrdOucStream *cfg, bool rcf)
{
   // Generic class directive processor

   if (!d || !(d->fVal))
      // undefined inputs
      return -1;

   return ((XrdProofdConfig *)d->fVal)->DoDirective(d, val, cfg, rcf);
}

//______________________________________________________________________________
int DoDirectiveInt(XrdProofdDirective *d, char *val, XrdOucStream *cfg, bool rcf)
{
   // Process directive for an integer

   if (!d || !(d->fVal) || !val)
      // undefined inputs
      return -1;

   if (rcf && !d->fRcf)
      // Not re-configurable: do nothing
      return 0;

   // Check deprecated 'if' directive
   if (d->fHost && cfg)
      if (XrdProofdAux::CheckIf(cfg, d->fHost) == 0)
         return 0;

   int v = strtol(val,0,10);
   *((int *)d->fVal) = v;

   TRACE(DBG, "DoDirectiveInt: set "<<d->fName<<" to "<<*((int *)d->fVal));

   return 0;
}

//______________________________________________________________________________
int DoDirectiveString(XrdProofdDirective *d, char *val, XrdOucStream *cfg, bool rcf)
{
   // Process directive for a string

   if (!d || !(d->fVal) || !val)
      // undefined inputs
      return -1;

   if (rcf && !d->fRcf)
      // Not re-configurable: do nothing
      return 0;

   // Check deprecated 'if' directive
   if (d->fHost && cfg)
      if (XrdProofdAux::CheckIf(cfg, d->fHost) == 0)
         return 0;

   *((XrdOucString *)d->fVal) = val;

   TRACE(DBG, "DoDirectiveString: set "<<d->fName<<" to "<<*((XrdOucString *)d->fVal));
   return 0;
}

//__________________________________________________________________________
int SetHostInDirectives(const char *, XrdProofdDirective *d, void *h)
{
   // Set host field for directive 'd' to (const char *h)

   const char *host = (const char *)h;

   if (!d || !host || strlen(host) <= 0)
      // Dataset root dir undefined: we cannot continue
      return 1;

   d->fHost = host;

   // Process next
   return 0;
}

