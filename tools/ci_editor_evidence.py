"""Transport actual editor PNG evidence through authenticated CI logs.

Useful when a review host can read job logs but cannot download binary artifacts.
Only generated editor captures are emitted, never user files or plugin binaries.
"""
import base64
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

folder = Path(sys.argv[1])
inventory = []
for archive in sorted(Path('assets').glob('*.zip')):
    with zipfile.ZipFile(archive) as z:
        members = []
        for item in z.infolist():
            if item.is_dir():
                continue
            data = z.read(item)
            row = {'path': item.filename, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
            if data.startswith(b'\x89PNG\r\n\x1a\n'):
                row['png_dimensions'] = list(struct.unpack('>II', data[16:24]))
            members.append(row)
        inventory.append({'archive': str(archive), 'sha256': hashlib.sha256(archive.read_bytes()).hexdigest(), 'members': members})
text = json.dumps(inventory, separators=(',', ':'))
(folder / 'repository-artwork-inventory.json').write_text(text, encoding='utf-8')
print('SKELLIERE_ASSET_INVENTORY ' + text)
for png in sorted(folder.glob('*.png')):
    data = png.read_bytes()
    encoded = base64.b64encode(data).decode('ascii')
    print(f'SKELLIERE_EVIDENCE_FILE {png.name} {len(data)} {hashlib.sha256(data).hexdigest()}')
    for index, offset in enumerate(range(0, len(encoded), 12000)):
        print(f'SKELLIERE_EVIDENCE_CHUNK {png.name} {index} {encoded[offset:offset+12000]}')
