#include <iostream>
#include <ctime>
#include <Windows.h>
#include <assert.h>
#include <vector>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <DirectXMath.h>

using namespace std;
using namespace DirectX;

#include "GroundShader_VS.csh"
#include "GroundShader_PS.csh"
#include "DebugVS.csh"
#include "DebugPS.csh"
#include "DDSTextureLoader.h"
#include "Export.h"


class WIN_APP
{
#pragma region Privates
	HRESULT							hr;
	HINSTANCE						application;
	WNDPROC							appWndProc;
	HWND							window;

	ID3D11Device *device;
	ID3D11DeviceContext *deviceContext;
	ID3D11RenderTargetView *RTV;
	IDXGISwapChain *swapChain;
	D3D11_VIEWPORT viewport;


	struct World
	{
		XMMATRIX WorldMatrix;
	};

	struct View
	{
		XMMATRIX ViewMatrix;
		XMMATRIX ProjectionMatrix;
	};

	View view;
	World groundWorld;

	ID3D11Texture2D* depthStencil = NULL;
	D3D11_TEXTURE2D_DESC depthDesc;

	D3D11_DEPTH_STENCIL_DESC stencilDesc;
	ID3D11DepthStencilState *stencilState;
	ID3D11DepthStencilView* stencilView;
	D3D11_DEPTH_STENCIL_VIEW_DESC stencilViewdesc;

	ID3D11RasterizerState* solidState;
	ID3D11RasterizerState* wireState;
	vector<World> boneWorld;
	vector<XMMATRIX> jointMats;
#pragma endregion

#pragma region Publics
public:

	struct GVERTEX
	{
		XMVECTOR pos;
		XMVECTOR color;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
	};

	struct debugVert
	{
		XMVECTOR pos;
		float clr[4];
	};

	GVERTEX	groundPlane[4];

	ID3D11Buffer *groundBuffer;
	D3D11_BUFFER_DESC groundBufferdesc;
	ID3D11Buffer *groundConstant;
	D3D11_BUFFER_DESC groundConstantdesc;
	ID3D11Buffer *groundIndex;
	ID3D11VertexShader *groundVertshader;
	ID3D11PixelShader *groundPixshader;
	D3D11_BUFFER_DESC groundIndexdesc;
	ID3D11ShaderResourceView *groundshaderView;
	ID3D11SamplerState *groundSample;
	D3D11_SAMPLER_DESC groundsampleDesc;
	ID3D11InputLayout *groundInputlayout;

	ID3D11Buffer * viewConstant;
	D3D11_BUFFER_DESC viewConstdesc;

	ID3D11Buffer* teddyBuffer;
	ID3D11Buffer* teddyIndex;
	D3D11_BUFFER_DESC teddyVertBuffDesc;
	D3D11_BUFFER_DESC teddyIndBuffDesc;
	ID3D11ShaderResourceView *teddyshaderView;
	ID3D11SamplerState *teddySample;
	D3D11_SAMPLER_DESC teddysampleDesc;
	int teddyVertCount;
	vector<DllExport::JointMatrix> teddyJoints;

	ID3D11Buffer* debugBuffer;
	D3D11_BUFFER_DESC debugBuffDesc;
	vector<debugVert> debugArray[1024];
	ID3D11VertexShader *debugVertshader;
	ID3D11PixelShader *debugPixshader; 
	ID3D11InputLayout *debugInputlayout;
	int count = 0;

	POINT point;
	vector<XMVECTOR> teddyDebugPos;

	ID3D11Buffer* boneVertex, *boneIndex;
	D3D11_BUFFER_DESC bonevertexBufferDesc, boneindexBufferDesc;
	int boneVertcount;
	ID3D11ShaderResourceView *boneshaderView;
	ID3D11SamplerState *boneSample;
	D3D11_SAMPLER_DESC bonesampleDesc;
	vector<ID3D11Buffer*> boneConstant;
	D3D11_BUFFER_DESC boneConstantdesc;

#pragma endregion

