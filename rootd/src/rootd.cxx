// @(#)root/rootd:$Name:  $:$Id: rootd.cxx,v 1.43 2002/06/25 23:53:26 rdm Exp $
// Author: Fons Rademakers   11/08/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/* Parts of this file are copied from the MIT krb5 distribution and
 * are subject to the following license:
 *
 * Copyright 1990,1991 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 */

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Rootd                                                                //
//                                                                      //
// Root remote file server daemon.                                      //
// This small server is started either by inetd when a client requests  //
// a connection to a rootd server or by hand (i.e. from the command     //
// line). The rootd server works with the ROOT TNetFile class. It       //
// allows remote access to ROOT database files in either read or        //
// write mode. By default TNetFile uses port 1094 (allocated by IANA,   //
// www.iana.org, to rootd). To run rootd via inetd add the              //
// following line to /etc/services:                                     //
//                                                                      //
// rootd     1094/tcp                                                   //
//                                                                      //
// and to /etc/inetd.conf:                                              //
//                                                                      //
// rootd stream tcp nowait root /user/rdm/root/bin/rootd rootd -i       //
//                                                                      //
// Force inetd to reread its conf file with "kill -HUP <pid inetd>".    //
// You can also start rootd by hand running directly under your private //
// account (no root system priviliges needed). For example to start     //
// rootd listening on port 5151 just type:                              //
//                                                                      //
// rootd -p 5151                                                        //
//                                                                      //
// Notice: no & is needed. Rootd will go in background by itself.       //
//                                                                      //
// Rootd arguments:                                                     //
//   -i                says we were started by inetd                    //
//   -p port#          specifies a different port to listen on          //
//   -b tcpwindowsize  specifies the tcp window size in bytes (e.g. see //
//                     http://www.psc.edu/networking/perf_tune.html)    //
//                     Default is 65535. Only change default for pipes  //
//                     with a high bandwidth*delay product.             //
//   -d level          level of debug info written to syslog            //
//                     0 = no debug (default)                           //
//                     1 = minimum                                      //
//                     2 = medium                                       //
//                     3 = maximum                                      //
//                                                                      //
// Rootd can also be configured for anonymous usage (like anonymous     //
// ftp). To setup rootd to accept anonymous logins do the following     //
// (while being logged in as root):                                     //
//                                                                      //
// - Add the following line to /etc/passwd:                             //
//                                                                      //
//   rootd:*:71:72:Anonymous rootd:/var/spool/rootd:/bin/false          //
//                                                                      //
//   where you may modify the uid, gid (71, 72) and the home directory  //
//   to suite your system.                                              //
//                                                                      //
// - Add the following line to /etc/group:                              //
//                                                                      //
//   rootd:*:72:rootd                                                   //
//                                                                      //
//   where the gid must match the gid in /etc/passwd.                   //
//                                                                      //
// - Create the directories:                                            //
//                                                                      //
//   mkdir /var/spool/rootd                                             //
//   mkdir /var/spool/rootd/tmp                                         //
//   chmod 777 /var/spool/rootd/tmp                                     //
//                                                                      //
//   Where /var/spool/rootd must match the rootd home directory as      //
//   specified in the rootd /etc/passwd entry.                          //
//                                                                      //
// - To make writeable directories for anonymous do, for example:       //
//                                                                      //
//   mkdir /var/spool/rootd/pub                                         //
//   chown rootd:rootd /var/spool/rootd/pub                             //
//                                                                      //
// That's all.                                                          //
//                                                                      //
// Several remarks:                                                     //
//  - you can login to an anonymous server either with the names        //
//    "anonymous" or "rootd".                                           //
//  - the passwd should be of type user@host.do.main. Only the @ is     //
//    enforced for the time being.                                      //
//  - in anonymous mode the top of the file tree is set to the rootd    //
//    home directory, therefore only files below the home directory     //
//    can be accessed.                                                  //
//  - anonymous mode only works when the server is started via inetd.   //
//                                                                      //
//  When your system uses shadow passwords you have to compile rootd    //
//  with -DR__SHADOWPW. Since shadow passwords can only be accessed     //
//  while being superuser (root) this works only when the server is     //
//  started via inetd. Another solution is to create a file             //
//  ~/.rootdpass containing an encrypted password. If this file exists  //
//  its password is used for authentication. This method overrides      //
//  all other authentication methods. To create an encrypted password   //
//  do something like:                                                  //
//     perl -e '$pw = crypt("<secretpasswd>","salt"); print "$pw\n"'    //
//  and store this string in ~/.rootdpass.                              //
//                                                                      //
//  To use AFS for authentication compile rootd with the -DR__AFS flag. //
//  In that case you also need to link with the AFS libraries. See      //
//  the Makefiles for more details.                                     //
//                                                                      //
//  To use Secure Remote Passwords (SRP) for authentication compile     //
//  rootd with the -DR__SRP flag. In that case you also need to link    //
//  with the SRP and gmp libraries. See the Makefile for more details.  //
//  SRP is described at: http://srp.stanford.edu/.                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Protocol changes (see gProtocol):
// 2 -> 3: added handling of kROOTD_FSTAT message.
// 3 -> 4: added support for TFTP (i.e. kROOTD_PUTFILE, kROOTD_GETFILE, etc.)
// 4 -> 5: added support for "+read" to allow readers when file is opened for writing
// 5 -> 6: added support for kerberos5 authentication

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
#include <netinet/in.h>
#include <errno.h>
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

#if defined(linux) || defined(__hpux) || defined(_AIX) || defined(__alpha) || \
    defined(__sun) || defined(__sgi) || defined(__FreeBSD__) || \
    defined(__APPLE__)
#define HAVE_MMAP
#endif

#ifdef HAVE_MMAP
#   include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE 0           /* compatability flag */
#endif
#endif

#if defined(linux)
#   include <features.h>
#   if __GNU_LIBRARY__ == 6
#      ifndef R__GLIBC
#         define R__GLIBC
#      endif
#   endif
#endif
#if defined(__MACH__) && !defined(__APPLE__)
#   define R__GLIBC
#endif

#if (defined(__FreeBSD__) && (__FreeBSD__ < 4)) || defined(__APPLE__)
#include <sys/file.h>
#define lockf(fd, op, sz)   flock((fd), (op))
#define F_LOCK             (LOCK_EX | LOCK_NB)
#define F_ULOCK             LOCK_UN
#endif

#if defined(linux) || defined(__sun) || defined(__sgi) || \
    defined(_AIX) || defined(__FreeBSD__) || defined(__APPLE__) || \
    defined(__MACH__)
#include <grp.h>
#include <sys/types.h>
#include <signal.h>
#endif

#if defined(__sun) || defined(R__GLIBC)
#include <crypt.h>
#endif

#if defined(__osf__) || defined(__sgi)
extern "C" char *crypt(const char *, const char *);
#endif

#if defined(__alpha) && !defined(linux) && !defined(__FreeBSD__)
extern "C" int initgroups(const char *name, int basegid);
#endif

#if defined(__sgi) && !defined(__GNUG__) && (SGI_REL<62)
extern "C" {
   int seteuid(int euid);
   int setegid(int egid);
}
#endif

#if defined(_AIX)
extern "C" {
   //int initgroups(const char *name, int basegid);
   int seteuid(uid_t euid);
   int setegid(gid_t egid);
}
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
extern "C" int ka_UserAuthenticateGeneral(int,char*,char*,char*,char*,int,int,int,char**);
#endif

#ifdef R__SRP
extern "C" {
#include <t_pwd.h>
#include <t_server.h>
}
#endif

#ifdef R__KRB5
extern "C" {
   #include <com_err.h>
   #include <krb5.h>
   int krb5_net_write(krb5_context, int, const char *, int);
}
#include <string>
extern krb5_deltat krb5_clockskew;
#endif

#include "rootdp.h"


//--- Globals ------------------------------------------------------------------

const char kRootdService[] = "rootd";
const char kRootdTab[]     = "/usr/tmp/rootdtab";
const char kRootdPass[]    = ".rootdpass";
const char kSRootdPass[]   = ".srootdpass";
const int  kMAXPATHLEN     = 1024;
enum { kBinary, kAscii };

