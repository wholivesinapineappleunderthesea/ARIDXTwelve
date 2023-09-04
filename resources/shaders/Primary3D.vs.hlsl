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


struct VS_INPUT
{

    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};
            
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.position = float4(input.position.xyz, 1.0);
    output.position = mul(output.position, modelMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    output.texcoord = input.texcoord * uvScale + uvOffset;
    output.normal = float4(input.normal.xyz, 1.0);
    output.color = input.color;
    return output;
}