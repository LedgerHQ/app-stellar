# ****************************************************************************
#    Ledger App Stellar
#    (c) 2024 Ledger SAS.
#    (c) 2024 overcat.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
# ****************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.defines

########################################
#        Mandatory configuration       #
########################################
# Application name
APPNAME = "Stellar"

# Application version
APPVERSION_M = 5
APPVERSION_N = 5
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src

# Application icons following guidelines:
# https://developers.ledger.com/docs/embedded-app/design-requirements/#device-icon
ICON_NANOS = icons/nanos_app_stellar.gif
ICON_NANOX = icons/nanox_app_stellar.gif
ICON_NANOSP = icons/nanox_app_stellar.gif
ICON_STAX = icons/stax_app_stellar.gif
ICON_FLEX = icons/flex_app_stellar.gif

# Application allowed derivation curves.
# Possibles curves are: secp256k1, secp256r1, ed25519 and bls12381g1
# If your app needs it, you can specify multiple curves by using:
# `CURVE_APP_LOAD_PARAMS = <curve1> <curve2>`
CURVE_APP_LOAD_PARAMS = ed25519

# Application allowed derivation paths.
# You should request a specific path for your app.
# This serve as an isolation mechanism.
# Most application will have to request a path according to the BIP-0044
# and SLIP-0044 standards.
# If your app needs it, you can specify multiple path by using:
PATH_APP_LOAD_PARAMS = "44'/148'"

# Setting to allow building variant applications
# - <VARIANT_PARAM> is the name of the parameter which should be set
#   to specify the variant that should be build.
# - <VARIANT_VALUES> a list of variant that can be build using this app code.
#   * It must at least contains one value.
#   * Values can be the app ticker or anything else but should be unique.
VARIANT_PARAM = COIN
VARIANT_VALUES = stellar

# Enabling DEBUG flag will enable PRINTF and disable optimizations
DEBUG = 0

########################################
#     Application custom permissions   #
########################################
# See SDK `include/appflags.h` for the purpose of each permission
#HAVE_APPLICATION_FLAG_DERIVE_MASTER = 1
#HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1
#HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
#HAVE_APPLICATION_FLAG_LIBRARY = 1

########################################
# Application communication interfaces #
########################################
ENABLE_BLUETOOTH = 1
#ENABLE_NFC = 1

########################################
#         NBGL custom features         #
########################################
ENABLE_NBGL_QRCODE = 1
#ENABLE_NBGL_KEYBOARD = 1
#ENABLE_NBGL_KEYPAD = 1

########################################
#          Features disablers          #
########################################
# These advanced settings allow to disable some feature that are by
# default enabled in the SDK `Makefile.standard_app`.
#DISABLE_STANDARD_APP_FILES = 1 
#DISABLE_DEFAULT_IO_SEPROXY_BUFFER_SIZE = 1 # To allow custom size declaration
#DISABLE_STANDARD_APP_DEFINES = 1 # Will set all the following disablers
#DISABLE_STANDARD_SNPRINTF = 1
#DISABLE_STANDARD_USB = 1
#DISABLE_STANDARD_WEBUSB = 1
#DISABLE_STANDARD_BAGL_UX_FLOW = 1
#DISABLE_DEBUG_LEDGER_ASSERT = 1
#DISABLE_DEBUG_THROW = 1

ENABLE_SWAP = 1

WITH_LIBSTELLAR=1
ifneq ($(WITH_LIBSTELLAR),0)
    SOURCE_FILES += $(wildcard libstellar/*.c)
    CFLAGS       += -Ilibstellar/include
endif

include $(BOLOS_SDK)/Makefile.standard_app

ifeq ($(TARGET_NAME), TARGET_NANOS)
APP_FLAGS_APP_LOAD_PARAMS = 0x800  # APPLICATION_FLAG_LIBRARY
else
APP_FLAGS_APP_LOAD_PARAMS = 0xa00  # APPLICATION_FLAG_LIBRARY + APPLICATION_FLAG_BOLOS_SETTINGS
endif

tests-unit:
	cd tests_common_js && npm install && npm run build
	cd tests_generate_binary && npm install && npm run generate unit
	rm -rf tests_unit/build && cmake -Btests_unit/build -Htests_unit/ && make -C tests_unit/build/ && make -C tests_unit/build test

tests-zemu:
	./build_elfs.sh
	cd tests_common_js && npm install && npm run build
	cd tests_zemu && npm install && rm -rf snapshots-tmp && npm run test

build-installer:
	rm -rf installer
	make clean && make BOLOS_SDK=$(NANOSP_SDK) && python scripts/installer_generator.py
	make clean && make BOLOS_SDK=$(NANOS_SDK) && python scripts/installer_generator.py