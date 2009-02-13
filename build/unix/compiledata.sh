#! /bin/bash

# Script to generate the file include/compiledata.h.
# Called by main Makefile.
#
# Author: Fons Rademakers, 29/2/2000

COMPILEDATA=$1
CXX=$2
CXXOPT=$3
CXXDEBUG=$4
CXXFLAGS=$5
SOFLAGS=$6
LDFLAGS=$7
SOEXT=$8
SYSLIBS=$9
shift
LIBDIR=$9
shift
ROOTLIBS=$9
shift
RINTLIBS=$9
shift
INCDIR=$9
shift
CUSTOMSHARED=$9
shift
CUSTOMEXE=$9
shift
ARCH=$9
shift
ROOTBUILD=$9
shift
EXPLICITLINK=$9
shift

MACOSXTARGET=""

if [ "$INCDIR" = "$ROOTSYS/include" ]; then
   INCDIR=\$ROOTSYS/include
fi
if [ "$LIBDIR" = "$ROOTSYS/lib" ]; then
   LIBDIR=\$ROOTSYS/lib
fi

if [ "$EXPLICITLINK" = "yes" ]; then
   EXPLLINKLIBS="\$LinkedLibs"
else
   EXPLLINKLIBS="\$DepLibs"
fi

if [ "$ARCH" = "macosx" ] || [ "$ARCH" = "macosxxlc" ] || \
   [ "$ARCH" = "macosx64" ] || [ "$ARCH" = "macosxicc" ]; then
   macosx_minor=`sw_vers | sed -n 's/ProductVersion://p' | cut -d . -f 2`
   SOEXT="so"
   if [ $macosx_minor -ge 5 ]; then
      if [ "x`echo $SOFLAGS | grep -- '-install_name'`" != "x" ]; then
         # If install_name is specified, remove it.
         SOFLAGS="$OPT -dynamiclib -single_module -undefined dynamic_lookup"
      fi
      MACOSXTARGET="MACOSX_DEPLOYMENT_TARGET=10.$macosx_minor"
   elif [ $macosx_minor -ge 3 ]; then
      SOFLAGS="-bundle $OPT -undefined dynamic_lookup"
      EXPLLINKLIBS=""
      MACOSXTARGET="MACOSX_DEPLOYMENT_TARGET=10.$macosx_minor"
   else
      SOFLAGS="-bundle $OPT -undefined suppress"
      EXPLLINKLIBS=""
   fi
elif [ "x`echo $SOFLAGS | grep -- '-soname,$'`" != "x" ]; then
    # If soname is specified, add the library name.
    SOFLAGS=$SOFLAGS\$LibName.$SOEXT
    # Alternatively we could remove the soname flag.
    #    SOFLAGS=`echo $SOFLAGS | sed  -e 's/-soname,/ /' -e 's/ -Wl, / /' `
fi

# Remove -Iinclude since it is 'location' depedent
CXXFLAGS=`echo $CXXFLAGS | sed 's/-Iinclude //' `

# Determine the compiler version
COMPILERVERS="$CXX"
if [ "$CXX" = "g++" ] || [ "$CXX" = "icc" ]; then
   cxxTemp=`$CXX -dumpversion`
   cxxMajor=`echo $cxxTemp 2>&1 | cut -d'.' -f1`
   cxxMinor=`echo $cxxTemp 2>&1 | cut -d'.' -f2`
   cxxPatch=`echo $cxxTemp 2>&1 | cut -d'.' -f3`
   if [ "$CXX" = "g++" ] ; then
      COMPILERVERS="gcc"
   fi
   if [ "$cxxMajor" != "x" ] ; then
      COMPILERVERS="$COMPILERVERS$cxxMajor"
      if [ "$cxxMinor" != "x" ] ; then
         COMPILERVERS="$COMPILERVERS$cxxMinor"
         if [ "$cxxPatch" != "x" ] ; then
            COMPILERVERS="$COMPILERVERS$cxxPatch"
         fi
      fi
   fi
fi

rm -f __compiledata

echo "/* This is file is automatically generated */" > __compiledata
echo "#define BUILD_ARCH \"$ARCH\"" >> __compiledata
echo "#define BUILD_NODE \""`uname -a`"\" " >> __compiledata
echo "#define COMPILER \""`type -path $CXX`"\" " >> __compiledata
echo "#define COMPILERVERS \"$COMPILERVERS\"" >> __compiledata
if [ "$CUSTOMSHARED" = "" ]; then
      echo "#define MAKESHAREDLIB  \"cd \$BuildDir ; $CXX -c \$Opt $CXXFLAGS \$IncludePath \$SourceFiles ; $MACOSXTARGET $CXX \$ObjectFiles $SOFLAGS $LDFLAGS $EXPLLINKLIBS -o \$SharedLib\"" >> __compiledata
else
   echo "#define MAKESHAREDLIB \"$CUSTOMSHARED\"" >> __compiledata
fi
if [ "$CUSTOMEXE" = "" ]; then
   echo "#define MAKEEXE \"cd \$BuildDir ; $CXX -c $OPT $CXXFLAGS \$IncludePath \$SourceFiles; $CXX \$ObjectFiles $LDFLAGS -o \$ExeName \$LinkedLibs $SYSLIBS\""  >> __compiledata
else
   echo "#define MAKEEXE \"$CUSTOMEXE\"" >> __compiledata
fi
echo "#define CXXOPT \"$CXXOPT\"" >> __compiledata
echo "#define CXXDEBUG \"$CXXDEBUG\"" >> __compiledata
echo "#define ROOTBUILD \"$ROOTBUILD\"" >> __compiledata
echo "#define LINKEDLIBS \"-L$LIBDIR $ROOTLIBS $RINTLIBS \""  >> __compiledata
echo "#define INCLUDEPATH \"-I$INCDIR\"" >> __compiledata
echo "#define OBJEXT \"o\" " >> __compiledata
echo "#define SOEXT \"$SOEXT\" " >> __compiledata

(
if [ -r $COMPILEDATA ]; then
   diff __compiledata $COMPILEDATA > /dev/null; status=$?;
   if [ "$status" -ne "0" ]; then
      echo "Running $0"
      echo "Changing $COMPILEDATA"
      mv __compiledata $COMPILEDATA;
   else
      rm -f __compiledata; fi
else
   echo "Running $0"
   echo "Making $COMPILEDATA"
   mv __compiledata $COMPILEDATA; fi
)

exit 0
