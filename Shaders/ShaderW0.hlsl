// ShaderW0.hlsl

/**
 * constant buffer 정의
 * register(b#)를 사용하여 버퍼 레지스터 슬롯을 지정할 수 있다.
 */
cbuffer constants : register(b0)
{
    float3 Offset;
    float3 Scale;
}

cbuffer FMatrix : register(b1)
{
    matrix World;
    matrix View;
    matrix Proj;
};

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
    float depth : DEPTH;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // Pass the position directly to the pixel shader (no transformation)
    // output.position = input.position;

    // 상수버퍼를 통해 넘겨 받은 Scale을 곱하고 Offset을 더해서 픽셀쉐이더로 넘김

    //월드 뷰 프로젝션 행렬을 곱해준다 TranslationMatrix 
    output.position = mul(input.position, World);
    output.position = mul(output.position, View);
    output.position = mul(output.position, Proj);

    output.depth = output.position.z / output.position.w;
    //output.position = float4(input.position.xyz * Scale + Offset, input.position.w);
    // Pass the color to the pixel shader
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
