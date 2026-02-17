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

#include "hap_stream.h"
#include <string.h>

void hap_stream_init(hap_stream_t*s){
    s->parsed=0;
    s->state=0;
}

int hap_stream_feed(hap_stream_t*s,const void*buf,size_t len){
    const char*c=buf;
    for(size_t i=0;i<len;i++){
        if(s->state==0 && c[i]=='\r') s->state=1;
        else if(s->state==1 && c[i]=='\n') s->state=2;
        else if(s->state==2 && c[i]=='\r') s->state=3;
        else if(s->state==3 && c[i]=='\n') return 1;
        else s->state=0;
    }
    s->parsed+=len;
    return 0;
}
