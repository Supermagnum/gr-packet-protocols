#!/bin/bash

# save as: diagnose_gr_module.sh

echo "=== 1. Check GNU Radio installation ==="

python3 -c "import gnuradio; print('GR path:', gnuradio.__path__)"

echo -e "\n=== 2. Check if packet_protocols package exists ==="

python3 -c "
import gnuradio
import os
gr_path = gnuradio.__path__[0]
pp_path = os.path.join(gr_path, 'packet_protocols')
print('Expected path:', pp_path)
print('Exists:', os.path.exists(pp_path))
if os.path.exists(pp_path):
    print('Contents:', os.listdir(pp_path))
"

echo -e "\n=== 3. Check for the compiled .so file ==="

find /usr -name "*packet_protocols*.so" 2>/dev/null
find /usr/local -name "*packet_protocols*.so" 2>/dev/null
find ~/.local -name "*packet_protocols*.so" 2>/dev/null

echo -e "\n=== 4. Check what gets imported ==="

python3 -c "
import gnuradio.packet_protocols as pp
print('Package location:', pp.__file__)
print('Package contents:', dir(pp))
"

echo -e "\n=== 5. Check for the pybind11 module specifically ==="

python3 -c "
import gnuradio.packet_protocols as pp
import os
pkg_dir = os.path.dirname(pp.__file__)
print('Looking in:', pkg_dir)
for f in os.listdir(pkg_dir):
    if 'python' in f or '.so' in f:
        print('  Found:', f)
"

echo -e "\n=== 6. Try direct import of the compiled module ==="

python3 -c "
import sys
# Try common installation paths
paths = [
    '/usr/lib/python3/dist-packages/gnuradio/packet_protocols',
    '/usr/local/lib/python3/dist-packages/gnuradio/packet_protocols',
    '/usr/lib/python3.11/site-packages/gnuradio/packet_protocols',
    '/usr/lib/python3.12/site-packages/gnuradio/packet_protocols',
]
for p in paths:
    if p not in sys.path:
        sys.path.insert(0, p)

# Now try to find any .so files
import os
for p in paths:
    if os.path.exists(p):
        print(f'In {p}:')
        for f in os.listdir(p):
            print(f'  {f}')
"

echo -e "\n=== 7. Check __init__.py content ==="

python3 -c "
import gnuradio.packet_protocols as pp
import inspect
print(inspect.getfile(pp))
" 2>/dev/null && \
python3 -c "
import gnuradio.packet_protocols as pp
import inspect
with open(inspect.getfile(pp), 'r') as f:
    print(f.read()[:500])
"

