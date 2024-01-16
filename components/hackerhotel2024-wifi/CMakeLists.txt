idf_component_register(
  REQUIRES driver
           esp_wifi
           hackerhotel2024-bsp
           esp_http_client
           esp_https_ota
           app_update
           wpa_supplicant
           esp-tls
  SRCS
    "wifi_cert.c"
    "wifi_defaults.c"
    "wifi_ota.c"
    "wifi_test.c"
    "wifi_connect.c"
    "wifi_connection.c"
  INCLUDE_DIRS include
  EMBED_TXTFILES resources/isrgrootx1.pem
                 resources/custom_ota_cert.pem
)