	WIN_APP(HINSTANCE hinst, WNDPROC proc);
	void add_debug_line(XMVECTOR pointA, XMVECTOR pointB, float color[4]);
	bool Run();
	bool ShutDown();
};

void WIN_APP::add_debug_line(XMVECTOR pointA, XMVECTOR pointB, float color[4])
{
	debugVert tmp;
	tmp.pos = pointA;
	tmp.clr[0] = color[0];
	tmp.clr[1] = color[1];
	tmp.clr[2] = color[2];
	tmp.clr[3] = color[3];
	debugArray->push_back(tmp);

	debugVert tmp2;
	tmp2.pos = pointA;
	tmp2.clr[0] = color[0];
	tmp2.clr[1] = color[1];
	tmp2.clr[2] = color[2];
	tmp2.clr[3] = color[3]; 
	debugArray->push_back(tmp2);

	count += 2;
}

XMMATRIX floatArrayToMatrix(float arr[16])
{
	XMMATRIX temp(arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14], arr[15]);
	return temp;
}

WIN_APP::WIN_APP(HINSTANCE hinst, WNDPROC proc)
{
#pragma region Windows Initialization
	application = hinst;
	appWndProc = proc;

	WNDCLASSEX  wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = appWndProc;
	wndClass.lpszClassName = L"RTA Project";
	wndClass.hInstance = application;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	RegisterClassEx(&wndClass);

	RECT window_size = { 0, 0, 1280, 720 };
	AdjustWindowRect(&window_size, WS_OVERLAPPEDWINDOW, false);

	window = CreateWindow(L"RTA Project", L"RTA Project", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, window_size.right - window_size.left, window_size.bottom - window_size.top,
		NULL, NULL, application, this);

	ShowWindow(window, SW_SHOW);
#pragma endregion

#pragma region Device and SwapChain Initialization
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.BufferCount = 1;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferDesc.Width = 1280;
	desc.BufferDesc.Height = 720;
	desc.Flags = D3D11_CREATE_DEVICE_DEBUG;
	desc.OutputWindow = window;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	D3D_FEATURE_LEVEL feature[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		feature,
		1,
		D3D11_SDK_VERSION,
		&desc,
		&swapChain,
		&device,
		NULL,
		&deviceContext);
#pragma endregion

#pragma region Render Target View Setup
	ID3D11Resource *p_RT;
	swapChain->GetBuffer(0, __uuidof(p_RT), reinterpret_cast<void**>(&p_RT));

	device->CreateRenderTargetView(p_RT, NULL, &RTV);
	p_RT->Release();

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 1280;
	viewport.Height = 720;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
#pragma endregion

#pragma region Rasterizer Setup
	D3D11_RASTERIZER_DESC descRas = {};
	descRas.FillMode = D3D11_FILL_SOLID;
	descRas.CullMode = D3D11_CULL_NONE;
	descRas.FrontCounterClockwise = FALSE;
	descRas.DepthBias = 0;
	descRas.SlopeScaledDepthBias = 0.0f;
	descRas.DepthBiasClamp = 0.0f;
	descRas.DepthClipEnable = TRUE;
	descRas.ScissorEnable = FALSE;
	descRas.MultisampleEnable = FALSE;
	descRas.AntialiasedLineEnable = TRUE;

	device->CreateRasterizerState(&descRas, &solidState);

	descRas.FillMode = D3D11_FILL_WIREFRAME;

	device->CreateRasterizerState(&descRas, &wireState);
#pragma endregion

#pragma region Ground Quad Initialization
	groundPlane[0].pos.m128_f32[0] = -15;
	groundPlane[0].pos.m128_f32[1] = -5;
	groundPlane[0].pos.m128_f32[2] = -15;
	groundPlane[0].pos.m128_f32[3] = 1;
	groundPlane[0].uv.x = 0;
	groundPlane[0].uv.y = 1;
	groundPlane[0].normal.x = 0;
	groundPlane[0].normal.y = 1;
	groundPlane[0].normal.z = 0;

	groundPlane[1].pos.m128_f32[0] = -15;
	groundPlane[1].pos.m128_f32[1] = -5;
	groundPlane[1].pos.m128_f32[2] = 15;
	groundPlane[1].pos.m128_f32[3] = 1;
	groundPlane[1].uv.x = 0;
	groundPlane[1].uv.y = 0;
	groundPlane[1].normal.x = 0;
	groundPlane[1].normal.y = 1;
	groundPlane[1].normal.z = 0;

	groundPlane[2].pos.m128_f32[0] = 15;
	groundPlane[2].pos.m128_f32[1] = -5;
	groundPlane[2].pos.m128_f32[2] = -15;
	groundPlane[2].pos.m128_f32[3] = 1;
	groundPlane[2].uv.x = 1;
	groundPlane[2].uv.y = 1;
	groundPlane[2].normal.x = 0;
	groundPlane[2].normal.y = 1;
	groundPlane[2].normal.z = 0;

	groundPlane[3].pos.m128_f32[0] = 15;
	groundPlane[3].pos.m128_f32[1] = -5;
	groundPlane[3].pos.m128_f32[2] = 15;
	groundPlane[3].pos.m128_f32[3] = 1;
	groundPlane[3].uv.x = 1;
	groundPlane[3].uv.y = 0;
	groundPlane[3].normal.x = 0;
	groundPlane[3].normal.y = 1;
	groundPlane[3].normal.z = 0;

	UINT groundIndexes[12]
	{
		0, 1, 2,
		3, 2, 1,
		2, 1, 0,
		1, 2, 3
	};

	groundBufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
	groundBufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	groundBufferdesc.CPUAccessFlags = NULL;
	groundBufferdesc.ByteWidth = sizeof(GVERTEX) * 4;
	groundBufferdesc.MiscFlags = 0;
	groundBufferdesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = groundPlane;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	device->CreateBuffer(&groundBufferdesc, &InitData, &groundBuffer);

	
	groundConstantdesc.Usage = D3D11_USAGE_DYNAMIC;
	groundConstantdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	groundConstantdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	groundConstantdesc.ByteWidth = sizeof(World);
	groundConstantdesc.MiscFlags = 0;
	groundConstantdesc.StructureByteStride = 0;

	InitData.pSysMem = &groundWorld;
	device->CreateBuffer(&groundConstantdesc, &InitData, &groundConstant);

	groundIndexdesc.Usage = D3D11_USAGE_IMMUTABLE;
	groundIndexdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	groundIndexdesc.CPUAccessFlags = NULL;
	groundIndexdesc.ByteWidth = sizeof(groundIndexes);
	groundIndexdesc.MiscFlags = 0;
	groundIndexdesc.StructureByteStride = 0;

	InitData.pSysMem = groundIndexes;
	device->CreateBuffer(&groundIndexdesc, &InitData, &groundIndex);

	groundsampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	groundsampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	groundsampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	groundsampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	groundsampleDesc.MinLOD = (-FLT_MAX);
	groundsampleDesc.MaxLOD = (FLT_MAX);
	groundsampleDesc.MipLODBias = 0.0f;
	groundsampleDesc.MaxAnisotropy = 1;
	groundsampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	groundsampleDesc.BorderColor[0] = 1;
	groundsampleDesc.BorderColor[1] = 1;
	groundsampleDesc.BorderColor[2] = 1;
	groundsampleDesc.BorderColor[3] = 1;

#pragma endregion

#pragma region Teddy Initialization
	vector<DllExport::PNUVertex> verts;
	DllExport::Export test;

	verts = test.LoadFBX(verts, "Teddy_Idle.fbx");
	teddyVertCount = (int)verts.size();

	teddyJoints = test.GetJoints(teddyJoints, "Teddy_Idle.fbx");

	for (int i = 0; i < teddyJoints.size(); i++)
	{
		XMMATRIX temp = floatArrayToMatrix(teddyJoints[i].global_xform);
		jointMats.push_back(temp);
	}

	GVERTEX* teddyVerts = new GVERTEX[teddyVertCount];
	unsigned int* teddyIndicies = new unsigned int[teddyVertCount];

	DllExport::PNUVertex* teddyArray = &verts[0];
	for (unsigned int i = 0; i < teddyVertCount; i++)
	{
		teddyVerts[i].pos = { teddyArray[i].mPosition.x, teddyArray[i].mPosition.y, teddyArray[i].mPosition.z, 0.0f };
		teddyVerts[i].color = { 0, 0, 0, 0 };
		teddyVerts[i].uv.x = teddyArray[i].mUV.x;
		teddyVerts[i].uv.y = teddyArray[i].mUV.y;
		teddyVerts[i].normal.x = teddyArray[i].mNormal.x;
		teddyVerts[i].normal.y = teddyArray[i].mNormal.y;
		teddyVerts[i].normal.z = teddyArray[i].mNormal.z;
		teddyIndicies[i] = i;
	}

	teddyVertBuffDesc.Usage = D3D11_USAGE_IMMUTABLE;
	teddyVertBuffDesc.ByteWidth = sizeof(GVERTEX)*teddyVertCount;
	teddyVertBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	teddyVertBuffDesc.CPUAccessFlags = 0;
	teddyVertBuffDesc.MiscFlags = 0;
	teddyVertBuffDesc.StructureByteStride = 0;

	InitData.pSysMem = teddyVerts;
	device->CreateBuffer(&teddyVertBuffDesc, &InitData, &teddyBuffer);

	teddyIndBuffDesc.Usage = D3D11_USAGE_IMMUTABLE;
	teddyIndBuffDesc.ByteWidth = sizeof(unsigned long)*teddyVertCount;
	teddyIndBuffDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	teddyIndBuffDesc.CPUAccessFlags = 0;
	teddyIndBuffDesc.MiscFlags = 0;
	teddyIndBuffDesc.StructureByteStride = 0;

	InitData.pSysMem = teddyIndicies;
	device->CreateBuffer(&teddyIndBuffDesc, &InitData, &teddyIndex);

	teddysampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	teddysampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	teddysampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	teddysampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	teddysampleDesc.MinLOD = (-FLT_MAX);
	teddysampleDesc.MaxLOD = (FLT_MAX);
	teddysampleDesc.MipLODBias = 0.0f;
	teddysampleDesc.MaxAnisotropy = 1;
	teddysampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	teddysampleDesc.BorderColor[0] = 1;
	teddysampleDesc.BorderColor[1] = 1;
	teddysampleDesc.BorderColor[2] = 1;
	teddysampleDesc.BorderColor[3] = 1;

	delete[] teddyVerts;
	delete[] teddyIndicies;
#pragma endregion

#pragma region Bone Loading

	vector<DllExport::PNUVertex> bonefbxVerts;
	DllExport::Export fac;

	bonefbxVerts = fac.LoadFBX(bonefbxVerts, "Bone.fbx");


	int boneVerts = bonefbxVerts.size();
	boneVertcount = boneVerts;
	int boneIndexes = boneVerts;

	GVERTEX* bonevertices = new GVERTEX[boneVerts];

	unsigned int* boneindices = new unsigned int[boneIndexes];

	DllExport::PNUVertex* bonearr = &bonefbxVerts[0];

	for (int i = 0; i < boneVerts; i++)
	{
		bonevertices[i].pos = { bonearr[i].mPosition.x, bonearr[i].mPosition.y, bonearr[i].mPosition.z, 0.0f };
		bonevertices[i].color = { 0, 0, 0, 0 };
		bonevertices[i].uv.x = bonearr[i].mUV.x;
		bonevertices[i].uv.y = bonearr[i].mUV.y;
		bonevertices[i].normal.x = bonearr[i].mNormal.x;
		bonevertices[i].normal.y = bonearr[i].mNormal.y;
		bonevertices[i].normal.z = bonearr[i].mNormal.z;

		boneindices[i] = i;
	}

	bonevertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bonevertexBufferDesc.ByteWidth = sizeof(GVERTEX)*boneVerts;
	bonevertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bonevertexBufferDesc.CPUAccessFlags = 0;
	bonevertexBufferDesc.MiscFlags = 0;
	bonevertexBufferDesc.StructureByteStride = 0;

	InitData.pSysMem = bonevertices;
	device->CreateBuffer(&bonevertexBufferDesc, &InitData, &boneVertex);

	boneindexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	boneindexBufferDesc.ByteWidth = sizeof(unsigned long)*boneIndexes;
	boneindexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	boneindexBufferDesc.CPUAccessFlags = 0;
	boneindexBufferDesc.MiscFlags = 0;
	boneindexBufferDesc.StructureByteStride = 0;

	InitData.pSysMem = boneindices;
	device->CreateBuffer(&boneindexBufferDesc, &InitData, &boneIndex);

	boneConstantdesc.Usage = D3D11_USAGE_DYNAMIC;
	boneConstantdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	boneConstantdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	boneConstantdesc.ByteWidth = sizeof(World);
	boneConstantdesc.MiscFlags = 0;
	boneConstantdesc.StructureByteStride = 0;

	for (size_t i = 0; i < teddyJoints.size(); i++)
	{
		World bone;
		DllExport::JointMatrix boneJoint;
		bone.WorldMatrix = jointMats[i];
		boneWorld.push_back(bone);
	}

	for (size_t i = 0; i < teddyJoints.size(); i++)
	{
		InitData.pSysMem = &boneWorld[i];
		ID3D11Buffer *bone;
		device->CreateBuffer(&boneConstantdesc, &InitData, &bone);
		boneConstant.push_back(bone);
	}

	bonesampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	bonesampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	bonesampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	bonesampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	bonesampleDesc.MinLOD = (-FLT_MAX);
	bonesampleDesc.MaxLOD = (FLT_MAX);
	bonesampleDesc.MipLODBias = 0.0f;
	bonesampleDesc.MaxAnisotropy = 1;
	bonesampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	bonesampleDesc.BorderColor[0] = 1;
	bonesampleDesc.BorderColor[1] = 1;
	bonesampleDesc.BorderColor[2] = 1;
	bonesampleDesc.BorderColor[3] = 1;
#pragma endregion

#pragma region Debug Renderer Buffers
	debugBuffDesc.Usage = D3D11_USAGE_DYNAMIC;
	debugBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	debugBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	debugBuffDesc.ByteWidth = sizeof(debugVert) * 37;
	debugBuffDesc.MiscFlags = 0;
	debugBuffDesc.StructureByteStride = 0;

	//D3D11_SUBRESOURCE_DATA InitData;
	//InitData.pSysMem = groundPlane;
	//InitData.SysMemPitch = 0;
	//InitData.SysMemSlicePitch = 0;

	InitData.pSysMem = &debugArray[0];
	device->CreateBuffer(&debugBuffDesc, &InitData, &debugBuffer);
#pragma endregion

#pragma region Depth Stencil Initialization
	depthDesc.Width = 1280;
	depthDesc.Height = 720;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	device->CreateTexture2D(&depthDesc, NULL, &depthStencil);

	stencilDesc.DepthEnable = true;
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	stencilDesc.StencilEnable = true;
	stencilDesc.StencilReadMask = 0xFF;
	stencilDesc.StencilWriteMask = 0xFF;

	stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&stencilDesc, &stencilState);

	stencilViewdesc.Format = DXGI_FORMAT_D32_FLOAT;
	stencilViewdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	stencilViewdesc.Texture2D.MipSlice = 0;
	stencilViewdesc.Flags = NULL;

	device->CreateDepthStencilView(depthStencil, &stencilViewdesc, &stencilView);
