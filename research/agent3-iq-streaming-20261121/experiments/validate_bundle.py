#!/usr/bin/env python3
"""Portable standard-library-only validation for the Agent 3 sandbox bundle.
Run: python experiments/validate_bundle.py
Writes bundle_validation.log beside this script; does not contact GitHub.
"""
from pathlib import Path
import csv
import json
import struct
import zlib

ROOT=Path(__file__).resolve().parent.parent
EXP=ROOT/'experiments'
NAMES=[
 'IQ_COMPRESSION_COMPETITIVE_LANDSCAPE.md',
 'ADVANCED_IQ_CODEC_RESEARCH.md',
 'ORCSTREAM_PROTOCOL_ARCHITECTURE.md',
 'ORCSTREAM_CODEC_PORTABILITY_MATRIX.md',
 'ORCSTREAM_FIDELITY_AND_BENCHMARK_SPEC.md',
 'ORCSTREAM_COMPETITIVE_DIFFERENTIATION.md',
 'ORCSTREAM_ENGINEERING_ROADMAP.md',
 'ORCSTREAM_V1_PROTOCOL_PROPOSAL.md',
]
lines=[]
for n in NAMES:
 p=ROOT/'docs'/n
 assert p.is_file() and p.stat().st_size>2000,n
 lines.append('PASS doc '+n+' '+str(p.stat().st_size)+' bytes')
with (EXP/'synthetic_results.csv').open(newline='') as fp:
 rows=list(csv.DictReader(fp))
assert len({r['source'] for r in rows})==9
for src in {r['source'] for r in rows}:
 r={x['codec']:x for x in rows if x['source']==src}
 assert int(r['raw_cs16']['encoded_bytes'])==524288
 assert int(r['packed_cs12']['encoded_bytes'])==393216
 assert abs(float(r['bfp8_256']['encoded_bits_per_complex'])-16.03125)<1e-7
 assert float(r['zstd1_packed12']['encoded_bits_per_complex'])>=0
lines.append('PASS synthetic CSV 9 sources and native12 pack+fixed BFP budget')
j=json.loads((EXP/'proposed_v1_wire_vectors.json').read_text())
assert len(j['fixtures'])==2
raw=packed=None
for f in j['fixtures']:
 b=bytes.fromhex(f['frame_hex']);p=b[80:]
 assert len(b)==f['frame_bytes'] and b[0:4]==b'ORCS'
 assert b[4:6]==bytes([1,0]) and struct.unpack_from('>H',b,6)[0]==80
 assert b[8]==5 and struct.unpack_from('>I',b,36)[0]==3
 assert struct.unpack_from('>I',b,72)[0]==zlib.crc32(b[:72]+b[76:80]+p)&0xffffffff
 assert struct.unpack_from('>I',b,64)[0]==len(p)
 assert struct.unpack_from('>I',b,68)[0]==12
 if f['codec_id']==0:raw=p
 elif f['codec_id']==1:packed=p
assert len(raw)==12 and len(packed)==9
vals=[]
for st in range(0,len(packed),3):
 a,b,c=packed[st:st+3]
 x=a|((b&15)<<8);y=(b>>4)|(c<<4)
 vals.extend([x-4096 if x>=2048 else x,y-4096 if y>=2048 else y])
assert struct.pack('<'+'h'*len(vals),*vals)==raw
lines.append('PASS v1 proposal fixed-offset, encoded/decoded sizes, CRC, exact CS12 reconstruction')
(EXP/'bundle_validation.log').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('\n'.join(lines))
