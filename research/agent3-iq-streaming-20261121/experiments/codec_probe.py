#!/usr/bin/env python3
"""Synthetic CS12 IQ codec probe (not a hardware benchmark).

Run from any directory: python experiments/codec_probe.py
Writes CSV + run log beside this script. Requires numpy; lz4.frame, zstd CLI optional.
All samples are signed 12-bit complex, stored I,Q as signed little-endian 16-bit.
"""
from __future__ import annotations
import csv
import hashlib
import math
import platform
import shutil
import subprocess
import sys
import time
import zlib
from pathlib import Path
import numpy as np

OUT = Path(__file__).resolve().parent
SEED = 20261121
N = 131072
RNG = np.random.default_rng(SEED)
try:
    import lz4.frame as lz4frame
except ImportError:
    lz4frame = None


def q12(iq):
    return np.clip(np.rint(iq), -2048, 2047).astype(np.int16)


def sources():
    n = np.arange(N, dtype=np.float64)
    yield 'zeros', np.zeros((N,2), dtype=np.int16)
    yield 'full_uniform_12b', RNG.integers(-2048,2048, size=(N,2),dtype=np.int16)
    yield 'gaussian_noise', q12(RNG.normal(0,360,size=(N,2)))
    z = 1800*np.exp(2j*np.pi*(0.0511*n + 0.000000001*n*n))
    yield 'cw_chirp', q12(np.column_stack([z.real,z.imag]))
    phase = 2*np.pi*(0.031*n + 0.009* np.sin(2*np.pi*n/4096))
    z = 1400*np.exp(1j*phase)
    yield 'fm', q12(np.column_stack([z.real,z.imag]))
    # Random OFDM time-domain blocks, independent frequency bins and no CP.
    mod=RNG.integers(0,4,size=(N//256,256),dtype=np.int8)
    sym=np.exp(1j*(np.pi/4 + mod*np.pi/2))
    z=np.fft.ifft(sym,axis=1).ravel()*17500
    yield 'ofdm_qpsk', q12(np.column_stack([z.real,z.imag]))
    z=RNG.normal(0,170,N)+1j*RNG.normal(0,170,N)+ 950*np.exp(2j*np.pi*0.0781*n)
    yield 'tone_plus_noise', q12(np.column_stack([z.real,z.imag]))
    burst=RNG.normal(0,600,(N,2)); burst[(n.astype(int)//8192)%4 != 0,:]=0
    yield 'bursty_sparse', q12(burst)
    # Predictors are often hurt by abruptly alternating full-range samples.
    pathological=np.empty((N,2),dtype=np.int16)
    pathological[:,0]=np.where((n.astype(int)%2)==0,-2048,2047)
    pathological[:,1]=RNG.integers(-2048,2048,N,dtype=np.int16)
    yield 'pathological', pathological


def pack12(arr):
    a=arr[:,0].astype(np.uint16)&0x0fff
    b=arr[:,1].astype(np.uint16)&0x0fff
    out=np.empty((len(arr),3),dtype=np.uint8)
    out[:,0]=a&255
    out[:,1]=((a>>8)&15)|((b&15)<<4)
    out[:,2]=b>>4
    return out.tobytes()


def unpack12(data):
    a=np.frombuffer(data,dtype=np.uint8).reshape(-1,3).astype(np.uint16)
    x=(a[:,0]|((a[:,1]&15)<<8)).astype(np.int16)
    y=((a[:,1]>>4)|(a[:,2]<<4)).astype(np.int16)
    x=((x.astype(np.int32)^0x800)-0x800).astype(np.int16)
    y=((y.astype(np.int32)^0x800)-0x800).astype(np.int16)
    return np.stack([x,y],axis=1)


def bfp8(arr,block=256):
    # Saturation-free signed arithmetic right-shift: reconstruction error <= 2^exponent-1.
    x=arr.astype(np.int32)
    assert len(arr)%block==0
    out=np.empty_like(x,dtype=np.int8)
    exps=np.empty(len(arr)//block,dtype=np.uint8)
    for j,st in enumerate(range(0,len(arr),block)):
        v=x[st:st+block]
        e=0
        while (v.min() >> e) < -128 or (v.max() >> e) > 127:
            e+=1
        exps[j]=e
        out[st:st+block]=(v>>e).astype(np.int8)
    rec=out.astype(np.int32)
    for j,st in enumerate(range(0,len(arr),block)):
        rec[st:st+block]<<=int(exps[j])
    return out.tobytes()+exps.tobytes(),rec.astype(np.int16),exps


def zstd_compress(payload):
    exe=shutil.which('zstd')
    if not exe:
        return None
    return subprocess.run([exe,'-q','-1','-c'],input=payload,capture_output=True,check=True).stdout


def main():
    records=[]; log=[]
    log.append(f'utc_label=2026-11-21 synthetic exploratory probe seed={SEED} N={N}')
    log.append(f'python={sys.version.split()[0]} numpy={np.__version__} os={platform.platform()}')
    log.append(f'zstd={shutil.which("zstd")} lz4_python={bool(lz4frame)}')
    for name,a in sources():
        assert len(a)==N and a.dtype==np.int16
        raw=a.astype('<i2').tobytes()
        packed=pack12(a)
        assert np.array_equal(unpack12(packed),a)
        bfp,bfp_rec,exps=bfp8(a)
        residual=bfp_rec.astype(np.float64)-a.astype(np.float64)
        mse=np.mean(np.sum(residual*residual,axis=1))
        sig=np.mean(np.sum(a.astype(np.float64)**2,axis=1))
        evm=math.sqrt(mse/sig)*100 if sig else 0.
        peak=int(np.abs(residual).max())
        enc={
          'raw_cs16':raw, 'packed_cs12':packed,
          'zlib1_raw16':zlib.compress(raw,1),
          'zlib1_packed12':zlib.compress(packed,1),
          'zlib6_packed12':zlib.compress(packed,6),
          'bfp8_256':bfp
        }
        if lz4frame:
            enc['lz4_packed12']=lz4frame.compress(packed,compression_level=0)
        if shutil.which('zstd'):
            enc['zstd1_packed12']=zstd_compress(packed)
        for codec,blob in enc.items():
            records.append(dict(source=name,codec=codec,source_bytes=len(raw),native_bits_per_complex=24,
                                encoded_bytes=len(blob),encoded_bits_per_complex=round(8*len(blob)/N,5),
                                savings_vs_cs16_pct=round((1-len(blob)/len(raw))*100,4),
                                savings_vs_native24_pct=round((1-len(blob)/len(packed))*100,4),
                                lossless='no' if codec.startswith('bfp') else 'yes',
                                evm_pct=round(evm,4) if codec.startswith('bfp') else 0,
                                peak_error_lsb=peak if codec.startswith('bfp') else 0))
        log.append(f'{name}: raw={len(raw)} packed={len(packed)} sha256_raw={hashlib.sha256(raw).hexdigest()} bfp8_evm_pct={evm:.4f} peak_err={peak} exp_range={exps.min()}..{exps.max()}')
    out=OUT/'synthetic_results.csv'
    with out.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(records[0]));w.writeheader();w.writerows(records)
    (OUT/'run.log').write_text('\n'.join(log)+'\n',encoding='utf-8')
    print('\n'.join(log))
    print('results_file=',out)

if __name__=='__main__':main()
