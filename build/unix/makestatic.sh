#! /bin/sh

# Script to generate a archive library and statically linked executable.
# Called by main Makefile.
#
# Author: Fons Rademakers, 21/01/2001

PLATFORM=$1
CXX=$2
CC=$3
LD=$4
LDFLAGS=$5
XLIBS=$6
SYSLIBS=$7

ROOTALIB=lib/libRoot.a
ROOTAEXE=bin/roota
PROOFAEXE=bin/proofserva

rm -f $ROOTALIB $ROOTAEXE $PROOFAEXE

excl="main proof/proofd net/rootd net/xrootd rootx pythia pythia6 \
      mysql pgsql rfio sapdb \
      hbook core/newdelete table core/utils net/srputils net/krb5auth \
      net/globusauth chirp dcache \
      x11ttf net/alien asimage net/ldap pyroot qt qtroot math/quadp \
      ruby vmc xml \
      xmlparser gl roofit roofitcore oracle net/netx net/auth \
      net/rpdutils math/mathmore \
      math/minuit2 gfal net/monalisa proof/proofx math/fftw qtgsi odbc \
      castor math/unuran geom/gdml cint/cint7 g4root eve net/glite"

objs=""
gobjs=""
for i in * ; do
   inc=$i
   for j in $excl ; do
      if [ $j = $i ]; then
         continue 2
      fi
   done
   ls $inc/src/*.o > /dev/null 2>&1 && objs="$objs $inc/src/*.o"
   ls $inc/src/G__*.o > /dev/null 2>&1 && gobjs="$gobjs $inc/src/G__*.o"
   if [ -d $i ]; then
      for k in $i/* ; do
         inc=$k
         for j in $excl ; do
            if [ $j = $k ]; then
               continue 2
            fi
         done
         ls $inc/src/*.o > /dev/null 2>&1 && objs="$objs $inc/src/*.o"
         ls $inc/src/G__*.o > /dev/null 2>&1 && gobjs="$gobjs $inc/src/G__*.o"
      done
   fi
done

echo "Making $ROOTALIB..."
echo ar rv $ROOTALIB cint/cint/main/G__setup.o cint/cint/src/dict/*.o $objs
ar rv $ROOTALIB cint/cint/main/G__setup.o cint/cint/src/dict/*.o $objs > /dev/null 2>&1

arstat=$?
if [ $arstat -ne 0 ]; then
   exit $arstat
fi

dummyc=R__dummy.c
dummyo=""
if [ $PLATFORM = "alpha" ] && [ $CXX = "cxx" ]; then
   echo 'void dnet_conn() {}' > $dummyc
   $CC -c $dummyc
   dummyo=R__dummy.o
fi

echo "Making $ROOTAEXE..."
echo $LD $LDFLAGS -o $ROOTAEXE main/src/rmain.o $dummyo $gobjs $ROOTALIB \
   $XLIBS $SYSLIBS lib/libfreetype.a lib/libpcre.a
$LD $LDFLAGS -o $ROOTAEXE main/src/rmain.o $dummyo $gobjs $ROOTALIB \
   $XLIBS $SYSLIBS lib/libfreetype.a lib/libpcre.a

linkstat=$?
if [ $linkstat -ne 0 ]; then
   exit $linkstat
fi

echo "Making $PROOFAEXE..."
echo $LD $LDFLAGS -o $PROOFAEXE main/src/pmain.o  $dummyo $gobjs $ROOTALIB \
   $XLIBS $SYSLIBS lib/libfreetype.a lib/libpcre.a
$LD $LDFLAGS -o $PROOFAEXE main/src/pmain.o  $dummyo $gobjs $ROOTALIB \
   $XLIBS $SYSLIBS lib/libfreetype.a lib/libpcre.a

linkstat=$?
if [ $linkstat -ne 0 ]; then
   exit $linkstat
fi

rm -f $dummyc $dummyo

exit 0
