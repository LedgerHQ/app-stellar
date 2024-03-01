#*******************************************************************************
#   Ledger Stellar App
#   (c) 2017-2022 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

APPNAME = Stellar
APPVERSION_M=5
APPVERSION_N=1
APPVERSION_P=0
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

ifeq ($(TARGET_NAME), TARGET_NANOS)
APP_LOAD_PARAMS = --appFlags 0x800  # APPLICATION_FLAG_LIBRARY
else
APP_LOAD_PARAMS = --appFlags 0xa00  # APPLICATION_FLAG_LIBRARY + APPLICATION_FLAG_BOLOS_SETTINGS
endif
APP_LOAD_PARAMS += --curve ed25519
APP_LOAD_PARAMS += --path "44'/148'"
APP_LOAD_PARAMS +=  $(COMMON_LOAD_PARAMS)

#prepare hsm generation
ifeq ($(TARGET_NAME),TARGET_NANOS)
	ICONNAME=icons/nanos_app_stellar.gif
else ifeq ($(TARGET_NAME),TARGET_STAX)
	ICONNAME=icons/stax_app_stellar.gif
else
	ICONNAME=icons/nanox_app_stellar.gif
endif

################
# Default rule #
################
all: default

############
# Platform #
############
ifneq ($(TARGET_NAME),TARGET_STAX)
    DEFINES   += HAVE_BAGL HAVE_UX_FLOW
endif

DEFINES   += OS_IO_SEPROXYHAL
DEFINES   += HAVE_SPRINTF
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=4 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES   += MAJOR_VERSION=$(APPVERSION_M) MINOR_VERSION=$(APPVERSION_N) PATCH_VERSION=$(APPVERSION_P)

DEFINES   += USB_SEGMENT_SIZE=64
DEFINES   += BLE_SEGMENT_SIZE=32 #max MTU, min 20
DEFINES   += APPVERSION=\"$(APPVERSION)\"

DEFINES   += HAVE_WEBUSB WEBUSB_URL_SIZE_B=0 WEBUSB_URL=""

# BLE
ifeq ($(TARGET_NAME),TARGET_NANOX)
DEFINES   += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000 HAVE_BLE_APDU
else ifeq ($(TARGET_NAME),TARGET_STAX)
DEFINES   += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000 HAVE_BLE_APDU
endif


ifeq ($(TARGET_NAME),TARGET_NANOS)
	DEFINES       += IO_SEPROXYHAL_BUFFER_SIZE_B=128
else ifeq ($(TARGET_NAME),TARGET_STAX)
	DEFINES       += IO_SEPROXYHAL_BUFFER_SIZE_B=300
	DEFINES       += NBGL_QRCODE
	SDK_SOURCE_PATH += qrcode
else
	DEFINES       += IO_SEPROXYHAL_BUFFER_SIZE_B=300
	DEFINES       += HAVE_GLO096
	DEFINES       += HAVE_BAGL BAGL_WIDTH=128 BAGL_HEIGHT=64
	DEFINES       += HAVE_BAGL_ELLIPSIS # long label truncation feature
	DEFINES       += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
	DEFINES       += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
	DEFINES       += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
endif

ifneq ($(NOCONSENT),)
	DEFINES   += NO_CONSENT
endif

DEBUG = 0
ifneq ($(DEBUG),0)
    DEFINES += HAVE_PRINTF
    ifeq ($(TARGET_NAME),TARGET_NANOS)
        DEFINES += PRINTF=screen_printf
    else
        DEFINES += PRINTF=mcu_usb_printf
    endif
else
        DEFINES += PRINTF\(...\)=
endif

##############
# Compiler #
##############
ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC       := $(CLANGPATH)clang
AS       := $(GCCPATH)arm-none-eabi-gcc
LD       := $(GCCPATH)arm-none-eabi-gcc
LDLIBS   += -lm -lgcc -lc

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb
SDK_SOURCE_PATH  += lib_stusb_impl

ifneq ($(TARGET_NAME),TARGET_STAX)
SDK_SOURCE_PATH += lib_ux
endif

ifeq ($(TARGET_NAME),TARGET_NANOX)
SDK_SOURCE_PATH  += lib_blewbxx lib_blewbxx_impl
else ifeq ($(TARGET_NAME),TARGET_STAX)
SDK_SOURCE_PATH  += lib_blewbxx lib_blewbxx_impl
endif


load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile.genericwallet


listvariants:
	@echo VARIANTS COIN stellar

tests-unit:
	cd tests_common_js && npm install && npm run build
	cd tests_generate_binary && npm install && npm run generate unit
	rm -rf tests_unit/build && cmake -Btests_unit/build -Htests_unit/ && make -C tests_unit/build/ && make -C tests_unit/build test

tests-zemu:
	./build_elfs.sh
	cd tests_common_js && npm install && npm run build
	cd tests_zemu && npm install && rm -rf snapshots-tmp && npm run test

fuzzing:
	cd tests_common_js && npm install && npm run build
	rm -rf fuzz/testcases && mkdir -p fuzz/testcases && cd tests_generate_binary && npm install && npm run generate fuzz
	cd fuzz && rm -rf build && cmake -DCMAKE_C_COMPILER=clang -Bbuild -H. && make -C build && ./build/fuzz_tx testcases
