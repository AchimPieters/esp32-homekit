#include <string.h>
#include <ctype.h>
#include "constants.h"
#include "debug.h"
#include "crypto.h"
#include "pairing.h"
#include "port.h"

#include "nvs_flash.h"
#include "nvs.h"

#define ACCESSORY_ID_KEY "hk_accessory_id"
#define ACCESSORY_KEY_KEY "hk_accessory_key"
#define PAIRING_KEY_PREFIX "hk_pairing_"

static nvs_handle_t nvs_handle;

int homekit_storage_init() {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);

        err = nvs_open("homekit", NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
                ERROR("Error (%s) opening NVS handle!", esp_err_to_name(err));
                return -1;
        }
        return 0;
}

int homekit_storage_reset() {
        esp_err_t err;

        err = nvs_erase_key(nvs_handle, ACCESSORY_ID_KEY);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ERROR("Failed to reset HomeKit: error removing accessory ID data (code %d)", err);
        }

        err = nvs_erase_key(nvs_handle, ACCESSORY_KEY_KEY);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ERROR("Failed to reset HomeKit: error removing accessory key data (code %d)", err);
        }

        nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, "homekit", NVS_TYPE_ANY);
        while (it != NULL) {
                nvs_entry_info_t info;
                nvs_entry_info(it, &info);

                if (strncmp(info.key, PAIRING_KEY_PREFIX, sizeof(PAIRING_KEY_PREFIX) - 1) == 0) {
                        err = nvs_erase_key(nvs_handle, info.key);
                        if (err != ESP_OK) {
                                ERROR("Failed to reset HomeKit: failed to remove pairing %s (code %d)", info.key, err);
                        }
                }
                it = nvs_entry_next(it);
        }

        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
                ERROR("Failed to commit NVS changes (code %d)", err);
                return -1;
        }

        return 0;
}

static char ishex(unsigned char c) {
        c = toupper(c);
        return isdigit(c) || (c >= 'A' && c <= 'F');
}

void homekit_storage_save_accessory_id(const char *accessory_id) {
        esp_err_t err = nvs_set_str(nvs_handle, ACCESSORY_ID_KEY, accessory_id);
        if (err != ESP_OK) {
                ERROR("Failed to write accessory ID to HomeKit storage (code %d)", err);
        }
        nvs_commit(nvs_handle);
}

int homekit_storage_load_accessory_id(char *data) {
        size_t required_size;
        esp_err_t err = nvs_get_str(nvs_handle, ACCESSORY_ID_KEY, NULL, &required_size);
        if (err != ESP_OK) {
                ERROR("Failed to read accessory ID from HomeKit storage (code %d)", err);
                return -1;
        }

        char *key = malloc(required_size);
        err = nvs_get_str(nvs_handle, ACCESSORY_ID_KEY, key, &required_size);
        if (err != ESP_OK) {
                ERROR("Failed to read accessory ID from HomeKit storage (code %d)", err);
                free(key);
                return -1;
        }

        if (strlen(key) != ACCESSORY_ID_SIZE) {
                free(key);
                return -2;
        }

        for (int i = 0; i < ACCESSORY_ID_SIZE; i++) {
                if (i % 3 == 2) {
                        if (key[i] != ':') {
                                free(key);
                                return -3;
                        }
                } else if (!ishex(key[i])) {
                        free(key);
                        return -4;
                }
        }

        strcpy(data, key);
        free(key);

        return 0;
}

void homekit_storage_save_accessory_key(const ed25519_key *key) {
        byte key_data[ACCESSORY_KEY_SIZE];
        size_t key_data_size = sizeof(key_data);
        int r = crypto_ed25519_export_key(key, key_data, &key_data_size);
        if (r) {
                ERROR("Failed to export accessory key (code %d)", r);
                return;
        }

        esp_err_t err = nvs_set_blob(nvs_handle, ACCESSORY_KEY_KEY, key_data, key_data_size);
        if (err != ESP_OK) {
                ERROR("Failed to write accessory key to HomeKit storage (code %d)", err);
        }
        nvs_commit(nvs_handle);
}

int homekit_storage_load_accessory_key(ed25519_key *key) {
        size_t key_data_size = ACCESSORY_KEY_SIZE;
        byte key_data[ACCESSORY_KEY_SIZE];

        esp_err_t err = nvs_get_blob(nvs_handle, ACCESSORY_KEY_KEY, key_data, &key_data_size);
        if (err != ESP_OK) {
                ERROR("Failed to read accessory key from HomeKit storage (code %d)", err);
                return -1;
        }

        crypto_ed25519_init(key);
        int r = crypto_ed25519_import_key(key, key_data, key_data_size);
        if (r) {
                ERROR("Failed to import accessory key (code %d)", r);
                return -2;
        }

        return 0;
}

typedef struct {
        byte device_public_key[32];
        byte permissions;
} pairing_data_t;

bool homekit_storage_can_add_pairing() {
        return true;
}

