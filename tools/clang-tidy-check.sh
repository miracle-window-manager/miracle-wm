#!/bin/bash
set -e

fail=0
find . \( -name \*.h -o -name \*.cpp \)| while read f
do
  if [[ $f == ./src/** || $f == ./miraclemsg/** || $f == ./tests/** ]]
  then
    clang-format --dry-run --Werror -style=file -i $f
    if [ $? -ne 0 ]
    then
      fail = 1
    fi
  fi
done

exit $fail

