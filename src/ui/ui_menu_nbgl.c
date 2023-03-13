/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2022 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/
#ifdef HAVE_NBGL
#include "ui.h"
#include "globals.h"
#include "settings.h"

#include "os.h"
#include "os_io_seproxyhal.h"
#include "glyphs.h"
#include "ux.h"
#include "nbgl_use_case.h"
#include "settings.h"

static void displaySettingsMenu(void);
static void settingsControlsCallback(int token, uint8_t index);
static bool settingsNavCallback(uint8_t page, nbgl_pageContent_t* content);

enum { SWITCH_HASH_SET_TOKEN = FIRST_USER_TOKEN, SWITCH_SEQUENCE_SET_TOKEN };

#define NB_INFO_FIELDS 2
static const char* const infoTypes[] = {"Version", "Stellar App"};
static const char* const infoContents[] = {APPVERSION, "(c) 2022 Ledger"};

#define NB_SETTINGS_SWITCHES 2
#define SWITCH_IDX(token)    (token - SWITCH_HASH_SET_TOKEN)
static uint8_t settings[NB_SETTINGS_SWITCHES] = {S_HASH_SIGNING_ENABLED, S_SEQUENCE_NUMBER_ENABLED};
static nbgl_layoutSwitch_t switches[NB_SETTINGS_SWITCHES];

void onQuitCallback(void) {
    os_sched_exit(-1);
}

static bool settingsNavCallback(uint8_t page, nbgl_pageContent_t* content) {
    if (page == 0) {
        switches[0].text = "Hash signing";
        switches[0].subText = "Enable transaction hash signing";
        switches[0].token = SWITCH_HASH_SET_TOKEN;
        switches[0].tuneId = TUNE_TAP_CASUAL;
        switches[1].text = "Sequence number";
        switches[1].subText = "Display sequence in transactions";
        switches[1].token = SWITCH_SEQUENCE_SET_TOKEN;
        switches[1].tuneId = TUNE_TAP_CASUAL;

        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = NB_SETTINGS_SWITCHES;
        content->switchesList.switches = (nbgl_layoutSwitch_t*) switches;
    } else if (page == 1) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = NB_INFO_FIELDS;
        content->infosList.infoTypes = (const char**) infoTypes;
        content->infosList.infoContents = (const char**) infoContents;
    } else {
        return false;
    }
    return true;
}

static void settingsControlsCallback(int token, uint8_t index) {
    UNUSED(index);
    switch (token) {
        case SWITCH_HASH_SET_TOKEN:
        case SWITCH_SEQUENCE_SET_TOKEN:
            switches[SWITCH_IDX(token)].initState = !(switches[SWITCH_IDX(token)].initState);
            SETTING_TOGGLE(settings[SWITCH_IDX(token)]);
            displaySettingsMenu();
            break;
        default:
            PRINTF("Should not happen !");
            break;
    }
}

static void displaySettingsMenu(void) {
    nbgl_useCaseSettings("Stellar settings",
                         0,
                         2,
                         true,
                         ui_menu_main,
                         settingsNavCallback,
                         settingsControlsCallback);
}

void ui_menu_main(void) {
    switches[SWITCH_IDX(SWITCH_HASH_SET_TOKEN)].initState =
        (HAS_SETTING(S_HASH_SIGNING_ENABLED)) ? ON_STATE : OFF_STATE;
    switches[SWITCH_IDX(SWITCH_SEQUENCE_SET_TOKEN)].initState =
        (HAS_SETTING(S_SEQUENCE_NUMBER_ENABLED)) ? ON_STATE : OFF_STATE;
    nbgl_useCaseHome("Stellar",
                     &C_icon_stellar_64px,
                     "This app confirms actions on\nthe Stellar network.",
                     true,
                     displaySettingsMenu,
                     onQuitCallback);
}
#endif  // HAVE_NBGL
