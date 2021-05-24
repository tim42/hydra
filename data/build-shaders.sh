#!/bin/bash
# a shell script to build all the shaders into spirv

extension_list=".comp .vert .tesc .tese .geom .frag"
quiet=true
[ "$1" == "-v" ] && quiet=false

echo "    -- cleaning spirv (.spv) files"
find . -name '*.spv' -delete

function filter_ouput
{
  $1 && grep -vE '^(\./.+|Linked.+|Warning, version .+)?$' || cat
}

for extension in $extension_list
do
  find . -name '*'$extension -exec bash -c "echo '    -- {}' && glslangValidator -V -t -o '{}'.spv '{}'" ';' | filter_ouput $quiet
done

