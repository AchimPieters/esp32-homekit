/**

   Copyright 2024 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   for more information visit https://www.studiopieters.nl

 **/

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "debug.h"
#include "crypto.h"
#include "pairing.h"
#include "port.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "storage.h"

#define NVS_NAMESPACE "homekit"
#define MAX_PAIRINGS 16
#define PAIRING_KEY_PREFIX "pairing_"
#define PAIRING_KEY_MAX_LEN (sizeof(PAIRING_KEY_PREFIX) + 10 + 1) // 10 for maximum int32 + 1 for null terminator
#define ACCESSORY_KEY_SIZE 64
#define CONFIG_STATE_KEY "config_state"
#define IID_MAP_KEY "iid_map"

static nvs_handle_t homekit_nvs_handle;
static bool homekit_nvs_opened = false;

static esp_err_t homekit_storage_open() {
        if (homekit_nvs_opened) {
                return ESP_OK;
        }

        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                err = nvs_flash_init();
        }
        if (err == ESP_ERR_NVS_INVALID_STATE) {
                // NVS is already initialized by the application.
                err = ESP_OK;
        }
        if (err != ESP_OK) {
                ERROR("Failed to initialize NVS flash: %s", esp_err_to_name(err));
                return err;
        }

        err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &homekit_nvs_handle);
        if (err != ESP_OK) {
                ERROR("Failed to open NVS handle: %s", esp_err_to_name(err));
                return err;
        }

        homekit_nvs_opened = true;
        return ESP_OK;
}

typedef struct {
        char magic[4];
        uint8_t permissions;
        char device_id[DEVICE_ID_SIZE];
        uint8_t device_public_key[32];
        uint8_t _reserved[7]; // Padding for alignment
} pairing_data_t;

typedef struct {
        uint32_t config_hash;
        uint32_t config_number;
} config_state_data_t;

// Implement the storage functions

static inline void pairing_key_from_index(int idx, char *key, size_t key_size) {
        snprintf(key, key_size, PAIRING_KEY_PREFIX "%d", idx);
}

static esp_err_t homekit_storage_read(const char* key, void *dst, size_t size) {
        esp_err_t err = homekit_storage_open();
        if (err != ESP_OK) {
                return err;
        }

        err = nvs_get_blob(homekit_nvs_handle, key, dst, &size);
        if (err != ESP_OK) {
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        memset(dst, 0, size); // return zeroed buffer if not found
                } else {
                        DEBUG("NVS read failed: %s (0x%x)", esp_err_to_name(err), err);
                        return err;
                }
        }
        return ESP_OK;
}

static esp_err_t homekit_storage_write(const char* key, const void *src, size_t size) {
        esp_err_t err = homekit_storage_open();
        if (err != ESP_OK) {
                return err;
        }

        err = nvs_set_blob(homekit_nvs_handle, key, src, size);
        if (err != ESP_OK) {
                DEBUG("NVS write failed: %s (0x%x)", esp_err_to_name(err), err);
                return err;
        }
        err = nvs_commit(homekit_nvs_handle);
        if (err != ESP_OK) {
                DEBUG("NVS commit failed: %s (0x%x)", esp_err_to_name(err), err);
                return err;
        }
        return ESP_OK;
}

static esp_err_t homekit_storage_commit() {
        esp_err_t err = homekit_storage_open();
        if (err != ESP_OK) {
                return err;
        }

        err = nvs_commit(homekit_nvs_handle);
        if (err != ESP_OK) {
                DEBUG("NVS commit failed: %s (0x%x)", esp_err_to_name(err), err);
                return err;
        }
        return ESP_OK;
}

int homekit_storage_init() {
        esp_err_t err = homekit_storage_open();
        if (err != ESP_OK) {
                return -1;
        }

        char magic[sizeof("HAP")];
        if (homekit_storage_read("magic", (byte *)magic, sizeof(magic)) != ESP_OK) {
                ERROR("Failed to read HomeKit storage magic");
        }

        if (strncmp(magic, "HAP", sizeof(magic)) != 0) {
                INFO("Formatting HomeKit storage");
                strncpy(magic, "HAP", sizeof(magic));
                if (homekit_storage_write("magic", (byte *)magic, sizeof(magic)) != ESP_OK) {
                        ERROR("Failed to write HomeKit storage magic");
                        return -1;
                }
                return 1;
        }

        return 0;
}

int homekit_storage_reset() {
        esp_err_t err = nvs_erase_all(homekit_nvs_handle);
        if (err != ESP_OK) {
                ERROR("Failed to reset HomeKit storage");
                return -1;
        }

        return homekit_storage_init();
}

int homekit_storage_save_config_state(uint32_t config_hash, uint32_t config_number) {
        config_state_data_t data = {
                .config_hash = config_hash,
                .config_number = config_number,
        };

        return (homekit_storage_write(CONFIG_STATE_KEY, &data, sizeof(data)) == ESP_OK) ? 0 : -1;
}

