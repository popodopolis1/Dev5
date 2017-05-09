struct INPUT
{
	float4 projCoord : SV_POSITION;
	float4 color : COLOR;
};


float4 main(INPUT input) : SV_TARGET
{
	return input.color;
}