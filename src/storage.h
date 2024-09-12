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

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "pairing.h"

typedef struct {
        int idx;
} pairing_iterator_t;

int homekit_storage_reset();

int homekit_storage_init();

int homekit_storage_save_accessory_id(const char *accessory_id);
int homekit_storage_load_accessory_id(char *accessory_id);

int homekit_storage_save_accessory_key(const ed25519_key *key);
int homekit_storage_load_accessory_key(ed25519_key *key);

int homekit_storage_can_add_pairing();
int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, uint8_t permissions);
int homekit_storage_update_pairing(const char *device_id, uint8_t permissions);
int homekit_storage_remove_pairing(const char *device_id);
int homekit_storage_find_pairing(const char *device_id, pairing_t *pairing);

void homekit_storage_pairing_iterator_init(pairing_iterator_t *it);
int homekit_storage_next_pairing(pairing_iterator_t *it, pairing_t *pairing);
void homekit_storage_pairing_iterator_done(pairing_iterator_t *it);

#endif // __STORAGE_H__
