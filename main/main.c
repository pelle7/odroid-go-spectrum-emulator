#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_spiffs.h"
#include "driver/rtc_io.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"


#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_system.h"

#include "../components/spectrum/compr.c"
#include "../components/spectrum/interf.c"
#include "../components/spectrum/keynames.c"
#include "../components/spectrum/loadim.c"
#include "../components/spectrum/misc.c"
#include "../components/spectrum/rom_imag.c"
#include "../components/spectrum/snapshot.c"
#include "../components/spectrum/spconf.c"
#include "../components/spectrum/spect.c"
#include "../components/spectrum/spectkey.c"
#include "../components/spectrum/spkey.c"
#include "../components/spectrum/spmain.c"
#include "../components/spectrum/spperif.c"
#include "../components/spectrum/spscr.c"
#include "../components/spectrum/spsound.c"
#include "../components/spectrum/sptape.c"
#include "../components/spectrum/sptiming.c"
#include "../components/spectrum/stubs.c"
#include "../components/spectrum/tapefile.c"
#include "../components/spectrum/vgakey.c"
#include "../components/spectrum/vgascr.c"
#include "../components/spectrum/z80.c"
#include "../components/spectrum/z80_op1.c"
#include "../components/spectrum/z80_op2.c"
#include "../components/spectrum/z80_op3.c"
#include "../components/spectrum/z80_op4.c"
#include "../components/spectrum/z80_op5.c"
#include "../components/spectrum/z80_op6.c"
#include "../components/spectrum/z80optab.c"
#include "../components/spectrum/z80_step.c"

#include <string.h>

extern int debug_trace;


odroid_battery_state battery_state;

#define AUDIO_SAMPLE_RATE (16000)

int BatteryPercent = 100;

uint16_t* menuFramebuffer = 0;


/* TODO
static void DoHome()
{
    esp_err_t err;
    uint16_t* param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");
    odroid_audio_terminate();


    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning) { vTaskDelay(1); }


    // state
    printf("PowerDown: Saving state.\n");
    SaveState();


    // Set menu application
    odroid_system_application_set(0);


    // Reset
    esp_restart();
}
*/
//----------------------------------------------------------------
void app_main(void)
{

    printf("spectrum start.\n");

    nvs_flash_init();

    odroid_system_init();

    odroid_input_gamepad_init();

    odroid_input_battery_level_init(); //djk

    // Boot state overrides
    bool forceConsoleReset = false;

    ili9341_prepare();

    // Disable LCD CD to prevent garbage
    const gpio_num_t LCD_PIN_NUM_CS = GPIO_NUM_5;

    gpio_config_t io_conf = { 0 };
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LCD_PIN_NUM_CS);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);
    gpio_set_level(LCD_PIN_NUM_CS, 1);


    switch (esp_sleep_get_wakeup_cause())
    {
        case ESP_SLEEP_WAKEUP_EXT0:
        {
            printf("app_main: ESP_SLEEP_WAKEUP_EXT0 deep sleep wake\n");
            break;
        }

        case ESP_SLEEP_WAKEUP_EXT1:
        case ESP_SLEEP_WAKEUP_TIMER:
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
        case ESP_SLEEP_WAKEUP_ULP:
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        {
            printf("app_main: Non deep sleep startup\n");

            odroid_gamepad_state bootState = odroid_input_read_raw();

            if (bootState.values[ODROID_INPUT_MENU])
            {
                // Force return to factory app to recover from
                // ROM loading crashes

                // Set factory app
                const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
                if (partition == NULL)
                {
                    abort();
                }

                esp_err_t err = esp_ota_set_boot_partition(partition);
                if (err != ESP_OK)
                {
                    abort();
                }

                // Reset
                esp_restart();
            }

            if (bootState.values[ODROID_INPUT_START])
            {
                // Reset emulator if button held at startup to
                // override save state
                forceConsoleReset = true;
            }

            break;
        }
        default:
            printf("app_main: Not a deep sleep reset\n");
            break;
    }


    // Display
    ili9341_init();
    odroid_display_show_splash();

    // Audio hardware
    odroid_audio_init(AUDIO_SAMPLE_RATE);

//djk
//----------------------------------
    odroid_gamepad_state previousState;
    odroid_input_gamepad_read(&previousState);

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;
    uint16_t muteFrameCount = 0;
    uint16_t powerFrameCount = 0;

    bool ignoreMenuButton = previousState.values[ODROID_INPUT_MENU];
//----------------------------------


    // Start Spectrum emulation
    sp_init();
    start_spectemu();
}