int     gInetdFlag         = 0;
int     gPort              = 0;
int     gDebug             = 0;
int     gSockFd            = -1;
int     gAuth              = 0;
int     gAnon              = 0;
int     gFd                = -1;
int     gWritable          = 0;
int     gProtocol          = 6;       // increase when protocol changes
int     gUploaded          = 0;
int     gDownloaded        = 0;
int     gFtp               = 0;
double  gBytesRead         = 0;
double  gBytesWritten      = 0;
char    gUser[64]          = { 0 };
char    gPasswd[64]        = { 0 };   // only used for anonymous access
char    gOption[32]        = { 0 };
char    gFile[kMAXPATHLEN] = { 0 };

#ifdef R__KRB5
krb5_keytab  gKeytab       = 0;       // to allow specifying on the command line
krb5_context gKcontext;
#endif

//--- Machine specific routines ------------------------------------------------

#if !defined(__hpux)
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
#endif


//--- Rootd routines -----------------------------------------------------------

const char *shellMeta   = "~*[]{}?$";
const char *shellStuff  = "(){}<>\"'";
const char  shellEscape = '\\';

//______________________________________________________________________________
static int EscChar(const char *src, char *dst, int dstlen, const char *specchars, char escchar)
{
   // Escape specchars in src with escchar and copy to dst.

   const char *p;
   char *q, *end = dst+dstlen-1;

   for (p = src, q = dst; *p && q < end; ) {
      if (strchr(specchars, *p)) {
         *q++ = escchar;
         if (q < end)
            *q++ = *p++;
      } else
         *q++ = *p++;
   }
   *q = '\0';

   if (*p != 0)
      return -1;
   return q-dst;
}

//______________________________________________________________________________
static const char *HomeDirectory(const char *name)
{
   // Returns the user's home directory.

   static char path[kMAXPATHLEN], mydir[kMAXPATHLEN];
   struct passwd *pw;

   if (name) {
      pw = getpwnam(name);
      if (pw) {
         strncpy(path, pw->pw_dir, kMAXPATHLEN);
         return path;
      }
   } else {
      if (mydir[0])
         return mydir;
      pw = getpwuid(getuid());
      if (pw) {
         strncpy(mydir, pw->pw_dir, kMAXPATHLEN);
         return mydir;
      }
   }
   return 0;
}

//______________________________________________________________________________
char *RootdExpandPathName(const char *name)
{
   // Expand a pathname getting rid of special shell characters like ~.$, etc.
   // Returned string must be freed by caller.

   const char *patbuf = name;

   // skip leading blanks
   while (*patbuf == ' ')
      patbuf++;

   // any shell meta characters?
   for (const char *p = patbuf; *p; p++)
      if (strchr(shellMeta, *p))
         goto needshell;

   return strdup(name);

needshell:
   // escape shell quote characters
   char escPatbuf[kMAXPATHLEN];
   EscChar(patbuf, escPatbuf, sizeof(escPatbuf), shellStuff, shellEscape);

   char cmd[kMAXPATHLEN];
#ifdef __hpux
   strcpy(cmd, "/bin/echo ");
#else
   strcpy(cmd, "echo ");
#endif

   // emulate csh -> popen executes sh
   if (escPatbuf[0] == '~') {
      const char *hd;
      if (escPatbuf[1] != '\0' && escPatbuf[1] != '/') {
         // extract user name
         char uname[70], *p, *q;
         for (p = &escPatbuf[1], q = uname; *p && *p !='/';)
            *q++ = *p++;
         *q = '\0';
         hd = HomeDirectory(uname);
         if (hd == 0)
            strcat(cmd, escPatbuf);
         else {
            strcat(cmd, hd);
            strcat(cmd, p);
         }
      } else {
         hd = HomeDirectory(0);
         if (hd == 0) {
            ErrorSys(kErrFatal, "RootdExpandPathName: no home directory");
            return 0;
         }
         strcat(cmd, hd);
         strcat(cmd, &escPatbuf[1]);
      }
   } else
      strcat(cmd, escPatbuf);

   FILE *pf;
   if ((pf = ::popen(&cmd[0], "r")) == 0) {
      ErrorSys(kErrFatal, "RootdExpandPathName: error in popen(%s)", cmd);
      return 0;
   }

   // read first argument
   char expPatbuf[kMAXPATHLEN];
   int  ch, i, cnt = 0;
again:
   for (i = 0, ch = fgetc(pf); ch != EOF && ch != ' ' && ch != '\n'; i++, ch = fgetc(pf)) {
      expPatbuf[i] = ch;
      cnt++;
   }
   // this will be true if forked process was not yet ready to be read
   if (cnt == 0 && ch == EOF) goto again;

   // skip rest of pipe
   while (ch != EOF) {
      ch = fgetc(pf);
      if (ch == ' ' || ch == '\t') {
         ::pclose(pf);
         ErrorFatal(kErrFatal, "RootdExpandPathName: expression ambigous");
         return 0;
      }
   }

   ::pclose(pf);

   return strdup(expPatbuf);
}

//______________________________________________________________________________
int RootdCheckTab(int mode)
{
   // Checks kRootdTab file to see if file can be opened. If mode=1 then
   // check if file can safely be opened in write mode, i.e. see if file
   // is not already opened in either read or write mode. If mode=0 then
   // check if file can safely be opened in read mode, i.e. see if file
   // is not already opened in write mode. Returns 1 if file can be
   // opened safely, otherwise 0. If mode is -1 check write mode like 1
   // but do not update rootdtab file.
   //
   // The format of the file is:
   // filename inode mode username pid
   // where inode is the unique file ref number, mode is either "read"
   // or "write", username the user who has the file open and pid is the
   // pid of the rootd having the file open.

   // Open rootdtab file. Try first /usr/tmp and then /tmp.
   // The lockf() call can fail if the directory is NFS mounted
   // and the lockd daemon is not running.

   const char *sfile = kRootdTab;
   int fid, create = 0;

   int noupdate = 0;
   if (mode < 0) {
      mode = 1;
      noupdate = 1;
   }

again:
   if (access(sfile, F_OK) == -1) {
      fid = open(sfile, O_CREAT|O_RDWR, 0644);
      if (fid != -1) fchmod(fid, 0666);    // override umask setting
      create = 1;
   } else
      fid = open(sfile, O_RDWR);

   if (fid == -1) {
      if (sfile[1] == 'u') {
         sfile = kRootdTab+4;
         goto again;
      }
      ErrorSys(kErrFatal, "RootdCheckTab: error opening %s", sfile);
   }

   // lock the file
   if (lockf(fid, F_LOCK, (off_t)1) == -1) {
      if (sfile[1] == 'u' && create) {
         close(fid);
         remove(sfile);
         sfile = kRootdTab+4;
         goto again;
      }
      ErrorSys(kErrFatal, "RootdCheckTab: error locking %s", sfile);
   }
   if (gDebug > 2)
      ErrorInfo("RootdCheckTab: file %s locked", sfile);

   struct stat sbuf;
   fstat(fid, &sbuf);
   size_t siz = sbuf.st_size;

   ino_t inode;
   if (stat(gFile, &sbuf) == -1)
      inode = 0;
   else
      inode = sbuf.st_ino;

   char msg[kMAXPATHLEN];
   const char *smode = (mode == 1) ? "write" : "read";
   int result = 1;

   if (siz > 0) {
      int changed = 0;
      char *fbuf = new char[siz+1];
      char *flast = fbuf + siz;

      while (read(fid, fbuf, siz) < 0 && GetErrno() == EINTR)
         ResetErrno();
      fbuf[siz] = 0;

      char *n, *s = fbuf;
      while ((n = strchr(s, '\n')) && siz > 0) {
         n++;
         char user[64], gmode[32];
         int  pid;
         unsigned long ino;
         sscanf(s, "%s %lu %s %s %d", msg, &ino, gmode, user, &pid);
         if (kill(pid, 0) == -1 && GetErrno() == ESRCH) {
            ErrorInfo("Remove Stale Lock (%s %u %s %s %d)\n", msg, ino, gmode, user, pid);
            if (n >= flast) {
               siz = int(s - fbuf);
               changed = 1;
               break;
            } else {
               int l = int(flast - n) + 1;
               memmove(s, n, l);
               siz -= int(n - s);
               n = s;
            }
            flast = fbuf + siz;
            changed = 1;
         } else if (ino == inode) {
            if (mode == 1)
               result = 0;
            else if (!strcmp(gmode, "write"))
               result = 0;
         }
         s = n;
      }
      if (changed) {
         ftruncate(fid, 0);
         lseek(fid, 0, SEEK_SET);
         if (siz > 0) {
            while (write(fid, fbuf, siz) < 0 && GetErrno() == EINTR)
               ResetErrno();
         }
      }
      delete [] fbuf;
   }

   if (result && !noupdate) {
      unsigned long ino = inode;
      sprintf(msg, "%s %lu %s %s %d\n", gFile, ino, smode, gUser, (int) getpid());
      write(fid, msg, strlen(msg));
   }

   // unlock the file
   lseek(fid, 0, SEEK_SET);
   if (lockf(fid, F_ULOCK, (off_t)1) == -1)
      ErrorSys(kErrFatal, "RootdCheckTab: error unlocking %s", sfile);
   if (gDebug > 2)
      ErrorInfo("RootdCheckTab: file %s unlocked", sfile);

   close(fid);

   return result;
}

