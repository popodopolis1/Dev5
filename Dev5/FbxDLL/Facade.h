#pragma once

#include <vector>
#ifdef FBXDLL_EXPORTS
#define DLLEXPORT1 __declspec(dllexport)
#else
#define DLLEXPORT1 __declspec(dllimport)
#endif

class DLLEXPORT1 Facade
{
public:
	Facade();
	~Facade();
};

