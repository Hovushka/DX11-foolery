// #pragma pack_matrix(row_major)

cbuffer projBuffer : register(b0) {
	matrix pers;
	matrix ortho;
};

cbuffer cubeModelBuffer : register(b1) {
	matrix model;
};

Texture2D woodTexView : register(t0);
Texture2D heartTexView : register(t1);
SamplerState texSampler : register(s0);

struct CVSInput {
	float3 pos : POSITION0;
	float2 tex : TEXCOORD0;
};

struct VSInput {
	float2 pos : POSITION0;
	float2 tex : TEXCOORD0;
	float3x3 model : MODEL0; // Sprite-specific
};

struct PSInput {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

// Cube vertex shader entry point
PSInput cubeVS(CVSInput vert) {
	float2 UV = vert.tex;
	UV.y = 1.0 - UV.y;

	PSInput ret = {
		mul(mul(float4(vert.pos.xyz, 1.0), model), pers),
		UV
	};

	return ret;
}

// Font vertex shader entry point
PSInput fontVS(VSInput vert) {
	PSInput ret = {
		mul(float4(vert.pos.xy, 1.0, 1.0), ortho),
		vert.tex
	};

	return ret;
}

// Sprite vertex shader entry point
PSInput spriteVS(VSInput vert) {
	float2 UV = vert.tex;
	UV.y = 1.0 - UV.y;

	PSInput ret = {
		mul(float4(mul(float3(vert.pos.xy, 1.0), vert.model), 1.0), ortho),
		// mul(float4(vert.pos, 1.0, 1.0), proj),
		UV
	};

	return ret;
}

// Default pixel shader entry point
float4 spritePS(PSInput frag) : SV_TARGET {
	return woodTexView.Sample(texSampler, frag.tex);
}

// Combined pixel shader entrypoint
float4 combiPS(PSInput frag) : SV_TARGET {
	float4 wood = woodTexView.Sample(texSampler, frag.tex);
	float4 heart = heartTexView.Sample(texSampler, frag.tex);

	return lerp(wood, heart * wood, float4(heart.w, heart.w, heart.w, heart.w));
}