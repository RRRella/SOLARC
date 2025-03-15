get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(is_multi_config)
	set(CMAKE_CONFIGURATION_TYPES "R_DEBUG;R_RELEASE;R_PROFILE;R_TEST" CACHE STRING "Types of configurations are/
	ENERF_DEBUG , ENERF_RELEASE , ENERF_PROFILE and ENERF_TEST ." FORCE)
else()
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "ENERF_DEBUG" CACHE STRING "Default config type is ENERF_DEBUG" FORCE)
	endif()
endif()

add_compile_definitions($<CONFIG>)
