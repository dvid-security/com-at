# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/eunous/esp-idf/components/bootloader/subproject"
  "/home/eunous/git/dvid/com-at/com-at/build/bootloader"
  "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix"
  "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix/tmp"
  "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix/src/bootloader-stamp"
  "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix/src"
  "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/eunous/git/dvid/com-at/com-at/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
