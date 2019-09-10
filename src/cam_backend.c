#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <cJSON.h>

#include "cam_backend.h"
#include "debug.h"
#include "port.h"
#include "base64.h"

static uint8_t _cam_status;
static pid_t cam_pid;
static uint32_t assrc;
static uint32_t vssrc;
static uint16_t vrtp_port;
static uint16_t artp_port;
static uint8_t controller_ip[16];

struct srtp_cfg
{
    uint8_t master_key[16];
    uint8_t master_salt[14];
};

static struct srtp_cfg vsrtp;
static struct srtp_cfg asrtp;


void cam_backend_init()
{
    _cam_status = CAM_UNINITIALIZED;
    cam_pid = 0;
}

//support AES_CM_128_HMAC_SHA1_80
void cam_prepare(uint16_t _artp_port, uint16_t _vrtp_port, uint8_t* _ip,
        uint8_t* _vrtp_master_key, uint8_t* _vrtp_master_salt,
        uint8_t* _artp_master_key, uint8_t* _artp_master_salt)
{
    //printf("cam_prepare: %s rtp ports: a,%d v,%d\n", _ip, _artp_port, _vrtp_port);

    strncpy(controller_ip, _ip, 16);

    memcpy(vsrtp.master_key, _vrtp_master_key, 16);
    memcpy(vsrtp.master_salt, _vrtp_master_salt, 14);
    memcpy(asrtp.master_key, _vrtp_master_key, 16);
    memcpy(asrtp.master_salt, _vrtp_master_salt, 14);

    artp_port = _artp_port;
    vrtp_port = _vrtp_port;
    vssrc = homekit_random() & 0x00FFFFFF;
    assrc = homekit_random() & 0x00FFFFFF;

    //printf("vssrc=%u, assrc=%u\n", vssrc, assrc);

    _cam_status = CAM_INITIALIZED;
}


static char ffmpeg_cmd_path[]="/usr/bin/ffmpeg";
static char _ff_video_size[] = "640x360";
static char _ff_fps[] = "10";
static char _ff_vssrc[16];
static char _ff_vsrtp[128];
static char _ff_url[1024];
#if 0
static char* ffmpeg_args[] = {
    ffmpeg_cmd_path,
    "-f", "video4linux2",
    "-input_format", "h264",
    "-video_size", _ff_video_size,
    "-framerate", _ff_fps,
    "-i", "/dev/video0",
    "-vcodec", "copy",
    "-copyts", "-an", "-payload_type", "99",
    "-ssrc", _ff_vssrc,
    "-f", "rtp",
    "-srtp_out_suite", "AES_CM_128_HMAC_SHA1_80",
    "-srtp_out_params", _ff_vsrtp, _ff_url,
    NULL
};
#else
static char* ffmpeg_args[] = {
    ffmpeg_cmd_path,
    "-analyzeduration", "50",
    "-i", "http://127.0.0.1:7777/live",
    "-input_format", "h264",
    "-video_size", _ff_video_size,
    "-framerate", _ff_fps,
    "-vcodec", "copy",
    "-tune", "zerolatency",
    "-copyts", "-an", "-payload_type", "99",
    "-ssrc", _ff_vssrc,
    "-f", "rtp",
    "-srtp_out_suite", "AES_CM_128_HMAC_SHA1_80",
    "-srtp_out_params", _ff_vsrtp, _ff_url,
    NULL
};
#endif
#define FF_SRTP_URL_TEMPLATE \
    "srtp://%s:%d?"\
    "rtcpport=%d&localrtcpport=%d&pkt_size=1378"
#if 1
void cam_start()
{
    if (_cam_status!=CAM_INITIALIZED)
        return;
    cam_pid = fork();
    _cam_status = CAM_STARTED;
    if (cam_pid==0)
    {
        //I'm the child process
        system("v4l2-ctl --set-ctrl video_bitrate=300000");
        snprintf(_ff_vssrc, 16, "%d", vssrc);
        base64_encode((uint8_t*)&vsrtp,sizeof(struct srtp_cfg), _ff_vsrtp);
        snprintf(_ff_url, 1024,
                FF_SRTP_URL_TEMPLATE,
                controller_ip,
                vrtp_port,vrtp_port,vrtp_port);
        printf("%s ", ffmpeg_cmd_path);
        char** aps = ffmpeg_args;
        int i=0;
        while (aps[i]!=NULL)
        {
            printf("%s ",aps[i]);
            i++;
        }
        printf("\n");
        execv(ffmpeg_cmd_path, ffmpeg_args);
        fprintf(stderr,"error!\n");
        exit(0);
    }
}
void cam_kill()
{
    if (_cam_status!=CAM_STARTED)
        return;
    int r = kill(cam_pid,SIGKILL);
    cam_pid = 0;
    _cam_status = CAM_UNINITIALIZED;
}


#else
//call camera_daemon to start streaming
void cam_start()
{
    if (_cam_status!=CAM_INITIALIZED)
        return;

    base64_encode((uint8_t*)&vsrtp,sizeof(struct srtp_cfg), _ff_vsrtp);

    cJSON* srtp_cfg = cJSON_CreateObject();
    cJSON* json_addr = cJSON_CreateString(controller_ip);
    cJSON_AddItemToObject(srtp_cfg, "addr", json_addr);
    cJSON* json_port = cJSON_CreateNumber(vrtp_port);
    cJSON_AddItemToObject(srtp_cfg, "port", json_port);
    cJSON* json_ssrc = cJSON_CreateNumber(vssrc);
    cJSON_AddItemToObject(srtp_cfg, "ssrc", json_ssrc);
    cJSON* json_key = cJSON_CreateString(_ff_vsrtp);
    cJSON_AddItemToObject(srtp_cfg, "key", json_key);

    char* str = cJSON_Print(srtp_cfg);
    FILE* cfg = fopen("/tmp/srtp_cfg.json", "w");
    fwrite(str, sizeof(uint8_t), strlen(str), cfg);
    fclose(cfg);
    cJSON_Delete(srtp_cfg);

    system("curl -d \"@/tmp/srtp_cfg.json\" -X POST http://127.0.0.1:7777/srtp_cast_to");

}
void cam_kill()
{
    system("curl http://127.0.0.1:7777/stop_srtp");
}
#endif

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

