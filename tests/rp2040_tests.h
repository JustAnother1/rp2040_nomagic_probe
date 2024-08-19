#ifndef TESTS_CLI_TESTS_H_
#define TESTS_CLI_TESTS_H_

#include "munit.h"
#include <stdint.h>
#include <stdbool.h>

void* rp2040_setup(const MunitParameter params[], void* user_data);
MunitResult test_swd_v2(const MunitParameter params[], void* user_data);
MunitResult test_get_core_id(const MunitParameter params[], void* user_data);
MunitResult test_get_apsel(const MunitParameter params[], void* user_data);
MunitResult test_target_info(const MunitParameter params[], void* user_data);
MunitResult test_target_send_file_target_xml(const MunitParameter params[], void* user_data);
MunitResult test_target_send_file_threads(const MunitParameter params[], void* user_data);
MunitResult test_target_send_file_memory_map(const MunitParameter params[], void* user_data);
MunitResult test_target_send_file_invalid(const MunitParameter params[], void* user_data);
#endif /* TESTS_CLI_TESTS_H_ */
