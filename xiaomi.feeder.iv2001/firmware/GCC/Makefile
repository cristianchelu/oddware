# petfeeder — IV2001 firmware
# SDK scaffold files are referenced by path, not copied. See build-integration.md.

SOURCE_DIR  = ../../../../..
BINPATH     = ~/gcc-arm-none-eabi/bin
PWD         = $(shell pwd)
SDK_PATH    = $(abspath $(PWD)/$(SOURCE_DIR))
FEATURE    ?= feature.mk

include $(FEATURE)

-include $(SOURCE_DIR)/.config
-include $(SOURCE_DIR)/config/chip/$(IC_CONFIG)/chip.mk
-include $(SOURCE_DIR)/config/board/$(BOARD_CONFIG)/board.mk

include $(SOURCE_DIR)/driver/chip/mt7686/module.mk
-include $(SOURCE_DIR)/driver/board/mt7686_hdk/ept/module.mk
include $(SOURCE_DIR)/kernel/rtos/FreeRTOS/module.mk
include $(SOURCE_DIR)/kernel/service/module.mk
include $(SOURCE_DIR)/driver/board/iv2001/util/module.mk

ifeq ($(MTK_MINICLI_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/minicli/module.mk
endif

ifeq ($(MTK_NVDM_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/nvdm/module.mk
endif

ifeq ($(MTK_FOTA_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/fota/module.mk
endif

ifeq ($(MTK_CM4_WIFI_TASK_ENABLE),y)
ifeq ($(MTK_WIFI_ROM_ENABLE),y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/wifi_stack),)
include $(SOURCE_DIR)/middleware/MTK/wifi_stack/module.mk
else
include $(SOURCE_DIR)/prebuilt/middleware/MTK/wifi_stack/module.mk
endif
endif
endif

include $(SOURCE_DIR)/middleware/MTK/connsys/module.mk
include $(SOURCE_DIR)/middleware/third_party/lwip/module.mk
include $(SOURCE_DIR)/middleware/MTK/dhcpd/module.mk
include $(SOURCE_DIR)/middleware/MTK/wifi_service/combo/module.mk

DEBUG           = 0
FLOAT_TYPE      = hard
BUILD_DIR       = $(PWD)/build
PROJ_NAME       = $(notdir $(abspath $(dir $(PWD))))
PROJ_PATH       = $(PWD)
OUTPATH         = $(PWD)/build

APP_PATH        = $(patsubst $(SDK_PATH)/%,%,$(abspath $(dir $(PWD))))
APP_PATH_SRC    = $(APP_PATH)/src
APP_PATH_ADAPT  = $(APP_PATH)/adapters

MINICLI         = project/mt7682_hdk/apps/minicli
MQTT_CLIENT     = project/mt7682_hdk/apps/mqtt_client

APP_FILES      += $(APP_PATH_SRC)/main.c
APP_FILES      += $(APP_PATH_SRC)/boot_bank.c
APP_FILES      += $(APP_PATH_SRC)/boot_bank_target.c
APP_FILES      += $(APP_PATH_SRC)/app_cli.c
APP_FILES      += $(APP_PATH_SRC)/wifi_cred.c
APP_FILES      += $(APP_PATH_SRC)/app_wifi_cli.c
APP_FILES      += $(APP_PATH_SRC)/wifi_sta.c
APP_FILES      += $(APP_PATH_SRC)/wifi_scan_oui.c
APP_FILES      += $(APP_PATH_SRC)/nvdm_boot.c
APP_FILES      += $(APP_PATH_ADAPT)/nvdm_adapter.c
APP_FILES      += $(APP_PATH_ADAPT)/wifi_adapter.c
APP_FILES      += $(APP_PATH_SRC)/ept_gpio_var.c
APP_FILES      += $(APP_PATH_SRC)/ept_eint_var.c
APP_FILES      += $(MINICLI)/src/regions_init.c
APP_FILES      += $(MQTT_CLIENT)/src/sys_init.c
APP_FILES      += $(MQTT_CLIENT)/src/mem_layout_info.c
APP_FILES      += $(MQTT_CLIENT)/src/wifi_lwip_helper.c
APP_FILES      += $(MQTT_CLIENT)/src/network_default_config.c
APP_FILES      += $(MQTT_CLIENT)/src/nvdm_config.c
APP_FILES      += $(MQTT_CLIENT)/src/wifi_nvdm_config.c
APP_FILES      += $(MINICLI)/GCC/syscalls.c

SYS_FILES      += $(MINICLI)/src/system_mt7682.c

C_FILES        += $(APP_FILES) $(SYS_FILES)
S_FILES        += $(MINICLI)/GCC/startup_mt7682.s

CFLAGS := -I$(SOURCE_DIR)/$(APP_PATH)/inc $(CFLAGS)

C_OBJS          = $(C_FILES:%.c=$(BUILD_DIR)/%.o)
S_OBJS          = $(S_FILES:%.s=$(BUILD_DIR)/%.o)

CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH)/ports
CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH)/adapters
CFLAGS += -I$(SOURCE_DIR)/$(MINICLI)/inc
CFLAGS += -I$(SOURCE_DIR)/$(MQTT_CLIENT)/inc
CFLAGS += -I$(SOURCE_DIR)/driver/chip/mt7686/inc
CFLAGS += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS += -I$(SOURCE_DIR)/driver/board/mt7686_hdk/ept/inc
CFLAGS += -I$(SOURCE_DIR)/driver/CMSIS/Device/MTK/mt7682/Include
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/nvdm/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/wifi_service/combo/inc

LDFLAGS += $(ALLFLAGS) $(FPUFLAGS) --specs=nano.specs -lnosys -nostartfiles
LDFLAGS += -Wl,-wrap=malloc -Wl,-wrap=calloc -Wl,-wrap=realloc -Wl,-wrap=free
LDFLAGS += -u _printf_float -u _scanf_float
LDFLAGS += -Wl,-Tmt7682_flash.ld -Wl,--gc-sections

.PHONY: proj clean $(MODULE_PATH)

all: cleanlog proj
	@mkdir -p $(BUILD_DIR)
	@$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf
	@$(SOURCE_DIR)/tools/scripts/build/copy_firmware.sh $(SOURCE_DIR) $(OUTPATH) $(IC_CONFIG) $(BOARD_CONFIG) $(PROJ_NAME).bin $(PWD)

MOD_EXTRA = BUILD_DIR=$(BUILD_DIR) OUTPATH=$(OUTPATH) PROJ_PATH=$(PROJ_PATH)

$(LIBS): $(MODULE_PATH)

$(MODULE_PATH):
	@+make -C $@ $(MOD_EXTRA) $($@_EXTRA)

proj: $(OUTPATH)/$(PROJ_NAME).elf

$(OUTPATH)/$(PROJ_NAME).elf: $(C_OBJS) $(S_OBJS) $(LIBS) $(WIFI_ROM_SYM)
	@echo Linking...
	@if [ -e "$@" ]; then rm -f "$@"; fi
	@if [ -e "$(OUTPATH)/$(PROJ_NAME).map" ]; then rm -f "$(OUTPATH)/$(PROJ_NAME).map"; fi
	@if [ -e "$(OUTPATH)/$(PROJ_NAME).hex" ]; then rm -f "$(OUTPATH)/$(PROJ_NAME).hex"; fi
	@if [ -e "$(OUTPATH)/$(PROJ_NAME).bin" ]; then rm -f "$(OUTPATH)/$(PROJ_NAME).bin"; fi
	@echo $(LDFLAGS) -Wl,--whole-archive -Wl,--start-group $(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%, $^) -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=$(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%,$(OUTPATH))/$(PROJ_NAME).map -lm -o $(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%, $@) > $(OUTPATH)/link_option.tmp
	@$(CXX) @$(OUTPATH)/link_option.tmp 2>>$(ERR_LOG)
	@rm -f $(OUTPATH)/link_option.tmp
	@$(OBJDUMP) -D $(OUTPATH)/$(PROJ_NAME).elf > $(OUTPATH)/$(PROJ_NAME).dis
	@$(OBJCOPY) -O ihex $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).hex
	@$(OBJCOPY) -O binary $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).bin
	@echo Done

include $(SOURCE_DIR)/.rule.mk

clean:
	@rm -rf $(OUTPATH)
