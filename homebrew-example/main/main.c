#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "../components/odroid/odroid_system.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_display.h"

#include "../components/tetroidgo/game.h"
#include "../components/tetroidgo/random.h"

#include "bootloader_random.h"

#include "sdkconfig.h"

odroid_gamepad_state joystick;

odroid_battery_state battery_state;

int BatteryPercent = 100;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void app_main(void)
{
    printf("tetroid-go (%s-%s).\n", COMPILEDATE, GITREV);
    
    nvs_flash_init();

    odroid_system_init();

    odroid_input_gamepad_init();

    printf("intializing battery\n");
    odroid_input_battery_level_init();
    // bool
    // Display
    ili9341_prepare();
    ili9341_init();

    // TODO: need mutex to ensure this is disabled whenever we play sound
    printf("intializing random\n");
    bootloader_random_enable();
    initRandom();

    // TODO: sound
    // odroid_audio_init(16000);
    // bootloader_random_disable(); // have to disable TRNG since sound is I2S

    printf("clearing screen\n");
    ili9341_clear(0); // clear screen

    // Finished init, hand control to game()
    game();

    // TODO: net code/multiplayer
    // tcpip_adapter_init();
    // ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    // ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    // ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    // wifi_config_t sta_config = {
    //     .sta = {
    //         .ssid = "access_point_name",
    //         .password = "password",
    //         .bssid_set = false
    //     }
    // };
    // ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    // ESP_ERROR_CHECK( esp_wifi_start() );
    // ESP_ERROR_CHECK( esp_wifi_connect() );


    // vTaskDelay(300 / portTICK_PERIOD_MS);
}

