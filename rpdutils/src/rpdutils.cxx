// @(#)root/rpdutils:$Name:  $:$Id: rpdutils.cxx,v 1.28 2003/12/30 21:42:27 brun Exp $
// Author: Gerardo Ganis    7/4/2003

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// rpdutils                                                             //
//                                                                      //
// Set of utilities for rootd/proofd daemon authentication.             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "config.h"

#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>

#if defined(__CYGWIN__) && defined(__GNUC__)
#   define cygwingcc
#endif
#if defined(__alpha) && !defined(linux)
#   ifdef _XOPEN_SOURCE
#      if _XOPEN_SOURCE+0 > 0
#         define R__TRUE64
#      endif
#   endif
#include <sys/mount.h>
#ifndef R__TRUE64
extern "C" int fstatfs(int file_descriptor, struct statfs *buffer);
#endif
#elif defined(__APPLE__)
#include <sys/mount.h>
extern "C" int fstatfs(int file_descriptor, struct statfs *buffer);
#elif defined(linux) || defined(__hpux)
#include <sys/vfs.h>
#elif defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/statfs.h>
#endif

#if defined(linux)
#   include <features.h>
#   if __GNU_LIBRARY__ == 6
#      ifndef R__GLIBC
#         define R__GLIBC
#      endif
#   endif
#endif
#if defined(cygwingcc) || (defined(__MACH__) && !defined(__APPLE__))
#   define R__GLIBC
#endif

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif
#if (defined(__FreeBSD__) && (__FreeBSD__ < 4)) || \
    (defined(__APPLE__) && (!defined(MAC_OS_X_VERSION_10_3) || \
     (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_3)))
#include <sys/file.h>
#define lockf(fd, op, sz)   flock((fd), (op))
#ifndef F_LOCK
#define F_LOCK             (LOCK_EX | LOCK_NB)
#endif
#ifndef F_ULOCK
#define F_ULOCK             LOCK_UN
#endif
#endif

#if defined(cygwingcc)
#define F_LOCK F_WRLCK
#define F_ULOCK F_UNLCK
static int fcntl_lockf(int fd, int op, off_t off)
{
   flock fl;
   fl.l_whence = SEEK_SET;
   fl.l_start  = off;
   fl.l_len    = 0;       // whole file
   fl.l_pid    = getpid();
   fl.l_type   = op;
   return fcntl(fd, F_SETLK, &fl);
}
#define lockf fcntl_lockf
#endif

#if defined(__sun) || defined(R__GLIBC)
#include <crypt.h>
#endif

#if defined(__osf__) || defined(__sgi)
extern "C" char *crypt(const char *, const char *);
#endif

#if defined(__sun)
#ifndef R__SHADOWPW
#define R__SHADOWPW
#endif
#endif

#ifdef R__SHADOWPW
#include <shadow.h>
#endif

#ifdef R__AFS
//#include <afs/kautils.h>
#define KA_USERAUTH_VERSION 1
#define KA_USERAUTH_DOSETPAG 0x10000
#define NOPAG  0xffffffff
extern "C" int ka_UserAuthenticateGeneral(int, char *, char *, char *,
                                          char *, int, int, int, char **);
#endif

#ifdef R__SRP
extern "C" {
   #include <t_pwd.h>
   #include <t_server.h>
}
#endif

#ifdef R__KRB5
#include "Krb5Auth.h"
#include <string>
extern krb5_deltat krb5_clockskew;
#endif

#include "rpdp.h"
extern "C" {
   #include "rsadef.h"
   #include "rsalib.h"
}

//--- Machine specific routines ------------------------------------------------

#if defined(__sgi) && !defined(__GNUG__) && (SGI_REL<62)
extern "C" {
   int seteuid(int euid);
   int setegid(int egid);
}
#endif

#if defined(_AIX)
extern "C" {
   int seteuid(uid_t euid);
   int setegid(gid_t egid);
}
#endif

#if !defined(__hpux) && !defined(linux) && !defined(__FreeBSD__) || \
    defined(cygwingcc)
static int setresgid(gid_t r, gid_t e, gid_t)
{
   if (setgid(r) == -1)
      return -1;
   return setegid(e);
}

static int setresuid(uid_t r, uid_t e, uid_t)
{
   if (setuid(r) == -1)
      return -1;
   return seteuid(e);
}
#else
#if defined(linux) && !defined(HAS_SETRESUID)
extern "C" {
   int setresgid(gid_t r, gid_t e, gid_t s);
   int setresuid(uid_t r, uid_t e, uid_t s);
}
#endif
#endif


namespace ROOT {

//--- Globals ------------------------------------------------------------------
const char *kAuthMeth[kMAXSEC] = { "UsrPwd", "SRP", "Krb5", "Globus", "SSH", "UidGid" };
const char kRootdPass[]    = ".rootdpass";
const char kSRootdPass[]   = ".srootdpass";
const char kDaemonRc[]     = ".rootdaemonrc"; // file containing daemon access rules

// To control user access
char *gUserAllow[kMAXSEC] = { 0 };
char *gUserIgnore[kMAXSEC] = { 0 };
unsigned int gUserAlwLen[kMAXSEC] = { 0 };
unsigned int gUserIgnLen[kMAXSEC] = { 0 };

char gAltSRPPass[kMAXPATHLEN] = { 0 };
char gAnonUser[64] = "rootd";
char gSystemDaemonRc[kMAXPATHLEN] = { 0 }; // path to kDaemonRc
char gExecDir[kMAXPATHLEN] = { 0 };   // needed to localize ssh2rpd
char gFileLog[kMAXPATHLEN] = { 0 };
char gOpenHost[256] = "????";         // same length as in net.cxx ...
char gPasswd[64] = { 0 };
char gRpdAuthTab[kMAXPATHLEN] = { 0 };  // keeps track of authentication info
char gService[10] = "????";         // "rootd" or "proofd", defined in proofd/rootd.cxx ...
char gTmpDir[kMAXPATHLEN] = { 0 };  // RW dir for temporary files
char gUser[64] = { 0 };

int gAltSRP = 0;
int gAnon = 0;
int gAuth = 0;
int gClientProtocol = 0;
int gDebug = 0;
int gGlobus = -1;
int gNumAllow = -1;
int gNumLeft = -1;
int gOffSet = -1;
int gPort = 0;
int gPortA = 0;
int gPortB = 0;
int gRemPid = -1;
int gReUseAllow = 0x1F;  // define methods for which previous auth can be reused
int gRootLog = 0;
int gSshdPort = 22;

// Globals of internal linkage
int  gAllowMeth[kMAXSEC];
int  gCryptRequired = -1;
int  gHaveMeth[kMAXSEC];
int  gMethInit = 0;
char gPubKey[kMAXPATHLEN] = { 0 };
int  gReUseRequired = -1;
int  gRSAKey = 0;
rsa_NUMBER gRSA_n;
rsa_NUMBER gRSA_d;
int gRSAInit = 0;
rsa_KEY gRSAPriKey;
rsa_KEY gRSAPubKey;
rsa_KEY_export gRSAPubExport;
int  gSaltRequired = -1;
int  gSec = -1;
int  gTriedMeth[kMAXSEC];

#ifdef R__KRB5
krb5_keytab gKeytab = 0;        // to allow specifying on the command line
krb5_context gKcontext;
#endif


#ifdef R__GLBS
int gShmIdCred = -1;            // global, to pass the shm ID to proofserv
gss_ctx_id_t GlbContextHandle = GSS_C_NO_CONTEXT;
#endif

static int gRandInit = 0;

} //namespace ROOT

// Masks for authentication methods
const int kAUTH_CLR_MSK = 0x1;
const int kAUTH_SRP_MSK = 0x2;
const int kAUTH_KRB_MSK = 0x4;
const int kAUTH_GLB_MSK = 0x8;
const int kAUTH_SSH_MSK = 0x10;

