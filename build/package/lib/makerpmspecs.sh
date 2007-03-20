#!/bin/sh -e 
#
# $Id: makerpmspecs.sh,v 1.5 2005/07/14 15:00:44 rdm Exp $
#
# Writes an entry in ../root.spec 
#
tgtdir=$1 ; shift
cmndir=$1 ; shift
rpmdir=$1 ; shift 
pkg=$1

### echo %%% "tgtdir:	$tgtdir"
### echo %%% "cmndir:	$cmndir"
### echo %%% "rpmdir:	$rpmdir"
### echo %%% "pkg:	$pkg"

### echo %%% Check if skeleton and description file exist 
if [ ! -f $cmndir/$pkg.dscr ] || [ ! -f $rpmdir/$pkg.spec.in ] ; then 
    echo "$0: couldn't find one and/or both of" 1>&2
    echo "   $cmndir/$pkg.dscr"                 1>&2
    echo "   $rpmdir/$pkg.spec.in"              1>&2
    echo "giving up"                            1>&2 
    exit 4
fi 

### echo %%% Find the short description 
short=`sed -n 's/^short: \(.*\)$/\1/p' < ${cmndir}/${pkg}.dscr` 
if [ "x$short" = "x" ] ; then 
    echo "$0: short description empty - giving up" 1>&2
    exit 4
fi

### echo %%%  Insert the short description 
sed -e "s|@short@|$short|"  < $rpmdir/$pkg.spec.in > ${cmndir}/tmp

### echo %%%  Now prepare to insert long description 
### echo %%% 
### echo %%%  first split the file at mark '@long@'
csplit -q -f ${cmndir}/tmp. -k ${cmndir}/tmp "/@long@/"

### echo %%%  cat first part to new file 
cat ${cmndir}/tmp.00 > ${cmndir}/tmp

### echo %%%  then output the long description 
sed -e '/^#.*$/d' \
    -e '/^short:.*$/d' \
    -e '/^long:.*/d' \
    < ${cmndir}/${pkg}.dscr >> ${cmndir}/tmp

### echo %%%  If this is not the bin package,  give a refernce to that 
if [ "x$pkg" != "xroot-bin" ] ; then 
  echo "" >> ${cmndir}/tmp
  echo "See also root-bin package documentation for more information" >> \
	${cmndir}/tmp
fi

### echo %%%  Insert the general description 
if [ -f ${cmndir}/general.dscr ] ; then 
    # put an 'empty' line
    echo "" >> ${cmndir}/tmp

    # put the general documentation in the end 
    sed -e '/^#.*$/d' \
	-e '/^general:.*/d' \
	< ${cmndir}/general.dscr >> ${cmndir}/tmp 
fi

### echo %%%  and finally cat the last part of the file to new file 
cat ${cmndir}/tmp.01 >> ${cmndir}/tmp 

### echo %%%  remove temporary split files
rm ${cmndir}/tmp.00 ${cmndir}/tmp.01

### echo %%%  remove the remainder of the split, and insert tgtdir
sed -e "/^@long@.*$/d" \
    -e "s|@tgtdir@|${tgtdir}|" \
    -e "s|@files@|${tgtdir}/${pkg}.files|" \
    < ${cmndir}/tmp > ${cmndir}/spec.tmp
rm -f ${cmndir}/tmp

cat ${cmndir}/spec.tmp 
echo "" 
rm ${cmndir}/spec.tmp 

#
# EOF
#
