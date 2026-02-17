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

#include <string.h>
#include "mbedtls/aes.h"

static const unsigned char key[16]={1,9,8,4,2,7,6,3,5,0,1,2,3,4,5,6};

void hap_encrypt(void *buf,size_t len){
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx,key,128);
    for(size_t i=0;i<len;i+=16)
        mbedtls_aes_crypt_ecb(&ctx,MBEDTLS_AES_ENCRYPT,(unsigned char*)buf+i,(unsigned char*)buf+i);
    mbedtls_aes_free(&ctx);
}

void hap_decrypt(void *buf,size_t len){
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_dec(&ctx,key,128);
    for(size_t i=0;i<len;i+=16)
        mbedtls_aes_crypt_ecb(&ctx,MBEDTLS_AES_DECRYPT,(unsigned char*)buf+i,(unsigned char*)buf+i);
    mbedtls_aes_free(&ctx);
}
