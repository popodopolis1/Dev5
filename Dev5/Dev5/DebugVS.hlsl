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

OUT main(IN input)
{
	OUT output = (OUT)0;

	float4 localH = float4(input.posIn.xyz, 1);
	output.colorOut = input.colorIn;
	return output;
}