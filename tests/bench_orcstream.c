/* Portable host CPU-time microbench. Not end-to-end transport throughput.
 * SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#define MAX_LOOPS 600u
static volatile uint32_t sink=0;
static uint32_t crc_only(const uint8_t *p,size_t n) {
 uint32_t crc=0xffffffffu;size_t i;
 for(i=0;i<n;++i) {unsigned j,loops;crc^=p[i];for(j=0;j<8;++j)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
 return ~crc;
}
static double elapsed(clock_t a,clock_t b) {
 double delta=(double)(b-a)/(double)CLOCKS_PER_SEC;
 return delta>0.0?delta:0.000001;
}
static int bench(unsigned format,unsigned payload,unsigned rate) {
 uint8_t *raw,*frame;orcstream_packet pkt,decoded;size_t written,used;unsigned j,loops;
 clock_t a,b,c,d;double enc,dec,crc,mb,overhead,required;
 size_t bps=orcstream_complex_sample_bytes((uint8_t)format);
 if(!bps||payload>ORCSTREAM_MAX_PAYLOAD||payload%bps) return 1;
 loops=payload>=8192u ? 64u : MAX_LOOPS;
 raw=(uint8_t *)malloc(payload);frame=(uint8_t *)malloc(payload+ORCSTREAM_HEADER_SIZE);
 if(!raw||!frame) {free(raw);free(frame);return 1;}
 for(j=0;j<payload;++j) raw[j]=(uint8_t)((j*37u+13u)&255u);
 memset(&pkt,0,sizeof(pkt));pkt.type=ORCSTREAM_PACKET_IQ;pkt.format=(uint8_t)format;
 pkt.stream_id=1;pkt.sample_rate_num=rate;pkt.sample_rate_den=1;
 pkt.payload_len=payload;pkt.payload=raw;
 a=clock();
 for(j=0;j<loops;++j) {
  if(orcstream_packet_encode(frame,payload+ORCSTREAM_HEADER_SIZE,&pkt,&written)!=ORCSTREAM_OK) return 1;
  sink^=frame[60];
 }
 b=clock();
 for(j=0;j<loops;++j) {
  if(orcstream_packet_decode(frame,written,&decoded,&used)!=ORCSTREAM_OK) return 1;
  sink^=decoded.payload[0];
 }
 c=clock();
 for(j=0;j<loops;++j) sink^=crc_only(raw,payload);
 d=clock();
 enc=elapsed(a,b);dec=elapsed(b,c);crc=elapsed(c,d);
 mb=(double)payload*(double)loops/1000000.0;
 overhead=100.0*ORCSTREAM_HEADER_SIZE/(payload+ORCSTREAM_HEADER_SIZE);
 required=((double)rate*(double)bps/1e6)*(1.0+(double)ORCSTREAM_HEADER_SIZE/payload);
 printf("format=%u rate=%u payload=%u frames=%u encode_MBps=%.2f decode_MBps=%.2f crc_only_MBps=%.2f encode_CPU_ms=%.1f decode_CPU_ms=%.1f crc_CPU_ms=%.1f overhead_pct=%.3f ideal_wire_MBps=%.3f decoder_buffer=%u\n",
  format,rate,payload,loops,mb/enc,mb/dec,mb/crc,enc*1000,dec*1000,crc*1000,
  overhead,required,ORCSTREAM_HEADER_SIZE+payload);
 free(frame);free(raw);return 0;
}
int main(void) {
 const unsigned sizes[]={64,512,1152,8192,32768,65536};size_t i;
 for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) {
  if(bench(1,sizes[i],960000)||bench(1,sizes[i],2400000)||
     bench(3,sizes[i],960000)||bench(3,sizes[i],2400000)||
     bench(5,sizes[i],2400000)) return 1;
 }
 if(sink==0xFFFFFFFFu) puts("ignore");
 return 0;
}
