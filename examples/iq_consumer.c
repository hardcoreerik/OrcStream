/* Independent offline byte-exact OrcStream IQ consumer.
 * Requires HELLO, one stream, strict continuous sequence and explicit EOS.
 * SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#define READ_CHUNK 113u
static int parse_uint(const char *s,unsigned long *v) {
 char *end;errno=0;*v=strtoul(s,&end,10);
 return *s!='\0'&&*end=='\0'&&errno==0;
}
static int receive_packet(const orcstream_packet *p,FILE *out,orcstream_tracker *tracker,
                          unsigned expected_format,int *hello,int *ended,uint64_t *sample_count) {
 orcstream_continuity c;orcstream_hello ann;size_t bps;
 if(*ended) return 0;
 if(p->type==ORCSTREAM_PACKET_HELLO) {
  if(*hello||orcstream_hello_decode(p->payload,p->payload_len,&ann)!=ORCSTREAM_OK||
     !(ann.format_mask&(1u<<expected_format))) return 0;
  *hello=1;return 1;
 }
 if(p->type!=ORCSTREAM_PACKET_IQ||!(*hello)||p->format!=expected_format) return 0;
 if(!tracker->initialized&&(p->sequence!=0||p->sample_index!=0||
      (p->flags&ORCSTREAM_FLAG_DISCONTINUITY))) return 0;
 if(orcstream_tracker_observe(tracker,p,&c)!=ORCSTREAM_OK||
    (c!=ORCSTREAM_CONTINUITY_FIRST&&c!=ORCSTREAM_CONTINUITY_OK)) return 0;
 bps=orcstream_complex_sample_bytes(p->format);
 if(p->payload_len&&fwrite(p->payload,1,p->payload_len,out)!=p->payload_len) return 0;
 *sample_count+=(uint64_t)(p->payload_len/bps);
 if(p->flags&ORCSTREAM_FLAG_END_OF_STREAM) *ended=1;
 return 1;
}
int main(int argc,char **argv) {
 unsigned long format_arg;FILE *in,*out;uint8_t input[READ_CHUNK],storage[ORCSTREAM_MAX_FRAME];
 orcstream_reader reader;orcstream_packet packet;orcstream_tracker tracker;
 size_t n,off,used;uint64_t samples=0;int hello=0,ended=0,fail=0;
 if(argc!=4||!parse_uint(argv[3],&format_arg)||format_arg==0||format_arg>6) {
  fprintf(stderr,"usage: %s input.orcs output.raw format_id\n",argv[0]);return 2;
 }
 in=fopen(argv[1],"rb");if(!in) {perror("input");return 1;}
 out=fopen(argv[2],"wb");if(!out) {perror("output");fclose(in);return 1;}
 orcstream_tracker_reset(&tracker);
 if(orcstream_reader_init(&reader,storage,sizeof(storage))!=ORCSTREAM_OK) fail=1;
 while(!fail&&(n=fread(input,1,sizeof(input),in))!=0) {
  off=0;
  while(off<n) {
   orcstream_status rc=orcstream_reader_feed(&reader,input+off,n-off,&used,&packet);
   if(rc!=ORCSTREAM_OK&&rc!=ORCSTREAM_NEED_MORE) {fprintf(stderr,"decode error: %s\n",orcstream_status_string(rc));fail=1;break;}
   if(!used) {fail=1;break;}
   off+=used;
   if(rc==ORCSTREAM_OK&&!receive_packet(&packet,out,&tracker,(unsigned)format_arg,&hello,&ended,&samples)) {
    fprintf(stderr,"continuity, metadata, packet order or EOS error\n");fail=1;break;
   }
  }
 }
 if(ferror(in)||reader.used||!hello||!ended||!tracker.initialized||
    tracker.missing_samples||tracker.missing_packets||tracker.late_or_duplicate) fail=1;
 if(fclose(in)) fail=1;
 if(fclose(out)) fail=1;
 if(fail) {fprintf(stderr,"consumer: incomplete, invalid or missing IQ stream\n");return 1;}
 printf("consumer: format=%lu samples=%llu missing_packets=0 missing_samples=0 EOS=1\n",
     format_arg,(unsigned long long)samples);
 return 0;
}
