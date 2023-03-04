
struct SceneConstantBuffer
{
	row_major float4x4 inversedProjectionMatrix;
	row_major float4x4 projectionMatrix;
	row_major float4x4 inversedViewMatrix;
	row_major float4x4 viewMatrix;
	row_major float4x4 _viewMatrix;
	row_major float4x4 inversedViewMatrixOrigin;
	float4 cameraPosition;
	float4 _cameraPosition;
	float4 lightPosition;
	float4 GIColor;
	float4 samplingData;  // x reflections y samples z colorbleeding w frameNumber
	float4 lightBools; // x == Shadows  on? y == Global illumination light on? z - Sky Light on? ,w - direct light on?
	float4 textureMappings; // 0 - Normal maps 1 - Specular maps
	float4 lightfallOff_AmbientIntensity_CosineSampling_DayNight; 
};

struct PostProcessData
{
	float2 textureDim;
	float2 invTextureDim;
	float4 lightPosition;
	float4 GIColor;
	//float4 cameraPosition;
	row_major float4x4 viewMatrix;
	row_major float4x4 projectionMatrix;
	//row_major float4x4 inversedViewMatrix;
	//row_major float4x4 inversedProjectionMatrix;
};

struct RaytracedPointLights
{
	float4 PointLightPosition[20];
	int Counter;
	int MaxLights;
};

struct DenoiserData
{
	float2 textureDim;
	float2 invTextureDim;
	float kernelSize;
	int atrousStepSize;
	float2 temporalFadeVarianceEstimation;
};

struct CompositorData
{
	uint2 textureDim;
	float2 invTextureDim;
	bool denoiserOn;
	float renderOutput;
};

struct RayPayload
{
	float tHit;
	float3 startDirection;
	float4 color;
	float reflectionIndex;
	float raytype; // 0 is Camera, 1 is radiance , 2 is shadow , 3 is water ray 
	float3 lastHitPosition;
	float3 lastHitNormal;
	bool didHitGeometry;
	bool didHitEmissiveSurface;
	bool didHitSky;
};

struct GIRayPayload
{
	float4 GlobalIllumination;
	float4 IndirectAlbedo;
	float ReflectionIndex;
	float Raytype;  // 0 camera, 1 Radiance, 2 Shadow  3 water 4 reflectionsample 5 Point Light
	float tHit;
	bool DidHitEmissiveSurface;
};
struct HitData
{
	float2 traingleUV;
	float3 triangleNormal;
	float3 hitPosition;
	float4 diffuseAlbedo;
	float4 colors;
	void Initialize()
	{
		traingleUV = float2(0, 0);
		triangleNormal = float3(0, 0,0);
		hitPosition = float3(0, 0,0);
		diffuseAlbedo = float4(0, 0, 0,0);
		colors = float4(0, 0, 0, 0);
	};
};
struct AlignedHemishpere
{
	float3 value;
	uint padding; // Padding to 16B
};

struct IrradianceCache
{
	float3 vertexPosition;
	float3 vertexNormal;
	float4 GI;
	void Initialize()
	{
		vertexPosition = float3(0, 0,0);
		vertexNormal = float3(0, 0, 0);
		GI = float4(0, 0, 0,0);
	};
};
struct Rgba8
{
	uint r;
	uint g;
	uint b;
	uint a;
};
typedef uint Index;
struct Vertex_PNCUTB
{
	float3 m_position;
	float3 m_normal;
	float4 m_color;
	float3 m_tangent;
	float3 m_bitangent;
	float2 m_uvTexCoords;
};
