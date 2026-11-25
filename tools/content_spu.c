/* Build-time encoder: predictive PS1 ADPCM, one-shot samples, no platform SDK.
 * Select the lowest-error filter and shift, updating history from decoded PCM. */
#include <stdint.h>
#include <limits.h>
static const int co[5][2]={{0,0},{60,0},{115,-52},{98,-55},{122,-60}};
static int sat(int v) { return v>32767?32767:v<-32768?-32768:v; }
int content_spu_encode(const int16_t *in,int n,uint8_t *out) {
    int a=0,b=0,pos=0;
    for(int off=0;off<n;off+=28) {
        long long best=LLONG_MAX; int bf=0,bs=0,ba=0,bb=0; uint8_t chosen[28]={0};
        for(int f=0;f<5;f++) for(int sh=0;sh<=12;sh++) {
            int x=a,y=b,step=1<<(12-sh); long long err=0; uint8_t code[28];
            for(int k=0;k<28;k++) {
                int sample=off+k<n?in[off+k]:0,pred=(x*co[f][0]+y*co[f][1]+32)>>6;
                int d=sample-pred, q=d>=0?(d+step/2)/step:-((-d+step/2)/step);
                if(q>7)q=7;if(q<-8)q=-8;
                int v=sat(pred+q*step),e=sample-v;
                err+=(long long)e*e;code[k]=(uint8_t)(q&15);y=x;x=v;
                if(err>=best)break;
            }
            if(err<best) {
                best=err;bf=f;bs=sh;ba=x;bb=y;
                for(int k=0;k<28;k++)chosen[k]=code[k];
            }
        }
        out[pos]=(uint8_t)(bf*16+bs);out[pos+1]=(off+28>=n)?1:0;
        for(int k=0;k<14;k++)out[pos+2+k]=(uint8_t)(chosen[2*k]|chosen[2*k+1]<<4);
        pos+=16;a=ba;b=bb;
    }
    return pos;
}