namespace ROOT {
//______________________________________________________________________________
static int rpdstrncasecmp(const char *str1, const char *str2, int n)
{
   // Case insensitive string compare of n characters.

   while (n > 0) {
      int c1 = *str1;
      int c2 = *str2;

      if (isupper(c1))
         c1 = tolower(c1);

      if (isupper(c2))
         c2 = tolower(c2);

      if (c1 != c2)
         return c1 - c2;

      str1++;
      str2++;
      n--;
   }
   return 0;
}

//______________________________________________________________________________
static int rpdstrcasecmp(const char *str1, const char *str2)
{
   // Case insensitive string compare.

   return rpdstrncasecmp(str1, str2, strlen(str2) + 1);
}

#ifdef R__KRB5
//______________________________________________________________________________
static void PrintPrincipal(krb5_principal principal)
{
   ErrorInfo("PrintPrincipal: realm == '%s'",
             principal->realm.data);
   ErrorInfo("PrintPrincipal: length == %d type == %d",
             principal->length, principal->type);
   for (int i = 0; i < principal->length; i++) {
      ErrorInfo("PrintPrincipal: data[%d] == %s",
                i, principal->data[i].data);
   }
}
#endif

//______________________________________________________________________________
void RpdSetDebugFlag(int Debug)
{
   // Change the value of the static gDebug to Debug.

   gDebug = Debug;
   if (gDebug > 2)
      ErrorInfo("RpdSetDebugFlag: gDebug set to %d", gDebug);
}

//______________________________________________________________________________
void RpdSetRootLogFlag(int RootLog)
{
   // Change the value of the static gRootLog to RootLog.
   // Recognized values:
   //                       0      log to syslog (for root started daemons)
   //                       1      log to stderr ( for user started daemons)

   gRootLog = RootLog;
   if (gDebug > 2)
      ErrorInfo("RpdSetRootLogFlag: gRootLog set to %d", gRootLog);
}

//______________________________________________________________________________
void RpdSetAuthTabFile(char *AuthTabFile)
{
   // Change the value of the static gRpdAuthTab to AuthTabFile.

   strcpy(gRpdAuthTab, AuthTabFile);
   if (gDebug > 2)
      ErrorInfo("RpdSetAuthTabFile: gRpdAuthTab set to '%s'", AuthTabFile);
}

//______________________________________________________________________________
int RpdGetAuthMethod(int kind)
{
   int Meth = -1;

   if (kind == kROOTD_USER)
      Meth = 0;
   if (kind == kROOTD_SRPUSER)
      Meth = 1;
   if (kind == kROOTD_KRB5)
      Meth = 2;
   if (kind == kROOTD_GLOBUS)
      Meth = 3;
   if (kind == kROOTD_SSH)
      Meth = 4;
   if (kind == kROOTD_RFIO)
      Meth = 5;

   return Meth;
}

//______________________________________________________________________________
int RpdUpdateAuthTab(int opt, char *line, char **token)
{
   // Update tab file.
   // If opt=0 then eliminates all inactive entries,
   // if opt=1 append 'line'.
   // Returns offset for 'line' (opt=1) or -1 if any error occurs
   // and token.

   int retval = -1;
   int itab = 0;
   char fbuf[kMAXPATHLEN];

   if (gDebug > 2)
      ErrorInfo("RpdUpdateAuthTab: analyzing: opt: %d, line: %s", opt,
                line);

   if (opt == -1) {
      if (!access(gRpdAuthTab, F_OK)) {
         // Save the content ...
         char *bak = new char[strlen(gRpdAuthTab) + 10];
         sprintf(bak, "%s.bak", gRpdAuthTab);
         FILE *fbak = fopen(bak, "w");
         FILE *ftab = fopen(gRpdAuthTab, "r");
         char buf[kMAXPATHLEN];
         while (fgets(buf, sizeof(buf), ftab)) {
            fprintf(fbak, "%s", buf);
         }
         fclose(fbak);
         fclose(ftab);
         // ... before deleting the original ...
         unlink(gRpdAuthTab);
         if (bak) delete[] bak;
      }
      return 0;
   } else if (opt == 0) {
      // Open file for update
      itab = open(gRpdAuthTab, O_RDWR | O_CREAT, 0666);
      if (itab == -1) {
         ErrorInfo
             ("RpdUpdateAuthTab: opt=%d: error opening %s (errno: %d)",
              opt, gRpdAuthTab, GetErrno());
         return retval;
      }
      // override umask setting
      fchmod(itab, 0666);
      // lock tab file
      if (lockf(itab, F_LOCK, (off_t) 1) == -1) {
         ErrorInfo
             ("RpdUpdateAuthTab: opt=%d: error locking %s (errno: %d)",
              opt, gRpdAuthTab, GetErrno());
         close(itab);
         return retval;
      }
      // File is open: get FILE descriptor
      FILE *ftab = fdopen(itab, "a");
      // and set indicator to beginning
      lseek(itab, 0, SEEK_SET);

      // Now scan over entries
      int pr = 0, pw = 0;
      int lsec, act;
      char line[kMAXPATHLEN], dumm[kMAXPATHLEN];
      bool good = 0, fwr = 0;

      while (fgets(line, sizeof(line), ftab)) {
         pr = lseek(itab, 0, SEEK_CUR);
         sscanf(line, "%d %d %s", &lsec, &act, dumm);
         good = (act == 1);
         if (good) {
            if (fwr) {
               lseek(itab, pw, SEEK_SET);
               sprintf(fbuf, "%s\n", line);
               while (write(itab, fbuf, strlen(fbuf)) < 0
                      && GetErrno() == EINTR)
                  ResetErrno();
               pw = lseek(itab, 0, SEEK_CUR);
               lseek(itab, pr, SEEK_SET);
            } else
               pw = lseek(itab, 0, SEEK_CUR);
         } else {
            fwr = 1;
         }
      }

      // Truncate file to new length
      ftruncate(itab, pw);

      retval = 0;

   } else if (opt == 1) {
      // open file for append
      if (gDebug > 2)
         ErrorInfo("RpdUpdateAuthTab: opening file %s", gRpdAuthTab);

      if (access(gRpdAuthTab, F_OK)) {
         itab = open(gRpdAuthTab, O_RDWR | O_CREAT, 0666);
         if (itab == -1) {
            ErrorInfo
                ("RpdUpdateAuthTab: opt=%d: error opening %s (errno: %d)",
                 opt, gRpdAuthTab, GetErrno());
            return retval;
         }
         // override umask setting
         fchmod(itab, 0666);
      } else {
         itab = open(gRpdAuthTab, O_RDWR);
      }
      if (itab == -1) {
         ErrorInfo
             ("RpdUpdateAuthTab: opt=%d: error opening or creating %s (errno: %d)",
              opt, gRpdAuthTab, GetErrno());
         return retval;
      }
      // lock tab file
      if (gDebug > 2)
         ErrorInfo("RpdUpdateAuthTab: locking file %s", gRpdAuthTab);
      if (lockf(itab, F_LOCK, (off_t) 1) == -1) {
         ErrorInfo
             ("RpdUpdateAuthTab: opt=%d: error locking %s (errno: %d)",
              opt, gRpdAuthTab, GetErrno());
         close(itab);
         return retval;
      }
      // saves offset
      retval = lseek(itab, 0, SEEK_END);
      if (gDebug > 2)
         ErrorInfo("RpdUpdateAuthTab: offset is %d", retval);

      // Generate token
      *token = RpdGetRandString(3, 8);   // 8 crypt-like chras
      char *CryptToken = crypt(*token, *token);
      sprintf(fbuf, "%s %s\n", line, CryptToken);
      if (gDebug > 2)
         ErrorInfo("RpdUpdateAuthTab: token: '%s'", CryptToken);

      // adds line
      while (write(itab, fbuf, strlen(fbuf)) < 0 && GetErrno() == EINTR)
         ResetErrno();

   } else {
      ErrorInfo("RpdUpdateAuthTab: unrecognized option (opt= %d)", opt);
      return retval;
   }

   // unlock the file
   lseek(itab, 0, SEEK_SET);
   if (lockf(itab, F_ULOCK, (off_t) 1) == -1) {
      ErrorInfo("RpdUpdateAuthTab: error unlocking %s", gRpdAuthTab);
   }
   // closing file ...
   close(itab);

   return retval;
}

//______________________________________________________________________________
int RpdCleanupAuthTab(char *Host, int RemId)
{
   // Cleanup (set inactive) entries in tab file,
   // if Host="all" or RemId=0 discard all entries.
   // Return number of entries not cleaned properly ...

   int retval = 0;

   if (gDebug > 2)
      ErrorInfo("RpdCleanupAuthTab: cleaning for Host: '%s', RemId:%d",
                Host, RemId);

   // Open file for update
   int itab = -1;
   if (access(gRpdAuthTab,F_OK) == 0) {

      itab = open(gRpdAuthTab, O_RDWR);
      if (itab == -1) {
         ErrorInfo("RpdCleanupAuthTab: error opening %s (errno: %d)",
                  gRpdAuthTab, GetErrno());
         //     return retval;
         return -1;
      }
   } else {
      if (gDebug > 0)
         ErrorInfo("RpdCleanupAuthTab: file %s does not exist",gRpdAuthTab);
      return -3;
   }

   // lock tab file
   if (lockf(itab, F_LOCK, (off_t) 1) == -1) {
      ErrorInfo("RpdCleanupAuthTab: error locking %s (errno: %d)",
                gRpdAuthTab, GetErrno());
      close(itab);
      //     return retval;
      return -2;
   }
   // File is open: get FILE descriptor
   FILE *ftab = fdopen(itab, "r+");

   // Now scan over entries
   int pr = 0, pw = 0;
   int nw, lsec, act, parid, remid, pkey;
   char line[kMAXPATHLEN], line1[kMAXPATHLEN], host[kMAXPATHLEN];
   char dumm[kMAXPATHLEN], user[kMAXPATHLEN];
#ifdef R__GLBS
   char subj[kMAXPATHLEN];
#endif

   // and set indicator to beginning
   pr = lseek(itab, 0, SEEK_SET);
   pw = pr;
   while (fgets(line, sizeof(line), ftab)) {
      pr += strlen(line);
      if (gDebug > 2)
         ErrorInfo("RpdCleanupAuthTab: pr:%d pw:%d (line:%s)", pr, pw,
                   line);

      nw = sscanf(line, "%d %d %d %d %d %s %s %s", &lsec, &act, &pkey,
                  &parid, &remid, host, user, dumm);
      if (nw > 5) {
         if (!strcmp(Host, "all") || (RemId == 0) ||
             (!strcmp(Host, host) && (RemId == remid))) {

            // Delete Public Key file
            char PubKeyFile[kMAXPATHLEN];
            sprintf(PubKeyFile, "%s/rpk_%d", gTmpDir, pw);

            if (gDebug > 0) {
              struct stat st;
              if (stat(PubKeyFile, &st) == 0) {
                ErrorInfo("RpdCleanupAuthTab: file uid:%d gid:%d",st.st_uid,st.st_gid);
              }
              ErrorInfo("RpdCleanupAuthTab: proc uid:%d gid:%d",getuid(),getgid());
            }

            if (unlink(PubKeyFile) == -1) {
               if (gDebug > 0) {
                  ErrorInfo
                     ("RpdCleanupAuthTab: problems unlinking pub key file '%s' (errno: %d)",
                      PubKeyFile,GetErrno());
               }
            }

            if (act == 1) {
               if (lsec == 3) {
#ifdef R__GLBS
                  int shmid;
                  nw = sscanf(line, "%d %d %d %d %d %s %s %d %s %s", &lsec,
                              &act, &pkey, &parid, &remid, host, user,
                              &shmid, subj, dumm);
                  struct shmid_ds shm_ds;
                  if (shmctl(shmid, IPC_RMID, &shm_ds) == -1) {
                     ErrorInfo
                         ("RpdCleanupAuthTab: unable to mark shared memory segment %d for desctruction (errno: %d)",
                          shmid, GetErrno());
                     retval++;
                  }
                  sprintf(line1, "%d %d %d %d %d %s %s %d %s %s\n", lsec,
                          0, pkey, parid, remid, host, user, shmid, subj,
                          dumm);
#else
                  ErrorInfo
                      ("RpdCleanupAuthTab: compiled without Globus support: you shouldn't have got here!");
                  sprintf(line1,
                          "%d %d %d %d %d %s %s %s - WARNING: bad line\n",
                          lsec, 0, pkey, parid, remid, host, user, dumm);
#endif
               } else {
                  sprintf(line1, "%d %d %d %d %d %s %s %s\n", lsec, 0,
                          pkey, parid, remid, host, user, dumm);
               }
               lseek(itab, pw, SEEK_SET);
               while (write(itab, line1, strlen(line1)) < 0
                      && GetErrno() == EINTR)
                  ResetErrno();
               lseek(itab, pr, SEEK_SET);
            }
         }
      }
      pw = pr;
   }

   // unlock the file
   lseek(itab, 0, SEEK_SET);
   if (lockf(itab, F_ULOCK, (off_t) 1) == -1) {
      ErrorInfo("RpdCleanupAuthTab: error unlocking %s", gRpdAuthTab);
   }
   // closing file ...
   close(itab);

   return retval;
}

//______________________________________________________________________________
int RpdCheckAuthTab(int Sec, char *User, char *Host, int RemId, int *OffSet)
{
   // Check authentication entry in tab file.

   int retval = 0;
   bool GoodOfs = 0;
   int ofs = *OffSet >= 0 ? *OffSet : 0;

   if (gDebug > 2)
      ErrorInfo("RpdCheckAuthTab: analyzing: %d %s %s %d %d", Sec, User,
                Host, RemId, *OffSet);

   // First check if file exists and can be read
   if (access(gRpdAuthTab, R_OK)) {
      ErrorInfo("RpdCheckAuthTab: can't read file %s (errno: %d)",
                gRpdAuthTab, GetErrno());
      return retval;
   }
   // Open file
   int itab = open(gRpdAuthTab, O_RDWR);
   if (itab == -1) {
      ErrorInfo("RpdCheckAuthTab: error opening %s (errno: %d)",
                gRpdAuthTab, GetErrno());
      return retval;
   }
   // lock tab file
   if (lockf(itab, F_LOCK, (off_t) 1) == -1) {
      ErrorInfo("RpdCheckAuthTab: error locking %s (errno: %d)",
                gRpdAuthTab, GetErrno());
      close(itab);
      return retval;
   }
   // File is open: set position at wanted location
   FILE *ftab = fdopen(itab, "r+");
   fseek(ftab, ofs, SEEK_SET);

   // Now read the entry
   char line[kMAXPATHLEN];
   fgets(line, sizeof(line), ftab);

   // and parse its content according to auth method
   int lsec, act, parid, remid, shmid;
   char host[kMAXPATHLEN], user[kMAXPATHLEN], subj[kMAXPATHLEN],
       dumm[kMAXPATHLEN], tkn[20];
   int nw =
       sscanf(line, "%d %d %d %d %d %s %s %s %s", &lsec, &act, &gRSAKey,
              &parid, &remid, host, user, tkn, dumm);
   if (gDebug > 2)
      ErrorInfo("RpdCheckAuthTab: found line: %s", line);
   if (nw > 5) {
      if ((lsec == Sec)) {
         if (lsec == 3) {
            sscanf(line, "%d %d %d %d %d %s %s %d %s %s %s", &lsec, &act,
                   &gRSAKey, &parid, &remid, host, user, &shmid, subj, tkn,
                   dumm);
            //if ((parid == getppid()) && (remid == RemId)
            if ((remid == RemId)
                && !strcmp(host, Host) && !strcmp(subj, User))
               GoodOfs = 1;
         } else {
            //if ((parid == getppid()) && (remid == RemId) &&
            if ((remid == RemId) &&
                !strcmp(host, Host) && !strcmp(user, User))
               GoodOfs = 1;
         }
      }
   }
   if (!GoodOfs) {
      // Tab may have been cleaned in the meantime ... try a scan
      fseek(ftab, 0, SEEK_SET);
      ofs = ftell(ftab);
      while (fgets(line, sizeof(line), ftab)) {
         nw = sscanf(line, "%d %d %d %d %d %s %s %s %s", &lsec, &act,
                     &gRSAKey, &parid, &remid, host, user, tkn, dumm);
         if (gDebug > 2)
            ErrorInfo("RpdCheckAuthTab: found line: %s", line);
         if (nw > 5) {
            if (lsec == Sec) {
               if (lsec == 3) {
                  sscanf(line, "%d %d %d %d %d %s %s %d %s %s %s", &lsec,
                         &act, &gRSAKey, &parid, &remid, host, user,
                         &shmid, subj, tkn, dumm);
                  //if ((parid == getppid()) && (remid == RemId)
                  if ((remid == RemId)
                      && !strcmp(host, Host) && !strcmp(subj, User)) {
                     GoodOfs = 1;
                     goto found;
                  }
               } else {
                  //if ((parid == getppid()) && (remid == RemId) &&
                  if ((remid == RemId) &&
                      !strcmp(host, Host) && !strcmp(user, User)) {
                     GoodOfs = 1;
                     goto found;
                  }
               }
            }
         }
      }
   }

 found:
   if (gDebug > 2)
      ErrorInfo("RpdCheckAuthTab: GoodOfs: %d", GoodOfs);

   // Rename the key file, if needed
   if (*OffSet > 0 && *OffSet != ofs) {
      char *OldName = new char[strlen(gTmpDir) + 50];
      char *NewName = new char[strlen(gTmpDir) + 50];
      //sprintf(OldName, "%s/rpk_%d_%d", gTmpDir, getppid(), *OffSet);
      //sprintf(NewName, "%s/rpk_%d_%d", gTmpDir, getppid(), ofs);
      sprintf(OldName, "%s/rpk_%d", gTmpDir, *OffSet);
      sprintf(NewName, "%s/rpk_%d", gTmpDir, ofs);
      if (rename(OldName, NewName) == -1) {
         if (gDebug > 0)
            ErrorInfo
                ("RpdCheckAuthTab: Error renaming public key file (errno: %d)",
                 GetErrno());
         fseek(ftab, ofs, SEEK_SET);
         // set entry inactive
         if (Sec == 3) {
#ifdef R__GLBS
            // kGlobus:
            sprintf(line, "%d %d %d %d %d %s %s %d %s %s\n", lsec, 0,
                    gRSAKey, parid, remid, host, user, shmid, subj,
                    tkn);
#else
            ErrorInfo
                ("RpdCheckAuthTab: compiled without Globus support: you shouldn't have got here!");
#endif
         } else {
            sprintf(line, "%d %d %d %d %d %s %s %s\n", lsec, 0, gRSAKey,
                    parid, remid, host, user, tkn);
         }
         while (write(itab, line, strlen(line)) < 0
                && GetErrno() == EINTR)
            ResetErrno();
      }
      if (OldName) delete[] OldName;
      if (NewName) delete[] NewName;
   }

   // Receive Token
   char *token = 0;
   if (gRSAKey > 0) {
      // Get Public Key
      char PubKeyFile[kMAXPATHLEN];
      sprintf(PubKeyFile, "%s/rpk_%d", gTmpDir, ofs);
      if (gDebug > 2)
         ErrorInfo("RpdCheckAuthTab: RSAKey ofs file: %d %d '%s' ",
                   gRSAKey, ofs, PubKeyFile);
      if (RpdGetRSAKeys(PubKeyFile, 1) > 0) {
         if (RpdSecureRecv(&token) == -1) {
            ErrorInfo
                ("RpdCheckAuthTab: problems secure-receiving token - may result in authentication failure ");
         }
      }
   } else {
      EMessageTypes kind;
      int Tlen = 9;
      token = new char[Tlen];
      NetRecv(token, Tlen, kind);
      if (kind != kMESS_STRING)
         ErrorInfo
             ("RpdCheckAuthTab: got msg kind: %d instead of %d (kMESS_STRING)",
              kind, kMESS_STRING);
      // Invert Token
      for (int i = 0; i < (int) strlen(token); i++) {
         token[i] = ~token[i];
      }
   }

   if (gDebug > 2)
      ErrorInfo
          ("RpdCheckAuthTab: received from client: token: '%s' ",
           token);

   // Now check Token validity
   if (GoodOfs && (act == 1) && token && RpdCheckToken(token, tkn)) {

      if (Sec == 3) {
#ifdef R__GLBS
         // kGlobus:
         if (GlbsToolCheckContext(shmid)) {
            retval = 1;
            strcpy(gUser, user);
         } else {
            // set entry inactive
            fseek(ftab, ofs, SEEK_SET);
            sprintf(line, "%d %d %d %d %d %s %s %d %s %s\n", lsec, 0,
                    gRSAKey, parid, remid, host, user, shmid, subj,
                    tkn);
            while (write(itab, line, strlen(line)) < 0
                   && GetErrno() == EINTR)
               ResetErrno();
         }
#else
         ErrorInfo
                ("RpdCheckAuthTab: compiled without Globus support: you shouldn't have got here!");
#endif
      } else {
            retval = 1;
      }

      // Comunicate new offset to remote client
      if (retval) *OffSet = ofs;
   }

   // unlock the file
   lseek(itab, 0, SEEK_SET);
   if (lockf(itab, F_ULOCK, (off_t) 1) == -1) {
      ErrorInfo("RpdCheckAuthTab: error unlocking %s", gRpdAuthTab);
   }
   // closing file ...
   close(itab);

   return retval;
}

//______________________________________________________________________________
bool RpdCheckToken(char *token, char *tknref)
{
   // Check token validity.

   // Get rid of '\n'
   char *s = strchr(token, '\n');
   if (s)
      *s = 0;
   s = strchr(tknref, '\n');
   if (s)
      *s = 0;

   char *tkn_crypt = crypt(token, tknref);

   if (gDebug > 2)
      ErrorInfo("RpdCheckToken: ref:'%s' crypt:'%s'", tknref, tkn_crypt);

   if (!strncmp(tkn_crypt, tknref, 13))
      return 1;
   else
      return 0;
}

//______________________________________________________________________________
bool RpdReUseAuth(const char *sstr, int kind)
{
   // Check the requiring subject has already authenticated during this session
   // and its 'ticket' is still valid.
   // Not implemented for SRP and Krb5 (yet).

   int Ulen, OffSet, Opt;
   gOffSet = -1;
   gAuth = 0;

   if (gDebug > 2)
      ErrorInfo("RpdReUseAuth: analyzing: %s, %d", sstr, kind);

   char *User = new char[strlen(sstr)];
   char *Token = 0;

   // kClear
   if (kind == kROOTD_USER) {
      if (!(gReUseAllow & kAUTH_CLR_MSK))
         return 0;              // re-authentication required by administrator
      gSec = 0;
      // Decode subject string
      sscanf(sstr, "%d %d %d %d %s", &gRemPid, &OffSet, &Opt, &Ulen, User);
      User[Ulen] = '\0';
      if ((gReUseRequired = (Opt & kAUTH_REUSE_MSK))) {
         gOffSet = OffSet;
         if (gRemPid > 0 && gOffSet > -1) {
            gAuth =
                RpdCheckAuthTab(gSec, User, gOpenHost, gRemPid, &gOffSet);
         }
         if ((gAuth == 1) && (OffSet != gOffSet))
            gAuth = 2;
         // Fill gUser and free allocated memory
         strcpy(gUser, User);
      }
   }
   // kSRP
   if (kind == kROOTD_SRPUSER) {
      if (!(gReUseAllow & kAUTH_SRP_MSK))
         return 0;              // re-authentication required by administrator
      gSec = 1;
      // Decode subject string
      sscanf(sstr, "%d %d %d %d %s", &gRemPid, &OffSet, &Opt, &Ulen, User);
      User[Ulen] = '\0';
      if ((gReUseRequired = (Opt & kAUTH_REUSE_MSK))) {
         gOffSet = OffSet;
         if (gRemPid > 0 && gOffSet > -1) {
            gAuth =
                RpdCheckAuthTab(gSec, User, gOpenHost, gRemPid, &gOffSet);
         }
         if ((gAuth == 1) && (OffSet != gOffSet))
            gAuth = 2;
         // Fill gUser and free allocated memory
         strcpy(gUser, User);
      }
   }
   // kKrb5
   if (kind == kROOTD_KRB5) {
      if (!(gReUseAllow & kAUTH_KRB_MSK))
         return 0;              // re-authentication required by administrator
      gSec = 2;
      // Decode subject string
      sscanf(sstr, "%d %d %d %d %s", &gRemPid, &OffSet, &Opt, &Ulen, User);
      User[Ulen] = '\0';
      if ((gReUseRequired = (Opt & kAUTH_REUSE_MSK))) {
         gOffSet = OffSet;
         if (gRemPid > 0 && gOffSet > -1) {
            gAuth =
                RpdCheckAuthTab(gSec, User, gOpenHost, gRemPid, &gOffSet);
         }
         if ((gAuth == 1) && (OffSet != gOffSet))
            gAuth = 2;
         // Fill gUser and free allocated memory
         strcpy(gUser, User);
      }
   }
   // kGlobus
   if (kind == kROOTD_GLOBUS) {
      if (!(gReUseAllow & kAUTH_GLB_MSK))
         return 0;              //  re-authentication required by administrator
      gSec = 3;
      // Decode subject string
      int Slen;
      sscanf(sstr, "%d %d %d %d %s", &gRemPid, &OffSet, &Opt, &Slen, User);
      User[Slen] = '\0';
      if ((gReUseRequired = (Opt & kAUTH_REUSE_MSK))) {
         gOffSet = OffSet;
         if (gRemPid > 0 && gOffSet > -1) {
            gAuth =
                RpdCheckAuthTab(gSec, User, gOpenHost, gRemPid, &gOffSet);
         }
         if ((gAuth == 1) && (OffSet != gOffSet))
            gAuth = 2;
      }
   }
   // kSSH
   if (kind == kROOTD_SSH) {
      if (!(gReUseAllow & kAUTH_SSH_MSK))
         return 0;              //  re-authentication required by administrator
      gSec = 4;
      // Decode subject string
      char *Pipe = new char[strlen(sstr)];
      sscanf(sstr, "%d %d %d %s %d %s", &gRemPid, &OffSet, &Opt, Pipe,
             &Ulen, User);
      User[Ulen] = '\0';
      if ((gReUseRequired = (Opt & kAUTH_REUSE_MSK))) {
         gOffSet = OffSet;
         if (gRemPid > 0 && gOffSet > -1) {
            gAuth =
                RpdCheckAuthTab(gSec, User, gOpenHost, gRemPid, &gOffSet);
         }
         if ((gAuth == 1) && (OffSet != gOffSet))
            gAuth = 2;
         // Fill gUser and free allocated memory
         strcpy(gUser, User);
      }
      if (Pipe) delete[] Pipe;
   }

   if (User) delete[] User;
   if (Token) delete[] Token;

   // Return value
   if (gAuth >= 1) {
      return 1;
   } else {
      return 0;
   }
}

//______________________________________________________________________________
int RpdCheckAuthAllow(int Sec, char *Host)
{
   // Check if required auth method is allowed for 'Host'.
   // If 'yes', returns 0, if 'no', returns 1, the number of allowed
   // methods in NumAllow, and the codes of the allowed methods (in order
   // of preference) in AllowMeth. Memory for AllowMeth must be allocated
   // outside. Info read from gSystemDaemonRc.

   int retval = 1, found = 0;

   char theDaemonRc[kMAXPATHLEN] = { 0 };

   // Check if user has a private daemon access file ...
   struct passwd *pw = getpwuid(getuid());
   if (pw != 0) {
      sprintf(theDaemonRc, "%s/%s", pw->pw_dir, kDaemonRc);
   }
   if (pw == 0 || access(theDaemonRc, R_OK)) {
      strcpy(theDaemonRc, gSystemDaemonRc);
   }
   if (gDebug > 2)
      ErrorInfo
          ("RpdCheckAuthAllow: Checking file: %s for meth:%d host:%s (gNumAllow: %d)",
           theDaemonRc, Sec, Host, gNumAllow);

   // Check if info already loaded (not first call ...)
   if (gMethInit == 1) {

      // Look for the method in the allowed list and flag this method
      // as tried, if found ...
      int newtry = 0, i;
      for (i = 0; i < gNumAllow; i++) {
         if (gTriedMeth[i] == 0 && gAllowMeth[i] == Sec) {
            newtry = 1;
            gTriedMeth[i] = 1;
            gNumLeft--;
         }
      }
      if (newtry == 0) {
         ErrorInfo
             ("RpdCheckAuthAllow: new auth method proposed by %s",
              " client not in the list or already attempted");
         return retval;
      }
      retval = 0;

   } else {
      // This is the first call ... check for host specific directives
      gMethInit = 1;

      // First check if file exists and can be read
      if (access(theDaemonRc, R_OK)) {
         ErrorInfo("RpdCheckAuthAllow: can't read file %s (errno: %d)",
                   theDaemonRc, GetErrno());
         return retval;
      }
      // Open file
      FILE *ftab = fopen(theDaemonRc, "r");
      if (ftab == 0) {
         ErrorInfo("RpdCheckAuthAllow: error opening %s (errno: %d)",
                   theDaemonRc, GetErrno());
         return retval;
      }
      // Now read the entry
      char line[kMAXPATHLEN], host[kMAXPATHLEN], rest[kMAXPATHLEN],
          cmth[kMAXPATHLEN];
      int nmet = 0, mth[6] = { 0 };

      int cont = 0, jm = -1;
      while (fgets(line, sizeof(line), ftab)) {
         int rc = 0, i;
         if (line[0] == '#')
            continue;           // skip comment lines
         if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';   // get rid of '\n', if any ...
         // Analyze the line now ...
         int nw = 0;
         char *pstr = line;
         // Check if a continuation line
         if (cont == 1) {
            cont = 0;
            nw = sscanf(pstr, "%s", rest);
            if (nw == 0)
               continue;        // empty line
         } else {
            jm = -1;
            // Get 'host' first ...
            nw = sscanf(pstr, "%s %s", host, rest);
            if (nw < 2)
               continue;        // no method defined for this host
            pstr = line + strlen(host) + 1;

            // Check if a service is specified
            char *pcol = strstr(host, ":");
            if (pcol) {
               if (!strstr(pcol+1, gService))
                  continue;
               else
                  host[(int)(pcol-host)] = '\0';
            }
            if (strlen(host) == 0)
               strcpy(host, "default");

            if (gDebug > 2)
               ErrorInfo("RpdCheckAuthAllow: found host: %s ", host);

            if (strcmp(host, "default")) {
               // now check validity of 'host' format
               if (!RpdCheckHost(Host,host)) {
                  goto next;
               }
            } else {
               // This is a default entry: ignore it if a host-specific entry was already
               // found, analyse it otherwise ...
               if (found == 1)
                  goto next;
            }

            if (rc != 0)
               continue;        // bad or unmatched name

            // Reset mth[kMAXSEC]
            nmet = 0;
            for (i = 0; i < kMAXSEC; i++) {
               mth[i] = -1;
            }

         }

         // We are at the end and there will be a continuation line ...
         if (rest[0] == '\\') {
            cont = 1;
            continue;
         }

         while (pstr != 0) {
            int tmet = -1;
            char *pd = 0, *pd2 = 0;
            cmth[0] = '\0';
            rest[0] = '\0';
            nw = sscanf(pstr, "%s %s", cmth, rest);
            if (!strcmp(cmth, "none")) {
               nmet = 0;
               goto nexti;
            }
            pd = strchr(cmth, ':');
            // Parse the method
            char tmp[20];
            if (pd != 0) {
               int mlen = pd - cmth;
               strncpy(tmp, cmth, mlen);
               tmp[mlen] = '\0';
            } else {
               strcpy(tmp, cmth);
            }

            if (strlen(tmp) > 1) {

               for (tmet = 0; tmet < kMAXSEC; tmet++) {
                  if (!rpdstrcasecmp(kAuthMeth[tmet], tmp))
                     break;
               }
               if (tmet < kMAXSEC) {
                  ErrorInfo("RpdCheckAuthAllow: tmet %d", tmet);
               } else {
                  ErrorInfo("RpdCheckAuthAllow: unknown methods %s - ignore", tmp);
                  goto nexti;
               }

            } else {
               tmet = atoi(tmp);
            }
            jm = -1;
            if (gDebug > 2)
               ErrorInfo("RpdCheckAuthAllow: found method %d (have?:%d)",
                         tmet, gHaveMeth[tmet]);
            if (tmet >= 0 && tmet <= kMAXSEC) {
               if (gHaveMeth[tmet] == 1) {
                  int i;
                  for (i = 0; i < nmet; i++) {
                     if (mth[i] == tmet) {
                        jm = i;
                     }
                  }
               } else
                  goto nexti;
            } else
               goto nexti;
            if (jm == -1) {
               // New method ...
               mth[nmet] = tmet;
               jm = nmet;
               nmet++;
            }
            // Now parse users list, if any ...
            while (pd != 0 && (int) (pd[1]) != 32) {
               pd2 = strchr(pd + 1, ':');
               if (pd[1] == '-') {
                  pd += 2;
                  // Ignore
                  if (gUserIgnore[mth[jm]] == 0) {
                     gUserIgnLen[mth[jm]] = kMAXPATHLEN;
                     gUserIgnore[mth[jm]] = new char[gUserIgnLen[mth[jm]]];
                     gUserIgnore[mth[jm]][0] = '\0';
                  }
                  if (strlen(gUserIgnore[mth[jm]]) >
                      (gUserIgnLen[mth[jm]] - 10)) {
                     char *UItmp = strdup(gUserIgnore[mth[jm]]);
                     free(gUserIgnore[mth[jm]]);
                     gUserIgnLen[mth[jm]] += kMAXPATHLEN;
                     gUserIgnore[mth[jm]] = new char[gUserIgnLen[mth[jm]]];
                     strcpy(gUserIgnore[mth[jm]], UItmp);
                     free(UItmp);
                  }
                  char usr[256];
                  if (pd2 != 0) {
                     int ulen = pd2 - pd;
                     strncpy(usr, pd, ulen);
                     usr[ulen] = '\0';
                  } else {
                     strcpy(usr, pd);
                  }
                  struct passwd *pw = getpwnam(usr);
                  if (pw != 0)
                     sprintf(gUserIgnore[mth[jm]], "%s %d",
                             gUserIgnore[mth[jm]], (int)pw->pw_uid);
               } else {
                  pd += 1;
                  if (pd[1] == '+')
                     pd += 1;
                  // Keep
                  if (gUserAllow[mth[jm]] == 0) {
                     gUserAlwLen[mth[jm]] = kMAXPATHLEN;
                     gUserAllow[mth[jm]] = new char[gUserAlwLen[mth[jm]]];
                     gUserAllow[mth[jm]][0] = '\0';
                  }
                  if (strlen(gUserAllow[mth[jm]]) >
                      (gUserAlwLen[mth[jm]] - 10)) {
                     char *UItmp = strdup(gUserAllow[mth[jm]]);
                     free(gUserAllow[mth[jm]]);
                     gUserAlwLen[mth[jm]] += kMAXPATHLEN;
                     gUserAllow[mth[jm]] = new char[gUserAlwLen[mth[jm]]];
                     strcpy(gUserAllow[mth[jm]], UItmp);
                     free(UItmp);
                  }
                  char usr[256];
                  if (pd2 != 0) {
                     int ulen = pd2 - pd;
                     strncpy(usr, pd, ulen);
                     usr[ulen] = '\0';
                  } else {
                     strcpy(usr, pd);
                  }
                  struct passwd *pw = getpwnam(usr);
                  if (pw != 0)
                     sprintf(gUserAllow[mth[jm]], "%s %d",
                             gUserAllow[mth[jm]], (int)pw->pw_uid);
               }
               pd = pd2;
            }
            // Get next item
          nexti:
            if (nw > 1 && (int) rest[0] != 92) {
               pstr = strstr(pstr, rest);
            } else {
               if ((int) rest[0] == 92)
                  cont = 1;
               pstr = 0;
            }
         }
         if (gDebug > 2) {
            ErrorInfo("RpdCheckAuthAllow: for host %s found %d methods",
                      host, nmet);
            ErrorInfo("RpdCheckAuthAllow: %d %d %d %d %d %d", mth[0],
                      mth[1], mth[2], mth[3], mth[4], mth[5]);
         }
         // Found new entry matching: superseed previous result
         found = 1;
         retval = 1;
         gNumAllow = gNumLeft = nmet;
         for (i = 0; i < kMAXSEC; i++) {
            gAllowMeth[i] = -1;
            gTriedMeth[i] = 0;
            if (i < gNumAllow) {
               gAllowMeth[i] = mth[i];
               if (Sec == mth[i]) {
                  retval = 0;
                  gNumLeft--;
                  gTriedMeth[i] = 1;
               }
            }
         }
       next:
         continue;
      }

      // closing file ...
      fclose(ftab);

      // Use defaults if nothing found
      if (!found) {
         if (gDebug > 2)
            ErrorInfo
            ("RpdCheckAuthAllow: no specific or 'default' entry found: %s",
             "using system defaults");
         int i;
         for (i = 0; i < gNumAllow; i++) {
            if (Sec == gAllowMeth[i]) {
               retval = 0;
               gNumLeft--;
               gTriedMeth[i] = 1;
            }
         }

      }

   }
   if (gDebug > 2) {
      ErrorInfo
          ("RpdCheckAuthAllow: returning: %d (gNumAllow: %d, gNumLeft:%d)",
           retval, gNumAllow, gNumLeft);
      int i, jm;
      for (i = 0; i < kMAXSEC; i++) {
         jm = gAllowMeth[i];
         if (gUserAlwLen[jm] > 0)
            ErrorInfo("RpdCheckAuthAllow: users allowed for method %d: %s",
                      jm, gUserAllow[jm]);
      }
      for (i = 0; i < kMAXSEC; i++) {
         jm = gAllowMeth[i];
         if (gUserIgnLen[jm] > 0)
            ErrorInfo("RpdCheckAuthAllow: users ignored for method %d: %s",
                      jm, gUserIgnore[jm]);
      }
   }

   return retval;
}

//______________________________________________________________________________
int RpdCheckHost(const char *Host, const char *host)
{
   // Checks if 'host' is compatible with 'Host' taking into account
   // wild cards in the host name
   // Returns 1 if successful, 0 otherwise ...

   int rc = 1;

   // Strings must be both defined
   if (!Host || !host)
      return 0;

   // If host is a just wild card accept it
   if (!strcmp(host,"*"))
      return 1;

   // Try now to understand whether it is an address or a name ...
   int name = 0, i = 0;
   for (i = 0; i < (int) strlen(host); i++) {
      if ((host[i] < 48 || host[i] > 57) &&
           host[i] != '*' && host[i] != '.') {
         name = 1;
         break;
      }
   }

   // If ref host is an IP, get IP of Host
   char *H;
   if (!name) {
      H = RpdGetIP(Host);
      if (gDebug > 2)
         ErrorInfo("RpdCheckHost: Checking Host IP: %s", H);
   } else {
      H = new char[strlen(Host)+1];
      strcpy(H,Host);
      if (gDebug > 2)
         ErrorInfo("RpdCheckHost: Checking Host name: %s", H);
   }

   // Check if starts with wild
   // Starting with '.' defines a full (sub)domain
   int sos = 0;
   if (host[0] == '*' || host[0] == '.')
      sos = 1;

   // Check if ends with wild
   // Ending with '.' defines a name
   int eos = 0, le = strlen(host);
   if (host[le-1] == '*' || host[le-1] == '.')
      eos = 1;

   int first= 1;
   int ends= 0;
   int starts= 0;
   char *h = new char[strlen(host)+1];
   strcpy(h,host);
   char *tk = strtok(h,"*");
   while (tk) {

      char *ps = strstr(H,tk);
      if (!ps) {
         rc = 0;
         break;
      }
      if (!sos && first && ps == H)
         starts = 1;
      first = 0;

      if (ps == H + strlen(H) - strlen(tk))
         ends = 1;

      tk = strtok(0,"*");

   }
   if (h) delete[] h;
   if (H) delete[] H;

   if ((!sos || !eos) && !starts && !ends)
      rc = 0;

   return rc;
}

//______________________________________________________________________________
char *RpdGetIP(const char *host)
{
   // Get IP address of 'host' as a string. String must be deleted by
   // the user.

   struct hostent *h;
   unsigned long ip;
   unsigned char ip_fld[4];

   // Check server name
   if ((h = gethostbyname(host)) == 0) {
      ErrorInfo("RpdGetIP: unknown host %s", host);
      return 0;
   }
   // Decode ...
   ip = ntohl(*(unsigned long *) h->h_addr_list[0]);
   ip_fld[0] = (unsigned char) ((0xFF000000 & ip) >> 24);
   ip_fld[1] = (unsigned char) ((0x00FF0000 & ip) >> 16);
   ip_fld[2] = (unsigned char) ((0x0000FF00 & ip) >> 8);
   ip_fld[3] = (unsigned char) ((0x000000FF & ip));

   // Prepare output
   char *output = new char[20];
   sprintf(output, "%d.%d.%d.%d",
           ip_fld[0], ip_fld[1], ip_fld[2], ip_fld[3]);

   // return
   return output;
}

//______________________________________________________________________________
void RpdSendAuthList()
{
   // Send list of authentication methods not yet tried.

   if (gDebug > 2)
      ErrorInfo("RpdSendAuthList: analyzing (gNumLeft: %d)", gNumLeft);

   // Send Number of methods left
   NetSend(gNumLeft, kROOTD_NEGOTIA);

   if (gNumLeft > 0) {
      int i, ldum = gNumLeft * 3;
      char *sdum = new char[ldum];
      sdum[0] = '\0';
      for (i = 0; i < gNumAllow; i++) {
         if (gDebug > 2)
            ErrorInfo("RpdSendAuthList: gTriedMeth[%d]: %d", i,
                      gTriedMeth[i]);
         if (gTriedMeth[i] == 0) {
            sprintf(sdum, "%s %d", sdum, gAllowMeth[i]);
         }
      }
      NetSend(sdum, ldum, kMESS_STRING);
      if (gDebug > 2)
         ErrorInfo("RpdSendAuthList: sent list: %s", sdum);
      if (sdum) delete[] sdum;
   }
}


//______________________________________________________________________________
void RpdSshAuth(const char *sstr)
{
   // Reset global variable.

   gAuth = 0;

   if (gDebug > 2)
      ErrorInfo("RpdSshAuth: contacted by host: %s for user %s", gOpenHost,
                sstr);

   // Decode subject string
   char *User = new char[strlen(sstr)], *Pipe = new char[strlen(sstr)];
   int Ulen, ofs, opt;
   char dumm[20];
   sscanf(sstr, "%d %d %d %s %d %s %s", &gRemPid, &ofs, &opt, Pipe, &Ulen,
          User, dumm);
   User[Ulen] = '\0';
   gReUseRequired = (opt & kAUTH_REUSE_MSK);

   // Check if we have been called to notify failure ...
   if (gRemPid < 0) {
      if (gDebug > 2)
         ErrorInfo
             ("RpdSshAuth: this is a failure notification (%s,%s,%d,%s)",
              User, gOpenHost, gRemPid, Pipe);
      if (SshToolNotifyFailure(Pipe)) {
         ErrorInfo
             ("RpdSshAuth: failure notification perhaps unsuccessful ... ");
      }
      if (User) delete[] User;
      if (Pipe) delete[] Pipe;
      return;
   }
   // Check user existence and get its environment
   struct passwd *pw = getpwnam(User);
   if (!pw) {
      ErrorInfo("RpdSshAuth: entry for user % not found in /etc/passwd",
                User);
      NetSend(-2, kROOTD_SSH);
      if (User) delete[] User;
      if (Pipe) delete[] Pipe;
      return;
   }
   // Method cannot be attempted for anonymous users ... (ie data servers )...
   if (!strcmp(pw->pw_shell, "/bin/false")) {
      ErrorInfo("RpdSshAuth: no SSH for anonymous user '%s' ", User);
      NetSend(-2, kROOTD_SSH);
      if (User) delete[] User;
      if (Pipe) delete[] Pipe;
      return;
   }

   // Now we create an internal (UNIX) socket to listen to the result of sshd from ssh2rpd
   // Path will be /tmp/rootdSSH_<random_string>
   int UnixFd;
   char *UniquePipe = new char[22];
   if ((UnixFd =
        SshToolAllocateSocket(pw->pw_uid, pw->pw_gid, &UniquePipe)) < 0) {
      ErrorInfo
          ("RpdSshAuth: can't allocate UNIX socket for authentication");
      NetSend(0, kROOTD_SSH);
      if (User) delete[] User;
      if (Pipe) delete[] Pipe;
      if (UniquePipe) delete[] UniquePipe;
      return;
   }
   // Communicate command to be executed via ssh ...
   char *CmdInfo = new char[kMAXPATHLEN];
   if (gRootLog == 0 && strlen(gFileLog) > 0) {
      sprintf(CmdInfo, "%s/ssh2rpd %d %s %ld %d %s", gExecDir, gDebug,
              UniquePipe, (long)getpid(), gRemPid, gFileLog);
   } else {
      sprintf(CmdInfo, "%s/ssh2rpd %d %s %ld %d", gExecDir, gDebug,
              UniquePipe, (long)getpid(), gRemPid);
   }
   if (gSshdPort != 22) {
      sprintf(CmdInfo, "%s port:%d", CmdInfo, gSshdPort);
   }

   if (gDebug > 2)
      ErrorInfo("RpdSshAuth: sending CmdInfo (%d) %s", strlen(CmdInfo),
                CmdInfo);
   NetSend(strlen(CmdInfo), kROOTD_SSH);
   NetSend(CmdInfo, strlen(CmdInfo), kROOTD_SSH);

   // Wait for verdict form sshd (via ssh2rpd ...)
   gAuth = SshToolGetAuth(UnixFd);

   // Close socket
   SshToolDiscardSocket(UniquePipe, UnixFd);

   // If failure, notify and return ...
   if (gAuth == 0) {
      NetSend(kErrAuthNotOK, kROOTD_ERR);  // Send message length first
      if (User) delete[] User;
      if (Pipe) delete[] Pipe;
      if (UniquePipe) delete[] UniquePipe;
      return;
   }
   // notify the client
   if (gDebug > 0 && gAuth == 1)
      ErrorInfo("RpdSshAuth: user %s authenticated by sshd", User);

   // Save username ...
   strcpy(gUser, User);

   char line[kMAXPATHLEN];
   if ((gReUseAllow & kAUTH_SSH_MSK) && gReUseRequired) {

      // Ask for the RSA key
      NetSend(1, kROOTD_RSAKEY);

      // Receive the key securely
      if (RpdRecvClientRSAKey()) {
         ErrorInfo
             ("RpdSshAuth: could not import a valid key - switch off reuse for this session");
         gReUseRequired = 0;
      }

      // Set an entry in the auth tab file for later (re)use, if required ...
      int OffSet = -1;
      char *token = 0;
      if (gReUseRequired) {
         sprintf(line, "%d %d %d %ld %d %s %s", 4, 1, gRSAKey, (long)getppid(),
                 gRemPid, gOpenHost, gUser);
         OffSet = RpdUpdateAuthTab(1, line, &token);
      }
      // Comunicate login user name to client
      sprintf(line, "%s %d", gUser, OffSet);
      NetSend(strlen(line), kROOTD_SSH);   // Send message length first
      NetSend(line, kMESS_STRING);

      if (gReUseRequired) {
         // Send over the token
         if (RpdSecureSend(token) == -1) {
            ErrorInfo
                ("RpdSshAuth: problems secure-sending token - may result in corrupted token");
         }
         if (token) delete[] token;

         // Save RSA public key into file for later use by other rootd/proofd
         RpdSavePubKey(gPubKey, OffSet, gUser);
      }
   } else {
      // Comunicate login user name to client
      sprintf(line, "%s -1", gUser);
      NetSend(strlen(line), kROOTD_SSH);   // Send message length first
      NetSend(line, kMESS_STRING);
   }

   // Release allocated memory
   if (User)       delete[] User;
   if (Pipe)       delete[] Pipe;
   if (UniquePipe) delete[] UniquePipe;
   if (CmdInfo)    delete[] CmdInfo;

   return;
}

//______________________________________________________________________________
void RpdKrb5Auth(const char *sstr)
{
   // Authenticate via Kerberos.

   gAuth = 0;

#ifdef R__KRB5
   NetSend(1, kROOTD_KRB5);
   // TAuthenticate will respond to our encouragement by sending krb5
   // authentication through the socket

   int retval;

   if (gDebug > 2)
      ErrorInfo("RpdKrb5Auth: analyzing ... %s", sstr);

   if (gClientProtocol > 8) {
      int Ulen, ofs, opt;
      char dumm[256];
      // Decode subject string
      sscanf(sstr, "%d %d %d %d %s", &gRemPid, &ofs, &opt, &Ulen, dumm);
      gReUseRequired = (opt & kAUTH_REUSE_MSK);
   }

   // get service principal
   const char *service = gService;

   if (gDebug > 2)
      ErrorInfo("RpdKrb5Auth: gService: %s, Port: %d ",gService,gPort);

   if ((strcmp(gService, "rootd") == 0  && gPort == 1094) ||
       (strcmp(gService, "proofd") == 0 && gPort == 1093)) {
      // keep the real service name
   } else {
      // default to host
      service = "host";
   }

   if (gDebug > 2)
      ErrorInfo("RpdKrb5Auth: using service: %s ",service);

   krb5_principal server;
   if ((retval = krb5_sname_to_principal(gKcontext, 0, service,
                                         KRB5_NT_SRV_HST, &server))) {
      ErrorInfo("RpdKrb5Auth: while generating service name (%s): %d %s",
                service, retval, error_message(retval));
      return;
   }

   // listen for authentication from the client
   krb5_auth_context auth_context = 0;
   krb5_ticket *ticket;
   char proto_version[100] = "krootd_v_1";
   int sock = gSockFd;

   if (gDebug > 2)
      ErrorInfo("RpdKrb5Auth: recvauth ... ");

   if ((retval = krb5_recvauth(gKcontext, &auth_context,
                               (krb5_pointer) &sock, proto_version, server,
                               0, gKeytab,   // default gKeytab is 0
                               &ticket))) {
      ErrorInfo("RpdKrb5Auth: recvauth failed--%s", error_message(retval));
      return;
   }

   // get client name
   char *cname;
   if ((retval =
        krb5_unparse_name(gKcontext, ticket->enc_part2->client, &cname))) {
      ErrorInfo("RpdKrb5Auth: unparse failed: %s", error_message(retval));
      return;
   }

   using std::string;
   string user = cname;
   free(cname);
   string reply = "authenticated as ";
   reply += user;

   // set user name
   user = user.erase(user.find("@"));   // cut off realm
   string::size_type pos = user.find("/");   // see if there is an instance
   if (pos != string::npos)
      user = user.erase(pos);   // drop the instance
   strncpy(gUser, user.c_str(), 64);

   string targetUser = gUser;

   if (gClientProtocol >= 9) {

       // Receive target name

      if (gDebug > 2)
         ErrorInfo("RpdKrb5Auth: receiving target user ... ");

       EMessageTypes kind;
       char buffer[66];
       NetRecv(buffer, 65, kind);

       if (kind != kROOTD_KRB5) {
          ErrorInfo("RpdKrb5Auth: protocol error, received message of type %d instead of %d\n",
                    kind,kROOTD_KRB5);
       }
       buffer[65] = 0;
       targetUser = buffer;

      if (gDebug > 2)
         ErrorInfo("RpdKrb5Auth: received target user %s ",buffer);
   }

   // const char* targetUser = gUser; // "cafuser";
   if (krb5_kuserok(gKcontext, ticket->enc_part2->client, targetUser.c_str())) {
      strncpy(gUser, targetUser.c_str(), 64);
      reply =  "authenticated as ";
      reply += gUser;
   } else {
      ErrorInfo
        ("RpdKrb5Auth: could not change user from %s to %s",gUser,targetUser.c_str());
   }

   if (gClientProtocol >= 9) {

      char *data = 0;
      int size = 0;
      if (gDebug > 2)
         ErrorInfo("RpdKrb5Auth: receiving forward cred ... ");

      {
         EMessageTypes kind;
         char BufLen[20];
         NetRecv(BufLen, 20, kind);

         if (kind != kROOTD_KRB5) {
            ErrorInfo("RpdKrb5Auth: protocol error, received message of type %d instead of %d\n",
                      kind, kROOTD_KRB5);
         }

         size = atoi(BufLen);
         if (gDebug > 3)
            ErrorInfo("RpdKrb5Auth: got len '%s' %d ", BufLen, size);

         data = new char[size+1];

         // Receive and decode encoded public key
         int Nrec = NetRecvRaw(data, size);

         if (gDebug > 3)
            ErrorInfo("RpdKrb5Auth: received %d ", Nrec);
      }

      krb5_data forwardCreds;
      forwardCreds.data = data;
      forwardCreds.length = size;

      if (gDebug > 2)
         ErrorInfo("RpdKrb5Auth: received forward cred ... %d %d %d",
                   data[0], data[1], data[2]);

      int net = sock;
      retval = krb5_auth_con_genaddrs(gKcontext, auth_context,
                                      net, KRB5_AUTH_CONTEXT_GENERATE_REMOTE_FULL_ADDR);
      if (retval) {
         ErrorInfo("RpdKrb5Auth: failed auth_con_genaddrs is: %s\n",
                   error_message(retval));
      }

      krb5_creds **creds = 0;
      if ((retval = krb5_rd_cred(gKcontext, auth_context, &forwardCreds, &creds, 0))) {
         ErrorInfo("RpdKrb5Auth: rd_cred failed--%s", error_message(retval));
         return;
      }

      struct passwd *pw = getpwnam(gUser);
      if (pw) {
         Int_t fromUid = getuid();

         if (setresuid(pw->pw_uid, pw->pw_uid, fromUid) == -1) {
            ErrorInfo("RpdKrb5Auth: can't setuid for user %s", gUser);
            return;
         }

         if (gDebug>5)
            ErrorInfo("RpdKrb5Auth: saving ticket to cache ...");

         krb5_context context = gKcontext;

         krb5_ccache cache = 0;
         if ((retval = krb5_cc_default(context, &cache))) {
            ErrorInfo("RpdKrb5Auth: cc_default failed--%s", error_message(retval));
            return;
         }

         if (gDebug>5)
            ErrorInfo("RpdKrb5Auth: working (1) on ticket to cache (%s) ... ",
                      krb5_cc_get_name(context,cache));

         // this is not working (why?)
         // this would mean that if a second user comes in, it will tremple
         // the existing one :(
         //       if ((retval = krb5_cc_gen_new(context,&cache))) {
         //          ErrorInfo("RpdKrb5Auth: cc_gen_new failed--%s", error_message(retval));
         //          return;
         //       }

         const char *cacheName = krb5_cc_get_name(context,cache);

         if (gDebug>5)
            ErrorInfo("RpdKrb5Auth: working (2) on ticket to cache (%s) ... ",cacheName);

         if ((retval = krb5_cc_initialize(context,cache,
                                          ticket->enc_part2->client))) {
            ErrorInfo("RpdKrb5Auth: cc_initialize failed--%s", error_message(retval));
            return;
         }

         if ((retval = krb5_cc_store_cred(context,cache, *creds))) {
            ErrorInfo("RpdKrb5Auth: cc_store_cred failed--%s", error_message(retval));
            return;
         }

         if ((retval = krb5_cc_close(context,cache))) {
            ErrorInfo("RpdKrb5Auth: cc_close failed--%s", error_message(retval));
            return;
         }

         //       if ( chown( cacheName, pw->pw_uid, pw->pw_gid) != 0 ) {
         //          ErrorInfo("RpdKrb5Auth: could not change the owner ship of the cache file %s",cacheName);
         //       }

         if (gDebug>5)
            ErrorInfo("RpdKrb5Auth: done ticket to cache (%s) ... ",cacheName);

         if (setresuid(fromUid,fromUid, fromUid) == -1) {
            ErrorInfo("RpdKrb5Auth: can't setuid back to original uid");
            return;
         }
      }
   }

   NetSend(reply.c_str(), kMESS_STRING);
   krb5_auth_con_free(gKcontext, auth_context);

   // Authentication was successfull
   gAuth = 1;

   if (gClientProtocol > 8) {

      char line[kMAXPATHLEN];
      if ((gReUseAllow & kAUTH_KRB_MSK) && gReUseRequired) {

         // Ask for the RSA key
         NetSend(1, kROOTD_RSAKEY);

         // Receive the key securely
         if (RpdRecvClientRSAKey()) {
            ErrorInfo
                ("RpdKrb5Auth: could not import a valid key - switch off reuse for this session");
            gReUseRequired = 0;
         }

         // Set an entry in the auth tab file for later (re)use, if required ...
         int OffSet = -1;
         char *token = 0;
         if (gReUseRequired) {
            sprintf(line, "%d %d %d %ld %d %s %s", 2, 1, gRSAKey, (long)getppid(),
                    gRemPid, gOpenHost, gUser);
            OffSet = RpdUpdateAuthTab(1, line, &token);
            if (gDebug > 2)
               ErrorInfo("RpdKrb5Auth: line:%s OffSet:%d", line, OffSet);
         }
         // Comunicate login user name to client
         sprintf(line, "%s %d", gUser, OffSet);
         NetSend(strlen(line), kROOTD_KRB5);   // Send message length first
         NetSend(line, kMESS_STRING);

         // Send Token
         if (gReUseRequired) {
            if (RpdSecureSend(token) == -1) {
               ErrorInfo
                   ("RpdKrb5Auth: problems secure-sending token - may result in corrupted token");
            }
            if (token) delete[] token;

            // Save RSA public key into file for later use by other rootd/proofd
            RpdSavePubKey(gPubKey, OffSet, gUser);
         }

      } else {

         // Comunicate login user name to client
         sprintf(line, "%s -1", gUser);
         NetSend(strlen(line), kROOTD_KRB5);   // Send message length first
         NetSend(line, kMESS_STRING);

      }
   } else {
      NetSend(user.c_str(), kMESS_STRING);
   }

   if (gDebug > 0)
      ErrorInfo("RpdKrb5Auth: user %s authenticated", gUser);

#else

   // no krb5 support
   if (sstr) { }   // remove compiler warning

   NetSend(0, kROOTD_KRB5);

#endif
}

//______________________________________________________________________________
void RpdSRPUser(const char *sstr)
{
   // Use Secure Remote Password protocol.
   // Check user id in $HOME/.srootdpass file.

   gAuth = 0;

   if (!*sstr) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: bad user name");
      return;
   }

