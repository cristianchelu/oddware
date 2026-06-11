# Host-side unit tests (gcc/clang). See spec/40-architecture/build-integration.md.

CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -std=c11 -pedantic
CPPFLAGS := -I. -I../inc -I../ports -Ifakes -Ifakes/shim -Iunity -DHOST_TEST -DAPP_LOG_LEVEL=APP_LOG_LEVEL_DEBUG
LDFLAGS  :=
BUILD    := build-host
RUNNER   := $(BUILD)/test_runner.c
UNITY_SRC := unity/unity.c
TEST_SRCS := $(wildcard test_*.c)
HOST_SRCS := ../src/app_log.c \
             ../src/port_err.c \
             ../src/boot_bank.c \
             ../src/wifi_cred.c \
             ../src/provision_form.c \
             ../src/provision_scan_list.c \
             ../src/provision_flow.c \
             ../src/provision_wifi_try.c \
             ../src/provision_pending.c \
             ../src/provision_portal.c \
             ../src/provision_reset.c \
             ../src/mqtt_cred.c \
             ../src/mqtt_backoff.c \
             ../src/mqtt_topics.c \
             ../src/mqtt_route.c \
             ../src/app.c \
             ../src/app_mqtt_dispatch.c \
             ../src/button_input.c \
             ../src/hopper_input.c \
             ../src/button_gesture.c \
             ../src/mqtt_client.c \
             ../src/ota_client.c \
             ../src/ota_url.c \
             ../src/ota_image.c \
             ../src/ota_rollback.c \
             ../src/flash_bank_logic.c \
             ../src/aw9523b.c \
             ../src/gpio_expander_bootstrap.c \
             ../src/display_rail.c \
             ../src/tm1637.c \
             ../src/display_driver.c \
             ../src/display_glyph.c \
             ../src/display_presentation.c \
             ../src/display_wifi_indicator.c \
             ../src/display_mqtt_indicator.c \
             ../src/display_anim_builtin.c \
             ../src/display_boot.c \
             ../src/display_cli.c \
             ../src/weigh_cli.c \
             ../src/motor_cli.c \
             ../src/dispense_cli.c \
             ../src/motor_ctrl.c \
             ../src/beam_cli.c \
             ../src/cs1270.c \
             ../src/weight_rail.c \
             ../src/weight_driver.c \
             ../src/motor_driver.c \
             ../src/adc_driver.c \
             ../src/adc_cal.c \
             ../src/adc_cli.c \
             ../src/weigh_cal.c \
             ../src/gpio_expander_loan.c \
             ../src/wfci_bus_state.c \
             ../adapters/motor_index_port_adapter.c \
             fakes/gpio_expander_port_provider_host.c \
             fakes/motor_index_port_provider_host.c \
             fakes/motor_port_provider_host.c \
             fakes/fake_i2c_bus.c \
             fakes/fake_wfci_bus_port.c \
             fakes/fake_gpio_expander_port.c \
             fakes/fake_button_port.c \
             fakes/fake_motor_index_port.c \
             fakes/fake_hopper_ir_port.c \
             fakes/fake_tm1637_gpio.c \
             fakes/fake_display_port.c \
             fakes/fake_weight_port.c \
             fakes/fake_motor_port.c \
             fakes/fake_adc_port.c \
             fakes/fake_config_port.c \
             fakes/ota_rollback_host_stubs.c \
             fakes/fake_mqtt_port.c \
             fakes/fake_ota_port.c \
             fakes/fake_wifi_port.c \
             fakes/fake_app_event_q.c \
             fakes/fake_time.c \
             fakes/freertos_shim.c \
             fakes/freertos_queue_shim.c \
             fakes/motor_index_irq_host_stub.c \
             fakes/adc_jam_isr_host_stub.c \
             fakes/host_stubs.c \
             fakes/port_providers_host.c

.PHONY: all test-host clean

all: test-host

test-host: $(BUILD)/test_host
	./$(BUILD)/test_host
	@../../tools/host-green-nudge.sh

$(BUILD)/test_host: $(RUNNER) $(TEST_SRCS) $(HOST_SRCS) $(UNITY_SRC) Makefile | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(RUNNER) $(TEST_SRCS) $(HOST_SRCS) $(UNITY_SRC) $(LDFLAGS)

$(RUNNER): $(TEST_SRCS) Makefile | $(BUILD)
	@echo '#include "unity.h"' > $@
	@echo '' >> $@
	@echo 'extern void motor_port_host_reset(void);' >> $@
	@echo 'extern void motor_index_port_host_reset(void);' >> $@
	@echo 'extern void fake_wfci_bus_reset(void);' >> $@
	@echo 'extern void gpio_expander_loan_end(void);' >> $@
	@for f in $(TEST_SRCS); do \
		grep -h '^void test_' $$f 2>/dev/null \
			| sed 's/void \(test_[^(]*\)(.*/extern void \1(void);/'; \
	done | sort -u >> $@
	@echo '' >> $@
	@echo 'void setUp(void)' >> $@
	@echo '{' >> $@
	@echo '    motor_port_host_reset();' >> $@
	@echo '    motor_index_port_host_reset();' >> $@
	@echo '    fake_wfci_bus_reset();' >> $@
	@echo '    gpio_expander_loan_end();' >> $@
	@echo '}' >> $@
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
