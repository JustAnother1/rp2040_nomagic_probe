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

#ifndef SOURCE_CFG_TARGET_ACTIONS_H_
#define SOURCE_CFG_TARGET_ACTIONS_H_

#include "probe_api/result.h"

// writing/erasing the flash is target specific
Result handle_target_reply_vFlashDone(action_data_typ* const action);
Result handle_target_reply_vFlashErase(action_data_typ* const action);
Result handle_target_reply_vFlashWrite(action_data_typ* const action);
// reading some special regions of the memory might be target specific
Result handle_target_reply_read_memory(action_data_typ* const action);

#endif /* SOURCE_CFG_TARGET_ACTIONS_H_ */