   if (kSRootdPass[0]) {
   }                            // remove compiler warning

#ifdef R__SRP

   char srootdpass[kMAXPATHLEN], srootdconf[kMAXPATHLEN];

   // Decode subject string
   char *user = new char[strlen(sstr) + 1];
   if (gClientProtocol > 8) {
      int Ulen, ofs, opt;
      char dumm[20];
      sscanf(sstr, "%d %d %d %d %s %s", &gRemPid, &ofs, &opt, &Ulen, user,
             dumm);
      user[Ulen] = '\0';
      gReUseRequired = (opt & kAUTH_REUSE_MSK);
   } else {
      strcpy(user, sstr);
   }

   struct passwd *pw = getpwnam(user);
   if (!pw) {
      NetSend(kErrNoUser, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: user %s unknown", user);
      return;
   }
   // Method cannot be attempted for anonymous users ... (ie data servers )...
   if (!strcmp(pw->pw_shell, "/bin/false")) {
      NetSend(kErrNotAllowed, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: no SRP for anonymous user '%s' ", user);
      return;
   }
   // If server is not started as root and user is not same as the
   // one who started rootd then authetication is not ok.
   uid_t uid = getuid();
   if (uid && uid != pw->pw_uid) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: user not same as effective user of rootd");
      return;
   }

   NetSend(gAuth, kROOTD_AUTH);

   strcpy(gUser, user);

   if (user) delete[] user;

   if (!gAltSRP) {
      sprintf(srootdpass, "%s/%s", pw->pw_dir, kSRootdPass);
      sprintf(srootdconf, "%s/%s.conf", pw->pw_dir, kSRootdPass);
   } else {
      sprintf(srootdpass, "%s", gAltSRPPass);
      sprintf(srootdconf, "%s.conf", gAltSRPPass);
   }

   FILE *fp1 = fopen(srootdpass, "r");
   if (!fp1) {
      NetSend(kErrFileOpen, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: error opening %s", srootdpass);
      return;
   }
   FILE *fp2 = fopen(srootdconf, "r");
   if (!fp2) {
      NetSend(kErrFileOpen, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: error opening %s", srootdconf);
      if (fp1)
         fclose(fp1);
      return;
   }

   struct t_pw *tpw = t_openpw(fp1);
   if (!tpw) {
      NetSend(kErrFileOpen, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: unable to open password file %s", srootdpass);
      fclose(fp1);
      fclose(fp2);
      return;
   }

   struct t_conf *tcnf = t_openconf(fp2);
   if (!tcnf) {
      NetSend(kErrFileOpen, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: unable to open configuration file %s",
                srootdconf);
      t_closepw(tpw);
      fclose(fp1);
      fclose(fp2);
      return;
   }
#if R__SRP_1_1
   struct t_server *ts = t_serveropen(gUser, tpw, tcnf);
#else
   struct t_server *ts = t_serveropenfromfiles(gUser, tpw, tcnf);
#endif
   if (!ts) {
      NetSend(kErrNoUser, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: user %s not found SRP password file", gUser);
      return;
   }

   if (tcnf)
      t_closeconf(tcnf);
   if (tpw)
      t_closepw(tpw);
   if (fp2)
      fclose(fp2);
   if (fp1)
      fclose(fp1);

   char hexbuf[MAXHEXPARAMLEN];

   // send n to client
   NetSend(t_tob64(hexbuf, (char *) ts->n.data, ts->n.len), kROOTD_SRPN);
   // send g to client
   NetSend(t_tob64(hexbuf, (char *) ts->g.data, ts->g.len), kROOTD_SRPG);
   // send salt to client
   NetSend(t_tob64(hexbuf, (char *) ts->s.data, ts->s.len),
           kROOTD_SRPSALT);

   struct t_num *B = t_servergenexp(ts);

   // receive A from client
   EMessageTypes kind;
   if (NetRecv(hexbuf, MAXHEXPARAMLEN, kind) < 0) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: error receiving A from client");
      return;
   }
   if (kind != kROOTD_SRPA) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: expected kROOTD_SRPA message");
      return;
   }

   unsigned char buf[MAXPARAMLEN];
   struct t_num A;
   A.data = buf;
   A.len = t_fromb64((char *) A.data, hexbuf);

   // send B to client
   NetSend(t_tob64(hexbuf, (char *) B->data, B->len), kROOTD_SRPB);

   t_servergetkey(ts, &A);

   // receive response from client
   if (NetRecv(hexbuf, MAXHEXPARAMLEN, kind) < 0) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: error receiving response from client");
      return;
   }
   if (kind != kROOTD_SRPRESPONSE) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdSRPUser: expected kROOTD_SRPRESPONSE message");
      return;
   }

   unsigned char cbuf[20];
   t_fromhex((char *) cbuf, hexbuf);

   if (!t_serververify(ts, cbuf)) {

      // authentication successful
      if (gDebug > 0)
         ErrorInfo("RpdSRPUser: user %s authenticated", gUser);
      gAuth = 1;

      if (gClientProtocol > 8) {

         char line[kMAXPATHLEN];
         if ((gReUseAllow & kAUTH_SRP_MSK) && gReUseRequired) {

            // Ask for the RSA key
            NetSend(1, kROOTD_RSAKEY);

            // Receive the key securely
            if (RpdRecvClientRSAKey()) {
               ErrorInfo
                   ("RpdSRPAuth: could not import a valid key - switch off reuse for this session");
               gReUseRequired = 0;
            }

            // Set an entry in the auth tab file for later (re)use, if required ...
            int OffSet = -1;
            char *token = 0;
            if (gReUseRequired) {
               sprintf(line, "%d %d %d %d %d %s %s", 1, 1, gRSAKey,
                       getppid(), gRemPid, gOpenHost, gUser);
               OffSet = RpdUpdateAuthTab(1, line, &token);
            }
            // Comunicate login user name to client
            sprintf(line, "%s %d", gUser, OffSet);
            NetSend(strlen(line), kROOTD_SRPUSER);   // Send message length first
            NetSend(line, kMESS_STRING);

            if (gReUseRequired) {
               // Send Token
               if (RpdSecureSend(token) == -1) {
                  ErrorInfo
                      ("RpdSRPUser: problems secure-sending token - may result in corrupted token");
               }
               if (token) delete[] token;

               // Save RSA public key into file for later use by other rootd/proofd
               RpdSavePubKey(gPubKey, OffSet, gUser);
            }

         } else {
            // Comunicate login user name to client
            sprintf(line, "%s -1", gUser);
            NetSend(strlen(line), kROOTD_SRPUSER);   // Send message length first
            NetSend(line, kMESS_STRING);
         }

      }

   } else {
      if (gClientProtocol > 8) {
         NetSend(kErrBadPasswd, kROOTD_ERR);
         ErrorInfo("RpdSRPUser: authentication failed for user %s", gUser);
         return;
      }
   }

   t_serverclose(ts);

#else
   NetSend(0, kROOTD_SRPUSER);
#endif
}

//______________________________________________________________________________
int RpdCheckSpecialPass(const char *passwd)
{
   // Check user's password against password in $HOME/.rootdpass. If matches
   // skip other authentication mechanism. Returns 1 in case of success
   // authentication, 0 otherwise.

   char rootdpass[kMAXPATHLEN];

   struct passwd *pw = getpwnam(gUser);

   sprintf(rootdpass, "%s/%s", pw->pw_dir, kRootdPass);

   int fid = open(rootdpass, O_RDONLY);
   if (fid == -1)
      return 0;

   int n;
   if ((n = read(fid, rootdpass, sizeof(rootdpass) - 1)) <= 0) {
      close(fid);
      return 0;
   }
   close(fid);

   rootdpass[n] = 0;
   char *s = strchr(rootdpass, '\n');
   if (s)
      *s = 0;

   n = strlen(rootdpass);

   if (strncmp(passwd, rootdpass, n + 1) != 0)
      return 0;

   if (gDebug > 0)
      ErrorInfo
          ("RpdCheckSpecialPass: user %s authenticated via ~/.rootdpass",
           gUser);

   return 1;
}

//______________________________________________________________________________
void RpdPass(const char *pass)
{
   // Check user's password.

   char passwd[64];
   char *passw;
   char *pass_crypt;
   struct passwd *pw;
#ifdef R__SHADOWPW
   struct spwd *spw;
#endif
#ifdef R__AFS
   char *reason;
   int afs_auth = 0;
#endif

   if (gDebug > 2)
      ErrorInfo("RpdPass: Enter");

   gAuth = 0;
   if (!*gUser) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdPass: user needs to be specified first");
      return;
   }