int homekit_storage_load_config_state(uint32_t *config_hash, uint32_t *config_number) {
        config_state_data_t data = {0};
        esp_err_t err = homekit_storage_read(CONFIG_STATE_KEY, &data, sizeof(data));
        if (err != ESP_OK)
                return -1;

        if (config_hash)
                *config_hash = data.config_hash;
        if (config_number)
                *config_number = data.config_number;

        return 0;
}

int homekit_storage_save_iid_map(const void *data, size_t size) {
        return (homekit_storage_write(IID_MAP_KEY, data, size) == ESP_OK) ? 0 : -1;
}

int homekit_storage_load_iid_map(void *data, size_t *size) {
        esp_err_t err = nvs_get_blob(homekit_nvs_handle, IID_MAP_KEY, data, size);
        if (err == ESP_ERR_NVS_NOT_FOUND)
                return 1;
        if (err != ESP_OK) {
                ERROR("Failed to read IID map from HomeKit storage: %s", esp_err_to_name(err));
                return -1;
        }

        return 0;
}

int homekit_storage_pairing_count() {
        pairing_data_t data;
        int count = 0;

        for (int i = 0; i < MAX_PAIRINGS; i++) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(i, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, &data, &size);
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        continue;
                }
                if (err != ESP_OK) {
                        ERROR("Failed to read pairing %d from HomeKit storage: %s", i, esp_err_to_name(err));
                        return -1;
                }

                if (strncmp(data.magic, "HAP", sizeof(data.magic)) == 0)
                        count++;
        }

        return count;
}

int homekit_storage_find_pairing(const char *device_id, pairing_t *pairing) {
        pairing_data_t data;
        for (int i = 0; i < MAX_PAIRINGS; i++) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(i, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, &data, &size);
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        continue; // No pairing in this slot
                } else if (err != ESP_OK) {
                        ERROR("Failed to read pairing %d from HomeKit storage: %s", i, esp_err_to_name(err));
                        return -1;
                }

                if (strncmp(data.magic, "HAP", sizeof(data.magic)) == 0 && strncmp(data.device_id, device_id, DEVICE_ID_SIZE) == 0) {
                        crypto_ed25519_init(&pairing->device_key);
                        if (crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key))) {
                                ERROR("Failed to import device public key");
                                return -1;
                        }
                        pairing->id = i;
                        strncpy(pairing->device_id, data.device_id, DEVICE_ID_SIZE);
                        pairing->device_id[DEVICE_ID_SIZE]=0; //terminate the string since strncpy won't do it for us
                        pairing->permissions = data.permissions;
                        return 0;
                }
        }
        return -1;
}

int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, uint8_t permissions) {
        pairing_data_t data;
        memset(&data, 0, sizeof(data));
        strncpy(data.magic, "HAP", sizeof(data.magic));
        data.permissions = permissions;
        strncpy(data.device_id, device_id, DEVICE_ID_SIZE);

        size_t key_size = sizeof(data.device_public_key);
        if (crypto_ed25519_export_public_key(device_key, data.device_public_key, &key_size)) {
                ERROR("Failed to export device public key");
                return -1;
        }

        pairing_t existing_pairing;
        if (homekit_storage_find_pairing(device_id, &existing_pairing) == 0) {
                crypto_ed25519_done(&existing_pairing.device_key);
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(existing_pairing.id, key, sizeof(key));

                esp_err_t err = nvs_set_blob(homekit_nvs_handle, key, &data, sizeof(data));
                if (err != ESP_OK) {
                        ERROR("Failed to update existing pairing in HomeKit storage: %s", esp_err_to_name(err));
                        return -1;
                }

                return (homekit_storage_commit() == ESP_OK) ? 0 : -1;
        }

        for (int i = 0; i < MAX_PAIRINGS; i++) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(i, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, NULL, &size); //don't read data, only detect empty slot
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        // Write the pairing data to this empty slot
                        err = nvs_set_blob(homekit_nvs_handle, key, &data, sizeof(data));
                        if (err != ESP_OK) {
                                ERROR("Failed to write pairing data to HomeKit storage: %s", esp_err_to_name(err));
                                return -1;
                        }
                        return (homekit_storage_commit() == ESP_OK) ? 0 : -1;
                }
        }
        return -1;
}

int homekit_storage_update_pairing(const char *device_id, uint8_t permissions) {
        pairing_data_t data;
        for (int i = 0; i < MAX_PAIRINGS; i++) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(i, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, &data, &size);
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        continue; // No pairing in this slot
                } else if (err != ESP_OK) {
                        ERROR("Failed to read pairing %d from HomeKit storage: %s", i, esp_err_to_name(err));
                        return -1;
                }

                if (strncmp(data.magic, "HAP", sizeof(data.magic)) == 0 && strncmp(data.device_id, device_id, DEVICE_ID_SIZE) == 0) {
                        data.permissions = permissions;
                        err = nvs_set_blob(homekit_nvs_handle, key, &data, sizeof(data));
                        if (err != ESP_OK) {
                                ERROR("Failed to update pairing data in HomeKit storage: %s", esp_err_to_name(err));
                                return -1;
                        }
                        return (homekit_storage_commit() == ESP_OK) ? 0 : -1;
                }
        }
        return -1;
}

