#! /bin/sh

# Simple interface to LINK, tansforming -o <exe> to -out:<exe> and unix
# pathnames to windows pathnames.

args=
dll=
while [ "$1" != "" ]; do
   case "$1" in
   -o) args="$args -out:$2"
       dll="$2"
       shift ;;
   *) args="$args $1" ;;
   esac
   shift
done
if [ "$dll" != "bin/rmkdepend.exe" -a \
     "$dll" != "bin/bindexplib.exe" -a \
     "$dll" != "utils/src/rootcint_tmp.exe" -a \
     -r base/src/precompile.o ]; then
  args="$args base/src/precompile.o"
fi

link $args || exit $?
if [ "$dll" != "" -a -f $dll.manifest ]; then
   if [ "${dll%.dll}" == "$dll" ]
       then resourceID=1; # .exe
       else resourceID=2  #.dll
   fi
   mt -nologo -manifest $dll.manifest -outputresource:${dll}\;$resourceID
   rm $dll.manifest
fi

exit $?
