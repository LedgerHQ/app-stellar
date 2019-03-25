/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
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
 ********************************************************************************/

#ifndef STELLAR_VARS_H
#define STELLAR_VARS_H

#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include "stellar_types.h"

extern stellar_context_t ctx;
extern ux_state_t ux;
extern stellar_nv_state_t const N_state_pic;
#define N_stellar_pstate  (*(volatile  stellar_nv_state_t *)PIC(&N_state_pic))

#endif
