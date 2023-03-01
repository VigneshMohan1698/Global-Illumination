//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 localPosition : POSITION;
	float3 localNormal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD;

};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 PointLightPOSITION : POINTLIGHT;
};

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b4)
{
	float3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
	float3 padding;
};

cbuffer PointLights : register(b5)
{
	float3 PointLightPosition;
	float4 PointLightColor;
	float PointLightIntensity;
	float PointLightRadius;
	float3 PaddingLight;
};
//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);
	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);
	float4 localNormal = float4(input.localNormal, 0);
	float4 worldNormal = mul(ModelMatrix, localNormal);
	
	v2p_t v2p;
	v2p.position = clipPosition;
	v2p.normal = worldNormal.xyz;
	v2p.color = input.color * ModelColor;
	v2p.uv = input.uv;
	v2p.PointLightPOSITION.xyz = PointLightPosition.xyz - worldPosition.xyz;
	//v2p.PointLightPOSITION = normalize(v2p.PointLightPOSITION);
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 pointLColor,pointLIntensity;
	float lightDistance, lightRadius;
	lightRadius = PointLightRadius;
	pointLIntensity = 0;
	lightDistance = length(input.PointLightPOSITION);
	if (lightDistance < 3)
	{
		input.PointLightPOSITION = normalize(input.PointLightPOSITION);
		pointLIntensity = saturate(dot(normalize(input.normal), input.PointLightPOSITION));
	}
	pointLColor = PointLightColor * pointLIntensity;
	float ambient = AmbientIntensity;
	float directional = SunIntensity * saturate(dot(normalize(input.normal), -SunDirection));
	float4 lightColor = float4((ambient + directional + pointLIntensity).xxx, 1);
	float4 diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv);
	float4 color = lightColor * diffuseColor * input.color;
	//color = saturate(pointLColor) * color;
	clip(color.a - 0.5f);
	return float4(color);
}
