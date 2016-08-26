#!/bin/bash

MGCC=i686-w64-mingw32-gcc
MWINDRES=i686-w64-mingw32-windres
LIBNAME=libhdcd

if [ -z `which perl` ]; then echo "Needs perl"; exit 1; fi
if [ -z `which "$MGCC"` ]; then echo "Needs mingw gcc"; exit 1; fi
if [ -z `which "$MWINDRES"` ]; then echo "Needs mingw windres"; exit 1; fi

PVER=$(./package_version.sh)
WVER=$(echo "$PVER" | perl -e 'while(<>) {print ((/^([0-9]+)\.([0-9]+)-([0-9]+)/) ? "$1,$2,$3,0" : "0,0,0,0")}')
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

"$MGCC" -c ../hdcd_decode2.c ../hdcd_simple.c ../hdcd_libversion.c
"$MGCC" -shared -o $LIBNAME.dll hdcd_decode2.o  hdcd_libversion.o  hdcd_simple.o libhdcd.res -Wl,--out-implib,$LIBNAME.a

"$MGCC" -c -DBUILD_HDCD_EXE_COMPAT ../tool/hdcd-detect.c ../tool/wavreader.c ../tool/wavout.c
"$MGCC" -o hdcd.exe hdcd-detect.o wavreader.o wavout.o hdcd_decode2.o  hdcd_libversion.o  hdcd_simple.o hdcd.res
rm -f hdcd-detect.o wavreader.o wavout.o
rm -f hdcd_decode2.o hdcd_simple.o hdcd_libversion.o

"$MGCC" -c ../tool/hdcd-detect.c ../tool/wavreader.c ../tool/wavout.c
"$MGCC" -o hdcd-detect.exe hdcd-detect.o wavreader.o wavout.o hdcd-detect.res -L. -l$LIBNAME
rm -f hdcd-detect.o wavreader.o wavout.o

rm -f "libhdcd.res" "hdcd-detect.res" "hdcd.res"
rm -f "libhdcd.res.rc" "hdcd-detect.res.rc" "hdcd.res.rc"

cd ..