   int n = strlen(pass);
   // Passwd length should be in the correct range ...
   if (!n) {
      NetSend(kErrBadPasswd, kROOTD_ERR);
      ErrorInfo("RpdPass: null passwd not allowed");
      return;
   }
   if (n > (int) sizeof(passwd)) {
      NetSend(kErrBadPasswd, kROOTD_ERR);
      ErrorInfo("RpdPass: passwd too long");
      return;
   }
   // Inversion is done in RpdUser, if needed
   strcpy(passwd, pass);

   // Special treatment for anonimous ...
   if (gAnon) {
      strcpy(gPasswd, passwd);
      goto authok;
   }
   // ... and SpecialPass ...
   if (RpdCheckSpecialPass(passwd)) {
      goto authok;
   }
   // Get local passwd info for gUser
   pw = getpwnam(gUser);

#ifdef R__AFS
   afs_auth = !ka_UserAuthenticateGeneral(KA_USERAUTH_VERSION + KA_USERAUTH_DOSETPAG,
                                          gUser,        //user name
                                          (char *) 0,   //instance
                                          (char *) 0,   //realm
                                          passwd,       //password
                                          0,            //default lifetime
                                          0, 0,         //two spares
                                          &reason);     //error string

   if (!afs_auth) {
      ErrorInfo("RpdPass: AFS login failed for user %s: %s", gUser,
                reason);
      // try conventional login...
#endif

#ifdef R__SHADOWPW
      // System V Rel 4 style shadow passwords
      if ((spw = getspnam(gUser)) == 0) {
         ErrorInfo("RpdPass: Shadow passwd not available for user %s",
                   gUser);
         passw = pw->pw_passwd;
      } else
         passw = spw->sp_pwdp;
#else
      passw = pw->pw_passwd;
#endif
      //   if (gClientProtocol <= 8 || !gReUseRequired) {
      if (gClientProtocol <= 8 || !gCryptRequired) {
         pass_crypt = crypt(passwd, passw);   // Comment this
      } else {
         pass_crypt = passwd;
      }
      n = strlen(passw);

      if (strncmp(pass_crypt, passw, n + 1) != 0) {
         NetSend(kErrBadPasswd, kROOTD_ERR);
         ErrorInfo("RpdPass: invalid password for user %s", gUser);
         return;
      }
#ifdef R__AFS
   }                            // afs_auth
#endif

