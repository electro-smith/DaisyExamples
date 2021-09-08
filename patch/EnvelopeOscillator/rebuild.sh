#!/bin/bash
echo "rebuilding projects:"
start_dir=$PWD
HW=hardware_platforms
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

echo "building EnvOsc"
cd ${DIR}
make clean | grep "warning\|error" # grep for silencing make outputs when regenerating everything.
make | grep "warning\|error"
cd "$start_dir"
echo "done"

#!/bin/bash
read -p "Press any key to exit..." var
