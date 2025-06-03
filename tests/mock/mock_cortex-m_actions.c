/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "probe_api/result.h"
#include "probe_api/common.h"

bool target_command_halt_cortex_m_cpu(void)
{
    return false;
}


Result handle_cortex_m_halt(action_data_typ* const action)
{
    return ERR_WRONG_STATE;
}
