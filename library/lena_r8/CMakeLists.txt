set(component_srcs "source/bee_Lena_r8.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver"  "json"  "esp_wifi" 
                       REQUIRES "uart" "rs485" "led" "ota" "nvs")