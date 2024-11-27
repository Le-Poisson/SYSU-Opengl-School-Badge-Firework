#version 420 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 WorldPos; // Ƭ��������ռ��λ��
uniform vec3 viewPos; // �ӵ�λ��
uniform vec3 lightPos; // ��Դλ��
uniform vec3 lightColor; // ��Դ��ɫ

layout (binding = 0) uniform sampler2D normalTexture;

float FogFactor(float d) {
    const float FogMax = 750.0;
    if (d >= FogMax) return 1.0;
    return 1.0 - (FogMax - d) / (FogMax);
}

void main() {
    // ���ߴ���
    vec3 normalMap = texture(normalTexture, TexCoord).rgb;
    normalMap = vec3(normalMap.x, normalMap.z, normalMap.y); // ת������
    vec3 normal = normalize(normalMap * 2.0 - 1.0); // ��[0,1]ת����[-1,1]

    // ������
    vec3 terrainColor = vec3(0.31, 0.20, 0.08); // ������ɫ
    float ambientStrength = 0.05;
    vec3 ambient = ambientStrength * terrainColor;

    // ���ռ���
    vec3 lightDir = normalize(lightPos - WorldPos); // ��Դ����
    vec3 viewDir = normalize(viewPos - WorldPos); // �ӽǷ���
    vec3 halfDir = normalize(lightDir + viewDir); // �������

    // Blinn-Phong ����ģ��
    float specularStrength = 0.5; // �߹�ǿ��
    float shininess = 32.0; // �ֲڶ�
    float ndotl = max(dot(normal, lightDir), 0.0); // ���������
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess); // �߹����

    // �������չ���
    vec3 diffuse = ndotl * terrainColor; // ������
    vec3 specular = specularStrength * spec * lightColor; // �߹�
    vec3 lighting = ambient + diffuse + specular; // �ܹ���

    // ��Ч
    float d = distance(viewPos, WorldPos);
    float alpha = FogFactor(d);
    vec3 FogColor = vec3(0.09, 0.11, 0.09); // ��ɫ

    // ������ɫ
    FragColor.rgb = mix(lighting, FogColor, alpha);
    FragColor.a = 1.0;
}