add_library(COMMON_FLAGS INTERFACE)

# GCC/Clang
if (NOT MSVC)
  target_compile_options(COMMON_FLAGS INTERFACE
    -fstack-protector-strong -fPIC -faligned-new
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O2>
  )

  target_link_options(COMMON_FLAGS INTERFACE
    -Wl,--gc-sections -Wl,--icf=all -Wl,-z,norelro
    $<$<CONFIG:Release>:-flto>
    $<$<CONFIG:Debug>:-Wl,-v -Wl,-Map>
  )
else()

  target_compile_options(COMMON_FLAGS INTERFACE
    /permissive- /utf-8 /GS /Zc:__cplusplus /Zc:alignedNew /Zc:char8_t
    $<$<CONFIG:Debug>:/Zi /Od /MDd>
    $<$<CONFIG:Release>:/O2 /MD>
  )

  target_link_options(COMMON_FLAGS INTERFACE
    /OPT:REF /OPT:ICF /OPT:LBR /SUBSYSTEM:CONSOLE
    $<$<CONFIG:Release>:/LTCG /DEBUG:NONE /SUBSYSTEM:WINDOWS>
    $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL /MAP>
  )
endif()
