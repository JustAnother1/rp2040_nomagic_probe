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

#ifndef CFG_TARGET_SPECIFIC_ACTIONS_H_
#define CFG_TARGET_SPECIFIC_ACTIONS_H_

#define TARGET_SPECIFIC_ACTIONS_ENUM \
    GDB_CMD_MON_HALT,                \
    GDB_CMD_MON_RESET,               \
    GDB_MONITOR_REG,                 \
    HALT_CORTEX_M_CPU,               \
    RELEASE_CORTEX_M_CPU,

#define TARGET_SPECIFIC_ACTION_HANDLERS                     \
    handle_monitor_halt,       /* GDB_CMD_MON_HALT */       \
    handle_monitor_reset,      /* GDB_CMD_MON_RESET */      \
    handle_monitor_reg,        /* GDB_MONITOR_REG, */       \
    handle_cortex_m_halt,      /* HALT_CORTEX_M_CPU */      \
    handle_cortex_m_release,   /* RELEASE_CORTEX_M_CPU */

#define TARGET_SPECIFIC_ACTION_NAMES \
    "monitor_halt",                  \
    "monitor_reset",                 \
    "monitor_reg",                   \
    "cortex-m_halt",                 \
    "cortex-m_release",

#endif /* CFG_TARGET_SPECIFIC_ACTIONS_H_ */
