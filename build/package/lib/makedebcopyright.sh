#!/bin/sh -e 
#
# $Id$
#
# Writes a copyright file to debian/root-<pkg>.copyright
#
. build/package/lib/common.sh debian

### echo %%% if we did not get any argument, write the master copyright
### echo %%% file, using the toplevel LICENSE file 
if [ $# -lt 1 ] ; then 
    out=copyright
    skel=LICENSE
else
    out=$1.copyright
    skel=${debdir}/$1.copyright.in
fi

### echo %%%  make sure we get a fresh file 
rm -f ${tgtdir}/${out}

if [ -f ${debdir}/${out} ] 
then 
    ### echo %%% test to see if full file exist, if so, cat it to standard
    ### echo %%% out.  
    cat ${debdir}/${out} > ${tgtdir}/${out}
elif [ -f ${debdir}/${out}.in ] && [ -f ${skel} ] 
then 
    ### echo %%% if a skeleton and body exist, we insert the body into
    ### echo %%% the skeleton  and cat it to standard out 

    ### echo %%% first split the file $out at at mark @copyright@
    csplit -q -f ${cmndir}/tmp. \
	-k ${debdir}/${out}.in "/@copyright@/"

    ### echo %%% Then output the full file 
    cat ${cmndir}/tmp.00 \
	${skel} \
	${cmndir}/tmp.01 | \
	sed "/@copyright@/d"  > ${tgtdir}/${out}

    ### echo %%%  Clean up
    rm -f ${cmndir}/tmp.00 ${cmndir}/tmp.01
fi

#
# $Log$
#
