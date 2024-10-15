TEST_BIN_FOLDER = $(BIN_FOLDER)test/
TST_LFLAGS = -lgcov --coverage
TST_CFLAGS =  -c -Wall -Wextra -g3 -fprofile-arcs -ftest-coverage -Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-format
TST_DDEFS = -DUNIT_TEST=1
TST_DDEFS += -DFEAT_DEBUG_UART
TST_DDEFS += -DFEAT_GDB_SERVER
TST_INCDIRS = tests/
TST_INCDIRS = tests/unity/
TST_INCDIRS += source/
TST_INCDIRS += nomagic_probe/src/

TST_INCDIR = $(patsubst %,-I%, $(TST_INCDIRS))


# rp2040
TEST_EXECUTEABLES = $(TEST_BIN_FOLDER)rp2040
RP2040_OBJS =                                                          \
 $(TEST_BIN_FOLDER)rp2040_tests.o                                      \
 $(TEST_BIN_FOLDER)source/rp2040.o                                     \
 $(TEST_BIN_FOLDER)mock_flash_driver.o                                 \
 $(TEST_BIN_FOLDER)nomagic_probe/tests/mock/gdbserver/gdbserver_mock.o \
 $(TEST_BIN_FOLDER)nomagic_probe/tests/mock/lib/printf_mock.o          \
 $(TEST_BIN_FOLDER)nomagic_probe/tests/mock/target/common_mock.o

TEST_LOGS = $(patsubst %,%.txt, $(TEST_EXECUTEABLES))

FRAMEWORK_OBJS = $(TEST_BIN_FOLDER)unity/unity.o

# for reporting the results
# ! ! ! Important: ` are speciall ! ! !
IGNORES  = `grep -a -s IGNORE $(TEST_BIN_FOLDER)*.txt)`
FAILURES = `grep -a -s FAIL   $(TEST_BIN_FOLDER)*.txt`
PASSED   = `grep -a -s PASS   $(TEST_BIN_FOLDER)*.txt`


# Unit Test framework
$(TEST_BIN_FOLDER)unity/%.o: tests/unity/%.c
	@echo ""
	@echo "=== compiling (unity) $@"
	@$(MKDIR_P) $(@D)
	$(TST_CC) $(TST_CFLAGS) $(TST_DDEFS) $(TST_INCDIR) $< -o $@

# Tests
$(TEST_BIN_FOLDER)%.o: tests/%.c
	@echo ""
	@echo "=== compiling (tests) $@"
	@$(MKDIR_P) $(@D)
	$(TST_CC) $(TST_CFLAGS) $(TST_DDEFS) $(TST_INCDIR) $< -o $@

# mock from nomagic_probe
$(TEST_BIN_FOLDER)nomagic_probe/tests/%.o: nomagic_probe/tests/%.c
	@echo ""
	@echo "=== compiling (nomagic mock) $@"
	@$(MKDIR_P) $(@D)
	$(TST_CC) $(TST_CFLAGS) $(TST_DDEFS) $(TST_INCDIR) $< -o $@


# source code module to Test
$(TEST_BIN_FOLDER)source/%.o: source/%.c
	@echo ""
	@echo "=== compiling (tested) $@"
	@$(MKDIR_P) $(@D)
	$(TST_CC) $(TST_CFLAGS) $(TST_DDEFS) $(TST_INCDIR) $< -o $@


# Test executeables
$(TEST_BIN_FOLDER)rp2040: $(RP2040_OBJS) $(FRAMEWORK_OBJS)
	@echo ""
	@echo "linking test: rp2040"
	@echo "============================"
	$(TST_LD) $(TST_LFLAGS) -o $(TEST_BIN_FOLDER)rp2040 $(RP2040_OBJS) $(FRAMEWORK_OBJS)

# run all tests
$(TEST_BIN_FOLDER)%.txt: $(TEST_BIN_FOLDER)%
	@echo ""
	@echo "=== running test $@"
	-./$< > $@ 2>&1


# report results
test: $(TEST_LOGS)
	@echo ""
	@echo "-----------------------\nIGNORES:\n-----------------------"
	@echo "$(IGNORES)"
	@echo "-----------------------\nFAILURES:\n-----------------------"
	@echo "$(FAILURES)"
	@echo "-----------------------\nPASSED:\n-----------------------"
	@echo "$(PASSED)"
	@echo "\nDONE"


# coverage  #  --exclude "*/usr/include/*" 
lcov: $(TEST_LOGS)
	lcov  --directory $(TEST_BIN_FOLDER) -c -o $(TEST_BIN_FOLDER)lcov.info --exclude "*tests/*"
	genhtml -o test_coverage -t "coverage" --num-spaces 4 $(TEST_BIN_FOLDER)lcov.info -o $(TEST_BIN_FOLDER)test_coverage/

