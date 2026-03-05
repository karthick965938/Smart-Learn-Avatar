"""OGG/Opus container builder and parser for raw Opus packet streams."""
import struct

# OGG CRC-32 table (polynomial 0x04c11db7)
_CRC = [0] * 256
for _i in range(256):
    _r = _i << 24
    for _ in range(8):
        _r = ((_r << 1) ^ 0x04c11db7) if (_r & 0x80000000) else (_r << 1)
    _CRC[_i] = _r & 0xFFFFFFFF

def _crc32(data: bytes) -> int:
    crc = 0
    for b in data:
        crc = ((crc << 8) ^ _CRC[(crc >> 24) ^ b]) & 0xFFFFFFFF
    return crc

def _page(payload: bytes, serial: int, seq: int, granule: int,
          bos: bool = False, eos: bool = False) -> bytes:
    segs = []
    n = len(payload)
    while n >= 255:
        segs.append(255); n -= 255
    segs.append(n)
    flags = (0x02 if bos else 0) | (0x04 if eos else 0)
    h = bytearray(b'OggS' + bytes([0, flags])
                  + struct.pack('<q', granule)
                  + struct.pack('<I', serial)
                  + struct.pack('<I', seq)
                  + struct.pack('<I', 0)       # crc placeholder
                  + bytes([len(segs)]) + bytes(segs))
    h += payload
    h[22:26] = struct.pack('<I', _crc32(bytes(h)))
    return bytes(h)

def build_ogg_opus(packets: list[bytes], sample_rate: int = 16000,
                   channels: int = 1, frame_duration_ms: int = 60) -> bytes:
    """Wrap raw Opus packets into a valid OGG container readable by ffmpeg/decoders."""
    serial = 0x4F505553
    spf = sample_rate * frame_duration_ms // 1000  # samples per frame
    pre_skip = 120

    # OpusHead page (BOS)
    id_hdr = (b'OpusHead' + bytes([1, channels])
              + struct.pack('<H', pre_skip)
              + struct.pack('<I', sample_rate)
              + struct.pack('<hB', 0, 0))
    # OpusTags page
    vendor = b'py-ogg'
    tag_hdr = b'OpusTags' + struct.pack('<I', len(vendor)) + vendor + struct.pack('<I', 0)

    out = bytearray(_page(id_hdr, serial, 0, 0, bos=True))
    out += _page(tag_hdr, serial, 1, 0)

    granule = pre_skip
    for i, pkt in enumerate(packets):
        granule += spf
        out += _page(pkt, serial, 2 + i, granule, eos=(i == len(packets) - 1))
    return bytes(out)

def parse_ogg_packets(ogg_data: bytes) -> list[bytes]:
    """Extract individual Opus audio packets from an OGG container."""
    packets, pos = [], 0
    while pos < len(ogg_data):
        idx = ogg_data.find(b'OggS', pos)
        if idx == -1: break
        pos = idx
        if pos + 27 > len(ogg_data): break
        n = ogg_data[pos + 26]
        if pos + 27 + n > len(ogg_data): break
        segs = list(ogg_data[pos + 27: pos + 27 + n])
        hdr = 27 + n
        dsize = sum(segs)
        if pos + hdr + dsize > len(ogg_data): break
        data = ogg_data[pos + hdr: pos + hdr + dsize]
        pos += hdr + dsize
        if data.startswith(b'OpusHead') or data.startswith(b'OpusTags'): continue
        # Reconstruct packets from lacing
        dp, pkt = 0, bytearray()
        for s in segs:
            pkt += data[dp: dp + s]; dp += s
            if s < 255:
                if pkt: packets.append(bytes(pkt))
                pkt = bytearray()
        if pkt: packets.append(bytes(pkt))
    return packets