 authok:
   gAuth = 1;

   if (gClientProtocol > 8) {
      // Set an entry in the auth tab file for later (re)use, if required ...
      int OffSet = -1;
      char *token = 0;
      char line[kMAXPATHLEN];
      if ((gReUseAllow & kAUTH_CLR_MSK) && gReUseRequired) {

         sprintf(line, "%d %d %d %ld %d %s %s", 0, 1, gRSAKey, (long)getppid(),
                 gRemPid, gOpenHost, gUser);
         OffSet = RpdUpdateAuthTab(1, line, &token);
         if (gDebug > 2)
            ErrorInfo("RpdPass: got offset %d", OffSet);

         // Comunicate login user name to client
         sprintf(line, "%s %d", gUser, OffSet);
         if (gDebug > 2)
            ErrorInfo("RpdPass: sending back line %s", line);
         NetSend(strlen(line), kROOTD_PASS);   // Send message length first
         NetSend(line, kMESS_STRING);

         if (gDebug > 2)
            ErrorInfo("RpdPass: sending token %s (Crypt: %d)", token,
                      gCryptRequired);
         if (gCryptRequired) {
            // Send over the token
            if (RpdSecureSend(token) == -1) {
               ErrorInfo
                   ("RpdPass: problems secure-sending token - may result in corrupted token");
            }
         } else {
            // Send token inverted
            for (int i = 0; i < (int) strlen(token); i++) {
               token[i] = ~token[i];
            }
            NetSend(token, kMESS_STRING);
         }
         if (token) delete[] token;

      } else {
         // Comunicate login user name to client
         sprintf(line, "%s -1", gUser);
         if (gDebug > 2)
            ErrorInfo("RpdPass: sending back line %s", line);
         NetSend(strlen(line), kROOTD_PASS);   // Send message length first
         NetSend(line, kMESS_STRING);
      }

      if (gCryptRequired) {
         // Save RSA public key into file for later use by other rootd/proofd
         RpdSavePubKey(gPubKey, OffSet, gUser);
      }
   }
}

