#!/bin/sh
for path in "*.h" "*.hpp" "*.cpp" "*.c"
do
#astyle2.3 missess some options, removed:
#	astyle --style=attach --indent=tab  --indent-switches  --indent-cases --indent-col1-comments --pad-paren --pad-header  --keep-one-line-statements --close-templates   --pad-oper  --recursive "$path"
# http://sourceforge.net/projects/astyle/files/latest/download?source=files
#2.5:
	astyle --style=attach --indent=tab --attach-namespaces --attach-classes --attach-inlines --indent-switches  --indent-cases --indent-preproc-cond --indent-col1-comments --pad-paren --pad-header --remove-brackets --keep-one-line-statements --close-templates --remove-comment-prefix   --pad-oper  --recursive "$path"
done
