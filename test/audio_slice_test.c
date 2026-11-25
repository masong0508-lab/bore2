#include <assert.h>
#include <stdio.h>
#include "../src/audio_slice.h"
int main(void) {
    AudioSlice s=audio_slice_begin(118,17);
    unsigned off,n,ns=0,nq=0,calls=0;int phase;
    while((phase=audio_slice_next(&s,&off,&n))!=AU_SLICE_DONE) {
        assert(n>0&&n<=4);
        if(phase==AU_SLICE_SAMPLES){assert(off==ns&&nq==0);ns+=n;}
        else {assert(ns==118&&off==nq);nq+=n;}
        audio_slice_advance(&s,phase,n);calls++;
    }
    assert(ns==118&&nq==17&&calls==35);
    s=audio_slice_begin(9,3);assert(audio_slice_next(&s,&off,&n)==AU_SLICE_SAMPLES&&off==0&&n==4);
    audio_slice_advance(&s,AU_SLICE_SAMPLES,n);
    s=audio_slice_begin(1,1);assert(audio_slice_next(&s,&off,&n)==AU_SLICE_SAMPLES&&off==0&&n==1);
    unsigned polls=0;assert(audio_wait_ack(7,6,&polls)==AU_WAIT_PENDING);
    assert(audio_wait_ack(7,7,&polls)==AU_WAIT_READY);
    polls=0;for(unsigned i=1;i<AU_WAIT_LIMIT;i++)assert(audio_wait_ack(8,7,&polls)==AU_WAIT_PENDING);
    assert(audio_wait_ack(8,7,&polls)==AU_WAIT_TIMEOUT);
    assert(audio_wait_ack(8,8,&polls)==AU_WAIT_READY);
    puts("Bounded music reads, cancellation, and stop acknowledgement passed");return 0;
}
