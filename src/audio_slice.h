// Bounded, cancellable transfers for area music. No new audio buffers.
// Keep the samples ahead of the sequence: the sample transfer reuses the
// sequence buffer as scratch, and playback starts only after both finish.
#pragma once
enum { AU_SLICE_DONE, AU_SLICE_SAMPLES, AU_SLICE_SEQUENCE };
#define AU_SLICE_SECTORS 4
typedef struct { unsigned samples, sequence, sample_pos, seq_pos; } AudioSlice;
static inline AudioSlice audio_slice_begin(unsigned samples,unsigned sequence) {
    AudioSlice s={samples,sequence,0,0};return s;
}
static inline int audio_slice_next(const AudioSlice *s,unsigned *offset,unsigned *count) {
    unsigned left;
    if(s->sample_pos<s->samples) {
        *offset=s->sample_pos;left=s->samples-s->sample_pos;
        *count=left>AU_SLICE_SECTORS?AU_SLICE_SECTORS:left;return AU_SLICE_SAMPLES;
    }
    if(s->seq_pos<s->sequence) {
        *offset=s->seq_pos;left=s->sequence-s->seq_pos;
        *count=left>AU_SLICE_SECTORS?AU_SLICE_SECTORS:left;return AU_SLICE_SEQUENCE;
    }
    *offset=*count=0;return AU_SLICE_DONE;
}
static inline void audio_slice_advance(AudioSlice *s,int phase,unsigned count) {
    if(phase==AU_SLICE_SAMPLES && count<=s->samples-s->sample_pos)s->sample_pos+=count;
    else if(phase==AU_SLICE_SEQUENCE && count<=s->sequence-s->seq_pos)s->seq_pos+=count;
}
// Handheld stop acknowledgement: do not touch a shared sample buffer until
// the audio CPU confirms the stop, even after a timeout.
enum { AU_WAIT_PENDING, AU_WAIT_READY, AU_WAIT_TIMEOUT };
#define AU_WAIT_LIMIT 120
static inline int audio_wait_ack(unsigned expected,unsigned ack,unsigned *polls) {
    if(expected==ack)return AU_WAIT_READY;
    if(*polls<AU_WAIT_LIMIT)++*polls;
    return *polls>=AU_WAIT_LIMIT?AU_WAIT_TIMEOUT:AU_WAIT_PENDING;
}
