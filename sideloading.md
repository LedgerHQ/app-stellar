# Ledger Stellar App Sideloading Tutorial

> [!WARNING]
> We recommend that you install the latest Stellar App through Ledger Live. The version provided on the GitHub Release page may not have been audited yet, please be aware of the risks. If you encounter any problems, please report them to us.

> [!NOTE]
> You need to perform the following steps on Linux or MacOS. You may be able to run these steps on Windows via WSL, but we have not tested it.

## Supported Devices

All except the Ledger Nano X.

## Steps
### 1. Install required tools

You need to install Python 3 first. The method of installing Python 3 varies depending on the operating system and will not be reiterated here. Please search for it on your own.

We assume that `~/ledger` is our working directory. Let's create and activate a virtual environment, then install ledgerblue and strledger:

```shell
mkdir ~/ledger
cd ~/ledger
python3 -m venv .venv
source .venv/bin/activate
pip install ledgerblue strledger
```

### 2. Connect the Ledger device to your computer.

I suggest you first try to install the app through Ledger Live to see if it works properly. If everything is working fine, please exit Ledger Live and continue with the next steps.

If you encounter any connection issues, please refer to [the guide](https://support.ledger.com/hc/en-us/articles/115005165269-Fix-USB-connection-issues-with-Ledger-Live) provided by the Ledger team.

### 3. Installing a custom developer certificate (Optional)

This step is not mandatory, but this will make it more convenient for you to use side-loaded apps, please follow steps 1-4 of [this article](https://developers.ledger.com/docs/device-app/develop/tools#ledgerblue) to complete it. Finally, make sure that running `echo $SCP_PRIVKEY` correctly outputs your private key.

> [!NOTE]
> If you are using a more recent firmware, the target for Ledger Nano S is `0x33100004`, for Ledger Nano S Plus, it is `0x33100004`.

### 4. Download and execute the installation script
Download the installation script from [GitHub release page](https://github.com/lightsail-network/app-stellar/releases) and place it in the `~/ledger` directory, make it executable, and run it.

For Ledger Nano S:
```shell
chmod +x installer_nano_s.sh
./installer_nano_s.sh load
```

For Leder Nano S Plus:
```shell
chmod +x installer_nano_s_plus.sh
./installer_nano_s_plus.sh load
```

> [!NOTE]
> If you encounter an error, please first try to upgrade the firmware version to the latest through Ledger Live.

### 5. Interact with the Stellar App
After successful installation, you can open the Stellar App. Then you can use `strledger get-address` to obtain your Stellar address to test if it is functioning properly. [`strledger`](https://github.com/lightsail-network/strledger) is a command-line tool for interacting with the Ledger Stellar App, and you can view all available commands by running `strledger --help`.