//______________________________________________________________________________
void RpdGlobusAuth(const char *sstr)
{
   // Authenticate via Globus.

   gAuth = 0;

#ifndef R__GLBS

   if (sstr) { }  // use sstr
   NetSend(0, kROOTD_GLOBUS);
   return;

#else

   OM_uint32 MajStat = 0;
   OM_uint32 MinStat = 0;
   OM_uint32 GssRetFlags = 0;
   gss_ctx_id_t GlbContextHandle = GSS_C_NO_CONTEXT;
   gss_cred_id_t GlbCredHandle = GSS_C_NO_CREDENTIAL;
   gss_cred_id_t GlbDelCredHandle = GSS_C_NO_CREDENTIAL;
   int GlbTokenStatus = 0;
   char *GlbClientName;
   FILE *FILE_SockFd;
   char *gridmap_default = "/etc/grid-security/grid-mapfile";
   EMessageTypes kind;
   int lSubj, OffSet = -1;
   char *user = 0;
   int ulen = 0;

   if (gDebug > 2)
      ErrorInfo("RpdGlobusAuth: contacted by host: %s", gOpenHost);

   // Tell the remote client that we may accept Globus credentials ...
   NetSend(1, kROOTD_GLOBUS);

   // Decode subject string
   char *Subj = new char[strlen(sstr) + 1];
   int opt;
   char dumm[20];
   sscanf(sstr, "%d %d %d %d %s %s", &gRemPid, &OffSet, &opt, &lSubj, Subj,
          dumm);
   Subj[lSubj] = '\0';
   gReUseRequired = (opt & kAUTH_REUSE_MSK);
   if (gDebug > 2)
      ErrorInfo("RpdGlobusAuth: gRemPid: %d, Subj: %s (%d %d)", gRemPid,
                Subj, lSubj, strlen(Subj));
   if (Subj) delete[] Subj;   // GlbClientName will be determined from the security context ...

   // Now wait for client to communicate the issuer name of the certificate ...
   char *answer = new char[20];
   NetRecv(answer, (int) sizeof(answer), kind);
   if (kind != kMESS_STRING) {
      Error(gErr, kErrAuthNotOK,
            "RpdGlobusAuth: client_issuer_name:received unexpected type of message (%d)",
            kind);
      return;
   }
   int client_issuer_name_len = atoi(answer);
   if (answer) delete[] answer;
   char *client_issuer_name = new char[client_issuer_name_len + 1];
   NetRecv(client_issuer_name, client_issuer_name_len, kind);
   if (kind != kMESS_STRING) {
      Error(gErr, kErrAuthNotOK,
            "RpdGlobusAuth: client_issuer_name:received unexpected type of message (%d)",
            kind);
      return;
   }
   if (gDebug > 2)
      ErrorInfo("RpdGlobusAuth: client issuer name is: %s",
                client_issuer_name);

   // Now we open the certificates and we check if we are able to
   // autheticate the client
   // In the affirmative case we sen our subject name to the client ...
   // NB: if we don't have su privileges we cannot make use of the
   // local host certificates, so we have to rely on the user proxies
   char *subject_name;
   int CertRc = 0;
   if (getuid() == 0)
     CertRc = GlbsToolCheckCert(client_issuer_name, &subject_name);
   else
     CertRc = GlbsToolCheckProxy(client_issuer_name, &subject_name);

   if (CertRc) {
      ErrorInfo("RpdGlobusAuth: %s (%s)",
                "host does not seem to have certificate for the requested CA",
                 client_issuer_name);
      NetSend(0, kROOTD_GLOBUS);   // Notify that we did not find it
      return;
   } else {
      int sjlen = strlen(subject_name) + 1;
      //      subject_name[sjlen] = '\0';

      int bsnd = NetSend(sjlen, kROOTD_GLOBUS);
      if (gDebug > 2)
         ErrorInfo("RpdGlobusAuth: sent: %d (due >=%d))", bsnd,
                   2 * sizeof(sjlen));

      bsnd = NetSend(subject_name, sjlen, kMESS_STRING);
      if (gDebug > 2)
         ErrorInfo("RpdGlobusAuth: sent: %d (due >=%d))", bsnd, sjlen);

      free(subject_name);
   }
   // not needed anymore ...
   if (client_issuer_name) delete[] client_issuer_name;

   // Inquire Globus credentials:
   // This is looking to file X509_USER_CERT for valid a X509 cert (default
   // /etc/grid-security/hostcert.pem) and to dir X509_CERT_DIR for trusted CAs
   // (default /etc/grid-security/certificates).
   if ((MajStat =
        globus_gss_assist_acquire_cred(&MinStat, GSS_C_ACCEPT,
                                       &GlbCredHandle)) !=
       GSS_S_COMPLETE) {
      GlbsToolError("RpdGlobusAuth: gss_assist_acquire_cred", MajStat,
                    MinStat, 0);
      if (getuid() > 0) {
         ErrorInfo
             ("RpdGlobusAuth: non-root: make sure you have initialized (manually) your proxies");
      }
      return;
   }
   // We need to associate a FILE* stream with the socket
   // It will automatically closed when the socket will be closed ...
   FILE_SockFd = fdopen(gSockFd, "w+");

   // Now we are ready to start negotiating with the Client
   if ((MajStat =
        globus_gss_assist_accept_sec_context(&MinStat, &GlbContextHandle,
                                             GlbCredHandle, &GlbClientName,
                                             &GssRetFlags, 0,
                                             &GlbTokenStatus,
                                             &GlbDelCredHandle,
                                             globus_gss_assist_token_get_fd,
                                             (void *) FILE_SockFd,
                                             globus_gss_assist_token_send_fd,
                                             (void *) FILE_SockFd)) !=
       GSS_S_COMPLETE) {
      GlbsToolError("RpdGlobusAuth: gss_assist_accept_sec_context",
                    MajStat, MinStat, GlbTokenStatus);
      return;
   } else {
      gAuth = 1;
      if (gDebug > 0)
         ErrorInfo("RpdGlobusAuth: user: %s \n authenticated",
                   GlbClientName);
   }

   // If we are master we need to autheticate the slaves ...
   if (gGlobus == 1) {          // There might be the need of credentials ...
      // Check that we got delegation to autheticate the slaves
      if (GssRetFlags | GSS_C_DELEG_FLAG) {
         if (gDebug > 2)
            ErrorInfo("RpdGlobusAuth: Pointer to del cred is 0x%x",
                      (int) GlbDelCredHandle);
      } else {
         Error(gErr, kErrAuthNotOK,
               "RpdGlobusAuth: did not get delegated credentials (RetFlags: 0x%x)",
               GssRetFlags);
         return;
      }
      // Now we have to export these delegated credentials to a shared memory segment
      // for later use in 'proofserv' ...
      //   credential= (gss_buffer_t)malloc(sizeof(gss_buffer_desc));
      gss_buffer_t credential = new gss_buffer_desc;
      if ((MajStat =
           gss_export_cred(&MinStat, GlbDelCredHandle, 0, 0,
                           credential)) != GSS_S_COMPLETE) {
         GlbsToolError("RpdGlobusAuth: gss_export_cred", MajStat, MinStat,
                       0);
         return;
      } else if (gDebug > 2)
         ErrorInfo("RpdGlobusAuth: credentials prepared for export");

      // Now store it in shm for later use in proofserv ...
      int rc;
      if ((rc = GlbsToolStoreToShm(credential, &gShmIdCred))) {
         ErrorInfo
             ("RpdGlobusAuth: credentials not correctly stored in shm (rc: %d)",
              rc);
      }
      if (gDebug > 2)
         ErrorInfo
             ("RpdGlobusAuth: credentials stored in shared memory segment %d",
              gShmIdCred);

      delete credential;
   } else {
      if (gDebug > 2)
          ErrorInfo("RpdGlobusAuth: no need for delegated credentials (gGlobus: %d)",
                     gGlobus);
   }

   // For Now we set the gUser to the certificate owner using the gridmap file ...
   // Should be understood if this is really necessary ...
   if (getenv("GRIDMAP") == 0) {
      // The installation did not specify a special location for the gridmap file
      // We assume the usual default ...
      setenv("GRIDMAP", gridmap_default, 1);
      if (gDebug > 2)
         ErrorInfo("RpdGlobusAuth: gridmap: using default file (%s)",
                   gridmap_default);
   } else if (gDebug > 2)
      ErrorInfo("RpdGlobusAuth: gridmap: using file %s",
                getenv("GRIDMAP"));

   // Get local login name for the subject ...
   if (globus_gss_assist_gridmap(GlbClientName, &user)) {
      if (gDebug > 2)
         ErrorInfo
             ("RpdGlobusAuth: unable to get local username from gridmap: using: %s",
              gAnonUser);
      user = strdup(gAnonUser);
      if (gDebug > 2)
         ErrorInfo("RpdGlobusAuth: user ", user);
   }
   if (!strcmp(user, "anonymous"))
      user = strdup(gAnonUser);
   if (!strcmp(user, gAnonUser))
      gAnon = 1;

   // Fill gUser and free allocated memory
   ulen = strlen(user);
   strncpy(gUser, user, ulen + 1);

   char line[kMAXPATHLEN];
   if ((gReUseAllow & kAUTH_GLB_MSK) && gReUseRequired) {

      // Ask for the RSA key
      NetSend(1, kROOTD_RSAKEY);

      // Receive the key securely
      if (RpdRecvClientRSAKey()) {
         ErrorInfo
             ("RpdGlobusAuth: could not import a valid key - switch off reuse for this session");
         gReUseRequired = 0;
      }

      // Store security context and related info for later use ...
      OffSet = -1;
      char *token = 0;
      if (gReUseRequired) {
         int ShmId = GlbsToolStoreContext(GlbContextHandle, user);
         if (ShmId > 0) {
            sprintf(line, "%d %d %d %d %d %s %s %d %s", 3, 1, gRSAKey,
                    getppid(), gRemPid, gOpenHost, user, ShmId,
                    GlbClientName);
            OffSet = RpdUpdateAuthTab(1, line, &token);
         } else if (gDebug > 0)
            ErrorInfo
                ("RpdGlobusAuth: unable to export context to shm for later use");
      }
      // Comunicate login user name to client (and token)
      sprintf(line, "%s %d", gUser, OffSet);
      NetSend(strlen(line), kROOTD_GLOBUS);   // Send message length first
      NetSend(line, kMESS_STRING);

      if (gReUseRequired) {
         // Send Token
         if (RpdSecureSend(token) == -1) {
            ErrorInfo
                ("RpdGlobusAuth: problems secure-sending token - may result in corrupted token");
         }
         if (token) delete[] token;

         // Save RSA public key into file for later use by other rootd/proofd
         RpdSavePubKey(gPubKey, OffSet, gUser);
      }
   } else {
      // Comunicate login user name to client (and token)
      sprintf(line, "%s %d", gUser, OffSet);
      NetSend(strlen(line), kROOTD_GLOBUS);   // Send message length first
      NetSend(line, kMESS_STRING);
   }

   // and free allocated memory
   free(user);
   free(GlbClientName);

   if (gDebug > 0)
      ErrorInfo("RpdGlobusAuth: logging as %s ", gUser);

#endif
}

//______________________________________________________________________________
void RpdRfioAuth(const char *sstr)
{
   // Check if user and group id specified in the request exist in the
   // passwd file. If they do then grant access. Very insecure: to be used
   // with care.

   gAuth = 0;

   if (gDebug > 2)
      ErrorInfo("RpdRfioAuth: analyzing ... %s", sstr);

   if (!*sstr) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo("RpdRfioAuth: subject string is empty");
      return;
   }
   // Decode subject string
   unsigned int uid, gid;
   sscanf(sstr, "%u %u", &uid, &gid);

   // Now inquire passwd ...
   struct passwd *pw;
   if ((pw = getpwuid((uid_t) uid)) == 0) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo("RpdRfioAuth: uid %u not found", uid);
      return;
   }
   // Check if authorized
   char cuid[20];
   sprintf(cuid, "%u", uid);
   if (gUserIgnLen[5] > 0 && strstr(gUserIgnore[5], cuid) != 0) {
      NetSend(kErrNotAllowed, kROOTD_ERR);
      ErrorInfo
          ("RpdRfioAuth: user (%u,%s) not authorized to use (uid:gid) method",
           uid, pw->pw_name);
      return;
   }
   if (gUserAlwLen[5] > 0 && strstr(gUserAllow[5], cuid) == 0) {
      NetSend(kErrNotAllowed, kROOTD_ERR);
      ErrorInfo
          ("RpdRfioAuth: user (%u,%s) not authorized to use (uid:gid) method",
           uid, pw->pw_name);
      return;
   }

   // Now check group id ...
   if (gid != (unsigned int) pw->pw_gid) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo
          ("RpdRfioAuth: group id does not match (remote:%u,local:%u)",
           gid, (unsigned int) pw->pw_gid);
      return;
   }
   // Set username ....
   strcpy(gUser, pw->pw_name);


   // Notify, if required ...
   if (gDebug > 0)
      ErrorInfo("RpdRfioAuth: user %s authenticated (uid:%u, gid:%u)",
                gUser, uid, gid);

   // Set Auth flag
   gAuth = 1;
}

//______________________________________________________________________________
void RpdCleanup(const char *sstr)
{
   // Cleanup auth table.

   char Host[kMAXPATHLEN] = { 0 };
   int rPid;

   sscanf(sstr, "%d %s", &rPid, Host);
   if (gDebug > 2)
      ErrorInfo("RpdCleanup: contacted by remote Host: %s, Pid: %d", Host,
                rPid);

   // Cleanup Auth tab
   int ns;
   if ((ns = RpdCleanupAuthTab(Host, rPid)) > 0)
      ErrorInfo("RpdCleanup: %d not properly cleaned", ns);

   // Trim Auth Tab file if call via RootdTerm (typically when rootd is shutting down ... )
   if (!strcmp(Host, "all") || (rPid == 0))
      RpdUpdateAuthTab(0, 0, 0);
}

//______________________________________________________________________________
void RpdCheckSession()
{

   // Check auth tab file size
   struct stat st;
   if (stat(gRpdAuthTab, &st) == 0) {

      // Cleanup auth tab file if too big
      if (st.st_size > kMAXTABSIZE)
         RpdUpdateAuthTab(0, 0, 0);

      // New file if still too big
      if (stat(gRpdAuthTab, &st) == 0) {
         if (st.st_size > kMAXTABSIZE)
            RpdUpdateAuthTab(-1, 0, 0);
      }
   }

   // Reset
   int i;
   gNumAllow = gNumLeft = 0;
   for (i = 0; i < kMAXSEC; i++) {
      gAllowMeth[i] = -1;
      gHaveMeth[i] = 1;
   }

   // List of default authentication methods (to be save in the session file)
   RpdDefaultAuthAllow();

   char cmeth[200];
   cmeth[0] = '\0';
   sprintf(cmeth, "%d", gNumAllow);
   for (i = 0; i < gNumAllow; i++) {
      sprintf(cmeth, "%s %d", cmeth, gAllowMeth[i]);
   }
}

