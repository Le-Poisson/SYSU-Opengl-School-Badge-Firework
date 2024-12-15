#version 460
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 texOffset[24]; // ʹ�� 24 ��ƫ���� (4*5 - 1)

void main()
{
    vec3 result = texture(screenTexture, TexCoords).rgb * 0.427027; // ��������Ȩ��

    // 5x5 ���Ĳ���ƫ��
    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216); // Ȩ��
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float weight = weights[abs(x)] * weights[abs(y)];
            result += texture(screenTexture, TexCoords + vec2(x, y) * texOffset[0]).rgb * weight;
        }
    }

    FragColor = vec4(result, 1.0);
}