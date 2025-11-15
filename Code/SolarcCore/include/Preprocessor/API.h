#if defined(_WIN32) || defined(_WIN64)
#ifdef SOLARC_CORE_STATIC
	#define SOLARC_CORE_API
#else
	#ifdef SOLARC_CORE_BUILD
		#define SOLARC_CORE_API __declspec(dllexport)
	#else
		#define SOLARC_CORE_API __declspec(dllimport)
	#endif
#endif
#endif