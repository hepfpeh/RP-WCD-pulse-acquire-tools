#!/bin/bash

bnumber=1


dstring=$(date +"%Y%m%d-%H%M%S%z")
hstring=$(hostname)

function generate_build_h () {
    printf "#ifndef _BUILD_H\n" >   build.h
    printf "#define _BUILD_H\n" >>  build.h
    printf "#define _BUILD \"%s-%03d@%s\"\n" $dstring $1 $hstring >> build.h
    printf "#endif /* _BUILD_H */" >> build.h
}


if [ -f buildnumber ]; then
    bnumber=$(cat buildnumber)
    ((bnumber++))
    echo "$bnumber" > "buildnumber"
    generate_build_h $bnumber
else
    echo "$bnumber" > "buildnumber"
    generate_build_h $bnumber
fi
