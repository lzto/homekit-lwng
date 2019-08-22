#define _GNU_SOURCE 1
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>

#include "debug.h"
#include "crypto.h"
#include "pairing.h"
#include "port.h"

#include <cJSON.h>

#pragma GCC diagnostic ignored "-Wunused-value"


#define ACCESSORY_ID_SIZE   17
#define ACCESSORY_KEY_SIZE  64


#define CFG_SUFFIX "/.homekit_lwng\0"

char CFG[128];

int homekit_storage_reset() {
    unlink(CFG);
    FILE* cfg = fopen(CFG,"w");
    fclose(cfg);
    return 0;
}

int homekit_storage_init() {

    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strncpy(CFG, homedir, 128);
    strcat(CFG, CFG_SUFFIX);

    FILE* file;
    if (file = fopen(CFG, "r"))
    {
        fclose(file);
        return 0;
    }
    return -1;
}

#define ACCESSORY_ID "accessory_id"
#define ACCESSORY_KEY "accessory_key"
#define PAIRINGS "pairings"

cJSON* load_cfg()
{
    cJSON * lwng_json = NULL;
    struct stat sb;
    int fd = open(CFG, O_RDONLY);
    if (fd == -1)
        goto end1;

    if (fstat(fd, &sb) == -1)           /* To obtain file size */
        goto end2;

    uint8_t* fmap = mmap(NULL, sb.st_size , PROT_READ, MAP_PRIVATE, fd, 0);
    if (fmap == MAP_FAILED)
        goto end2;
    //parse and get result
    lwng_json = cJSON_Parse(fmap);
    if (!lwng_json)
    {
        goto end3;
    }

end3:
    munmap(fmap, sb.st_size);
end2:
    close(fd);
end1:
    if (!lwng_json)
        lwng_json = cJSON_CreateObject();
    return lwng_json;

}