//______________________________________________________________________________
void RootdCloseTab(int force=0)
{
   // Removes from the kRootdTab file the reference to gFile for the
   // current rootd. If force = 1, then remove all references for gFile
   // from the kRootdTab file. This might be necessary in case something
   // funny happened and the original reference was not correctly removed.
   // Stale locks are detected by checking each pid and then removed.

   const char *sfile = kRootdTab;
   int fid;

again:
   if (access(sfile, F_OK) == -1) {
      if (sfile[1] == 'u') {
         sfile = kRootdTab+4;
         goto again;
      }
      ErrorInfo("RootdCloseTab: file %s does not exist", sfile);
      return;
   }

   fid = open(sfile, O_RDWR);

   if (fid == -1) {
      ErrorInfo("RootdCloseTab: error opening %s", sfile);
      return;
   }

   // lock the file
   if (lockf(fid, F_LOCK, (off_t)1) == -1) {
      ErrorInfo("RootdCloseTab: error locking %s", sfile);
      return;
   }
   if (gDebug > 2)
      ErrorInfo("RootdCloseTab: file %s locked", sfile);

   struct stat sbuf;
   fstat(fid, &sbuf);
   size_t siz = sbuf.st_size;

   stat(gFile, &sbuf);
   ino_t inode = sbuf.st_ino;

   if (siz > 0) {
      int changed = 0;
      int mypid   = getpid();
      char *fbuf  = new char[siz+1];
      char *flast = fbuf + siz;

      while (read(fid, fbuf, siz) < 0 && GetErrno() == EINTR)
         ResetErrno();
      fbuf[siz] = 0;

      char *n, *s = fbuf;
      while ((n = strchr(s, '\n')) && siz > 0) {
         n++;
         char msg[kMAXPATHLEN], user[64], gmode[32];
         int  pid, stale = 0;
         unsigned int ino;
         sscanf(s, "%s %u %s %s %d", msg, &ino, gmode, user, &pid);
         if (kill(pid, 0) == -1 && GetErrno() == ESRCH) {
            stale = 1;
            ErrorInfo("Remove Stale Lock (%s %u %s %s %d)\n", msg, ino, gmode, user, pid);
         }
         if (stale || (!force && mypid == pid) ||
             (force && inode == ino && !strcmp(gUser, user))) {
            if (n >= flast) {
               siz = int(s - fbuf);
               changed = 1;
               break;
            } else {
               int l = int(flast - n) + 1;
               memmove(s, n, l);
               siz -= int(n - s);
               n = s;
            }
            flast = fbuf + siz;
            changed = 1;
         }
         s = n;
      }
      if (changed) {
         ftruncate(fid, 0);
         lseek(fid, 0, SEEK_SET);
         if (siz > 0) {
            while (write(fid, fbuf, siz) < 0 && GetErrno() == EINTR)
               ResetErrno();
         }
      }
      delete [] fbuf;
   }

   // unlock the file
   lseek(fid, 0, SEEK_SET);
   if (lockf(fid, F_ULOCK, (off_t)1) == -1) {
      ErrorInfo("RootdCloseTab: error unlocking %s", sfile);
      return;
   }
   if (gDebug > 2)
      ErrorInfo("RootdCloseTab: file %s unlocked", sfile);

   close(fid);
}

//______________________________________________________________________________
int RootdIsOpen()
{
   if (gFd == -1) return 0;
   return 1;
}

//______________________________________________________________________________
void RootdCloseFtp()
{
   if (gDebug > 0)
      ErrorInfo("RootdCloseFtp: %d files uploaded, %d files downloaded, rd=%g, wr=%g, rx=%g, tx=%g",
                gUploaded, gDownloaded, gBytesRead, gBytesWritten, gBytesRecv, gBytesSent);
   else
      ErrorInfo("Rootd: %d files uploaded, %d files downloaded, rd=%g, wr=%g, rx=%g, tx=%g",
                gUploaded, gDownloaded, gBytesRead, gBytesWritten, gBytesRecv, gBytesSent);
}

//______________________________________________________________________________
void RootdClose()
{
   if (gFtp) {
      RootdCloseFtp();
      return;
   }

   if (RootdIsOpen()) {
      close(gFd);
      gFd = -1;
   }

   RootdCloseTab();

   if (gDebug > 0)
      ErrorInfo("RootdClose: file %s closed, rd=%g, wr=%g, rx=%g, tx=%g",
                gFile, gBytesRead, gBytesWritten, gBytesRecv, gBytesSent);
   else
      ErrorInfo("Rootd: file %s closed, rd=%g, wr=%g, rx=%g, tx=%g", gFile,
                gBytesRead, gBytesWritten, gBytesRecv, gBytesSent);
}

//______________________________________________________________________________
void RootdFlush()
{
   if (RootdIsOpen() && gWritable) {
#ifndef WIN32
      if (fsync(gFd) < 0)
         ErrorSys(kErrFatal, "RootdFlush: error flushing file %s", gFile);
#endif
   }

   if (gDebug > 0)
      ErrorInfo("RootdFlush: file %s flushed", gFile);
}

//______________________________________________________________________________
void RootdStat()
{

}

//______________________________________________________________________________
void RootdFstat()
{
   // Return file stat information in same format as TSystem::GetPathInfo().

   char msg[128];
   long id, size, flags, modtime;
   struct stat statbuf;

   if (RootdIsOpen() && fstat(gFd, &statbuf) >= 0) {
#if defined(__KCC) && defined(linux)
      id = (statbuf.st_dev.__val[0] << 24) + statbuf.st_ino;
#else
      id = (statbuf.st_dev << 24) + statbuf.st_ino;
#endif
      size = statbuf.st_size;
      modtime = statbuf.st_mtime;
      flags = 0;
      if (statbuf.st_mode & ((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6)))
         flags |= 1;
      if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
         flags |= 2;
      if ((statbuf.st_mode & S_IFMT) != S_IFREG &&
          (statbuf.st_mode & S_IFMT) != S_IFDIR)
         flags |= 4;
      sprintf(msg, "%ld %ld %ld %ld", id, size, flags, modtime);
   } else
      sprintf(msg, "-1 -1 -1 -1");
   NetSend(msg, kROOTD_FSTAT);
}

//______________________________________________________________________________
void RootdProtocol()
{
   // Return rootd protocol version id.

   NetSend(gProtocol, kROOTD_PROTOCOL);
}

//______________________________________________________________________________
void RootdLogin()
{
   // Authentication was successful, set user environment.

   struct passwd *pw = getpwnam(gUser);

   gAuth = 1;

   if (chdir(pw->pw_dir) == -1)
      ErrorFatal(kErrFatal, "RootdLogin: can't change directory to %s",
                 pw->pw_dir);

   if (getuid() == 0) {
      if (gAnon && chroot(pw->pw_dir) == -1)
         ErrorFatal(kErrFatal, "RootdLogin: can't chroot to %s", pw->pw_dir);

      // set access control list from /etc/initgroup
      initgroups(gUser, pw->pw_gid);

      if (setresgid(pw->pw_gid, pw->pw_gid, 0) == -1)
         ErrorFatal(kErrFatal, "RootdLogin: can't setgid for user %s", gUser);

      if (setresuid(pw->pw_uid, pw->pw_uid, 0) == -1)
         ErrorFatal(kErrFatal, "RootdLogin: can't setuid for user %s", gUser);
   }

   umask(022);

   NetSend(gAuth, kROOTD_AUTH);

   if (gDebug > 0) {
      if (gAnon)
         ErrorInfo("RootdLogin: user %s/%s authenticated", gUser, gPasswd);
      else
         ErrorInfo("RootdLogin: user %s authenticated", gUser);
   }
}

