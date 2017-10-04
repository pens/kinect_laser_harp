void main(uint id : SV_VertexID,
          out float4 pos : SV_POSITION,
          out float2 tex : TEX) {
    switch (id) {
    case 0:
    case 5: {
        pos = float4(-1, 1, 0, 1);
        tex = float2(0, 0);
        break;
    }
    case 1:
    case 4: {
        pos = float4(1, -1, 0, 1);
        tex = float2(1, 1);
        break;
    }
    case 2: {
        pos = float4(-1, -1, 0, 1);
        tex = float2(0, 1);
        break;
    }
    default: {
        pos = float4(1, 1, 0, 1);
        tex = float2(1, 0);
        break;
    }
    }
}