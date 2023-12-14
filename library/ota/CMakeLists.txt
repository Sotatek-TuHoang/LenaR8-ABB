set(component_srcs "bee_ota.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "esp_event" "app_update" "esp_http_client" "esp_https_ota" "nvs_flash" "json" "esp_wifi"
                       REQUIRES 
		       EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