//______________________________________________________________________________
void RpdDefaultAuthAllow()
{
   // Check configuration options and running daemons to build a default list
   // of secure methods.

   if (gDebug > 2)
      ErrorInfo("RpdDefaultAuthAllow: Enter");

   // UsrPwdClear
   gAllowMeth[gNumAllow] = 0;
   gNumAllow++;
   gNumLeft++;

   // SSH
   if (RpdCheckDaemon("sshd") > 0) {
      if (RpdCheckSshd() > 0) {
         gAllowMeth[gNumAllow] = 4;
         gNumAllow++;
         gNumLeft++;
      }
   } else if (RpdCheckSshd() > 0) {
      if (gDebug > 0)
         ErrorInfo
             ("RpdDefaultAuthAllow: sshd not found by 'ps' but a process is listening on the specified port (%d)",
              gSshdPort);
      // Try at least connection to port ...
      gAllowMeth[gNumAllow] = 4;
      gNumAllow++;
      gNumLeft++;
   }
   if (gNumAllow == 0) {
      // Don't have this method
      gHaveMeth[4] = 0;
   }
   // SRP
#ifdef R__SRP
   gAllowMeth[gNumAllow] = 1;
   gNumAllow++;
   gNumLeft++;
#else
   // Don't have this method
   gHaveMeth[1] = 0;
#endif

   // Kerberos
#ifdef R__KRB5
   gAllowMeth[gNumAllow] = 2;
   gNumAllow++;
   gNumLeft++;
#else
   // Don't have this method
   gHaveMeth[2] = 0;
#endif

   // Globus
#ifdef R__GLBS
   gAllowMeth[gNumAllow] = 3;
   gNumAllow++;
   gNumLeft++;
#else
   // Don't have this method
   gHaveMeth[3] = 0;
#endif

   if (gDebug > 2) {
      int i;
      char temp[200];
      temp[0] = '\0';
      if (gNumAllow == 0)
         strcpy(temp, "none");
      for (i = 0; i < gNumAllow; i++) {
         sprintf(temp, "%s %d", temp, gAllowMeth[i]);
      }
      ErrorInfo
          ("RpdDefaultAuthAllow: default list of secure methods available: %s",
           temp);
   }
}

//______________________________________________________________________________
int RpdCheckDaemon(const char *daemon)
{
   // Check the running of process 'daemon'.
   // Info got from 'ps ax'.

   char cmd[1024] = { 0 };
   int ch, i = 0, cnt = 0;

   if (gDebug > 2)
      ErrorInfo("RpdCheckDaemon: Enter ... %s", daemon);

   // Return if empty
   if (daemon == 0 || strlen(daemon) == 0)
      return cnt;

   // Build command
   sprintf(cmd, "ps ax | grep %s 2>/dev/null", daemon);

   // Run it ...
   FILE *fp = popen(cmd, "r");
   if (fp != 0) {
      for (ch = fgetc(fp); ch != EOF; ch = fgetc(fp)) {
         if (ch != 10) {
            cmd[i++] = ch;
         } else {
            cmd[i] = '\0';
            if (strstr(cmd, "grep") == 0 && strstr(cmd, "rootd") == 0
                && strstr(cmd, "proofd") == 0) {
               cnt++;
               if (gDebug > 2)
                  ErrorInfo("RpdCheckDaemon: read: %s", cmd);
            }
            i = 0;
         }
      }
      if (i > 0) {
         cmd[i] = '\0';
         cnt++;
         if (gDebug > 2)
            ErrorInfo("RpdCheckDaemon: read: %s", cmd);
      }
      pclose(fp);
      if (gDebug > 2)
         ErrorInfo("RpdCheckDaemon: read %d lines", cnt);

   } else {
      ErrorInfo("RpdCheckDaemon: problems executing cmd ...");
   }
   return cnt;
}

//______________________________________________________________________________
int RpdCheckSshd()
{
   // Tries to connect to sshd daemon on its standard port (22)
   // Used if RpdCheckDaemon returns a negative result

   if (gDebug > 2)
      ErrorInfo("RpdCheckSshd: Enter ... ");

   // Standard SSH port
   //  int SshdPort = 22;

   // First get local host address
   struct hostent *h = gethostbyname("localhost");
   if (h == 0) {
      // Make further attempt with HOSTNAME
      if (getenv("HOSTNAME") == 0) {
         ErrorInfo("RpdCheckSshd: unable to resolve local host name");
         return 0;
      } else {
         h = gethostbyname(getenv("HOSTNAME"));
         if (h == 0) {
            ErrorInfo
                ("RpdCheckSshd: local host name is unknown to gethostbyname: '%s'",
                 getenv("HOSTNAME"));
            return 0;
         }
      }
   }
   // Fill relevant sockaddr_in structure
   struct sockaddr_in servAddr;
   servAddr.sin_family = h->h_addrtype;
   memcpy((char *) &servAddr.sin_addr.s_addr, h->h_addr_list[0],
          h->h_length);
   servAddr.sin_port = htons(gSshdPort);

   // create AF_INET socket
   int sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0) {
      ErrorInfo("RpdCheckSshd: cannot open new AF_INET socket (errno:%d) ",
                errno);
      return 0;
   }

   /* bind any port number */
   struct sockaddr_in localAddr;
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   localAddr.sin_port = htons(0);
   int rc = bind(sd, (struct sockaddr *) &localAddr, sizeof(localAddr));
   if (rc < 0) {
      ErrorInfo("RpdCheckSshd: cannot bind to local port %u", gSshdPort);
      return 0;
   }
   // connect to server
   rc = connect(sd, (struct sockaddr *) &servAddr, sizeof(servAddr));
   if (rc < 0) {
      ErrorInfo("RpdCheckSshd: cannot connect to local port %u",
                gSshdPort);
      return 0;
   }
   // Sshd successfully contacted
   if (gDebug > 2)
      ErrorInfo("RpdCheckSshd: success!");
   return 1;
}

//______________________________________________________________________________
void RpdUser(const char *sstr)
{
   // Check user id. If user id is not equal to rootd's effective uid, user
   // will not be allowed access, unless effective uid = 0 (i.e. root).
   const int kMaxBuf = 256;
   char recvbuf[kMaxBuf];
   char rootdpass[kMAXPATHLEN];
   char specpass[64] = {0};
   EMessageTypes kind;
   struct passwd *pw;
#ifdef R__SHADOWPW
   struct spwd *spw;
#endif
   if (gDebug > 2)
      ErrorInfo("RpdUser: Enter ... %s", sstr);

   gAuth = 0;

   // Nothing can be done if empty message
   if (!*sstr) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo("RpdUser: received empty string");
      return;
   }
   // Parse input message
   char *user = new char[strlen(sstr) + 1];
   if (gClientProtocol > 8) {
      int ulen, ofs, opt;
      // Decode subject string
      sscanf(sstr, "%d %d %d %d %s", &gRemPid, &ofs, &opt, &ulen, user);
      user[ulen] = '\0';
      gReUseRequired = (opt & kAUTH_REUSE_MSK);
      gCryptRequired = (opt & kAUTH_CRYPT_MSK);
      gSaltRequired  = (opt & kAUTH_SSALT_MSK);
      gOffSet = ofs;
   } else {
      strcpy(user, sstr);
   }
   if (gDebug > 2)
      ErrorInfo("RpdUser: gReUseRequired: %d gCryptRequired: %d",
                gReUseRequired, gCryptRequired);

   ERootdErrors err = kErrNoUser;
   if (!strcmp(gService, "rootd")) {
      // Default anonymous account ...
      if (!strcmp(user, "anonymous")) {
         user[0] = '\0';
         strcpy(user, "rootd");
      }
   }

   if ((pw = getpwnam(user)) == 0) {
      NetSend(err, kROOTD_ERR);
      ErrorInfo("RpdUser: user %s unknown", user);
      return;
   }
   // Check if of type anonymous ...
   if (!strcmp(pw->pw_shell, "/bin/false")) {
      err = kErrNoAnon;
      gAnon = 1;
      gReUseRequired = 0;
   }
   // If server is not started as root and user is not same as the
   // one who started rootd then authetication is not ok.
   uid_t uid = getuid();
   if (uid && uid != pw->pw_uid) {
      NetSend(kErrBadUser, kROOTD_ERR);
      ErrorInfo("RpdUser: user not same as effective user of rootd");
      return;
   }
   // Check if authorized
   // If not anonymous, try to get passwd
   // (if our system uses shadow passwds and we are not superuser
   // we cannot authenticate users ...)
   //   char *passw = 0;
   char *passw = specpass;
   if (gAnon == 0) {

      // Try if special password is given via .rootdpass
      sprintf(rootdpass, "%s/%s", pw->pw_dir, kRootdPass);

      int fid = open(rootdpass, O_RDONLY);
      if (fid != -1) {
         if (read(fid, specpass, sizeof(specpass) - 1) > 0) {
            passw = specpass;
         }
         close(fid);
      }

      if (strlen(passw) == 0 || !strcmp(passw, "x")) {
#ifdef R__SHADOWPW
         // System V Rel 4 style shadow passwords
         if ((spw = getspnam(user)) == 0) {
            if (gDebug > 0) {
               ErrorInfo("RpdUser: Shadow passwd not accessible for user %s",user);
               ErrorInfo("RpdUser: trying normal or special root passwd");
            }
            passw = pw->pw_passwd;
         } else
            passw = spw->sp_pwdp;
#else
         passw = pw->pw_passwd;
#endif
      }
      // Check if successful
      if (strlen(passw) == 0 || !strcmp(passw, "x")) {
         NetSend(kErrNotAllowed, kROOTD_ERR);
         ErrorInfo("RpdUser: passwd hash not available for user %s", user);
         ErrorInfo
             ("RpdUser: user %s cannot be authenticated with this method",
              user);
         return;
      }
   }
   // Check if the administrator allows authentication
   char cuid[20];
   sprintf(cuid, "%d", (int)pw->pw_uid);
   if (gUserIgnLen[0] > 0 && strstr(gUserIgnore[0], cuid) != 0) {
      NetSend(kErrNotAllowed, kROOTD_ERR);
      ErrorInfo
          ("RpdUser: user (%d,%s) not authorized to use UsrPwd method",
           uid, pw->pw_name);
      return;
   }
   if (gUserAlwLen[0] > 0 && strstr(gUserAllow[0], cuid) == 0) {
      NetSend(kErrNotAllowed, kROOTD_ERR);
      ErrorInfo
          ("RpdUser: user (%d,%s) not authorized to use UsrPwd method",
           uid, pw->pw_name);
      return;
   }
   // Ok: Save username and go to next steps
   strcpy(gUser, user);
   if (user) delete[] user;

   if (gClientProtocol > 8) {

      // Prepare status flag to send back
      if (gAnon == 1) {
         // Anonymous user: we will receive a text pass in the form user@remote.host.dom
         NetSend(-1, kROOTD_AUTH);

      } else {

         if (gCryptRequired) {
            // Named user: first we receive a session public key
            // Ask for the RSA key
            NetSend(1, kROOTD_RSAKEY);

            // Receive the key securely
            if (RpdRecvClientRSAKey()) {
               ErrorInfo
                   ("RpdUser: could not import a valid key - switch off reuse for this session");
               gReUseRequired = 0;
            }

            // Determine Salt
            char Salt[20] = { 0 };
            int Slen = 0;

            if (gSaltRequired) {
               if (!strncmp(passw, "$1$", 3)) {
                  // Shadow passwd
                  char *pd = strstr(passw + 4, "$");
                  Slen = (int) (pd - passw);
                  strncpy(Salt, passw, Slen);
                  Salt[Slen] = 0;
               } else {
                  Slen = 2;
                  strncpy(Salt, passw, Slen);
                  Salt[Slen] = 0;
               }
               if (gDebug > 2)
                  ErrorInfo("RpdUser: salt: '%s' ",Salt);

               // Send it over encrypted
               if (RpdSecureSend(Salt) == -1) {
                  ErrorInfo
                      ("RpdUser: problems secure-sending salt - may result in corrupted salt");
               }
            } else {
               NetSend(0, kMESS_ANY);
            }
         } else {
            // We continue the aythentication process in clear
            NetSend(0, kROOTD_AUTH);
         }
      }

   } else {
      // If we are talking to a old client protocol
      NetSend(0, kROOTD_AUTH);
   }

   // Get the password hash or anonymous string
   if (NetRecv(recvbuf, kMaxBuf, kind) < 0) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdUser: error receiving message");
      return;
   }
   if (kind != kROOTD_PASS) {
      NetSend(kErrFatal, kROOTD_ERR);
      ErrorInfo("RpdUser: received wrong message type: %d (expecting: %d)",
                kind, (int) kROOTD_PASS);
      return;
   }
   if (!strncmp(recvbuf,"-1",2)) {
      if (gDebug > 0)
         ErrorInfo("RpdUser: client did not send a password - return");
      return;
   }
   // Get passwd
   char *passwd = 0;
   if (gAnon == 0 && gClientProtocol > 8 && gCryptRequired) {

      // Receive encrypted pass hash
      if (RpdSecureRecv(&passwd) == -1) {
         ErrorInfo
             ("RpdUser: problems secure-receiving pass hash - %s",
              "may result in authentication failure");
      }

   } else {

      // Receive clear or anonymous pass
      passwd = new char[strlen(recvbuf) + 1];

      // Re-invert pass
      int i, n = strlen(recvbuf);
      for (i = 0; i < n; i++)
         passwd[i] = ~recvbuf[i];
      passwd[i] = '\0';

      if (gDebug > 2 && gAnon)
         ErrorInfo("RpdUser: received anonymous pass: '%s'", passwd);
   }

   // Check the passwd and login if ok ...
   RpdPass(passwd);

   if (passwd) delete[] passwd;

}

//______________________________________________________________________________
int RpdGuessClientProt(const char *buf, EMessageTypes kind)
{
   // Try a guess of the client protocol from what she/he sent over
   // the net ...

   if (gDebug > 2)
      ErrorInfo("RpdGuessClientProt: Enter: buf: '%s', kind: %d", buf,
                (int) kind);

   // Assume same version as us.
   int proto = 9;

   // Clear authentication
   if (kind == kROOTD_USER) {
      char usr[64], rest[256];
      int ns = sscanf(buf, "%s %s", usr, rest);
      if (ns == 1)
         proto = 8;
   }
   // SRP authentication
   if (kind == kROOTD_SRPUSER) {
      char usr[64], rest[256];
      int ns = sscanf(buf, "%s %s", usr, rest);
      if (ns == 1)
         proto = 8;
   }
   // Kerberos authentication
   if (kind == kROOTD_KRB5) {
      if (strlen(buf) == 0)
         proto = 8;
   }

   if (gDebug > 2)
      ErrorInfo("RpdGuessClientProt: guess for gClientProtocol is %d",
                proto);

   // Return the guess
   return proto;
}

//______________________________________________________________________________
char *RpdGetRandString(int Opt, int Len)
{
   // Allocates and Fills a NULL terminated buffer of length Len+1 with
   // Len random characters.
   // Return pointer to the buffer (to be deleted by the caller)
   // Opt = 0      any non dangerous char
   //       1      letters and numbers  (upper and lower case)
   //       2      hex characters       (upper and lower case)
   //       3      crypt like           [a-zA-Z0-9./]

   int iimx[4][4] = { { 0x0, 0xffffff08, 0xafffffff, 0x2ffffffe }, // Opt = 0
                      { 0x0, 0x3ff0000,  0x7fffffe,  0x7fffffe },  // Opt = 1
                      { 0x0, 0x3ff0000,  0x7e,       0x7e },       // Opt = 2
                      { 0x0, 0x3ffc000,  0x7fffffe,  0x7fffffe }   // Opt = 3
   };

   char *cOpt[4] = { "Any", "LetNum", "Hex", "Crypt" };

   //  Default option 0
   if (Opt < 0 || Opt > 3) {
      Opt = 0;
      if (gDebug > 2)
         ErrorInfo("RpdGetRandString: Unknown option: %d : assume 0", Opt);
   }
   if (gDebug > 2)
      ErrorInfo("RpdGetRandString: Enter ... Len: %d %s", Len, cOpt[Opt]);

   // Allocate buffer
   char *Buf = new char[Len + 1];

   // Init Random machinery ...
   if (!gRandInit) {
      RpdInitRand();
      gRandInit = 1;
   }

   // randomize
   int k = 0;
   int i, j, l, m, frnd;
   while (k < Len) {
      frnd = rand();
      for (m = 7; m < 32; m += 7) {
         i = 0x7F & (frnd >> m);
         j = i / 32;
         l = i - j * 32;
         if ((iimx[Opt][j] & (1 << l))) {
            Buf[k] = i;
            k++;
         }
         if (k == Len)
            break;
      }
   }

   // NULL terminated
   Buf[Len] = 0;
   if (gDebug > 2)
      ErrorInfo("RpdGetRandString: got '%s' ", Buf);

   return Buf;
}

