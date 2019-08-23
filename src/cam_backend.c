#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "cam_backend.h"
#include "debug.h"
#include "port.h"

static uint8_t _cam_status;
static pid_t cam_pid;
static uint32_t assrc;
static uint32_t vssrc;
static uint16_t vrtp_port;
static uint16_t artp_port;
static uint8_t controller_ip[16];

void cam_backend_init()
{
    _cam_status = CAM_UNINITIALIZED;
    cam_pid = 0;
}

void cam_prepare(uint16_t _artp_port, uint16_t _vrtp_port, uint8_t* _ip)
{
    printf("cam_prepare: %s rtp ports: a,%d v,%d\n", _ip, _artp_port, _vrtp_port);
    strncpy(controller_ip, _ip, 16);
    artp_port = _artp_port;
    vrtp_port = _vrtp_port;
    vssrc = homekit_random() & 0x00FFFFFF;
    assrc = homekit_random() & 0x00FFFFFF;

    printf("vssrc=%u, assrc=%u\n", vssrc, assrc);

    _cam_status = CAM_INITIALIZED;
}

void cam_start()
{
    //cam_pid = fork();
    //if (!cam_pid)
    //  exec();
    //set flag
}

void cam_kill()
{
    //kill(cam_pid);
    //cam_pid = 0;
    _cam_status = CAM_UNINITIALIZED;
}

uint8_t cam_status()
{
    return _cam_status;
}

uint32_t cam_get_assrc()
{
    return assrc;
}

uint32_t cam_get_vssrc()
{
    return vssrc;
}

uint16_t cam_get_vrtp_port()
{
    return vrtp_port;
}

uint16_t cam_get_artp_port()
{
    return artp_port;
}

uint8_t* cam_get_ip()
{
    return controller_ip;
}

