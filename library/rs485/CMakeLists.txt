set(component_srcs "bee_rs485.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" "json" "esp_wifi"
                       REQUIRES "nvs")