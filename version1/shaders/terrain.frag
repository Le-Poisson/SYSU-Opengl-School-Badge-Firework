#version 420 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 WorldPos; // 片段在世界空间的位置
uniform vec3 viewPos; // 视点位置
uniform vec3 lightPos; // 光源位置
uniform vec3 lightColor; // 光源颜色

layout (binding = 0) uniform sampler2D normalTexture;

float FogFactor(float d) {
    const float FogMax = 750.0;
    if (d >= FogMax) return 1.0;
    return 1.0 - (FogMax - d) / (FogMax);
}

void main() {
    // 法线处理
    vec3 normalMap = texture(normalTexture, TexCoord).rgb;
    normalMap = vec3(normalMap.x, normalMap.z, normalMap.y); // 转换法线
    vec3 normal = normalize(normalMap * 2.0 - 1.0); // 从[0,1]转换到[-1,1]

    // 环境光
    vec3 terrainColor = vec3(0.31, 0.20, 0.08); // 地形颜色
    float ambientStrength = 0.05;
    vec3 ambient = ambientStrength * terrainColor;

    // 光照计算
    vec3 lightDir = normalize(lightPos - WorldPos); // 光源方向
    vec3 viewDir = normalize(viewPos - WorldPos); // 视角方向
    vec3 halfDir = normalize(lightDir + viewDir); // 半程向量

    // Blinn-Phong 光照模型
    float specularStrength = 0.5; // 高光强度
    float shininess = 32.0; // 粗糙度
    float ndotl = max(dot(normal, lightDir), 0.0); // 漫反射分量
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess); // 高光分量

    // 计算最终光照
    vec3 diffuse = ndotl * terrainColor; // 漫反射
    vec3 specular = specularStrength * spec * lightColor; // 高光
    vec3 lighting = ambient + diffuse + specular; // 总光照

    // 雾效
    float d = distance(viewPos, WorldPos);
    float alpha = FogFactor(d);
    vec3 FogColor = vec3(0.09, 0.11, 0.09); // 雾色

    // 最终颜色
    FragColor.rgb = mix(lighting, FogColor, alpha);
    FragColor.a = 1.0;
}