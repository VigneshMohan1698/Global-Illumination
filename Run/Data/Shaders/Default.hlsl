

float1 RangeMap(float1 inputValue,float1 inputStart, float1 inputEnd, float1 outputStart, float1 outputEnd )
{
	float1 range = inputEnd - inputStart;
	float1 fraction =  (inputValue - inputStart) / range;
	float1 returnValue = (outputStart + ((outputEnd - outputStart) * fraction));
	return returnValue;
}

cbuffer CameraBuffer : register(b2)
{
	float OrthoMinX;
	float OrthoMinY;
	float OrthoMinZ;
	float OrthoMaxX;
	float OrthoMaxY;
	float OrthoMaxZ;
	float padding1;
	float padding2;
};

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

struct vs_input_t
{
float3 localPosition : POSITION;
float4 color : COLOR;
float2 uv : TEXCOORD;
};

struct v2p_t
{
float4 position : SV_Position;
float4 color : COLOR;
float2 uv : TEXCOORD;
};

v2p_t VertexMain(vs_input_t input)
{
v2p_t v2p;
float1 x = RangeMap(input.localPosition.x, OrthoMinX, OrthoMaxX, -1, 1);
float1 y= RangeMap(input.localPosition.y, OrthoMinY, OrthoMaxY, -1, 1);
float1 z = 0;
v2p.position = float4(x, y, z,1);
v2p.color = input.color;
v2p.uv = input.uv;
return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 color = texture.sample(sampler,input.uv);
	return color * input.color;
}



