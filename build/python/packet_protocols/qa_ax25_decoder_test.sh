#!/usr/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/haaken/github-projects/gr-packet_protocols/python/packet_protocols
export GR_CONF_CONTROLPORT_ON=False
export PATH="/home/haaken/github-projects/gr-packet_protocols/build/python/packet_protocols":"$PATH"
export LD_LIBRARY_PATH="":$LD_LIBRARY_PATH
export PYTHONPATH=/home/haaken/github-projects/gr-packet_protocols/build/test_modules:$PYTHONPATH
/usr/bin/python3 /home/haaken/github-projects/gr-packet_protocols/python/packet_protocols/qa_ax25_decoder.py 
