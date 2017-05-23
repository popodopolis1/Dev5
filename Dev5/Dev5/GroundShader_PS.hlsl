struct INPUT
{
	float4 projCoord : SV_POSITION;
	float4 color : COLOR;
	float2 uv : UV;
	float3 normal : NORMAL;
	float3 worldpos : POSITION;
};

cbuffer Light : register(b0)
{
	float3 direction;
	float pointRadius;
	float4 color;
	float4 pointPosition;
	float4 pointColor;
	float4 spotColor;
	float4 spotPosition;
	float4 spotDirection;
	float spotRadius;
	float3 padding;
	float4 camPosition;
};

Texture2D tex;
SamplerState filter;

float4 main(INPUT input) : SV_TARGET
{
	float4 texturecolor = tex.Sample(filter, input.uv);

	float directionRatio = saturate(dot(-direction, input.normal));
	float4 directionResult = directionRatio * color;
	
	float3 pointDirection = normalize(pointPosition.xyz - input.worldpos.xyz);
	float pointRatio = saturate(dot(pointDirection, input.normal));
	float4 pointAttenuation = 1 - saturate(length(pointPosition.xyz - input.worldpos.xyz) / pointRadius);
		float4 pointResult = pointAttenuation * pointRatio * pointColor;
	
		float3 spotDir = normalize(spotPosition.xyz - input.worldpos.xyz);
		float spotRatio = saturate(dot(-spotDir.xyz, spotDirection.xyz));
	
	float spotLightratio = saturate(dot(spotDir, input.normal));
	float outerCone = cos(30.0f * (3.1415f / 180.0f));
	float innerCone = cos(20.0f * (3.1415f / 180.0f));
	float4 spotAttenuation = 1 - saturate((innerCone - spotRatio) / (innerCone - outerCone));
	
		float4 spotResult = spotAttenuation * spotLightratio * spotColor;
	
		float3 pointspecDir = normalize(camPosition.xyz - input.worldpos.xyz);
	
		float3 pointhalfVector = normalize((pointDirection)+pointspecDir);
		float pointintensity = pow(saturate(dot(input.normal, normalize(pointhalfVector))), 32.0f);
	float4 pointspecResult = pointColor * pointAttenuation * pointintensity;
	
	float3 halfVector = normalize((-direction) + pointspecDir);
	float intensity = pow(saturate(dot(input.normal, normalize(halfVector))), 32.0f);
	float4 specResult = color * intensity;

	return texturecolor * saturate(directionResult + spotResult + pointResult + pointspecResult + specResult);
	//return texturecolor;
}