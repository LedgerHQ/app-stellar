#!/usr/bin/env bash

set -e

# FILL THESE WITH YOUR OWN SDKs PATHS
# NANOSP_SDK=
# NANOX_SDK=
# STAX_SDK=
# FLEX_SDK=
# APEX_P_SDK=

# list of SDKS
DEVICE_SDKS=("$NANOSP_SDK" "$NANOX_SDK" "$STAX_SDK" "$FLEX_SDK" "$APEX_P_SDK")

# Do it only now since before the cd command, we might not have been inside the repository
GIT_REPO_ROOT=$(git rev-parse --show-toplevel)

# move to repo's root to build apps
cd "$GIT_REPO_ROOT" || exit 1

make clean

for sdk in "${DEVICE_SDKS[@]}"; do
    echo "* Building elfs for $(basename "$sdk")..."
    make -j BOLOS_SDK="$sdk"
done

echo "done"
