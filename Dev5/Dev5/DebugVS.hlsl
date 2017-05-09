#pragma pack_matrix(row_major)

struct IN
{
	float4 posIn : POSITION;
	float4 colorIn : COLOR;
};

struct OUT
{
	float4 posOut : SV_POSITION;
	float4 colorOut : COLOR;
};

cbuffer OBJECT : register(b1)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
}

OUT main(IN input)
{
	OUT output = (OUT)0;

	float4 localH = float4(input.posIn.xyz, 1);
	//float4 localH = input.posIn;
	localH = mul(localH, ViewMatrix);
	localH = mul(localH, ProjectionMatrix);
	output.posOut = localH;
	output.colorOut = input.colorIn;
	return output;
}