//______________________________________________________________________________
int RpdGetRSAKeys(char *PubKey, int Opt)
{
   // Get public key from file PubKey (Opt == 1) or string PubKey (Opt == 0).

   char Str[kMAXPATHLEN] = { 0 };
   int KeyType = 0;

   if (gDebug > 2)
      ErrorInfo("RpdGetRSAKeys: enter: string len: %d, opt %d ", strlen(PubKey), Opt);

   if (!PubKey)
      return KeyType;

   FILE *fKey = 0;
   // Parse input type
   KeyType = 1;
   if (Opt == 1) {
      // Input is a File name: should get the string first
      if (access(PubKey, R_OK)) {
         ErrorInfo("RpdGetRSAKeys: Key File cannot be read - return ");
         return 0;
      }
      fKey = fopen(PubKey, "r");
      if (!fKey) {
         ErrorInfo("RpdGetRSAKeys: cannot open key file %s ", PubKey);
         return 0;
      }
      fgets(Str, sizeof(Str), fKey);
   }

   if (Opt == 0) {
      strcpy(Str, PubKey);
   }
   if (strlen(Str) > 0) {
      // The format is #<hex_n>#<hex_d>#
      char *pd1 = strstr(Str, "#");
      char *pd2 = strstr(pd1 + 1, "#");
      char *pd3 = strstr(pd2 + 1, "#");
      if (pd1 && pd2 && pd3) {
         // Get <hex_n> ...
         int l1 = (int) (pd2 - pd1 - 1);
         char *RSA_n_exp = new char[l1 + 1];
         strncpy(RSA_n_exp, pd1 + 1, l1);
         RSA_n_exp[l1] = 0;
         if (gDebug > 2)
            ErrorInfo("RpdGetRSAKeys: got %d bytes for RSA_n_exp", strlen(RSA_n_exp));
         // Now <hex_d>
         int l2 = (int) (pd3 - pd2 - 1);
         char *RSA_d_exp = new char[l2 + 1];
         strncpy(RSA_d_exp, pd2 + 1, l2);
         RSA_d_exp[l2] = 0;
         if (gDebug > 2)
            ErrorInfo("RpdGetRSAKeys: got %d bytes for RSA_d_exp", strlen(RSA_d_exp));

         rsa_num_sget(&gRSA_n, RSA_n_exp);
         rsa_num_sget(&gRSA_d, RSA_d_exp);

         if (RSA_n_exp) delete[] RSA_n_exp;
         if (RSA_d_exp) delete[] RSA_d_exp;

      } else
         return 0;
   }

   if (fKey)
      fclose(fKey);

   return KeyType;

}

//______________________________________________________________________________
void RpdSavePubKey(char *PubKey, int OffSet, char *user)
{
   // Save RSA public key into file for later use by other rootd/proofd.

   if (gRSAKey == 0 || OffSet < 0)
      return;

   char PubKeyFile[kMAXPATHLEN];
   sprintf(PubKeyFile, "%s/rpk_%d", gTmpDir, OffSet);
   FILE *fKey = fopen(PubKeyFile, "w");
   if (fKey) {
      if (gRSAKey == 1) {
         fprintf(fKey, "%s", PubKey);
      }
   } else {
      ErrorInfo
          ("RpdSavePubKey: cannot save public key: set entry inactive");
      RpdCleanupAuthTab(gOpenHost, gRemPid);
   }

   if (fKey) {
      fclose(fKey);
      //      chmod(PubKeyFile, 0666);
      chmod(PubKeyFile, 0600);

      if (getuid() == 0) {
         // Set ownership of the pub key to the user

         struct passwd *pw = getpwnam(user);

         if (chown(PubKeyFile,pw->pw_uid,pw->pw_gid) == -1) {
            ErrorInfo
                ("RpdSavePubKey: cannot change ownership of %s (errno: %d)",
                  PubKeyFile,GetErrno());
         }
      }
   }
}

//______________________________________________________________________________
int RpdSecureSend(char *Str)
{
   // Encode null terminated Str using the session private key indcated by Key
   // and sends it over the network.
   // Returns number of bytes sent.or -1 in case of error.

   char BufTmp[kMAXSECBUF];
   char BufLen[20];

   int sLen = strlen(Str) + 1;

   int Ttmp = 0;
   int Nsen = -1;

   if (gRSAKey == 1) {
      strncpy(BufTmp, Str, sLen);
      BufTmp[sLen] = 0;
      Ttmp = rsa_encode(BufTmp, sLen, gRSA_n, gRSA_d);
      sprintf(BufLen, "%d", Ttmp);
      NetSend(BufLen, kROOTD_ENCRYPT);
      Nsen = NetSendRaw(BufTmp, Ttmp);
      if (gDebug > 4)
         ErrorInfo
             ("RpdSecureSend: Local: sent %d bytes (expected: %d)",
              Nsen, Ttmp);
   } else {
      ErrorInfo("RpdSecureSend: Unknown key option (%d) - return",
                gRSAKey);
   }

   return Nsen;

}

//______________________________________________________________________________
int RpdSecureRecv(char **Str)
{
   // Receive buffer and decode it in Str using key indicated by Key type.
   // Return number of received bytes or -1 in case of error.

   char BufTmp[kMAXSECBUF];
   char BufLen[20];

   int Nrec = -1;
   // We must get a pointer ...
   if (!Str)
      return Nrec;

   if (gDebug > 2)
      ErrorInfo("RpdSecureRecv: enter ... (key is %d)", gRSAKey);

   EMessageTypes kind;
   NetRecv(BufLen, 20, kind);
   int Len = atoi(BufLen);
   if (gDebug > 4)
      ErrorInfo("RpdSecureRecv: got len '%s' %d ", BufLen, Len);
   if (!strncmp(BufLen, "-1", 2))
      return Nrec;

   // Now proceed
   if (gRSAKey == 1) {
      Nrec = NetRecvRaw(BufTmp, Len);
      rsa_decode(BufTmp, Len, gRSA_n, gRSA_d);
      if (gDebug > 2)
         ErrorInfo("RpdSecureRecv: Local: decoded string is %d bytes long", strlen(BufTmp));
   } else {
      ErrorInfo("RpdSecureRecv: Unknown key option (%d) - return",
                gRSAKey);
   }

   *Str = new char[strlen(BufTmp) + 1];
   strcpy(*Str, BufTmp);

   return Nrec;

}

//______________________________________________________________________________
int RpdGenRSAKeys()
{
   // Generate a valid pair of private/public RSA keys to protect for authentication
   // token exchange
   // Returns 1 if a good key pair is not foun after kMAXRSATRIES attempts
   // Returns 0 if a good key pair is found

   if (gDebug > 2)
      ErrorInfo("RpdGenRSAKeys: enter");

   // Init Random machinery ...
   if (!gRandInit) {
      RpdInitRand();
      gRandInit = 1;
   }

   // Sometimes some bunch is not decrypted correctly
   // That's why we make retries to make sure that encryption/decryption works as expected
   bool NotOk = 1;
   rsa_NUMBER p1, p2, rsa_n, rsa_e, rsa_d;
   int l_n = 0, l_e = 0, l_d = 0;
   char buf[rsa_STRLEN];
   char buf_n[rsa_STRLEN], buf_e[rsa_STRLEN], buf_d[rsa_STRLEN];

   int NAttempts = 0;
   int thePrimeLen = 20;
   int thePrimeExp = 45;   // Prime probability = 1-0.5^thePrimeExp
   while (NotOk && NAttempts < kMAXRSATRIES) {

      NAttempts++;
      if (gDebug > 2 && NAttempts > 1) {
            ErrorInfo("RpdGenRSAKeys: retry no. %d",NAttempts);
         srand(rand());
      }

      // Valid pair of primes
      p1 = rsa_genprim(thePrimeLen, thePrimeExp);
      p2 = rsa_genprim(thePrimeLen+1, thePrimeExp);

      // Retry if equal
      int NPrimes = 0;
      while (rsa_cmp(&p1, &p2) == 0 && NPrimes < kMAXRSATRIES) {
         NPrimes++;
         if (gDebug > 2)
            ErrorInfo("RpdGenRSAKeys: equal primes: regenerate (%d times)",NPrimes);
         srand(rand());
         p1 = rsa_genprim(thePrimeLen, thePrimeExp);
         p2 = rsa_genprim(thePrimeLen+1, thePrimeExp);
      }

#if 1
      if (gDebug > 2) {
         rsa_num_sput(&p1, buf, rsa_STRLEN);
         ErrorInfo("RpdGenRSAKeys: local: p1: '%s' ", buf);
         rsa_num_sput(&p2, buf, rsa_STRLEN);
         ErrorInfo("RpdGenRSAKeys: local: p2: '%s' ", buf);
      }
#endif
      // Generate keys
      if (rsa_genrsa(p1, p2, &rsa_n, &rsa_e, &rsa_d)) {
         ErrorInfo("RpdGenRSAKeys: genrsa: unable to generate keys (%d)",NAttempts);
         continue;
      }

      // Determine their lengths
      rsa_num_sput(&rsa_n, buf_n, rsa_STRLEN);
      l_n = strlen(buf_n);
      rsa_num_sput(&rsa_e, buf_e, rsa_STRLEN);
      l_e = strlen(buf_e);
      rsa_num_sput(&rsa_d, buf_d, rsa_STRLEN);
      l_d = strlen(buf_d);

#if 1
      if (gDebug > 2) {
         ErrorInfo("RpdGenRSAKeys: local: n: '%s' length: %d", buf_n, l_n);
         ErrorInfo("RpdGenRSAKeys: local: e: '%s' length: %d", buf_e, l_e);
         ErrorInfo("RpdGenRSAKeys: local: d: '%s' length: %d", buf_d, l_d);
      }
#endif
      if (rsa_cmp(&rsa_n, &rsa_e) <= 0)
         continue;
      if (rsa_cmp(&rsa_n, &rsa_d) <= 0)
         continue;

      // Now we try the keys
      char Test[2 * rsa_STRLEN] = "ThisIsTheStringTest01203456-+/";
      Int_t lTes = 31;
      char *Tdum = RpdGetRandString(0, lTes - 1);
      strncpy(Test, Tdum, lTes);
      delete[]Tdum;
      char buf[2 * rsa_STRLEN];
      if (gDebug > 3)
         ErrorInfo("RpdGenRSAKeys: local: test string: '%s' ", Test);

      // Private/Public
      strncpy(buf, Test, lTes);
      buf[lTes] = 0;

      // Try encryption with private key
      int lout = rsa_encode(buf, lTes, rsa_n, rsa_e);
      if (gDebug > 3)
         ErrorInfo("GenRSAKeys: local: length of crypted string: %d bytes", lout);

      // Try decryption with public key
      rsa_decode(buf, lout, rsa_n, rsa_d);
      buf[lTes] = 0;
      if (gDebug > 3)
         ErrorInfo("RpdGenRSAKeys: local: after private/public : '%s' ", buf);

      if (strncmp(Test, buf, lTes))
         continue;

      // Public/Private
      strncpy(buf, Test, lTes);
      buf[lTes] = 0;

      // Try encryption with public key
      lout = rsa_encode(buf, lTes, rsa_n, rsa_d);
      if (gDebug > 3)
         ErrorInfo("RpdGenRSAKeys: local: length of crypted string: %d bytes ",
              lout);

      // Try decryption with private key
      rsa_decode(buf, lout, rsa_n, rsa_e);
      buf[lTes] = 0;
      if (gDebug > 3)
         ErrorInfo("RpdGenRSAKeys: local: after public/private : '%s' ", buf);

      if (strncmp(Test, buf, lTes))
         continue;

      NotOk = 0;
   }

   if (NotOk) {
      ErrorInfo("RpdGenRSAKeys: unable to generate good RSA key pair - return");
      return 1;
   }

   // Save Private key
   rsa_assign(&gRSAPriKey.n, &rsa_n);
   rsa_assign(&gRSAPriKey.e, &rsa_e);

   // Save Public key
   rsa_assign(&gRSAPubKey.n, &rsa_n);
   rsa_assign(&gRSAPubKey.e, &rsa_d);

#if 0
   if (gDebug > 2) {
      // Determine their lengths
      ErrorInfo("RpdGenRSAKeys: local: generated keys are:");
      ErrorInfo("RpdGenRSAKeys: local: n: '%s' length: %d", buf_n, l_n);
      ErrorInfo("RpdGenRSAKeys: local: e: '%s' length: %d", buf_e, l_e);
      ErrorInfo("RpdGenRSAKeys: local: d: '%s' length: %d", buf_d, l_d);
   }
#endif
   // Export form
   gRSAPubExport.len = l_n + l_d + 4;
   gRSAPubExport.keys = new char[gRSAPubExport.len];

   gRSAPubExport.keys[0] = '#';
   memcpy(gRSAPubExport.keys + 1, buf_n, l_n);
   gRSAPubExport.keys[l_n + 1] = '#';
   memcpy(gRSAPubExport.keys + l_n + 2, buf_d, l_d);
   gRSAPubExport.keys[l_n + l_d + 2] = '#';
   gRSAPubExport.keys[l_n + l_d + 3] = 0;
#if 0
   if (gDebug > 2)
      ErrorInfo("RpdGenRSAKeys: local: export pub: '%s'", gRSAPubExport.keys);
#else
   if (gDebug > 2)
      ErrorInfo("RpdGenRSAKeys: local: export pub length: %d bytes", gRSAPubExport.len);
#endif

   return 0;
}

//______________________________________________________________________________
int RpdRecvClientRSAKey()
{
   // Generates local public/private RSA key pair
   // Send request for Client Public Key and Local public key
   // Receive encoded Client Key
   // Decode Client public key
   // NB: key is not saved to file here

   if (gRSAInit == 0) {
      // Generate Local RSA keys for the session
      if (RpdGenRSAKeys()) {
         ErrorInfo("RpdRecvClientRSAKey: unable to generate local keys");
         return 1;
      }
   }

   // Send server public key
   NetSend(gRSAPubExport.keys, gRSAPubExport.len, kROOTD_RSAKEY);

   // Receive length of message with encode client public key
   EMessageTypes kind;
   char BufLen[20];
   NetRecv(BufLen, 20, kind);
   int Len = atoi(BufLen);
   if (gDebug > 3)
      ErrorInfo("RpdRecvClientRSAKey: got len '%s' %d ", BufLen, Len);

   // Receive and decode encoded public key
   NetRecvRaw(gPubKey, Len);
   rsa_decode(gPubKey, Len, gRSAPriKey.n, gRSAPriKey.e);
   if (gDebug > 2)
      ErrorInfo("RpdRecvClientRSAKey: Local: decoded string is %d bytes long ",
         strlen(gPubKey));

   // Import Key and Determine key type
   gRSAKey = RpdGetRSAKeys(gPubKey, 0);
   if (gRSAKey == 0) {
      ErrorInfo("RpdRecvClientRSAKey: could not import a valid key");
      return 2;
   }

   return 0;

}

//______________________________________________________________________________
void RpdInitRand()
{
   // Init random machine.

   const char *randdev = "/dev/urandom";

   int fd;
   unsigned int seed;
   if ((fd = open(randdev, O_RDONLY)) != -1) {
      if (gDebug > 2)
         ErrorInfo("RpdInitRand: taking seed from %s", randdev);
      read(fd, &seed, sizeof(seed));
      close(fd);
   } else {
      if (gDebug > 2)
         ErrorInfo("RpdInitRand: %s not available: using time()", randdev);
      seed = time(0);   //better use times() + win32 equivalent
   }
   srand(seed);
}

} // namespace ROOT
