#!/bin/bash

# build a windows binary package

MGCC=i686-w64-mingw32-gcc
MWINDRES=i686-w64-mingw32-windres
LIBNAME=libhdcd

if [ -z `which zip` ]; then echo "Needs zip"; exit 1; fi
if [ -z `which unix2dos` ]; then echo "Needs unix2dos"; exit 1; fi
if [ -z `which perl` ]; then echo "Needs perl"; exit 1; fi
if [ -z `which "$MGCC"` ]; then echo "Needs mingw gcc"; exit 1; fi
if [ -z `which "$MWINDRES"` ]; then echo "Needs mingw windres"; exit 1; fi

PVER=$(./package_version.sh)
WVER=$(echo "$PVER" | perl -e 'while(<>) {print ((/^([0-9]+)\.([0-9]+)-([0-9]+|)/) ? "$1,$2,".($3||0).",0" : "0,0,0,0")}')
echo "PVER: $PVER -- WVER: $WVER"

create_rc() {
RN="$1"
ON="$2"
cat << EOF > "$RN.rc"
1 VERSIONINFO
FILEVERSION     $WVER
PRODUCTVERSION  $WVER
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904E4"
    BEGIN
      VALUE "CompanyName", ""
      VALUE "FileDescription", "High Definition Compatible Digitial (HDCD) decoder library"
      VALUE "FileVersion", "$PVER"
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
"$MWINDRES" "$RN.rc" -O coff -o "$RN"
}

mkdir -p win-bin
cd win-bin

create_rc "libhdcd.res" "libhdcd.dll"
create_rc "hdcd-detect.res" "hdcd-detect.exe"
create_rc "hdcd.res" "hdcd.exe"

"$MGCC" -c ../src/hdcd_decode2.c ../src/hdcd_simple.c ../src/hdcd_libversion.c
"$MGCC" -shared -o $LIBNAME.dll hdcd_decode2.o hdcd_libversion.o hdcd_simple.o libhdcd.res -Wl,--out-implib,$LIBNAME.dll.a

"$MGCC" -c -DBUILD_HDCD_EXE_COMPAT ../tool/hdcd-detect.c ../tool/wavreader.c ../tool/wavout.c
"$MGCC" -o hdcd.exe hdcd-detect.o wavreader.o wavout.o hdcd_decode2.o  hdcd_libversion.o  hdcd_simple.o hdcd.res
rm -f hdcd-detect.o wavreader.o wavout.o
rm -f hdcd_decode2.o hdcd_simple.o hdcd_libversion.o

"$MGCC" -c ../tool/hdcd-detect.c ../tool/wavreader.c ../tool/wavout.c
"$MGCC" -o hdcd-detect.exe hdcd-detect.o wavreader.o wavout.o hdcd-detect.res -L. -l$LIBNAME
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
unix2dos -o "$PKN/LICENSE.txt"
unix2dos -o "$PKN/AUTHORS.txt"
unix2dos -o "$PKN/FILES.txt"
zip "$PKN.zip" "$PKN/hdcd.exe" "$PKN/hdcd-detect.exe" "$PKN/libhdcd.dll" "$PKN/LICENSE.txt" "$PKN/AUTHORS.txt" "$PKN/FILES.txt"
rm -f "$PKN/hdcd.exe" "$PKN/hdcd-detect.exe" "$PKN/libhdcd.dll" "$PKN/LICENSE.txt" "$PKN/AUTHORS.txt" "$PKN/FILES.txt"
rmdir "$PKN"

cd ..
