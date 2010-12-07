#! /bin/bash
# script to generate a standalone unit test for root daily tests
# Author: Eckhard v. Toerne, Dec 2010
# 
outfile=stressTMVA.cxx
cat <<EOF >$outfile
// 
// this stress test for TMVA is a shorter version of the extended TMVA testsuite 
// generated by script tmvaValidation/testsuite/RootTest/buildRootTest.sh
//
// Eckhard von Toerne, Dec 2010
//
EOF
for i in UnitTest UnitTestSuite utDataSetInfo utDataSet utEvent utReader utFactory utVariableInfo MethodUnitTestWithROCLimits RegressionUnitTestWithDeviation MethodUnitTestWithComplexData; do
    echo "// including file tmvaut/$i.h" >>$outfile
    cat ../tmvaut/$i.h >> $outfile
    echo "// including file tmvaut/$i.cxx" >>$outfile
    cat ../tmvaut/$i.cxx >> $outfile
done
echo "// including file stressTMVA.cxx" >> $outfile
cat ../stressTMVA.cxx >> $outfile

for ihead in UnitTest UnitTestSuite utDataSetInfo utDataSet utEvent utReader utFactory utVariableInfo MethodUnitTestWithROCLimits RegressionUnitTestWithDeviation MethodUnitTestWithComplexData; do
    cat $outfile | sed "s/#include \"$ihead.h\"//g" |  sed "s/#include \"tmvaut\/$ihead.h\"//g" > temp.cxx
    mv temp.cxx $outfile
done
