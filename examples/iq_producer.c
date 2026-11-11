/* Offline OrcStream producer; reads native interleaved IQ bytes verbatim.
 * SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#define MAX_CHUNK_BYTES 32768u
static int parse_uint(const char *s,unsigned long *v) {
 char *end;errno=0;*v=strtoul(s,&end,10);
 return *s!='\0'&&*end=='\0'&&errno==0;
}
static int output_frame(FILE *out,orcstream_packet *p,uint8_t *frame) {
 size_t written;
 if(orcstream_packet_encode(frame,ORCSTREAM_HEADER_SIZE+MAX_CHUNK_BYTES,p,&written)!=ORCSTREAM_OK) return 0;
 return fwrite(frame,1,written,out)==written;
}
int main(int argc,char **argv) {
 unsigned long format_arg,chunk_arg,rate_arg;size_t bps,chunk_bytes,n;
 FILE *in,*out;uint8_t iq[MAX_CHUNK_BYTES],frame[ORCSTREAM_HEADER_SIZE+MAX_CHUNK_BYTES],hello[ORCSTREAM_HELLO_SIZE];
 orcstream_packet p;orcstream_hello announce;uint64_t seq=0,index=0;int fail=0;
 if(argc!=6||!parse_uint(argv[3],&format_arg)||!parse_uint(argv[4],&chunk_arg)||
   !parse_uint(argv[5],&rate_arg)||format_arg>6||format_arg==0||rate_arg==0||
   rate_arg>UINT32_MAX||chunk_arg==0) {
  fprintf(stderr,"usage: %s input.raw output.orcs format_id chunk_samples sample_rate\n",argv[0]);return 2;
 }
 bps=orcstream_complex_sample_bytes((uint8_t)format_arg);
 if(chunk_arg>(unsigned long)(MAX_CHUNK_BYTES/bps)) {fprintf(stderr,"chunk exceeds bounded buffer\n");return 2;}
 chunk_bytes=(size_t)chunk_arg*bps;
 in=fopen(argv[1],"rb");if(!in) {perror("input");return 1;}
 out=fopen(argv[2],"wb");if(!out) {perror("output");fclose(in);return 1;}
 announce.capabilities=ORCSTREAM_CAP_IQ_RX;announce.format_mask=1u<<(unsigned)format_arg;
 announce.max_payload=MAX_CHUNK_BYTES;announce.max_streams=1;
 if(orcstream_hello_encode(hello,&announce)!=ORCSTREAM_OK) fail=1;
 memset(&p,0,sizeof(p));p.type=ORCSTREAM_PACKET_HELLO;p.payload=hello;p.payload_len=ORCSTREAM_HELLO_SIZE;
 if(!fail&&!output_frame(out,&p,frame)) fail=1;
 memset(&p,0,sizeof(p));p.type=ORCSTREAM_PACKET_IQ;p.format=(uint8_t)format_arg;
 p.stream_id=0xabc123u;p.flags=ORCSTREAM_FLAG_FREQUENCY_VALID;
 p.center_frequency_hz=100000000u;p.sample_rate_num=(uint32_t)rate_arg;p.sample_rate_den=1;
 p.payload=iq;
 while(!fail&&(n=fread(iq,1,chunk_bytes,in))>0) {
  if((n%bps)!=0||seq==UINT64_MAX||index>UINT64_MAX-(uint64_t)(n/bps)) {fail=1;break;}
  p.sequence=seq++;p.sample_index=index;p.payload_len=(uint32_t)n;
  if(!output_frame(out,&p,frame)) {fail=1;break;}
  index+=(uint64_t)(n/bps);
 }
 if(ferror(in)) fail=1;
 if(!fail&&seq!=UINT64_MAX) {
  p.sequence=seq;p.sample_index=index;p.payload_len=0;p.flags|=ORCSTREAM_FLAG_END_OF_STREAM;
  if(!output_frame(out,&p,frame)) fail=1;
 } else fail=1;
 if(fclose(in)) fail=1;
 if(fclose(out)) fail=1;
 if(fail) {fprintf(stderr,"producer: I/O, length or sequence error\n");return 1;}
 printf("producer: format=%lu rate=%lu samples=%llu data_packets=%llu EOS=1\n",
   format_arg,rate_arg,(unsigned long long)index,(unsigned long long)seq);
 return 0;
}
