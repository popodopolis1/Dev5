#pragma once
#include <fbxsdk.h>
#include <Windows.h>
#include <vector>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <d3d11.h>

#ifdef FBXDLL_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
using namespace DirectX;
using namespace std;

class DLLEXPORT Export
{
public:
	Export();
	~Export();
};

