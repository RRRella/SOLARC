#ifdef ENERF_CORE_STATIC
	#define ENERF_API
#else
	#ifdef ENERF_CORE_BUILD
		#define ENERF_CORE_API __declspec(dllexport)
	#else
		#define ENERF_CORE_API __declspec(dllimport)
	#endif
#endif