//______________________________________________________________________________
void RootdUser(const char *user)
{
   // Check user id. If user id is not equal to rootd's effective uid, user
   // will not be allowed access, unless effective uid = 0 (i.e. root).

   if (!*user)
      ErrorFatal(kErrBadUser, "RootdUser: bad user name");

   ERootdErrors err = kErrNoUser;

   if (!strcmp(user, "anonymous") || !strcmp(user, "rootd")) {
      user  = "rootd";
      err   = kErrNoAnon;
      gAnon = 1;
   }

   struct passwd *pw;
   if ((pw = getpwnam(user)) == 0)
      ErrorFatal(err, "RootdUser: user %s unknown", user);

   // If server is not started as root and user is not same as the
   // one who started rootd then authetication is not ok.
   uid_t uid = getuid();
   if (uid && uid != pw->pw_uid)
      ErrorFatal(kErrBadUser, "RootdUser: user not same as effective user of rootd");

   strcpy(gUser, user);

   NetSend(gAuth, kROOTD_AUTH);
}

//______________________________________________________________________________
void RootdKrb5Auth()
{
   // Authenticate via Kerberos.

#ifdef R__KRB5
   NetSend(1, kROOTD_KRB5);
   // TAuthenticate will respond to our encouragement by sending krb5
   // authentication through the socket
#else
   NetSend(0, kROOTD_KRB5);
   return;
#endif

#ifdef R__KRB5
   int retval;

   // get service principal
   krb5_principal server;
   if ((retval = krb5_sname_to_principal(gKcontext, 0, "rootd",
                                         KRB5_NT_SRV_HST, &server)))
      ErrorFatal(kErrFatal, "while generating service name (%s): %s",
                 "rootd", error_message(retval));

   // listen for authentication from the client
   krb5_auth_context auth_context = 0;
   krb5_ticket *ticket;
   char proto_version[100] = "krootd_v_1";
   int sock = gSockFd;
   if ((retval = krb5_recvauth(gKcontext, &auth_context, (krb5_pointer)&sock,
                               proto_version, server,
                               0,
                               gKeytab,    // default gKeytab is 0
                               &ticket)))
      ErrorFatal(kErrFatal, "recvauth failed--%s", error_message(retval));

   // get client name
   char *cname;
   if ((retval = krb5_unparse_name(gKcontext, ticket->enc_part2->client, &cname)))
      ErrorFatal(kErrFatal, "unparse failed: %s", error_message(retval));
   using std::string;
   string user = cname;
   free(cname);
   string reply = "authenticated as ";
   reply += user;

   NetSend(reply.c_str(), kMESS_STRING);

   krb5_auth_con_free(gKcontext, auth_context);

   // set user name
   user = user.erase(user.find("@"));        // cut off realm
   string::size_type pos = user.find("/");   // see if there is an instance
   if (pos != string::npos)
      user = user.erase(pos);                // drop the instance
   NetSend(user.c_str(), kMESS_STRING);

   strncpy(gUser, user.c_str(), 64);

   if (gDebug > 0)
      ErrorInfo("RootdKrb5Auth: user %s authenticated", gUser);

   RootdLogin();
#endif
}

//______________________________________________________________________________
void RootdSRPUser(const char *user)
{
   // Use Secure Remote Password protocol.
   // Check user id in $HOME/.srootdpass file.

   if (!*user)
      ErrorFatal(kErrBadUser, "RootdSRPUser: bad user name");

   if (kSRootdPass[0]) { }  // remove compiler warning

#ifdef R__SRP

   char srootdpass[kMAXPATHLEN], srootdconf[kMAXPATHLEN];

   struct passwd *pw = getpwnam(user);
   if (!pw)
      ErrorFatal(kErrNoUser, "RootdSRPUser: user %s unknown", user);

   // If server is not started as root and user is not same as the
   // one who started rootd then authetication is not ok.
   uid_t uid = getuid();
   if (uid && uid != pw->pw_uid)
      ErrorFatal(kErrBadUser, "RootdSRPUser: user not same as effective user of rootd");

   NetSend(gAuth, kROOTD_AUTH);

   strcpy(gUser, user);

   sprintf(srootdpass, "%s/%s", pw->pw_dir, kSRootdPass);
   sprintf(srootdconf, "%s/%s.conf", pw->pw_dir, kSRootdPass);

   FILE *fp1 = fopen(srootdpass, "r");
   if (!fp1) {
      NetSend(2, kROOTD_AUTH);
      ErrorInfo("RootdSRPUser: error opening %s", srootdpass);
      return;
   }
   FILE *fp2 = fopen(srootdconf, "r");
   if (!fp2) {
      NetSend(2, kROOTD_AUTH);
      ErrorInfo("RootdSRPUser: error opening %s", srootdconf);
      if (fp1) fclose(fp1);
      return;
   }

   struct t_pw *tpw = t_openpw(fp1);
   if (!tpw) {
      NetSend(2, kROOTD_AUTH);
      ErrorInfo("RootdSRPUser: unable to open password file %s", srootdpass);
      fclose(fp1);
      fclose(fp2);
      return;
   }

   struct t_conf *tcnf = t_openconf(fp2);
   if (!tcnf) {
      NetSend(2, kROOTD_AUTH);
      ErrorInfo("RootdSRPUser: unable to open configuration file %s", srootdconf);
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
   if (!ts)
      ErrorFatal(kErrNoUser, "RootdSRPUser: user %s not found SRP password file", gUser);

   if (tcnf) t_closeconf(tcnf);
   if (tpw)  t_closepw(tpw);
   if (fp2)  fclose(fp2);
   if (fp1)  fclose(fp1);

   char hexbuf[MAXHEXPARAMLEN];

   // send n to client
   NetSend(t_tob64(hexbuf, (char*)ts->n.data, ts->n.len), kROOTD_SRPN);
   // send g to client
   NetSend(t_tob64(hexbuf, (char*)ts->g.data, ts->g.len), kROOTD_SRPG);
   // send salt to client
   NetSend(t_tob64(hexbuf, (char*)ts->s.data, ts->s.len), kROOTD_SRPSALT);

   struct t_num *B = t_servergenexp(ts);

   // receive A from client
   EMessageTypes kind;
   if (NetRecv(hexbuf, MAXHEXPARAMLEN, kind) < 0)
      ErrorFatal(kErrFatal, "RootdSRPUser: error receiving A from client");
   if (kind != kROOTD_SRPA)
      ErrorFatal(kErrFatal, "RootdSRPUser: expected kROOTD_SRPA message");

   unsigned char buf[MAXPARAMLEN];
   struct t_num A;
   A.data = buf;
   A.len  = t_fromb64((char*)A.data, hexbuf);

   // send B to client
   NetSend(t_tob64(hexbuf, (char*)B->data, B->len), kROOTD_SRPB);

   t_servergetkey(ts, &A);

   // receive response from client
   if (NetRecv(hexbuf, MAXHEXPARAMLEN, kind) < 0)
      ErrorFatal(kErrFatal, "RootdSRPUser: error receiving response from client");
   if (kind != kROOTD_SRPRESPONSE)
      ErrorFatal(kErrFatal, "RootdSRPUser: expected kROOTD_SRPRESPONSE message");

   unsigned char cbuf[20];
   t_fromhex((char*)cbuf, hexbuf);

   if (!t_serververify(ts, cbuf)) {
      // authentication successful

      if (gDebug > 0)
         ErrorInfo("RootdSRPUser: user %s authenticated", gUser);

      RootdLogin();

   } else
      ErrorFatal(kErrBadPasswd, "RootdSRPUser: authentication failed for user %s", gUser);

   t_serverclose(ts);

#else
   NetSend(2, kROOTD_AUTH);
#endif
}

//______________________________________________________________________________
int RootdCheckSpecialPass(const char *passwd)
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
   if ((n = read(fid, rootdpass, sizeof(rootdpass)-1)) <= 0) {
      close(fid);
      return 0;
   }
   close(fid);

   rootdpass[n] = 0;
   char *s = strchr(rootdpass, '\n');
   if (s) *s = 0;

   char *pass_crypt = crypt(passwd, rootdpass);
   n = strlen(rootdpass);

   if (strncmp(pass_crypt, rootdpass, n+1) != 0)
      return 0;

   if (gDebug > 0)
      ErrorInfo("RootdCheckSpecialPass: user %s authenticated via ~/.rootdpass", gUser);

   return 1;
}

