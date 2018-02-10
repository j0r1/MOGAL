#!/bin/sh
aclocal
libtoolize -c -f
automake -c -a --foreign
autoconf
