include(CheckCSourceCompiles)

macro(TryCompileIconvOpen VARNAME)
  check_c_source_compiles("
#include <iconv.h>
int main (int argc, char **argv) {
  iconv_open(\"\", \"\");
  return 0;
}
" "${VARNAME}")
endmacro(TryCompileIconvOpen)

TryCompileIconvOpen(ICONV_BUILTIN)
if(ICONV_BUILTIN)
  set(ICONV_LIBRARIES)
else(ICONV_BUILTIN)
  set(CMAKE_REQUIRED_LIBRARIES iconv)
  TryCompileIconvOpen(ICONV_REQUIRES_LIB)
  if(ICONV_REQUIRES_LIB)
    set(ICONV_LIBRARIES "iconv")
  else(ICONV_REQUIRES_LIB)
    message(WARNING "Couldn't figure out how to compile iconv")
  endif(ICONV_REQUIRES_LIB)
endif(ICONV_BUILTIN)

set(CMAKE_REQUIRED_LIBRARIES)
