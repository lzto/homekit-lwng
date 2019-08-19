#pragma once

#include <stdint.h>

uint32_t homekit_random();
void homekit_random_fill(uint8_t *data, size_t size);

void homekit_system_restart();

void homekit_mdns_init();
void homekit_mdns_configure_init(const char *instance_name, int port);
void homekit_mdns_add_txt(const char *key, const char *format, ...);
void homekit_mdns_configure_finalize();
