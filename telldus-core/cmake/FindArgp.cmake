include(CheckCSourceCompiles)

macro(TryCompileArgpParse VARNAME)
  check_c_source_compiles("
#include <argp.h>
int main (int argc, char **argv) {
  argp_parse (0, argc, argv, 0, 0, 0);
  return 0;
}
" "${VARNAME}")
endmacro(TryCompileArgpParse)

TryCompileArgpParse(ARGP_BUILTIN)
if(ARGP_BUILTIN)
  set(ARGP_LIBRARIES)
else(ARGP_BUILTIN)
  set(CMAKE_REQUIRED_LIBRARIES argp)
  TryCompileArgpParse(ARGP_REQUIRES_LIB)
  if(${ARGP_REQUIRES_LIB})
    set(ARGP_LIBRARIES "argp")
  else(ARGP_REQUIRES_LIB)
    message(WARNING "Couldn't figure out how to compile argp")
  endif(ARGP_REQUIRES_LIB)
endif(ARGP_BUILTIN)

set(CMAKE_REQUIRED_LIBRARIES)
