Texture2D<float> depth : register(t0);
Texture2D albedo : register(t1);
SamplerState sam : register(s0);

float4 main(float4 pos : SV_POSITION,
            float2 tex : TEX) : SV_TARGET {
    //return depth.Sample(sam, tex) * 10;
    return albedo.Sample(sam, tex);
}