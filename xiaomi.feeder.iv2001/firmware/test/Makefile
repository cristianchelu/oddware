# Host-side unit tests (gcc/clang). See spec/40-architecture/build-integration.md.

CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -std=c11 -pedantic
CPPFLAGS := -I. -I../inc -I../ports -Ifakes -Ifakes/shim -Iunity
LDFLAGS  :=
BUILD    := build-host
RUNNER   := $(BUILD)/test_runner.c
UNITY_SRC := unity/unity.c
TEST_SRCS := $(wildcard test_*.c)
HOST_SRCS := ../src/boot_bank.c \
             ../src/wifi_cred.c \
             ../src/mqtt_cred.c \
             ../src/mqtt_backoff.c \
             ../src/mqtt_topics.c \
             ../src/mqtt_route.c \
             ../src/mqtt_client.c \
             ../src/ota_client.c \
             ../src/ota_url.c \
             ../src/ota_image.c \
             ../src/ota_rollback.c \
             ../src/flash_bank_logic.c \
             fakes/fake_config_port.c \
             fakes/ota_rollback_host_stubs.c \
             fakes/fake_mqtt_port.c \
             fakes/fake_ota_port.c \
             fakes/fake_wifi_port.c \
             fakes/fake_time.c \
             fakes/freertos_shim.c \
             fakes/host_stubs.c \
             fakes/port_providers_host.c

.PHONY: all test-host clean

all: test-host

test-host: $(BUILD)/test_host
	./$(BUILD)/test_host

$(BUILD)/test_host: $(RUNNER) $(TEST_SRCS) $(HOST_SRCS) $(UNITY_SRC) Makefile | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(RUNNER) $(TEST_SRCS) $(HOST_SRCS) $(UNITY_SRC) $(LDFLAGS)

$(RUNNER): $(TEST_SRCS) Makefile | $(BUILD)
	@echo '#include "unity.h"' > $@
	@echo '' >> $@
	@for f in $(TEST_SRCS); do \
		grep -h '^void test_' $$f 2>/dev/null \
			| sed 's/void \(test_[^(]*\)(.*/extern void \1(void);/'; \
	done | sort -u >> $@
	@echo '' >> $@
	@echo 'void setUp(void) {}' >> $@
	@echo 'void tearDown(void) {}' >> $@
	@echo '' >> $@
	@echo 'int main(void)' >> $@
	@echo '{' >> $@
	@echo '    UNITY_BEGIN();' >> $@
	@for f in $(TEST_SRCS); do \
		grep -h '^void test_' $$f 2>/dev/null \
			| sed 's/void \(test_[^(]*\)(.*/    RUN_TEST(\1);/'; \
	done >> $@
	@echo '    return UNITY_END();' >> $@
	@echo '}' >> $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
