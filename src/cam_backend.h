#ifndef CAM_BACKEND_H
#define CAM_BACKEND_H

#include "homekit/types.h"
#include "homekit/tlv.h"

void cam_backend_init();
//support AES_CM_128_HMAC_SHA1_80
void cam_prepare(uint16_t _artp_port, uint16_t _vrtp_port, uint8_t* _ip,
        uint8_t* _vrtp_master_key, uint8_t* _vrtp_master_salt,
        uint8_t* _artp_master_key, uint8_t* _artp_master_salt);

void cam_start();
void cam_kill();
uint8_t cam_status();
uint8_t* cam_get_ip();

uint32_t cam_get_assrc();
uint32_t cam_get_vssrc();

uint16_t cam_get_vrtp_port();
uint16_t cam_get_artp_port();

enum cam_backend_status
{
    CAM_UNINITIALIZED = 0,
    CAM_INITIALIZED = 1,
    CAM_STARTED = 2
};

#endif