bool store_cfg(cJSON* lwng_json)
{
    char* string = cJSON_Print(lwng_json);
    //printf("store CFG:%s\n", string);
    //write to file
    FILE* cfg = fopen(CFG,"w");
    if (!cfg)
    {
        printf("failed to open %s\n", CFG);
        return false;
    }
    fwrite(string, sizeof(uint8_t), strlen(string), cfg);
    fclose(cfg);
    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool store_property(const uint8_t* k, const uint8_t* v, size_t vlen)
{
    bool ret = false;
    cJSON *lwng_json = load_cfg();
    if (!lwng_json)
        goto fail1;
    cJSON *value = cJSON_CreateString(v);
    if (!value)
        goto fail2;
    cJSON_AddItemToObject(lwng_json, k, value);
    ret = store_cfg(lwng_json);
fail2:
    cJSON_Delete(lwng_json);
fail1:
    return ret;
}

bool read_property(const uint8_t* k, uint8_t* v, size_t vlen)
{
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto fail1;
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(lwng_json, k);
    if (cJSON_IsString(value) && (value->valuestring != NULL))
    {
        strncpy(v, value->valuestring, vlen);
    }
    cJSON_Delete(lwng_json);
    ret = true;
fail1:
    return ret;
}

bool append_array_property(const uint8_t* k, const uint8_t* v, size_t vlen)
{
    //printf("append_array_property %s=%s\n", k, v);
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto end1;

    cJSON *array = cJSON_GetObjectItemCaseSensitive(lwng_json, k);
    if (!array)
    {
        array = cJSON_CreateArray();
        cJSON_AddItemToObject(lwng_json,k,array);
    }

    cJSON *element = cJSON_CreateObject();
    cJSON *value = cJSON_CreateString(v);
    cJSON_AddItemToObject(element, k, value);
    cJSON_AddItemToArray(array, element);

    store_cfg(lwng_json);
    cJSON_Delete(lwng_json);
    ret = true;
end1:
    return ret;
}

void homekit_storage_save_accessory_id(const char *accessory_id) {
    if (!store_property(ACCESSORY_ID, (byte *)accessory_id, strlen(accessory_id))) {
        ERROR("Failed to write accessory ID to flash");
    }
}

static char ishex(unsigned char c) {
    c = toupper(c);
    return isdigit(c) || (c >= 'A' && c <= 'F');
}

char *homekit_storage_load_accessory_id() {
    byte data[ACCESSORY_ID_SIZE+1];
    if (!read_property(ACCESSORY_ID, data, sizeof(data))) {
        ERROR("Failed to read accessory ID from flash");
        return NULL;
    }
    if (!data[0])
        return NULL;
    data[sizeof(data)-1] = 0;

    for (int i=0; i<17; i++) {
        if (i % 3 == 2) {
            if (data[i] != ':')
                return NULL;
        } else if (!ishex(data[i]))
            return NULL;
    }

    return strndup((char *)data, sizeof(data));
}

void homekit_storage_save_accessory_key(const ed25519_key *key) {
    byte key_data[ACCESSORY_KEY_SIZE];
    size_t key_data_size = sizeof(key_data);
    int r = crypto_ed25519_export_key(key, key_data, &key_data_size);
    if (r) {
        ERROR("Failed to export accessory key (code %d)", r);
        return;
    }

    if (!store_property(ACCESSORY_KEY, key_data, key_data_size)) {
        ERROR("Failed to write accessory key to flash");
        return;
    }
}

ed25519_key *homekit_storage_load_accessory_key() {
    byte key_data[ACCESSORY_KEY_SIZE];
    if (!read_property(ACCESSORY_KEY, key_data, sizeof(key_data))) {
        ERROR("Failed to read accessory key from flash");
        return NULL;
    }

    ed25519_key *key = crypto_ed25519_new();
    int r = crypto_ed25519_import_key(key, key_data, sizeof(key_data));
    if (r) {
        ERROR("Failed to import accessory key (code %d)", r);
        crypto_ed25519_free(key);
        return NULL;
    }

    return key;
}

typedef struct {
    char device_id[36];
    byte device_public_key[32];
    byte permissions;
} pairing_data_t;


bool homekit_storage_can_add_pairing() {
    pairing_data_t* data;
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto end1;

    cJSON *array = cJSON_GetObjectItemCaseSensitive(lwng_json, PAIRINGS);
    cJSON *element;
    cJSON_ArrayForEach(element, array)
    {
        cJSON* value = cJSON_GetObjectItemCaseSensitive(element, PAIRINGS);
        if (cJSON_IsString(value) && (value->valuestring != NULL))
        {
            data = (pairing_data_t*) value->valuestring;
            ret = true;
            goto end2;
        }
    }
end2:
    cJSON_Delete(lwng_json);
end1:
    return ret;
}

int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, byte permissions) {
    pairing_data_t data;

    memset(&data, 0, sizeof(data));
    data.permissions = permissions;
    strncpy(data.device_id, device_id, sizeof(data.device_id));
    size_t device_public_key_size = sizeof(data.device_public_key);
    int r = crypto_ed25519_export_public_key(
            device_key, data.device_public_key, &device_public_key_size
            );
    if (r) {
        ERROR("Failed to export device public key (code %d)", r);
        return -1;
    }

    if (!append_array_property(PAIRINGS, (byte *)&data, sizeof(data))) {
        ERROR("Failed to write pairing info to flash");
        return -1;
    }

    return 0;
}


int homekit_storage_update_pairing(const char *device_id, byte permissions) {

    pairing_data_t* data;
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto end1;

    int i=0;
    cJSON *array = cJSON_GetObjectItemCaseSensitive(lwng_json, PAIRINGS);
    cJSON *element;
    cJSON_ArrayForEach(element, array)
    {
        cJSON* value = cJSON_GetObjectItemCaseSensitive(element, PAIRINGS);
        if (cJSON_IsString(value) && (value->valuestring != NULL))
        {
            data = (pairing_data_t*) value->valuestring;

            if (!strncmp(data->device_id, device_id, sizeof(data->device_id))) {
                ed25519_key *device_key = crypto_ed25519_new();
                int r = crypto_ed25519_import_public_key(device_key,
                        data->device_public_key, 
                        sizeof(data->device_public_key));
                if (r) {
                    ERROR("Failed to import device public key (code %d)", r);
                    crypto_ed25519_free(device_key);
                    return -2;
                }

                size_t device_public_key_size = sizeof(data->device_public_key);
                r = crypto_ed25519_export_public_key(
                        device_key, data->device_public_key, &device_public_key_size
                        );
                if (r) {
                    ERROR("Failed to export device public key (code %d)", r);
                    return -1;
                }

                data->permissions = permissions;

                crypto_ed25519_free(device_key);
                if (r) {
                    return -2;
                }
                store_cfg(lwng_json);
                cJSON_Delete(lwng_json);
                return 0;
            }
        }
    }
end1:
    return -1;
}


