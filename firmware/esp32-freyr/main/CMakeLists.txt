
idf_component_register(
  SRCS
    main.cpp
    freyr.cpp
    webif.cpp
    device.cpp
    modules/egress_esp32_rmt.c
    modules/mod_esp32_control.c
    
  INCLUDE_DIRS . ../../../src/ ../../../alpha4/src/ ../../../unicorn/c/
  PRIV_REQUIRES led_strip esp_http_server esp_eth app_update driver nvs_flash
)