#pragma endregion

#pragma region View Matrix Buffer Initialization
	viewConstdesc.Usage = D3D11_USAGE_DYNAMIC;
	viewConstdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	viewConstdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	viewConstdesc.ByteWidth = sizeof(View);
	viewConstdesc.MiscFlags = 0;
	viewConstdesc.StructureByteStride = 0;

	InitData.pSysMem = &view;

	device->CreateBuffer(&viewConstdesc, &InitData, &viewConstant);
#pragma endregion

#pragma region DDS Texture Creation
	CreateDDSTextureFromFile(device, L"stone_0001_c.dds", nullptr, &groundshaderView, 0);
	CreateDDSTextureFromFile(device, L"Teddy_Idle.dds", nullptr, &teddyshaderView, 0);
	CreateDDSTextureFromFile(device, L"BoneTexture.dds", nullptr, &boneshaderView, 0);
#pragma endregion

#pragma region Shader Initialization
	device->CreateVertexShader(GroundShader_VS, sizeof(GroundShader_VS), nullptr, &groundVertshader);
	device->CreatePixelShader(GroundShader_PS, sizeof(GroundShader_PS), nullptr, &groundPixshader);
	device->CreateVertexShader(DebugVS, sizeof(DebugVS), nullptr, &debugVertshader);
	device->CreatePixelShader(DebugPS, sizeof(DebugPS), nullptr, &debugPixshader);

	D3D11_INPUT_ELEMENT_DESC groundLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	D3D11_INPUT_ELEMENT_DESC debugLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	device->CreateSamplerState(&groundsampleDesc, &groundSample);
	device->CreateSamplerState(&teddysampleDesc, &teddySample);
	device->CreateSamplerState(&bonesampleDesc, &boneSample);
	device->CreateInputLayout(groundLayout, ARRAYSIZE(groundLayout), GroundShader_VS, sizeof(GroundShader_VS), &groundInputlayout);
	device->CreateInputLayout(debugLayout, ARRAYSIZE(debugLayout), DebugVS, sizeof(DebugVS), &debugInputlayout);