//______________________________________________________________________________
void RootdPass(const char *pass)
{
   // Check user's password.

   char   passwd[64];
   char  *passw;
   char  *pass_crypt;
   struct passwd *pw;
#ifdef R__SHADOWPW
   struct spwd *spw;
#endif
#ifdef R__AFS
   char  *reason;
   int    afs_auth = 0;
#endif

   if (!*gUser)
      ErrorFatal(kErrFatal, "RootdPass: user needs to be specified first");

   int i;
   int n = strlen(pass);

   if (!n)
      ErrorFatal(kErrBadPasswd, "RootdPass: null passwd not allowed");

   if (n > (int)sizeof(passwd))
      ErrorFatal(kErrBadPasswd, "RootdPass: passwd too long");

   for (i = 0; i < n; i++)
      passwd[i] = ~pass[i];
   passwd[i] = '\0';

   pw = getpwnam(gUser);

   if (gAnon) {
      strcpy(gPasswd, passwd);
      RootdLogin();
      return;
   }

   if (RootdCheckSpecialPass(passwd)) {
      RootdLogin();
      return;
   }

#ifdef R__AFS
   afs_auth = !ka_UserAuthenticateGeneral(
        KA_USERAUTH_VERSION + KA_USERAUTH_DOSETPAG,
        gUser,             //user name
        (char *) 0,        //instance
        (char *) 0,        //realm
        passwd,            //password
        0,                 //default lifetime
        0, 0,              //two spares
        &reason);          //error string

   if (!afs_auth) {
      ErrorInfo("RootdPass: AFS login failed for user %s: %s", gUser, reason);
      // try conventional login...
#endif

#ifdef R__SHADOWPW
   // System V Rel 4 style shadow passwords
   if ((spw = getspnam(gUser)) == 0) {
      ErrorInfo("RootdPass: Shadow passwd not available for user %s", gUser);
      passw = pw->pw_passwd;
   } else
      passw = spw->sp_pwdp;
#else
   passw = pw->pw_passwd;
#endif
   pass_crypt = crypt(passwd, passw);
   n = strlen(passw);

   if (strncmp(pass_crypt, passw, n+1) != 0)
      ErrorFatal(kErrBadPasswd, "RootdPass: invalid password for user %s", gUser);

#ifdef R__AFS
   }  // afs_auth
#endif

   RootdLogin();
}

