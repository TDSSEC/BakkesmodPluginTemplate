#pragma once

#if defined(__has_include)
#  if __has_include("bakkesmod/wrappers/Structs.h")
#    include "bakkesmod/wrappers/Structs.h"
#  elif __has_include("bakkesmod/wrappers/structs.h")
#    include "bakkesmod/wrappers/structs.h"
#  else
#    error "Unable to locate BakkesMod structs header. Ensure the BakkesMod SDK include path is configured."
#  endif
#else
#  include "bakkesmod/wrappers/Structs.h"
#endif