int homekit_storage_can_add_pairing() {
        pairing_data_t data;
        for (int i = 0; i < MAX_PAIRINGS; i++) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(i, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, &data, &size);
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        return 1; // There is an empty slot
                }
        }
        return 0;
}

int homekit_storage_remove_pairing(const char *device_id) {
        pairing_data_t data;
        bool found=false;
        for (int i = 0; i < MAX_PAIRINGS; i++) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(i, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, &data, &size);
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        continue; // No pairing in this slot
                } else if (err != ESP_OK) {
                        ERROR("Failed to read pairing %d from HomeKit storage: %s", i, esp_err_to_name(err));
                        return -1;
                }

                if (strncmp(data.magic, "HAP", sizeof(data.magic)) == 0 && strncmp(data.device_id, device_id, DEVICE_ID_SIZE) == 0) {
                        pairing_data_t erased = {0};
                        err = nvs_set_blob(homekit_nvs_handle, key, &erased, sizeof(erased));
                        if (err != ESP_OK) {
                                ERROR("Failed to overwrite pairing in HomeKit storage: %s", esp_err_to_name(err));
                                return -1;
                        }
                        err = nvs_erase_key(homekit_nvs_handle, key);
                        if (err != ESP_OK) {
                                ERROR("Failed to remove pairing from HomeKit storage: %s", esp_err_to_name(err));
                                return -1;
                        }
                        found=true;
                }
        }
        if (found) {
                return homekit_storage_commit();
        } else {
                return -1;
        }
}

void homekit_storage_pairing_iterator_init(pairing_iterator_t *it) {
        it->idx = 0;
}

void homekit_storage_pairing_iterator_done(pairing_iterator_t *it) {
        // Nothing to do here for NVS
}

int homekit_storage_next_pairing(pairing_iterator_t *it, pairing_t *pairing) {
        pairing_data_t data;
        while (it->idx < MAX_PAIRINGS) {
                char key[PAIRING_KEY_MAX_LEN];
                pairing_key_from_index(it->idx++, key, sizeof(key));

                size_t size = sizeof(data);
                esp_err_t err = nvs_get_blob(homekit_nvs_handle, key, &data, &size);
                if (err == ESP_ERR_NVS_NOT_FOUND) {
                        continue; // No pairing in this slot
                } else if (err != ESP_OK) {
                        ERROR("Failed to read pairing %d from HomeKit storage: %s", it->idx - 1, esp_err_to_name(err));
                        continue;
                }

                if (strncmp(data.magic, "HAP", sizeof(data.magic)) == 0) {
                        crypto_ed25519_init(&pairing->device_key);
                        if (crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key))) {
                                ERROR("Failed to import device public key");
                                continue;
                        }
                        pairing->id = it->idx - 1;
                        strncpy(pairing->device_id, data.device_id, DEVICE_ID_SIZE);
                        pairing->device_id[DEVICE_ID_SIZE]=0; //terminate the string since strncpy won't do it for us
                        pairing->permissions = data.permissions;
                        return 0;
                }
        }
        return -1;
}

// Missing functions: Save and Load Accessory ID and Key

int homekit_storage_save_accessory_id(const char *accessory_id) {
        esp_err_t err = homekit_storage_write("accessory_id", accessory_id, ACCESSORY_ID_SIZE);
        if (err != ESP_OK) {
                ERROR("Failed to save accessory ID: %s", esp_err_to_name(err));
                return -1;
        }
        return 0;
}

int homekit_storage_load_accessory_id(char *accessory_id) {
        esp_err_t err = homekit_storage_read("accessory_id", accessory_id, ACCESSORY_ID_SIZE);
        accessory_id[ACCESSORY_ID_SIZE]=0;
        if (err != ESP_OK) {
                ERROR("Failed to load accessory ID: %s", esp_err_to_name(err));
                return -1;
        }
        return 0;
}

int homekit_storage_save_accessory_key(const ed25519_key *key) {
        uint8_t key_data[ACCESSORY_KEY_SIZE];
        size_t key_data_size = sizeof(key_data);

        int r = crypto_ed25519_export_key(key, key_data, &key_data_size);
        if (r) {
                ERROR("Failed to export accessory key (code %d)", r);
                return -1;
        }

        esp_err_t err = homekit_storage_write("accessory_key", key_data, key_data_size);
        if (err != ESP_OK) {
                ERROR("Failed to save accessory key: %s", esp_err_to_name(err));
                return -1;
        }
        return 0;
}

int homekit_storage_load_accessory_key(ed25519_key *key) {
        uint8_t key_data[ACCESSORY_KEY_SIZE];
        esp_err_t err = homekit_storage_read("accessory_key", key_data, sizeof(key_data));
        if (err != ESP_OK) {
                ERROR("Failed to load accessory key: %s", esp_err_to_name(err));
                return -1;
        }

        crypto_ed25519_init(key);
        int r = crypto_ed25519_import_key(key, key_data, sizeof(key_data));
        if (r) {
                ERROR("Failed to import accessory key (code %d)", r);
                return -2;
        }
        return 0;
}
