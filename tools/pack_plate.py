#!/usr/bin/env python3
"""Convert an image to JCR1 (RGBA8, little-endian dimensions), without retouching."""
import argparse
import struct
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path)
parser.add_argument('destination', type=Path)
args = parser.parse_args()
with Image.open(args.source) as source:
    rgba = source.convert('RGBA')
    if not (1 <= rgba.width <= 4096 and 1 <= rgba.height <= 4096):
        parser.error('Dimensions must be between 1 and 4096 pixels')
    args.destination.write_bytes(b'JCR1' + struct.pack('<II', *rgba.size) + rgba.tobytes())