int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, byte permissions) {
        pairing_data_t data;

        data.permissions = permissions;
        size_t device_public_key_size = sizeof(data.device_public_key);
        int r = crypto_ed25519_export_public_key(device_key, data.device_public_key, &device_public_key_size);
        if (r) {
                ERROR("Failed to export device public key (code %d)", r);
                return -1;
        }

        char nvs_key[48];
        snprintf(nvs_key, sizeof(nvs_key), PAIRING_KEY_PREFIX "%s", device_id);

        esp_err_t err = nvs_set_blob(nvs_handle, nvs_key, &data, sizeof(data));
        if (err != ESP_OK) {
                ERROR("Failed to write pairing info to HomeKit storage (code %d)", err);
                return -1;
        }
        nvs_commit(nvs_handle);

        return 0;
}

int homekit_storage_update_pairing(const char *device_id, byte permissions) {
        char nvs_key[48];
        snprintf(nvs_key, sizeof(nvs_key), PAIRING_KEY_PREFIX "%s", device_id);

        pairing_data_t data;
        size_t data_size = sizeof(data);

        esp_err_t err = nvs_get_blob(nvs_handle, nvs_key, &data, &data_size);
        if (err != ESP_OK) {
                ERROR("Failed to update pairing: pairing does not exist (code %d)", err);
                return -2;
        }

        data.permissions = permissions;

        err = nvs_set_blob(nvs_handle, nvs_key, &data, sizeof(data));
        if (err != ESP_OK) {
                ERROR("Failed to update pairing: error writing updated pairing data (code %d)", err);
                return -1;
        }
        nvs_commit(nvs_handle);

        return 0;
}

int homekit_storage_remove_pairing(const char *device_id) {
        char nvs_key[48];
        snprintf(nvs_key, sizeof(nvs_key), PAIRING_KEY_PREFIX "%s", device_id);

        esp_err_t err = nvs_erase_key(nvs_handle, nvs_key);
        if (err != ESP_OK) {
                ERROR("Failed to remove pairing from HomeKit storage (code %d)", err);
                return -2;
        }
        nvs_commit(nvs_handle);

        return 0;
}

int homekit_storage_find_pairing(const char *device_id, pairing_t *pairing) {
        char nvs_key[48];
        snprintf(nvs_key, sizeof(nvs_key), PAIRING_KEY_PREFIX "%s", device_id);

        pairing_data_t data;
        size_t data_size = sizeof(data);

        esp_err_t err = nvs_get_blob(nvs_handle, nvs_key, &data, &data_size);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
                return -1;
        }

        if (err != ESP_OK) {
                ERROR("Failed to find pairing (code %d)", err);
                return -1;
        }

        crypto_ed25519_init(&pairing->device_key);
        int r = crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key));
        if (r) {
                ERROR("Failed to import device public key (code %d)", r);
                return -2;
        }

        strncpy(pairing->device_id, device_id, DEVICE_ID_SIZE);
        pairing->device_id[DEVICE_ID_SIZE] = 0;
        pairing->permissions = data.permissions;

        return 0;
}

void homekit_storage_pairing_iterator_init(pairing_iterator_t *it) {
        nvs_iterator_t iter = nvs_entry_find(NVS_DEFAULT_PART_NAME, "homekit", NVS_TYPE_BLOB);
        if (iter == NULL) {
                it->context = NULL;
                return;
        }

        it->context = iter;
}

void homekit_storage_pairing_iterator_done(pairing_iterator_t *it) {
        if (it->context) {
                nvs_release_iterator(it->context);
                it->context = NULL;
        }
}

int homekit_storage_next_pairing(pairing_iterator_t *it, pairing_t *pairing) {
        if (!it->context) {
                return -1;
        }

        nvs_entry_info_t info;
        nvs_entry_info(it->context, &info);

        if (strncmp(info.key, PAIRING_KEY_PREFIX, sizeof(PAIRING_KEY_PREFIX) - 1) == 0 &&
            strlen(info.key) == (sizeof(PAIRING_KEY_PREFIX) - 1 + DEVICE_ID_SIZE)) {

                pairing_data_t data;
                size_t data_size = sizeof(data);

                esp_err_t err = nvs_get_blob(nvs_handle, info.key, &data, &data_size);
                if (err != ESP_OK) {
                        ERROR("Failed to get pairing data (code %d)", err);
                        return -1;
                }

                crypto_ed25519_init(&pairing->device_key);
                int r = crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key));
                if (r) {
                        ERROR("Failed to import device public key (code %d)", r);
                        return -1;
                }

                strncpy(pairing->device_id, info.key + sizeof(PAIRING_KEY_PREFIX) - 1, DEVICE_ID_SIZE);
                pairing->device_id[DEVICE_ID_SIZE] = 0;
                pairing->permissions = data.permissions;

                it->context = nvs_entry_next(it->context);
                return 0;
        }

        it->context = nvs_entry_next(it->context);
        return -1;
}
