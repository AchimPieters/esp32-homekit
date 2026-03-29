/**

   Copyright 2026 Achim Pieters | StudioPieters®

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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


typedef struct _bitset {
        uint16_t size;
        uint8_t *data;
} bitset_t;


bitset_t *bitset_new(uint16_t size) {
        bitset_t *bs = malloc(sizeof(bitset_t) + (size + 7 / 8));
        bs->data = ((uint8_t*)bs) + sizeof(bitset_t);
        bs->size = size;
        memset(bs->data, 0, (size + 7) / 8);
        return bs;
}


void bitset_free(bitset_t *bs) {
        free(bs);
}


void bitset_clear_all(bitset_t *bs) {
        memset(bs->data, 0, (bs->size + 7) / 8);
}


bool bitset_isset(bitset_t *bs, uint16_t index) {
        return (bs->data[index / 8] & (1 << (index % 8))) != 0;
}


void bitset_set(bitset_t *bs, uint16_t index) {
        if (index >= bs->size)
                return;

        bs->data[index / 8] |= (1 << (index % 8));
}


void bitset_clear(bitset_t *bs, uint16_t index) {
        if (index >= bs->size)
                return;

        bs->data[index / 8] &= ~(1 << (index % 8));
}
