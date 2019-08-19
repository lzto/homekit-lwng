#include <stdio.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

bool led_on = false;

void led_write(bool on) {
}

void led_init() {
}

void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
}

homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_on = value.bool_value;
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
            HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "Sample LED"),
                    HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
                    HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "037A2BABF19D"),
                    HOMEKIT_CHARACTERISTIC(MODEL, "MyLED"),
                    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
                    HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
                    NULL
                    }),
            HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "Sample LED"),
                    HOMEKIT_CHARACTERISTIC(
                            ON, false,
                            .getter=led_on_get,
                            .setter=led_on_set
                            ),
                    NULL
                    }),
            NULL
            }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

int main()
{
    homekit_server_init(&config);
    return 0;
}

