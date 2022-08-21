#!/usr/bin/env bash

set -e

BUILD_FULL_PATH=$(dirname "$(realpath "$0")")

# FILL THESE WITH YOUR OWN SDKs PATHS
# NANOS_SDK=
# NANOSP_SDK=
# NANOX_SDK=

APPNAME="stellar"

# list of SDKS
NANO_SDKS=("$NANOS_SDK" "$NANOSP_SDK" "$NANOX_SDK")
# list of target elf file name suffix
FILE_SUFFIXES=("nanos" "nanosp" "nanox")

# move to the build directory
cd "$BUILD_FULL_PATH" || exit 1

# Do it only now since before the cd command, we might not have been inside the repository
GIT_REPO_ROOT=$(git rev-parse --show-toplevel)
BUILD_REL_PATH=$(realpath --relative-to="$GIT_REPO_ROOT" "$BUILD_FULL_PATH")

# create elfs directory if it doesn't exist
mkdir -p elfs

# move to repo's root to build apps
cd "$GIT_REPO_ROOT" || exit 1

for ((sdk_idx=0; sdk_idx < "${#NANO_SDKS[@]}"; sdk_idx++))
do
    nano_sdk="${NANO_SDKS[$sdk_idx]}"
    elf_suffix="${FILE_SUFFIXES[$sdk_idx]}"
    echo "* Building elfs for $(basename "$nano_sdk")..."

    echo "** Building app $appname..."
    make clean BOLOS_SDK="$nano_sdk"
    make -j DEBUG=1 BOLOS_SDK="$nano_sdk"
    cp bin/app.elf "$BUILD_REL_PATH/elfs/${APPNAME}_${elf_suffix}.elf"
done

echo "done"
