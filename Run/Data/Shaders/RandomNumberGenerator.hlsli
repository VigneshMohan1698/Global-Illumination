
uint SeedThread(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}


float hash(float3 p)
{
	p = frac(p * 0.3183099 + .1);
	p *= 17.0;
	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

uint Random(inout uint state)
{
	state ^= (state << 13);
	state ^= (state >> 17);
	state ^= (state << 5);
	return state;
}
float Random01(inout uint state)
{
	return asfloat(0x3f800000 | Random(state) >> 9) - 1.0;
}
uint RandomUpperLower(inout uint state, uint lower, uint upper)
{
	return lower + uint(float(upper - lower + 1) * Random01(state));
}

//float hash1(inout float seed)
//{
//	return frac(sin(seed += 0.1) * 43758.5453123);
//}

//float2 hash2(inout float seed)
//{
//	return frac(sin(float2(seed, seed)) * float2(43758.5453123, 22578.1459123));
//}

//float3 hash3(inout float seed)
//{
//	return frac(sin(float3(seed, seed, seed)) * float3(43758.5453123, 22578.1459123, 19642.3490423));
//}

//uint hash(uint x)
//{
//	x += (x << 10u);
//	x ^= (x >> 6u);
//	x += (x << 3u);
//	x ^= (x >> 11u);
//	x += (x << 15u);
//	return x;
//}

float nextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

//uint hashUint(float2 v)
//{
//	return hash(asuint(v.x) ^ asuint(hash(v.y)));
//}
//uint hash(float3 v)
//{
//	return hash(asuint(v.x) ^ asuint(hash(v.y)) ^ asuint(hash(v.z)));
//}
//uint hash(float4 v)
//{
//	return hash(asuint(v.x) ^ asuint(hash(v.y)) ^ asuint(hash(v.z)) ^ asuint(hash(v.w)));
//}

//float floatConstruct(uint m)
//{
//	const uint ieeeMantissa = 0x007FFFFFu; 
//	const uint ieeeOne = 0x3F800000u; 

//	m &= ieeeMantissa; 
//	m |= ieeeOne;

//	float f = asfloat(m); 
//	return f - 1.0; 
//}



//// Pseudo-random value in half-open range [0:1].
//float random(float x)
//{
//	return floatConstruct(hash(asuint(x)));
//}
//float random(float2 v)
//{
//	return floatConstruct(hash(asuint(v)));
//}
//float random(float3 v)
//{
//	return floatConstruct(hash(asuint(v)));
//}
//float random(float4 v)
//{
//	return floatConstruct(hash(asuint(v)));
//}