int homekit_storage_remove_pairing(const char *device_id) {
    pairing_data_t* data;
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto end1;

    int i=0;
    cJSON *array = cJSON_GetObjectItemCaseSensitive(lwng_json, PAIRINGS);
    cJSON *element;
    cJSON_ArrayForEach(element, array)
    {
        cJSON* value = cJSON_GetObjectItemCaseSensitive(element, PAIRINGS);
        if (cJSON_IsString(value) && (value->valuestring != NULL))
        {
            data = (pairing_data_t*) value->valuestring;

            //FIXME! have problem???
            if (!strncmp(data->device_id, device_id, sizeof(data->device_id))) {
                cJSON_DeleteItemFromArray(array,i);
                i--;
            }
        }
        i++;
    }
    store_cfg(lwng_json);
    cJSON_Delete(lwng_json);

end1:
    return 0;

}


pairing_t *homekit_storage_find_pairing(const char *device_id) {
    pairing_data_t* data;
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto end1;

    int i=0;
    cJSON *array = cJSON_GetObjectItemCaseSensitive(lwng_json, PAIRINGS);
    cJSON *element;
    cJSON_ArrayForEach(element, array)
    {
        cJSON* value = cJSON_GetObjectItemCaseSensitive(element, PAIRINGS);
        if (cJSON_IsString(value) && (value->valuestring != NULL))
        {
            data = (pairing_data_t*) value->valuestring;

            if (!strncmp(data->device_id, device_id, sizeof(data->device_id))) {
                ed25519_key *device_key = crypto_ed25519_new();
                int r = crypto_ed25519_import_public_key(device_key,
                        data->device_public_key, sizeof(data->device_public_key));
                if (r) {
                    ERROR("Failed to import device public key (code %d)", r);
                    return NULL;
                }

                pairing_t *pairing = pairing_new();
                pairing->id = i;
                pairing->device_id = strndup(data->device_id, sizeof(data->device_id));
                pairing->device_key = device_key;
                pairing->permissions = data->permissions;
                cJSON_Delete(lwng_json);
                return pairing;
            }
        }
        i++;
    }
end2:
    cJSON_Delete(lwng_json);
end1:
    return NULL;

}


typedef struct {
    int idx;
} pairing_iterator_t;


pairing_iterator_t *homekit_storage_pairing_iterator() {
    pairing_iterator_t *it = malloc(sizeof(pairing_iterator_t));
    it->idx = 0;
    return it;
}


void homekit_storage_pairing_iterator_free(pairing_iterator_t *iterator) {
    free(iterator);
}


pairing_t *homekit_storage_next_pairing(pairing_iterator_t *it) {
    pairing_data_t* data;
    bool ret = false;
    cJSON* lwng_json = load_cfg();
    if (!lwng_json)
        goto end1;

    int i=0;
    cJSON *array = cJSON_GetObjectItemCaseSensitive(lwng_json, PAIRINGS);
    cJSON *element;
    cJSON_ArrayForEach(element, array)
    {
        if (i<it->idx)
        {
            i++;
            continue;
        }
        int id = it->idx++;
        element = cJSON_GetArrayItem(array, id);
        cJSON* value = cJSON_GetObjectItemCaseSensitive(element, PAIRINGS);
        if (cJSON_IsString(value) && (value->valuestring != NULL))
        {
            data = (pairing_data_t*) value->valuestring;

            ed25519_key *device_key = crypto_ed25519_new();
            int r = crypto_ed25519_import_public_key(device_key,
                    data->device_public_key, sizeof(data->device_public_key));
            if (r) {
                ERROR("Failed to import device public key (code %d)", r);
                crypto_ed25519_free(device_key);
                it->idx++;
                continue;
            }

            pairing_t *pairing = pairing_new();
            pairing->id = id;
            pairing->device_id = strndup(data->device_id, sizeof(data->device_id));
            pairing->device_key = device_key;
            pairing->permissions = data->permissions;
            cJSON_Delete(lwng_json);
            return pairing;
        }
    }
end2:
    cJSON_Delete(lwng_json);
end1:
    return NULL;
}

