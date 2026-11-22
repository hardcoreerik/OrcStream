#!/usr/bin/env python3
"""Build proposed (NOT adopted!) v1.0 OrcStream golden frames, no third-party deps.
Writes proposed_v1_wire_vectors.json and v1_wire_run.log beside script.
"""
import hashlib
import json
import struct
import zlib
from pathlib import Path
P=Path(__file__).resolve().parent
SAMPLES=[(-2048,2047),(-1,0),(1024,-1024)]

def pack12(samps):
    b=bytearray()
    for i,q in samps:
        assert -2048<=i<=2047 and -2048<=q<=2047
        i &=0xfff;q &=0xfff
        b += bytes([i&255,((i>>8)&15)|((q&15)<<4),(q>>4)&255])
    return bytes(b)

def wire(codec_id,payload):
    h=bytearray(80)
    h[0:4]=b'ORCS'
    struct.pack_into('>BBHBBHIIQQIHHIIQIIIII',h,4,
       1,0,80, # major,minor,header_bytes
       5,0,codec_id, # type,flags,codec_id
       1,1, # stream id, epoch
       0,0, # sequence, sample index
       3,3,0, # sample_count, source_format CS16_LE, codec_param
       2400000,1, # rate
       0,0, # timestamp_ns,clock_id
       len(payload),12, # encoded_bytes, decoded_bytes
       0,1) # crc32, context_revision
    assert len(h)==80
    crc=zlib.crc32(h[:72]+h[76:80]+payload) &0xffffffff
    struct.pack_into('>I',h,72,crc)
    assert zlib.crc32(h[:72]+h[76:80]+payload)&0xffffffff==crc
    return bytes(h)+payload

raw=b''.join(struct.pack('<hh',i,q) for i,q in SAMPLES)
packed=pack12(SAMPLES)
assert len(raw)==12 and len(packed)==9
fixtures=[]
for cid,payload in [(0,raw),(1,packed)]:
    frame=wire(cid,payload)
    fixtures.append({'name':'raw_cs16le_3_iq' if cid==0 else 'packed_cs12_3_iq',
       'status':'DISCUSSION DRAFT - not adopted protocol',
       'source_samples_iq':SAMPLES,'codec_id':cid,'source_format_id':3,
       'decoded_bytes':12,'encoded_bytes':len(payload),'frame_bytes':len(frame),
       'payload_hex':payload.hex(' '),'crc32_hex':frame[72:76].hex(),
       'frame_hex':frame.hex(' '),'sha256_frame':hashlib.sha256(frame).hexdigest()})
manifest={'protocol':'OrcStream','version':'1.0-draft-agent3',
 'warning':'not a normative or shipped version; context_revision=1 assumed present in session',
 'source_context':{'stream_id':1,'epoch':1,'context_revision':1,'native_valid_bits':12,
        'source_format':'CS16_LE','sample_rate_num':2400000,'sample_rate_den':1},
 'fixtures':fixtures}
(P/'proposed_v1_wire_vectors.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(P/'v1_wire_run.log').write_text('PASS 2 proposed v1 wire vectors, CRC and 12-bit samples\n',encoding='utf-8')
print('PASS 2 proposed v1 wire vectors; raw bytes:',len(fixtures[0]['payload_hex'].split()),'packed bytes:',len(packed))
print('CRC:',fixtures[0]['crc32_hex'],fixtures[1]['crc32_hex'])
