#pragma pack_matrix(row_major)

struct IN
{
	float4 posIn : POSITION;
	float4 colorIn : COLOR;
	float2 uvIn : UV;
	float3 normalIn : NORMAL;
};

struct OUT
{
	float4 posOut : SV_POSITION;
	float4 colorOut : COLOR;
	float2 uvOut : UV;
	float3 normalOut : NORMAL;
	float3 worldpos : POSITION;
};

cbuffer WORLD : register(b0)
{
	float4x4 WorldMatrix;
}

cbuffer OBJECT : register(b1)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
}

OUT main(IN input)
{
	OUT output = (OUT)0;
	
	float4 localH = float4(input.posIn.xyz, 1);
		
		localH = mul(localH, WorldMatrix);
	float4 Worldpos = localH;

	localH = mul(localH, ViewMatrix);
	localH = mul(localH, ProjectionMatrix);
	output.posOut = localH;
	output.colorOut = input.colorIn;
	output.uvOut = input.uvIn;
	output.normalOut = mul((float3x3)WorldMatrix, input.normalIn);
	output.normalOut = normalize(output.normalOut);
	output.worldpos = Worldpos.xyz;
	return output;
}