//______________________________________________________________________________
void RootdOpen(const char *msg)
{
   // Open file in mode depending on specified option. If file is already
   // opened by another rootd in write mode, do not open the file.

   char file[kMAXPATHLEN], option[32];

   sscanf(msg, "%s %s", file, option);

   if (file[0] == '/')
      strcpy(gFile, &file[1]);
   else
      strcpy(gFile, file);

   strcpy(gOption, option);

   int forceOpen = 0;
   if (option[0] == 'f') {
      forceOpen = 1;
      strcpy(gOption, &option[1]);
   }

   int forceRead = 0;
   if (!strcmp(option, "+read")) {
      forceRead = 1;
      strcpy(gOption, &option[1]);
   }

   int create = 0;
   if (!strcmp(gOption, "new") || !strcmp(gOption, "create"))
      create = 1;
   int recreate = strcmp(gOption, "recreate") ? 0 : 1;
   int update   = strcmp(gOption, "update")   ? 0 : 1;
   int read     = strcmp(gOption, "read")     ? 0 : 1;
   if (!create && !recreate && !update && !read) {
      read = 1;
      strcpy(gOption, "read");
   }

   if (!gAnon) {
      char *fname;
      if ((fname = RootdExpandPathName(gFile))) {
         strcpy(gFile, fname);
         free(fname);
      } else
         ErrorFatal(kErrBadFile, "RootdOpen: bad file name %s", gFile);
   }

   if (forceOpen)
      RootdCloseTab(1);

   if (recreate) {
      if (!RootdCheckTab(-1))
         ErrorFatal(kErrFileWriteOpen, "RootdOpen: file %s already opened in read or write mode", gFile);
      if (!access(gFile, F_OK))
         unlink(gFile);
      recreate = 0;
      create   = 1;
      strcpy(gOption, "create");
   }

   if (create && !access(gFile, F_OK))
      ErrorFatal(kErrFileExists, "RootdOpen: file %s already exists", gFile);

   if (update) {
      if (access(gFile, F_OK)) {
         update = 0;
         create = 1;
      }
      if (update && access(gFile, W_OK))
         ErrorFatal(kErrNoAccess, "RootdOpen: no write permission for file %s", gFile);
   }

   if (read) {
      if (access(gFile, F_OK))
         ErrorFatal(kErrNoFile, "RootdOpen: file %s does not exist", gFile);
      if (access(gFile, R_OK))
         ErrorFatal(kErrNoAccess, "RootdOpen: no read permission for file %s", gFile);
   }

   if (create || update) {
      if (create) {
         // make sure file exists so RootdCheckTab works correctly
#ifndef WIN32
         gFd = open(gFile, O_RDWR | O_CREAT, 0644);
#else
         gFd = open(gFile, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
#endif
         close(gFd);
         gFd = -1;
      }
#ifndef WIN32
      gFd = open(gFile, O_RDWR, 0644);
#else
      gFd = open(gFile, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
#endif
      if (gFd == -1)
         ErrorSys(kErrFileOpen, "RootdOpen: error opening file %s in write mode", gFile);

      if (!RootdCheckTab(1)) {
         close(gFd);
         ErrorFatal(kErrFileWriteOpen, "RootdOpen: file %s already opened in read or write mode", gFile);
      }

      gWritable = 1;

   } else {
#ifndef WIN32
      gFd = open(gFile, O_RDONLY);
#else
      gFd = open(gFile, O_RDONLY | O_BINARY);
#endif
      if (gFd == -1)
         ErrorSys(kErrFileOpen, "RootdOpen: error opening file %s in read mode", gFile);

      if (!RootdCheckTab(0)) {
         if (!forceRead) {
            close(gFd);
            ErrorFatal(kErrFileReadOpen, "RootdOpen: file %s already opened in write mode", gFile);
         }
      }

      gWritable = 0;

   }

   NetSend(gWritable, kROOTD_OPEN);

   if (gDebug > 0)
      ErrorInfo("RootdOpen: file %s opened in mode %s", gFile, gOption);
   else {
      if (gAnon)
         ErrorInfo("Rootd: file %s (%s) opened by %s/%s", gFile, gOption,
                   gUser, gPasswd);
      else
         ErrorInfo("Rootd: file %s (%s) opened by %s", gFile, gOption, gUser);
   }
}

//______________________________________________________________________________
void RootdPut(const char *msg)
{
   // Receive a buffer and write it at the specified offset in the currently
   // open file.

   long  offsetl;
   int   len;
   off_t offset;

   sscanf(msg, "%ld %d", &offsetl, &len);

   offset = (off_t) offsetl;

   char *buf = new char[len];
   NetRecvRaw(buf, len);

   if (!RootdIsOpen() || !gWritable)
      ErrorFatal(kErrNoAccess, "RootdPut: file %s not opened in write mode", gFile);

   if (lseek(gFd, offset, SEEK_SET) < 0)
      ErrorSys(kErrFilePut, "RootdPut: cannot seek to position %d in file %s", offset, gFile);

   ssize_t siz;
   while ((siz = write(gFd, buf, len)) < 0 && GetErrno() == EINTR)
      ResetErrno();

   if (siz < 0)
      ErrorSys(kErrFilePut, "RootdPut: error writing to file %s", gFile);

   if (siz != len)
      ErrorFatal(kErrFilePut, "RootdPut: error writing all requested bytes to file %s, wrote %d of %d",
                 gFile, siz, len);

   NetSend(0, kROOTD_PUT);

   delete [] buf;

   gBytesWritten += len;

   if (gDebug > 0)
      ErrorInfo("RootdPut: written %d bytes starting at %d to file %s",
                len, offset, gFile);
}

//______________________________________________________________________________
void RootdGet(const char *msg)
{
   // Get a buffer from the specified offset from the currently open file
   // and send it to the client.

   long  offsetl;
   int   len;
   off_t offset;

   sscanf(msg, "%ld %d", &offsetl, &len);

   offset = (off_t) offsetl;

   char *buf = new char[len];

   if (!RootdIsOpen())
      ErrorFatal(kErrNoAccess, "RootdGet: file %s not open", gFile);

   if (lseek(gFd, offset, SEEK_SET) < 0)
      ErrorSys(kErrFileGet, "RootdGet: cannot seek to position %d in file %s", offset, gFile);

   ssize_t siz;
   while ((siz = read(gFd, buf, len)) < 0 && GetErrno() == EINTR)
      ResetErrno();

   if (siz < 0)
      ErrorSys(kErrFileGet, "RootdGet: error reading from file %s", gFile);

   if (siz != len)
      ErrorFatal(kErrFileGet, "RootdGet: error reading all requested bytes from file %s, got %d of %d",
                 gFile, siz, len);

   NetSend(0, kROOTD_GET);

   NetSendRaw(buf, len);

   delete [] buf;

   gBytesRead += len;

   if (gDebug > 0)
      ErrorInfo("RootdGet: read %d bytes starting at %d from file %s",
                len, offset, gFile);
}

//______________________________________________________________________________
void RootdPutFile(const char *msg)
{
   // Receive a file from the remote client (upload).

   char  file[kMAXPATHLEN];
   long  size, restartatl;
   int   blocksize, mode, forceopen = 0;
   off_t restartat;

   gFtp = 1;   // rootd is used for ftp instead of file serving

   sscanf(msg, "%s %d %d %ld %ld", file, &blocksize, &mode, &size, &restartatl);

   if (file[0] == '-') {
      forceopen = 1;
      strcpy(gFile, file+1);
   } else
      strcpy(gFile, file);

   restartat = (off_t) restartatl;

   // anon user may not overwrite existing files...
   struct stat st;
   if (!stat(gFile, &st)) {
      if (gAnon) {
         Error(kErrFileExists, "RootdPutFile: anonymous users may not overwrite existing file %s", gFile);
         return;
      }
   } else if (GetErrno() != ENOENT) {
      Error(kErrFatal, "RootdPutFile: can't check for file presence");
      return;
   }

   // remove lock from file
   if (restartat || forceopen)
      RootdCloseTab(1);

   // open local file
   int fd;
   if (!restartat) {

      // make sure file exists so RootdCheckTab works correctly
#ifndef WIN32
      fd = open(gFile, O_RDWR | O_CREAT, 0600);
#else
      fd = open(gFile, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
#endif
      if (fd < 0) {
         Error(kErrFileOpen, "RootdPutFile: cannot open file %s", gFile);
         return;
      }

      close(fd);

      // check if file is not in use by somebody and prevent from somebody
      // using it before upload is completed
      if (!RootdCheckTab(1)) {
         Error(kErrFileWriteOpen, "RootdPutFile: file %s already opened in read or write mode", gFile);
         return;
      }

#ifndef WIN32
      fd = open(gFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
#else
      if (mode == kBinary)
         fd = open(gFile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
                   S_IREAD | S_IWRITE);
      else
         fd = open(gFile, O_CREAT | O_TRUNC | O_WRONLY,
                   S_IREAD | S_IWRITE);
#endif
   } else {
#ifndef WIN32
      fd = open(gFile, O_WRONLY, 0600);
#else
      if (mode == kBinary)
         fd = open(gFile, O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
      else
         fd = open(gFile, O_WRONLY, S_IREAD | S_IWRITE);
#endif
      if (fd < 0) {
         Error(kErrFileOpen, "RootdPutFile: cannot open file %s", gFile);
         return;
      }
      if (!RootdCheckTab(1)) {
         close(fd);
         Error(kErrFileWriteOpen, "RootdPutFile: file %s already opened in read or write mode", gFile);
         return;
      }
   }

   // check file system space
   if (strcmp(gFile, "/dev/null")) {
      struct statfs statfsbuf;
#if defined(__sgi) || (defined(__sun) && !defined(linux))
      if (fstatfs(fd, &statfsbuf, sizeof(struct statfs), 0) == 0) {
         double space = (double)statfsbuf.f_bsize * (double)statfsbuf.f_bfree;
#else
      if (fstatfs(fd, &statfsbuf) == 0) {
         double space = (double)statfsbuf.f_bsize * (double)statfsbuf.f_bavail;
#endif
         if (space < size - restartat) {
            Error(kErrNoSpace, "RootdPutFile: not enough space to store file %s", gFile);
            close(fd);
            return;
         }
      }
   }

   // seek to restartat position
   if (restartat) {
      if (lseek(fd, restartat, SEEK_SET) < 0) {
         Error(kErrRestartSeek, "RootdPutFile: cannot seek to position %ld in file %s",
               (long)restartat, gFile);
         close(fd);
         return;
      }
   }

   // setup ok
   NetSend(0, kROOTD_PUTFILE);

   struct timeval started, ended;
   gettimeofday(&started, 0);

   char *buf = new char[blocksize];
   char *buf2 = 0;
   if (mode == 1)
      buf2 = new char[blocksize];

   long pos  = restartat & ~(blocksize-1);
   int  skip = int(restartat - pos);

   while (pos < size) {
      long left = size - pos;
      if (left > blocksize)
         left = blocksize;

      NetRecvRaw(buf, int(left-skip));

      int n = int(left-skip);

      // in case of ascii file, loop here over buffer and remove \r's
      ssize_t siz;
      if (mode == kAscii) {
         int i = 0, j = 0;
         while (i < n) {
            if (buf[i] == '\r')
               i++;
            else
               buf2[j++] = buf[i++];
         }
         n = j;
         while ((siz = write(fd, buf2, n)) < 0 && GetErrno() == EINTR)
            ResetErrno();
      } else {
         while ((siz = write(fd, buf, n)) < 0 && GetErrno() == EINTR)
            ResetErrno();
      }

      if (siz < 0)
         ErrorSys(kErrFilePut, "RootdPutFile: error writing to file %s", gFile);

      if (siz != n)
         ErrorFatal(kErrFilePut, "RootdPutFile: error writing all requested bytes to file %s, wrote %d of %d",
                    gFile, siz, int(left-skip));

      gBytesWritten += n;

      pos += left;
      skip = 0;
   }

   gettimeofday(&ended, 0);

   // file stored ok
   NetSend(0, kROOTD_PUTFILE);

   delete [] buf; delete [] buf2;

   fchmod(fd, 0644);

   close(fd);

   RootdCloseTab();

   gUploaded++;

   double speed, t;
   t = (ended.tv_sec + ended.tv_usec / 1000000.0) -
       (started.tv_sec + started.tv_usec / 1000000.0);
   if (t > 0)
      speed = (size - restartat) / t;
   else
      speed = 0.0;
   if (speed > 524288)
      ErrorInfo("RootdPutFile: uploaded file %s (%ld bytes, %.3f seconds, "
                "%.2f Mbytes/s)", gFile, size, t, speed / 1048576);
   else if (speed > 512)
      ErrorInfo("RootdPutFile: uploaded file %s (%ld bytes, %.3f seconds, "
                "%.2f Kbytes/s)", gFile, size, t, speed / 1024);
   else
      ErrorInfo("RootdPutFile: uploaded file %s (%ld bytes, %.3f seconds, "
                "%.2f bytes/s)", gFile, size, t, speed);
}

//______________________________________________________________________________
void RootdGetFile(const char *msg)
{
   // Send a file to a remote client (download).

   char  file[kMAXPATHLEN];
   long  restartatl;
   int   blocksize, mode, forceopen = 0;
   off_t restartat;

   gFtp = 1;   // rootd is used for ftp instead of file serving

   sscanf(msg, "%s %d %d %ld", file, &blocksize, &mode, &restartatl);

   if (file[0] == '-') {
      forceopen = 1;
      strcpy(gFile, file+1);
   } else
      strcpy(gFile, file);

   restartat = (off_t) restartatl;

   // remove lock from file
   if (forceopen)
      RootdCloseTab(1);

   // open file for reading
#ifndef WIN32
   int fd = open(gFile, O_RDONLY);
#else
   int fd = open(gFile, O_RDONLY | O_BINARY);
#endif
   if (fd < 0) {
      Error(kErrFileOpen, "RootdGetFile: cannot open file %s", gFile);
      return;
   }

   // check if file is not in use by somebody and prevent from somebody
   // using it before download is completed
   if (!RootdCheckTab(0)) {
      close(fd);
      Error(kErrFileOpen, "RootdGetFile: file %s is already open in write mode", gFile);
      return;
   }

   struct stat st;
   if (fstat(fd, &st)) {
      Error(kErrFatal, "RootdGetFile: cannot get size of file %s", gFile);
      close(fd);
      return;
   }
   long size = st.st_size;

   if (!S_ISREG(st.st_mode)) {
      Error(kErrBadFile, "RoodGetFile: not a regular file %s", gFile);
      close(fd);
      return;
   }

   // check if restartat value makes sense
   if (restartat && (restartat >= size))
      restartat = 0;

   // setup ok
   NetSend(0, kROOTD_GETFILE);

   char mess[64];
   sprintf(mess, "%ld", size);
   NetSend(mess, kROOTD_GETFILE);

   struct timeval started, ended;
   gettimeofday(&started, 0);

   long pos  = restartat & ~(blocksize-1);
   int  skip = int(restartat - pos);

#ifndef HAVE_MMAP
   char *buf = new char[blocksize];
   lseek(fd, (off_t) pos, SEEK_SET);
#endif

   while (pos < size) {
      long left = size - pos;
      if (left > blocksize)
         left = blocksize;
#ifdef HAVE_MMAP
      char *buf = (char*) mmap(0, (size_t) left, PROT_READ, MAP_FILE | MAP_SHARED,
                               fd, (off_t) pos);
      if (buf == (char *) -1)
         ErrorFatal(kErrFileGet, "RootdGetFile: mmap of file %s failed", gFile);
#else
      int siz;
      while ((siz = read(fd, buf, (int)left)) < 0 && GetErrno() == EINTR)
         ResetErrno();
      if (siz < 0 || siz != (int)left)
         ErrorFatal(kErrFileGet, "RootdGetFile: error reading from file %s", gFile);
#endif

      NetSendRaw(buf+skip, int(left-skip));

      gBytesRead += left-skip;

      pos += left;
      skip = 0;

#ifdef HAVE_MMAP
      munmap(buf, left);
#endif
   }

   gettimeofday(&ended, 0);

#ifndef HAVE_MMAP
   delete [] buf;
#endif

   close(fd);

   RootdCloseTab();

   gDownloaded++;

   double speed, t;
   t = (ended.tv_sec + ended.tv_usec / 1000000.0) -
       (started.tv_sec + started.tv_usec / 1000000.0);
   if (t > 0)
      speed = (size - restartat) / t;
   else
      speed = 0.0;
   if (speed > 524288)
      ErrorInfo("RootdGetFile: downloaded file %s (%ld bytes, %.3f seconds, "
                "%.2f Mbytes/s)", gFile, size, t, speed / 1048576);
   else if (speed > 512)
      ErrorInfo("RootdGetFile: downloaded file %s (%ld bytes, %.3f seconds, "
                "%.2f Kbytes/s)", gFile, size, t, speed / 1024);
   else
      ErrorInfo("RootdGetFile: downloaded file %s (%ld bytes, %.3f seconds, "
                "%.2f bytes/s)", gFile, size, t, speed);
}

//______________________________________________________________________________
void RootdChdir(const char *dir)
{
   // Change directory.

   char buffer[kMAXPATHLEN + 256];

   if (dir && *dir == '~') {
      struct passwd *pw;
      int i = 0;
      const char *p = dir;

      p++;
      while (*p && *p != '/')
         buffer[i++] = *p++;
      buffer[i] = 0;

      if ((pw = getpwnam(i ? buffer : gUser)))
         sprintf(buffer, "%s%s", pw->pw_dir, p);
      else
         *buffer = 0;
   } else
      *buffer = 0;

   if (chdir(*buffer ? buffer : (dir && *dir ? dir : "/")) == -1) {
      sprintf(buffer, "cannot change directory to %s", dir);
      Perror(buffer);
      NetSend(buffer, kROOTD_CHDIR);
      return;
   } else {
      FILE *msg;

      if ((msg = fopen(".message", "r"))) {
         int len = fread(buffer, 1, kMAXPATHLEN, msg);
         fclose(msg);
         if (len > 0 && len < 1024) {
            buffer[len] = 0;
            NetSend(buffer, kMESS_STRING);
         }
      }

      if (!getcwd(buffer, kMAXPATHLEN)) {
         if (*dir == '/')
            sprintf(buffer, "%s", dir);
      }
      NetSend(buffer, kROOTD_CHDIR);
   }
}

//______________________________________________________________________________
void RootdMkdir(const char *dir)
{
   // Make directory.

   char buffer[kMAXPATHLEN];

   if (gAnon) {
      sprintf(buffer, "anonymous users may not create directories");
      ErrorInfo("RootdMkdir: %s", buffer);
   } else if (mkdir(dir, 0755) < 0) {
      sprintf(buffer, "cannot create directory %s", dir);
      Perror(buffer);
      ErrorInfo("RootdMkdir: %s", buffer);
   } else
      sprintf(buffer, "created directory %s", dir);

   NetSend(buffer, kROOTD_MKDIR);
}

//______________________________________________________________________________
void RootdRmdir(const char *dir)
{
   // Delete directory.

   char buffer[kMAXPATHLEN];

   if (gAnon) {
      sprintf(buffer, "anonymous users may not delete directories");
      ErrorInfo("RootdRmdir: %s", buffer);
   } else if (rmdir(dir) < 0) {
      sprintf(buffer, "cannot delete directory %s", dir);
      Perror(buffer);
      ErrorInfo("RootdRmdir: %s", buffer);
   } else
      sprintf(buffer, "deleted directory %s", dir);

   NetSend(buffer, kROOTD_RMDIR);
}

//______________________________________________________________________________
void RootdLsdir(const char *cmd)
{
   // List directory.

   char buffer[kMAXPATHLEN];

   // make sure all commands start with ls (should use snprintf)
   if (gAnon) {
      if (strlen(cmd) < 2 || strncmp(cmd, "ls", 2))
         sprintf(buffer, "ls %s", cmd);
      else
         sprintf(buffer, "%s", cmd);
   } else {
      if (strlen(cmd) < 2 || strncmp(cmd, "ls", 2))
         sprintf(buffer, "ls %s 2>/dev/null", cmd);
      else
         sprintf(buffer, "%s 2>/dev/null", cmd);
   }

   FILE *pf;
   if ((pf = popen(buffer, "r")) == 0) {
      sprintf(buffer, "error in popen");
      Perror(buffer);
      NetSend(buffer, kROOTD_LSDIR);
      ErrorInfo("RootdLsdir: %s", buffer);
      return;
   }

   // read output of ls
   int  ch, i = 0, cnt = 0;
//again:
   for (ch = fgetc(pf); ch != EOF; ch = fgetc(pf)) {
      buffer[i++] = ch;
      cnt++;
      if (i == kMAXPATHLEN-1) {
         buffer[i] = 0;
         NetSend(buffer, kMESS_STRING);
         i = 0;
      }
   }
   // this will be true if forked process was not yet ready to be read
//   if (cnt == 0 && ch == EOF) goto again;

   pclose(pf);

   buffer[i] = 0;
   NetSend(buffer, kROOTD_LSDIR);
}

//______________________________________________________________________________
void RootdPwd()
{
   // Print path of working directory.

   char buffer[kMAXPATHLEN];

   if (!getcwd(buffer, kMAXPATHLEN)) {
      sprintf(buffer, "current directory not readable");
      Perror(buffer);
      ErrorInfo("RootdPwd: %s", buffer);
   }

   NetSend(buffer, kROOTD_PWD);
}

//______________________________________________________________________________
void RootdMv(const char *msg)
{
   // Rename a file.

   char file1[kMAXPATHLEN], file2[kMAXPATHLEN], buffer[kMAXPATHLEN];
   sscanf(msg, "%s %s", file1, file2);

   if (gAnon) {
      sprintf(buffer, "anonymous users may not rename files");
      ErrorInfo("RootdMv: %s", buffer);
   } else if (rename(file1, file2) < 0) {
      sprintf(buffer, "cannot rename file %s to %s", file1, file2);
      Perror(buffer);
      ErrorInfo("RootdMv: %s", buffer);
   } else
      sprintf(buffer, "renamed file %s to %s", file1, file2);

   NetSend(buffer, kROOTD_MV);
}

//______________________________________________________________________________
void RootdRm(const char *file)
{
   // Delete a file.

   char buffer[kMAXPATHLEN];

   if (gAnon) {
      sprintf(buffer, "anonymous users may not delete files");
      ErrorInfo("RootdRm: %s", buffer);
   } else if (unlink(file) < 0) {
      sprintf(buffer, "cannot unlink file %s", file);
      Perror(buffer);
      ErrorInfo("RootdRm: %s", buffer);
   } else
      sprintf(buffer, "removed file %s", file);

   NetSend(buffer, kROOTD_RM);
}

//______________________________________________________________________________
void RootdChmod(const char *msg)
{
   // Delete a file.

   char file[kMAXPATHLEN], buffer[kMAXPATHLEN];
   int  mode;

   sscanf(msg, "%s %d", file, &mode);

   if (gAnon) {
      sprintf(buffer, "anonymous users may not change file permissions");
      ErrorInfo("RootdChmod: %s", buffer);
   } else if (chmod(file, mode) < 0) {
      sprintf(buffer, "cannot chmod file %s to 0%o", file, mode);
      Perror(buffer);
      ErrorInfo("RootdChmod: %s", buffer);
   } else
      sprintf(buffer, "changed permission of file %s to 0%o", file, mode);

   NetSend(buffer, kROOTD_CHMOD);
}

//______________________________________________________________________________
void RootdParallel()
{
   // Handle initialization message from remote host. If size > 0 then
   // so many parallel sockets will be opened to the remote host.

   int buf[3];
   if (NetRecvRaw(buf, sizeof(buf)) < 0)
      ErrorFatal(kErrFatal, "RootdParallel: error receiving message");

   int size = ntohl(buf[1]);
   int port = ntohl(buf[2]);

   if (gDebug > 0)
      ErrorInfo("RootdParallel: port = %d, size = %d", port, size);

   if (size > 0)
      NetParOpen(port, size);
}

//______________________________________________________________________________
void RootdLoop()
{
   // Handle all rootd commands. Returns after file close command.

   const int kMaxBuf = 1024;
   char recvbuf[kMaxBuf];
   EMessageTypes kind;

   while (1) {
      if (NetRecv(recvbuf, kMaxBuf, kind) < 0)
         ErrorFatal(kErrFatal, "RootdLoop: error receiving message");

      if (kind != kROOTD_USER    && kind != kROOTD_PASS &&
          kind != kROOTD_SRPUSER && kind != kROOTD_KRB5 &&
          kind != kROOTD_PROTOCOL && gAuth == 0)
         ErrorFatal(kErrNoUser, "RootdLoop: not authenticated");

      if (gDebug > 2 && kind != kROOTD_PASS)
         ErrorInfo("RootdLoop: %d -- %s", kind, recvbuf);

      switch (kind) {
         case kROOTD_USER:
            RootdUser(recvbuf);
            break;
         case kROOTD_SRPUSER:
            RootdSRPUser(recvbuf);
            break;
         case kROOTD_PASS:
            RootdPass(recvbuf);
            break;
         case kROOTD_KRB5:
            RootdKrb5Auth();
            break;
         case kROOTD_OPEN:
            RootdOpen(recvbuf);
            break;
         case kROOTD_PUT:
            RootdPut(recvbuf);
            break;
         case kROOTD_GET:
            RootdGet(recvbuf);
            break;
         case kROOTD_FLUSH:
            RootdFlush();
            break;
         case kROOTD_CLOSE:
            RootdClose();
            return;
         case kROOTD_FSTAT:
            RootdFstat();
            break;
         case kROOTD_STAT:
            RootdStat();
            break;
         case kROOTD_PROTOCOL:
            RootdProtocol();
            break;
         case kROOTD_PUTFILE:
            RootdPutFile(recvbuf);
            break;
         case kROOTD_GETFILE:
            RootdGetFile(recvbuf);
            break;
         case kROOTD_CHDIR:
            RootdChdir(recvbuf);
            break;
         case kROOTD_MKDIR:
            RootdMkdir(recvbuf);
            break;
         case kROOTD_RMDIR:
            RootdRmdir(recvbuf);
            break;
         case kROOTD_LSDIR:
            RootdLsdir(recvbuf);
            break;
         case kROOTD_PWD:
            RootdPwd();
            break;
         case kROOTD_MV:
            RootdMv(recvbuf);
            break;
         case kROOTD_RM:
            RootdRm(recvbuf);
            break;
         case kROOTD_CHMOD:
            RootdChmod(recvbuf);
            break;
         default:
            ErrorFatal(kErrBadOp, "RootdLoop: received bad opcode %d", kind);
      }
   }
}

//______________________________________________________________________________
int main(int argc, char **argv)
{
   char *s;
   int   tcpwindowsize = 65535;

   ErrorInit(argv[0]);

#ifdef R__KRB5
   const char *kt_fname;

   int retval = krb5_init_context(&gKcontext);
   if (retval)
      ErrorFatal(kErrFatal, "%s while initializing krb5",
                 error_message(retval));
#endif

   while (--argc > 0 && (*++argv)[0] == '-')
      for (s = argv[0]+1; *s != 0; s++)
         switch (*s) {
            case 'i':
               gInetdFlag = 1;
               break;

            case 'p':
               if (--argc <= 0) {
                  if (!gInetdFlag)
                     fprintf(stderr, "-p requires a port number as argument\n");
                  ErrorFatal(kErrFatal, "-p requires a port number as argument");
               }
               gPort = atoi(*++argv);
               break;

            case 'd':
               if (--argc <= 0) {
                  if (!gInetdFlag)
                     fprintf(stderr, "-d requires a debug level as argument\n");
                  ErrorFatal(kErrFatal, "-d requires a debug level as argument");
               }
               gDebug = atoi(*++argv);
               break;

            case 'b':
               if (--argc <= 0) {
                  if (!gInetdFlag)
                     fprintf(stderr, "-b requires a buffersize in bytes as argument\n");
                  ErrorFatal(kErrFatal, "-b requires a buffersize in bytes as argument");
               }
               tcpwindowsize = atoi(*++argv);
               break;

#ifdef R__KRB5
            case 'S':
               if (--argc <= 0) {
                  if (!gInetdFlag)
                     fprintf(stderr, "-S requires a path to your keytab\n");
                  ErrorFatal(kErrFatal, "-S requires a path to your keytab\n");
               }
               kt_fname = *++argv;
               if ((retval = krb5_kt_resolve(gKcontext, kt_fname, &gKeytab)))
                  ErrorFatal(kErrFatal, "%s while resolving keytab file %s",
                             error_message(retval), kt_fname);
               break;
#endif

            default:
               if (!gInetdFlag)
                  fprintf(stderr, "unknown command line option: %c\n", *s);
               ErrorFatal(kErrFatal, "unknown command line option: %c", *s);
         }

   if (!gInetdFlag) {

      // Start rootd up as a daemon process (in the background).
      // Also initialize the network connection - create the socket
      // and bind our well-know address to it.

      DaemonStart(1);

      NetInit(kRootdService, gPort, tcpwindowsize);
   }

   if (gDebug > 0)
      ErrorInfo("main: pid = %d, gInetdFlag = %d, gProtocol = %d",
                getpid(), gInetdFlag, gProtocol);

   // Concurrent server loop.
   // The child created by NetOpen() handles the client's request.
   // The parent waits for another request. In the inetd case,
   // the parent from NetOpen() never returns.

   while (1) {
      if (NetOpen(gInetdFlag) == 0) {
         RootdParallel();  // see if we should use parallel sockets
         RootdLoop();      // child processes client's requests
         NetClose();       // till we are done
         exit(0);
      }

      // parent waits for another client to connect

   }

#ifdef R__KRB5
   // never called... needed?
   krb5_free_context(gKcontext);
#endif

}
