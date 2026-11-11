/* libFuzzer entrypoint (Clang): parser and incremental reader adversarial data.
 * SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size) {
 orcstream_packet pkt;size_t consumed,off=0;
 uint8_t buffer[ORCSTREAM_MAX_FRAME];orcstream_reader r;
 (void)orcstream_packet_decode(data,size,&pkt,&consumed);
 (void)orcstream_datagram_decode(data,size,&pkt);
 if(orcstream_reader_init(&r,buffer,sizeof(buffer))!=ORCSTREAM_OK) return 0;
 while(off<size) {
  orcstream_status status=orcstream_reader_feed(&r,data+off,size-off,&consumed,&pkt);
  off+=consumed;
  if(status!=ORCSTREAM_OK&&status!=ORCSTREAM_NEED_MORE) break;
  if(!consumed) break;
 }
 return 0;
}
