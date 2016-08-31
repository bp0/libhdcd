#!/bin/bash

# build a windows binary package

MAR=i686-w64-mingw32-ar
MGCC=i686-w64-mingw32-gcc
MWINDRES=i686-w64-mingw32-windres
CFLAGS="-O2 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=missing-prototypes"
LIBNAME=libhdcd

if [ -z `which zip` ]; then echo "Needs zip"; exit 1; fi
if [ -z `which awk` ]; then echo "Needs awk"; exit 1; fi
if [ -z `which egrep` ]; then echo "Needs egrep"; exit 1; fi
if [ -z `which sed` ]; then echo "Needs sed"; exit 1; fi
if [ -z `which perl` ]; then echo "Needs perl"; exit 1; fi
if [ -z `which "$MAR"` ]; then echo "Needs mingw ar"; exit 1; fi
if [ -z `which "$MGCC"` ]; then echo "Needs mingw gcc"; exit 1; fi
if [ -z `which "$MWINDRES"` ]; then echo "Needs mingw windres"; exit 1; fi

rm -f hdcd-detect.exe  hdcd.exe  libhdcd.a  libhdcd.dll  libhdcd.dll.a

PVER=$(./package_version.sh)
WVER=$(echo "$PVER" | perl -e 'while(<>) {print ((/^([0-9]+)\.([0-9]+)-([0-9]+|)/) ? "$1,$2,".($3||0).",0" : "0,0,0,0")}')
LVER=$(./abi_version.sh -winh)
DLLVER=$(./abi_version.sh -win)
MVER=$(./abi_version.sh -major)
echo "Package version: $PVER -- exe version: $WVER"
echo "Library version: $LVER -- dll version: $DLLVER"

create_rc() {
RN="$1"
ON="$2"
PP="$3"
cat << EOF > "$RN.rc"
#include <windows.h>
1 VERSIONINFO
#ifdef DLL
FILEVERSION     $DLLVER
FILETYPE        VFT_DLL
#else
FILEVERSION     $WVER
FILETYPE        VFT_APP
#endif
PRODUCTVERSION  $WVER
FILEOS          VOS_NT_WINDOWS32
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904E4"
    BEGIN
      VALUE "CompanyName", ""
#ifdef DLL
      VALUE "FileDescription", "High Definition Compatible Digitial (HDCD) decoder library"
      VALUE "FileVersion", "$LVER"
#else
      VALUE "FileDescription", "High Definition Compatible Digitial (HDCD) decoder"
      VALUE "FileVersion", "$PVER"
#endif
      VALUE "InternalName", "libhdcd"
      VALUE "LegalCopyright", "libhdcd AUTHORS"
      VALUE "OriginalFilename", "$ON"
      VALUE "ProductName", "libhdcd"
      VALUE "ProductVersion", "$PVER"
    END
  END

  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x409, 1252
  END
END
EOF
"$MWINDRES" "$PP" "$RN.rc" -O coff -o "$RN"
}

mkdir -p win-bin
cd win-bin

create_rc "libhdcd.res" "libhdcd.dll" "-DDLL"
create_rc "hdcd-detect.res" "hdcd-detect.exe" "-UDLL"
create_rc "hdcd.res" "hdcd.exe" "-UDLL"

cat << EOF > "libhdcd.ver"
LIBHDCD_$MVER {
    global:
        hdcd_*;
    local:
        *;
};
EOF

"$MGCC" $CFLAGS -c ../src/hdcd_decode2.c ../src/hdcd_simple.c ../src/hdcd_libversion.c
"$MAR" crsu $LIBNAME.a hdcd_decode2.o hdcd_libversion.o hdcd_simple.o
"$MGCC" -shared -Wl,--out-implib,$LIBNAME.dll.a -Wl,--version-script,libhdcd.ver -s -o $LIBNAME.dll hdcd_decode2.o hdcd_libversion.o hdcd_simple.o libhdcd.res
rm -f libhdcd.ver

"$MGCC" $CFLAGS -c -DBUILD_HDCD_EXE_COMPAT ../tool/hdcd-detect.c ../tool/wavreader.c ../tool/wavout.c
"$MGCC" -s -o hdcd.exe hdcd-detect.o wavreader.o wavout.o $LIBNAME.a hdcd.res
rm -f hdcd-detect.o wavreader.o wavout.o
rm -f hdcd_decode2.o hdcd_simple.o hdcd_libversion.o

"$MGCC" $CFLAGS -c ../tool/hdcd-detect.c ../tool/wavreader.c ../tool/wavout.c
"$MGCC" -s -o hdcd-detect.exe hdcd-detect.o wavreader.o wavout.o hdcd-detect.res -L. -l$LIBNAME
rm -f hdcd-detect.o wavreader.o wavout.o

rm -f "libhdcd.res" "hdcd-detect.res" "hdcd.res"
rm -f "libhdcd.res.rc" "hdcd-detect.res.rc" "hdcd.res.rc"

PKN="libhdcd-win-$PVER"
mkdir -p $PKN
cat << EOF > "$PKN/FILES.txt"
libhdcd
A library for High Definition Compatible Digital (HDCD) decoding and analysis.
https://github.com/bp0/libhdcd

Package version: win-$PVER

Files:
libhdcd.dll	- libhdcd shared library
hdcd-detect.exe	- HDCD detection/decoding tool
hdcd.exe	- version of the decoding tool that attempts to
........	  be compatible with Key's original hdcd.exe
LICENSE.txt	- license(s)
AUTHORS.txt	- authors
FILES.txt	- list of files in the package
EOF
cp "hdcd.exe" "$PKN"
cp "hdcd-detect.exe" "$PKN"
cp "libhdcd.dll" "$PKN"
cp ../LICENSE "$PKN/LICENSE.txt"
cp ../AUTHORS "$PKN/AUTHORS.txt"
sed -i -e 's/\r*$/\r/' "$PKN/LICENSE.txt"
sed -i -e 's/\r*$/\r/' "$PKN/AUTHORS.txt"
sed -i -e 's/\r*$/\r/' "$PKN/FILES.txt"
zip "$PKN.zip" "$PKN/hdcd.exe" "$PKN/hdcd-detect.exe" "$PKN/libhdcd.dll" "$PKN/LICENSE.txt" "$PKN/AUTHORS.txt" "$PKN/FILES.txt"
rm -f "$PKN/hdcd.exe" "$PKN/hdcd-detect.exe" "$PKN/libhdcd.dll" "$PKN/LICENSE.txt" "$PKN/AUTHORS.txt" "$PKN/FILES.txt"
rmdir "$PKN"

cd ..
