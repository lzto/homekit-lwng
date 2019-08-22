#include <stdio.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

//device identify
void led_identify(homekit_value_t _value) {
    printf("Got Identify Request!\n");
}

//what is this???
uint8_t* handle_resource(const char *body, size_t body_size)
{
    printf("handle resource : %s\n", body);
    return "/tmp/1.jpeg\0";
}

//streaming status
//0 - available
//1 - in use
//2 - unavailable
enum {
    CAM_STATUS_AVAILABLE=0,
    CAM_STATUS_IN_USE=1,
    CAM_STATUS_UNAVAILABLE=2,
}CamStatusCode;

homekit_value_t cam_status_get(const homekit_characteristic_t *ch)
{
    printf("cam_status_get\n");
    tlv_values_t *r = tlv_new();
    tlv_add_integer_value(r, 1, sizeof(uint8_t), CAM_STATUS_AVAILABLE);
    return HOMEKIT_TLV(r);
}

//selected rtp stream configuration
homekit_value_t cam_selected_rtp_stream_cfg_get(const homekit_characteristic_t *ch)
{
    printf("cam_selected_rtp_stream_cfg_get\n");
    tlv_values_t * r = tlv_new();
    return HOMEKIT_TLV(r);
}

void cam_selected_rtp_stream_cfg_set(homekit_characteristic_t *ch, const homekit_value_t value)
{
    printf("cam_selected_rtp_stream_cfg_set\n");
    //TODO!
}

//supported video stream configuration
homekit_value_t cam_supported_video_cfg_get(const homekit_characteristic_t *ch)
{
    printf("cam_supported_video_cfg_get\n");
    tlv_values_t * r = tlv_new();

    //video codec configuration
    tlv_values_t *part1 = tlv_new();

    //video codec type
    tlv_add_integer_value(part1, 1, sizeof(uint8_t), 0);
    //video codec parameters
    
    tlv_values_t* codec_parameters = tlv_new();
    //ProfileID
    //0 - constrained baseline profile
    //1 - main profile
    //2 - high profile
    tlv_add_integer_value(codec_parameters, 1, sizeof(uint8_t), 0);

    //Level
    tlv_add_integer_value(codec_parameters, 2, sizeof(uint8_t),  0);

    //Packetization mode
    tlv_add_integer_value(codec_parameters, 3, sizeof(uint8_t), 0);

    //CVO Enabled - 0 - cvo not supported - 1 - cvo supported
    tlv_add_integer_value(codec_parameters, 4, sizeof(uint8_t), 0);

    //cvo id 
    tlv_add_integer_value(codec_parameters, 5, sizeof(uint8_t), 1);

    tlv_add_tlv_value(part1, 2, codec_parameters);

    //video attributes
    tlv_values_t* video_attribute = tlv_new();

    //image width, height, framerate
    tlv_add_integer_value(video_attribute, 1, 2*sizeof(uint8_t), 640);
    tlv_add_integer_value(video_attribute, 2, 2*sizeof(uint8_t), 360);
    tlv_add_integer_value(video_attribute, 3, sizeof(uint8_t), 10);

    tlv_add_tlv_value(part1, 3, video_attribute);

    tlv_add_tlv_value(r, 1, part1);

    return HOMEKIT_TLV(r);
}

//supported audio stream configuration
homekit_value_t cam_supported_audio_cfg_get(const homekit_characteristic_t *ch)
{
    printf("cam_supported_audio_cfg_get\n");
    tlv_values_t * r = tlv_new();

    //part 1
    tlv_values_t * part1 = tlv_new();
    
    //2 - aac-eld
    //3 - opus
    //5 - amr
    //6 - amr-wb
    tlv_add_integer_value(part1, 1, 2*sizeof(uint8_t), 2);
    tlv_add_string_value(part1, 2, " ");

    tlv_add_tlv_value(r, 1, part1);
    //part 2 - comfort noise support - no support - 0
    tlv_add_integer_value(r, 2, sizeof(uint8_t), 0);

    return HOMEKIT_TLV(r);
}

//supported rtp configuration
homekit_value_t cam_supported_rtp_cfg_get(const homekit_characteristic_t *ch)
{
    printf("cam_supported_rtp_cfg_get\n");
    tlv_values_t * r = tlv_new();
    //support AES_CM_128_HMAC_SHA1_80
    tlv_add_integer_value(r, 2, sizeof(uint8_t), 0);

    return HOMEKIT_TLV(r);
}


//cam setup endpoints
homekit_value_t cam_setup_endpoints_get(const homekit_characteristic_t *ch)
{
    printf("cam_setup_endpoints_get\n");
    tlv_values_t * r = tlv_new();
    return HOMEKIT_TLV(r);
}

void cam_setup_endpoints_set(homekit_characteristic_t *ch, const homekit_value_t value)
{
    printf("cam_setup_endpoints_set\n");
}


#if 0
homekit_value_t pt_get() {
    printf("pt_get\n");
    return HOMEKIT_BOOL(false);
}

void pt_set(homekit_value_t value) {
    printf("pantilt: l\n");
}
#endif

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1,
            .category=homekit_accessory_category_ip_camera,
            .services=(homekit_service_t*[]){
            HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "Skycam"),
                    HOMEKIT_CHARACTERISTIC(MANUFACTURER, "RaspberryPi"),
                    HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "B827EB8D0462"),
                    HOMEKIT_CHARACTERISTIC(MODEL, "Skycam"),
                    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
                    HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
                    NULL
                    }),
#if 0
            HOMEKIT_SERVICE(CAMERA_CONTROL, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "camera-control"),
                    HOMEKIT_CHARACTERISTIC(
                            ON, false,
                            .getter=pt_get,
                            .setter=pt_set
                            ),
                    NULL
                    }),
#endif
            HOMEKIT_SERVICE(CAMERA_RTP_STREAM_MANAGEMENT,
                    .primary=true,
                    .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "Viewport"),
                    HOMEKIT_CHARACTERISTIC(
                            STREAMING_STATUS,
                            .getter_ex=cam_status_get,
                            ),
                    HOMEKIT_CHARACTERISTIC(
                            SELECTED_RTPS_CONFIGURATION,
                            .getter_ex=cam_selected_rtp_stream_cfg_get,
                            .setter_ex=cam_selected_rtp_stream_cfg_set
                            ),
                    HOMEKIT_CHARACTERISTIC(
                            SETUP_ENDPOINTS,
                            .getter_ex=cam_setup_endpoints_get,
                            .setter_ex=cam_setup_endpoints_set
                            ),
                    HOMEKIT_CHARACTERISTIC(
                            SUPPORTED_VIDEO_STREAM_CONFIGURATION,
                            .getter_ex=cam_supported_video_cfg_get
                            ),
                    HOMEKIT_CHARACTERISTIC(
                            SUPPORTED_AUDIO_STREAM_CONFIGURATION,
                            .getter_ex=cam_supported_audio_cfg_get
                            ),
                    HOMEKIT_CHARACTERISTIC(
                            SUPPORTED_RTP_CONFIGURATION,
                            .getter_ex=cam_supported_rtp_cfg_get
                            ),
                    NULL
                    }),
            NULL
            }),
            NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-22-333",
    .setupId="8086",
    .on_resource = handle_resource
};

int main()
{
    homekit_server_init(&config);
    return 0;
}

