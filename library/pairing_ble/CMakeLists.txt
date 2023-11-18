set(component_srcs "bee_Pairing_ble.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "esp_wifi" "esp_netif" "nvs_flash" "wifi_provisioning" "flash"
                       REQUIRES "lena_r8")