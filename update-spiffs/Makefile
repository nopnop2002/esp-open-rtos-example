PROGRAM=spiffs_example
EXTRA_COMPONENTS = extras/spiffs extras/jsmn
FLASH_SIZE = 32

# spiffs configuration
SPIFFS_BASE_ADDR = 0x100000
SPIFFS_SIZE = 0x010000

# SPIFFS_SINGLETON = 0  # for run-time configuration

include ../../common.mk

$(eval $(call make_spiffs_image,files))
