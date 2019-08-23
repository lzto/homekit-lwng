#ifndef CAM_BACKEND_H
#define CAM_BACKEND_H

#include "homekit/types.h"
#include "homekit/tlv.h"

void cam_backend_init();
void cam_prepare(uint16_t _artp_port, uint16_t _vrtp_port, uint8_t* _ip);
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
    CAM_INITIALIZED = 1
};

#endif

