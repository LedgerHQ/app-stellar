import os
import subprocess


template = """#!/usr/bin/env bash

set -e

VERSION="{version}"
MODEL="{model}"
APPNAME="Stellar"
APPPATH="44'/148'"
LOAD_PARAMS=(--targetId {target_id} --targetVersion="" {extra_params} --fileName bin/app.hex --appName "Stellar" --appFlags {app_flags} --delete --tlv --dataSize {data_size} --installparamsSize {install_params_size})
DELETE_PARAMS=(--targetId {target_id} --appName "Stellar")

APPHEX="{app_hex}"

# Check if Python 3 and ledgerblue are installed
command -v python3 >/dev/null 2>&1 || {{ echo >&2 "Python 3 is not installed. Aborting."; exit 1; }}
python3 -c "import ledgerblue" >/dev/null 2>&1 || {{ echo >&2 "ledgerblue module is not found. Please install using 'pip install ledgerblue'. Aborting."; exit 1; }}

# Add --rootPrivateKey to LOAD_PARAMS and DELETE_PARAMS if SCP_PRIVKEY is not empty
# See https://developers.ledger.com/docs/device-app/develop/tools#ledgerblue
if [ -n "${{SCP_PRIVKEY}}" ]; then
    LOAD_PARAMS+=("--rootPrivateKey" "${{SCP_PRIVKEY}}")
    DELETE_PARAMS+=("--rootPrivateKey" "${{SCP_PRIVKEY}}")
fi

# Define functions for load, delete, and version
load() {{
	mkdir -p ./bin
	echo -e "${{APPHEX}}" > "./bin/app.hex"
    python3 -m ledgerblue.loadApp "${{LOAD_PARAMS[@]}}"
}}

delete() {{
    python3 -m ledgerblue.deleteApp "${{DELETE_PARAMS[@]}}"
}}

# Main script
case "$1" in
    load) load ;;
    delete) delete ;;
    version) version ;;
    *)
        echo "Ledger Stellar App Installer [$MODEL] [$VERSION] [Warning: use only for test/demo apps]"
        echo "  load    - Load $APPNAME app"
        echo "  delete  - Delete $APPNAME app"
        ;;
esac"""


GET_DATA_SIZE_COMMAND = "echo $((0x`cat debug/app.map | grep _envram_data | tr -s ' ' | cut -f2 -d' ' |cut -f2 -d'x' ` - 0x`cat debug/app.map | grep _nvram_data | tr -s ' ' | cut -f2 -d' ' | cut -f2 -d'x'`))"
GET_INSTALL_PARAMS_SIZE_COMMAND = "echo $((0x`cat debug/app.map | grep _einstall_parameters | tr -s ' ' | cut -f2 -d' ' |cut -f2 -d'x'` - 0x`cat debug/app.map | grep _install_parameters | tr -s ' ' | cut -f2 -d' ' |cut -f2 -d'x'`))"

data_size = subprocess.check_output(GET_DATA_SIZE_COMMAND, shell=True).decode().strip()
install_params_size = (
    subprocess.check_output(GET_INSTALL_PARAMS_SIZE_COMMAND, shell=True)
    .decode()
    .strip()
)
version = (
    subprocess.check_output("git rev-parse --short HEAD", shell=True).decode().strip()
)

model = "nano_s"
with open("debug/app.map") as f:
    content = f.read()
    if "nanos2" in content:
        model = "nano_s_plus"

if model == "nano_s":
    target_id = "0x31100004"
    app_flags = "0x800"
    extra_params = ""
elif model == "nano_s_plus":
    target_id = "0x33100004"
    app_flags = "0xa00"
    extra_params = "--apiLevel 5"

with open("bin/app.hex") as f:
    app_hex = f.read()

script = template.format(
    version=version,
    target_id=target_id,
    app_flags=app_flags,
    data_size=data_size,
    install_params_size=install_params_size,
    app_hex=app_hex,
    model=model,
    extra_params=extra_params,
)

filename = f"installer/installer_{model}.sh"

if not os.path.exists("installer"):
    os.makedirs("installer")

with open(filename, "w") as f:
    f.write(script)

print(f"Installer script generated: {filename}")