#pragma endregion

#pragma region Matrix Setup
	view.ViewMatrix = XMMatrixInverse(0, XMMatrixTranslation(0, 5, -10));
	groundWorld.WorldMatrix = XMMatrixIdentity();

	//bindPose = boneWorld;

	XMMATRIX projection;
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(65.0f), (1280.0f / 720.0f), 0.1f, 1000.0f);
	view.ProjectionMatrix = projection;
#pragma endregion


	for (int i = 0; i < teddyJoints.size(); i++)
	{
		XMVECTOR vec;
		vec.m128_f32[0] = teddyJoints[i].global_xform[12];
		vec.m128_f32[1] = teddyJoints[i].global_xform[13];
		vec.m128_f32[2] = teddyJoints[i].global_xform[14];
		vec.m128_f32[3] = teddyJoints[i].global_xform[15];
		teddyDebugPos.push_back(vec);
	}

}

bool WIN_APP::Run()
{
#pragma region View Matrix Inverse
	view.ViewMatrix = XMMatrixInverse(0, view.ViewMatrix);
#pragma endregion

#pragma region Camera Controls
	if (GetAsyncKeyState(VK_SPACE))
	{
		XMMATRIX straight = XMMatrixTranslation(0, 0.05f * 2, 0);
		view.ViewMatrix = XMMatrixMultiply(straight, view.ViewMatrix);
	}
	if (GetAsyncKeyState(VK_LSHIFT))
	{
		XMMATRIX down = XMMatrixTranslation(0, -0.05f * 2, 0);
		view.ViewMatrix = XMMatrixMultiply(down, view.ViewMatrix);
	}
	if (GetAsyncKeyState('A'))
	{
		XMMATRIX left = XMMatrixTranslation(-0.05f * 2, 0, 0);
		view.ViewMatrix = XMMatrixMultiply(left, view.ViewMatrix);
	}
	if (GetAsyncKeyState('D'))
	{
		XMMATRIX right = XMMatrixTranslation(0.05f * 2, 0, 0);
		view.ViewMatrix = XMMatrixMultiply(right, view.ViewMatrix);
	}

	if (GetAsyncKeyState('W'))
	{
		XMMATRIX forward = XMMatrixTranslation(0, 0, 0.05f * 2);
		view.ViewMatrix = XMMatrixMultiply(forward, view.ViewMatrix);
	}

	if (GetAsyncKeyState('S'))
	{
		XMMATRIX backward = XMMatrixTranslation(0, 0, -0.05f * 2);
		view.ViewMatrix = XMMatrixMultiply(backward, view.ViewMatrix);
	}

	POINT newPos;
	GetCursorPos(&newPos);
	if (GetAsyncKeyState(VK_LBUTTON))
	{
		XMMATRIX Store = view.ViewMatrix;

		view.ViewMatrix.r[3].m128_f32[0] = 0;
		view.ViewMatrix.r[3].m128_f32[1] = 0;
		view.ViewMatrix.r[3].m128_f32[2] = 0;


		XMMATRIX XROT = -XMMatrixRotationX((newPos.y - point.y)  * 0.01f);
		XMMATRIX YROT = -XMMatrixRotationY((newPos.x - point.x)  * 0.01f);



		view.ViewMatrix = XMMatrixMultiply(view.ViewMatrix, YROT);
		view.ViewMatrix = XMMatrixMultiply(XROT, view.ViewMatrix);


		view.ViewMatrix.r[3].m128_f32[0] = Store.r[3].m128_f32[0];
		view.ViewMatrix.r[3].m128_f32[1] = Store.r[3].m128_f32[1];
		view.ViewMatrix.r[3].m128_f32[2] = Store.r[3].m128_f32[2];
	}

	view.ViewMatrix = XMMatrixInverse(0, view.ViewMatrix);

	point = newPos;
#pragma endregion

	deviceContext->ClearDepthStencilView(stencilView, D3D11_CLEAR_DEPTH, 1, 0);

#pragma region Mapping
	D3D11_MAPPED_SUBRESOURCE viewMap;
	D3D11_MAPPED_SUBRESOURCE groundMap;
	D3D11_MAPPED_SUBRESOURCE debugMap;
	UINT stride = sizeof(GVERTEX);
	UINT stride2 = sizeof(debugVert);
	UINT offset = 0;

	deviceContext->Map(viewConstant, 0, D3D11_MAP_WRITE_DISCARD, NULL, &viewMap);
	memcpy_s(viewMap.pData, sizeof(View), &view, sizeof(View));
	deviceContext->Unmap(viewConstant, 0);

	deviceContext->Map(groundConstant, 0, D3D11_MAP_WRITE_DISCARD, NULL, &groundMap);
	memcpy_s(groundMap.pData, sizeof(World), &groundWorld, sizeof(World));
	deviceContext->Unmap(groundConstant, 0);

	for (size_t i = 0; i < teddyJoints.size(); i++)
	{
		D3D11_MAPPED_SUBRESOURCE boneMap;
		deviceContext->Map(boneConstant[i], 0, D3D11_MAP_WRITE_DISCARD, NULL, &boneMap);
		memcpy_s(boneMap.pData, sizeof(World), &boneWorld[i], sizeof(World));
		deviceContext->Unmap(boneConstant[i], 0);
	}
#pragma endregion

#pragma region Device Context Setup
	deviceContext->OMSetRenderTargets(1, &RTV, stencilView);
	deviceContext->OMSetDepthStencilState(stencilState, 1);

	deviceContext->RSSetViewports(1, &viewport);

	float rgba[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	deviceContext->ClearRenderTargetView(RTV, rgba);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//deviceContext->DrawIndexed(36, 0, 0);

	deviceContext->ClearDepthStencilView(stencilView, D3D11_CLEAR_DEPTH, 1, 0);
#pragma endregion

#pragma region Ground Setup and Drawing
	deviceContext->VSSetConstantBuffers(0, 1, &groundConstant);
	deviceContext->VSSetConstantBuffers(1, 1, &viewConstant);
	deviceContext->IASetVertexBuffers(0, 1, &groundBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(groundIndex, DXGI_FORMAT_R32_UINT, 0);

	deviceContext->IASetInputLayout(groundInputlayout);
	deviceContext->VSSetShader(groundVertshader, NULL, 0);
	deviceContext->PSSetShader(groundPixshader, NULL, 0);
	deviceContext->PSSetShaderResources(0, 1, &groundshaderView);
	deviceContext->PSSetSamplers(0, 1, &groundSample);

	deviceContext->RSSetState(solidState);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->DrawIndexed(12, 0, 0);
#pragma endregion

#pragma region Teddy Setup and Drawing
	deviceContext->IASetVertexBuffers(0, 1, &teddyBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(teddyIndex, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->PSSetShaderResources(0, 1, &teddyshaderView);
	deviceContext->PSSetSamplers(0, 1, &teddySample);

	deviceContext->RSSetState(wireState);

	deviceContext->DrawIndexed(teddyVertCount, 0, 0);
#pragma endregion

	deviceContext->RSSetState(solidState);

	for (size_t i = 0; i < teddyJoints.size(); i++)
	{
		deviceContext->VSSetConstantBuffers(0, 1, &boneConstant[i]);
		deviceContext->VSSetConstantBuffers(1, 1, &viewConstant);
		deviceContext->IASetVertexBuffers(0, 1, &boneVertex, &stride, &offset);
		deviceContext->IASetIndexBuffer(boneIndex, DXGI_FORMAT_R32_UINT, 0);

		deviceContext->IASetInputLayout(groundInputlayout);
		deviceContext->VSSetShader(groundVertshader, NULL, 0);
		deviceContext->PSSetShader(groundPixshader, NULL, 0);
		deviceContext->PSSetShaderResources(0, 1, &boneshaderView);
		deviceContext->PSSetSamplers(0, 1, &boneSample);

		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		deviceContext->DrawIndexed(boneVertcount, 0, 0);
	}

	float n[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
	for (int i = 0; i < teddyDebugPos.size(); i++)
	{
		if (i != teddyDebugPos.size() - 1)
		{
			add_debug_line(teddyDebugPos[i], teddyDebugPos[i + 1], n);
		}
	}

	//add_debug_line(teddyDebugPos[0], teddyDebugPos[1], n);
	//add_debug_line(teddyDebugPos[0], teddyDebugPos[1], n);
	//add_debug_line(teddyDebugPos[1], teddyDebugPos[2], n);
	//add_debug_line(teddyDebugPos[1], teddyDebugPos[2], n);

	deviceContext->Map(debugBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &debugMap);
	memcpy_s(debugMap.pData, sizeof(debugVert) * count, &debugArray[0], sizeof(debugArray[0]));
	deviceContext->Unmap(debugBuffer, 0);
	count = 0;

	deviceContext->IASetVertexBuffers(0, 1, &debugBuffer, &stride2, &offset);
	deviceContext->IASetInputLayout(debugInputlayout);
	deviceContext->VSSetShader(debugVertshader, NULL, 0);
	deviceContext->PSSetShader(debugPixshader, NULL, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	deviceContext->Draw(debugArray[0].size(), 0);
	
	swapChain->Present(0, 0);

	return true;
}

bool WIN_APP::ShutDown()
{
#ifndef NDEBUG
	FreeConsole();
#endif

	UnregisterClass(L"RTA Project", application);
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
	srand(unsigned int(time(0)));
	WIN_APP myApp(hInstance, (WNDPROC)WndProc);

#ifndef NDEBUG
	AllocConsole();
	FILE* new_std_in_out;
	freopen_s(&new_std_in_out, "CONOUT$", "w", stdout);
	freopen_s(&new_std_in_out, "CONIN$", "r", stdin);
	std::cout << "Hello world!\n";
#endif


	MSG msg; ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT && myApp.Run())
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	myApp.ShutDown();
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (GetAsyncKeyState(VK_ESCAPE))
		message = WM_DESTROY;
	switch (message)
	{
	case (WM_DESTROY): { PostQuitMessage(0); }
					   break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}