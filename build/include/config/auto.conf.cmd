deps_config := \
	/home/justinegbert/esp/esp-idf/components/app_trace/Kconfig \
	/home/justinegbert/esp/esp-idf/components/aws_iot/Kconfig \
	/home/justinegbert/esp/esp-idf/components/bt/Kconfig \
	/home/justinegbert/esp/esp-idf/components/esp32/Kconfig \
	/home/justinegbert/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/justinegbert/esp/esp-idf/components/ethernet/Kconfig \
	/home/justinegbert/esp/esp-idf/components/fatfs/Kconfig \
	/home/justinegbert/esp/esp-idf/components/freertos/Kconfig \
	/home/justinegbert/esp/esp-idf/components/heap/Kconfig \
	/home/justinegbert/esp/esp-idf/components/libsodium/Kconfig \
	/home/justinegbert/esp/esp-idf/components/log/Kconfig \
	/home/justinegbert/esp/esp-idf/components/lwip/Kconfig \
	/home/justinegbert/esp/esp-idf/components/mbedtls/Kconfig \
	/home/justinegbert/esp/esp-idf/components/openssl/Kconfig \
	/home/justinegbert/esp/esp-idf/components/pthread/Kconfig \
	/home/justinegbert/esp/esp-idf/components/spi_flash/Kconfig \
	/home/justinegbert/esp/esp-idf/components/spiffs/Kconfig \
	/home/justinegbert/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/justinegbert/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/justinegbert/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/justinegbert/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/justinegbert/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/justinegbert/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
