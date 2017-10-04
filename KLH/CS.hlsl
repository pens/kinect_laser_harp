cbuffer cbuff : register(b0) {
    float4 slopes[2];
};
Texture2D<float2> depth : register(t0);
Texture2D<float4> albedo : register(t1);
RWTexture2D<float> depthOut : register(u0);
RWTexture2D<float4> albedoOut : register(u1);
RWStructuredBuffer<int> hit : register(u2);

void drawBeam(uint3 id, int num, float slope, int offset, float depthOffset);

static float farDepth = .016f;
static float nearDepth = .014f;
static float depthInc = .002f;
static uint2 emit = uint2(640, 900);
static uint beam = 5;

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    id.x = (1280 - id.x);
    id.y = (960 - id.y);
    depthOut[id.xy] = depth[id.xy].g;
    albedoOut[id.xy] = albedo[id.xy];
    drawBeam(id, 6, slopes[0].x, 150, .004f);
    drawBeam(id, 5, slopes[0].y, 100, .002f);
    drawBeam(id, 4, slopes[0].z, 50, .001f);
    drawBeam(id, 3, slopes[0].w, 0, .001f);
    drawBeam(id, 2, slopes[1].x, -50, .001f);
    drawBeam(id, 1, slopes[1].y, -100, .002f);
    drawBeam(id, 0, slopes[1].z, -150, .004f);
}

void drawBeam(uint3 id, int num, float slope, int offset, float depthOffset) {
    if (id.y <= emit.y && (hit[num] == 0 || id.y > hit[num])) {
        float y = float(id.y) / 960;
        float fd = farDepth + depthInc * y + depthOffset;
        float nd = nearDepth + depthInc * y + depthOffset;
        float xdiff;
        if (slope == 0) {
            xdiff = abs(int(id.x) - int(emit.x));
        } else {
            int x2 = int(id.x) - int(emit.x + offset);
            int y2 = int(id.y) - int(emit.y);
            xdiff = abs(y2 / slope - x2);
        }
        if (xdiff < beam) {
            if (depthOut[id.xy / 2] > nd && depthOut[id.xy / 2] < fd) {
                hit[num] = id.y;
                albedoOut[id.xy] = float4(0, 1, 0, 1);
            } else {
                albedoOut[id.xy] = albedo[id.xy] + 
                    float4(0, 1 - sqrt(xdiff / float(beam)), 0, 1);
            }
        }
    }
}