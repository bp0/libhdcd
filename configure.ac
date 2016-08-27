# Copyright (c) 2016 Luca Barbato
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

AC_PREREQ([2.53])

AC_INIT([hdcd_decode], [m4_esyscmd([./package_version.sh])])
AC_CONFIG_SRCDIR([src/hdcd_decode2.h])
AM_INIT_AUTOMAKE([1.6 foreign subdir-objects tar-ustar dist-zip])
m4_ifdef([AM_SILENT_RULES], [
    AM_SILENT_RULES([yes])
])

AC_CANONICAL_HOST

AC_PROG_SED
AC_PROG_AWK

HDCD_ABI_VERSION=m4_esyscmd([./abi_version.sh])
HDCD_PACKAGE_VERSION=m4_esyscmd([./package_version.sh])
HDCD_LIBTOOL_VERSION=m4_esyscmd([./abi_version.sh -libtool])

AC_SUBST(HDCD_ABI_VERSION)
AC_SUBST(HDCD_PACKAGE_VERSION)
AC_SUBST(HDCD_LIBTOOL_VERSION)

AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_SEARCH_LIBS([sin], [m], [], [
  AC_MSG_ERROR([unable to find the sin() function])
])

LT_INIT

DOLT

AC_CONFIG_MACRO_DIR([m4])

AC_OUTPUT([
Makefile
libhdcd.pc
])

