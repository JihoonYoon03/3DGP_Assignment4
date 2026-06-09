// 객체별 월드 행렬, 색상, 카메라 위치, 조명 정보
cbuffer ObjectConstants : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInverseTranspose;
    float4x4 gWorldViewProjection;
    float4 gColor;
    float4 gCameraPosition;
    float4 gLightDirection;
    float4 gAmbientColor;
    float4 gDiffuseColor;
    float4 gSpecularColor;
    float4 gLightingOptions;
};

struct VertexIn
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 worldPosition : WORLDPOSITION;
    float3 normal : NORMAL;
};

// 정점 변환, 조명 계산 좌표와 노멀 넘기기
VertexOut VSMain(VertexIn input)
{
    VertexOut output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), gWorld);
    output.position = mul(float4(input.position, 1.0f), gWorldViewProjection);
    output.color = input.color * gColor;
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul(float4(input.normal, 0.0f), gWorldInverseTranspose).xyz);
    return output;
}

// 앰비언트, 디퓨즈, 스페큘러 계산. 방향광 하나 사용
float4 PSMain(VertexOut input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightToSurface = normalize(gLightDirection.xyz);
    float3 surfaceToLight = -lightToSurface;
    float diffuseFactor = saturate(dot(normal, surfaceToLight));

    float3 viewDirection = normalize(gCameraPosition.xyz - input.worldPosition);
    float3 reflectDirection = reflect(lightToSurface, normal);
    float specularFactor = pow(saturate(dot(viewDirection, reflectDirection)), gLightingOptions.x);

    float3 litColor =
        gAmbientColor.rgb +
        gDiffuseColor.rgb * diffuseFactor +
        gSpecularColor.rgb * specularFactor;
    return float4(input.color.rgb * litColor, input.color.a);
}
