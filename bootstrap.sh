#!/bin/sh

set -ex

/c/opt/gnulib/gnulib-tool --libtool --update --import gethostname setenv
autoreconf -iv
autoreconf -iv tools
