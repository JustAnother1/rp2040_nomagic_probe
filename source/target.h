/*
 * target.h
 *
 *  Created on: 22.05.2024
 *      Author: lars
 */

#ifndef SOURCE_TARGET_H_
#define SOURCE_TARGET_H_


#include <stdint.h>
#include "probe_api/common.h"

bool target_is_SWDv2(void);
uint32_t target_get_SWD_core_id(uint32_t core_num); // only required for SWDv2 (TARGETSEL)
uint32_t target_get_SWD_APSel(uint32_t core_num);

void target_send_file(char* filename, uint32_t offset, uint32_t len);

#endif /* SOURCE_TARGET_H_ */
