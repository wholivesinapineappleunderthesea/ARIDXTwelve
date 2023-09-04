cbuffer CameraBuffer : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float2 viewportSize;
    float time;
};

cbuffer ModelBuffer : register(b1)
{
    float4x4 modelMatrix;
    float2 uvScale;
    float2 uvOffset;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};


float4 main(PS_INPUT input) : SV_Target
{
    const float c = 30.0;
    int4 col = int4(input.color.r * c, input.color.g * c, input.color.b * c, input.color.a);
    return float4(col.r / c, col.g / c, col.b / c, col.a);
}