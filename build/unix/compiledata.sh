#! /bin/sh

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

if [ "$INCDIR" = "$ROOTSYS/include" ]; then
   INCDIR=\$ROOTSYS/include
fi
if [ "$LIBDIR" = "$ROOTSYS/lib" ]; then
   LIBDIR=\$ROOTSYS/lib
fi

if [ "$ARCH" = "macosx" ]; then
   SOEXT="so"
   SOFLAGS="-bundle $OPT -flat_namespace -undefined suppress"
   LDFLAGS=`echo $LDFLAGS | sed  -e "s/ -flat_namespace / /" -e "s/ -$OPT / /" `
elif [ "x`echo $SOFLAGS | grep -- '-soname,$'`" != "x" ]; then
    # If soname is specified, add the library name.
    SOFLAGS=$SOFLAGS\$LibName.$SOEXT
    # Alternatively we could remove the soname flag.
    #    SOFLAGS=`echo $SOFLAGS | sed  -e 's/-soname,/ /' -e 's/ -Wl, / /' `
fi

rm -f __compiledata

echo "Running $0"
echo "/* This is file is automatically generated */" > __compiledata
echo "#define BUILD_ARCH \"$ARCH\"" >> __compiledata
echo "#define BUILD_NODE \""`uname -a`"\" " >> __compiledata
echo "#define COMPILER \""`type $CXX`"\" " >> __compiledata
if [ "$CUSTOMSHARED" = "" ]; then
      echo "#define MAKESHAREDLIB  \"cd \$BuildDir ; $CXX -c \$Opt $CXXFLAGS \$IncludePath \$SourceFiles ; $CXX \$ObjectFiles $SOFLAGS $LDFLAGS -o \$SharedLib \$([ -d /sw/lib ] && echo -L/sw/lib)\"" >> __compiledata
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
      echo "Changing $COMPILEDATA"
      mv __compiledata $COMPILEDATA;
   else
      rm -f __compiledata; fi
else
   echo "Making $COMPILEDATA"
   mv __compiledata $COMPILEDATA; fi
)

exit 0

