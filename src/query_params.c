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

#include <stdlib.h>
#include <string.h>
#include "query_params.h"
#include "debug.h"


int query_param_iterator_init(query_param_iterator_t *it, const char *s, size_t len) {
        it->data = (char *)s;
        it->len = len;
        it->pos = 0;

        return 0;
}

void query_param_iterator_done(query_param_iterator_t *it) {
}

bool query_param_iterator_next(query_param_iterator_t *it, query_param_t *param) {
        if (it->pos >= it->len || it->data[it->pos] == '#')
                return false;

        int pos = it->pos;
        while (it->pos < it->len &&
               it->data[it->pos] != '=' &&
               it->data[it->pos] != '&' &&
               it->data[it->pos] != '#')
                it->pos++;

        if (it->pos == pos) {
                return false;
        }

        param->name = &it->data[pos];
        param->name_len = it->pos - pos;

        param->value = NULL;
        param->value_len = 0;

        if (it->pos < it->len && it->data[it->pos] == '=') {
                it->pos++;
                pos = it->pos;
                while (it->pos < it->len &&
                       it->data[it->pos] != '&' &&
                       it->data[it->pos] != '#')
                        it->pos++;

                if (it->pos != pos) {
                        param->value = &it->data[pos];
                        param->value_len = it->pos - pos;
                }
        }

        return true;
}
