#import "template.typ": template
#import "@preview/cetz:0.3.1"
#import "@preview/tablex:0.0.9": tablex, rowspanx, colspanx, cellx

#show: doc => template(
  [烟花粒子系统说明文档],
  doc
)

//************************************************************//

= 项目说明

本项目的目的是对烟花粒子系统的构建。以下是本项目已完成或未完成的目标任务：

#figure(
  tablex(
    columns: 3,
    align: center + horizon,
    auto-vlines: false,
    //header-rows: 2,

    [目标任务], [], [完成情况],
    [粒子系统的构建], [], [#emoji.checkmark.box],
    [烟花系统的构建], [], [#emoji.checkmark.box],
    [天空盒], [], [#emoji.checkmark.box],
    [地面], [], [#emoji.checkmark.box],
    [烟花的点光源创建与更新], [], [#emoji.checkmark.box],
    [地面的Blinn-Phong光照], [], [#emoji.checkmark.box],
    [烟花爆炸的音效], [], [#emoji.checkmark.box],
    [中山大学校徽形状的烟花], [], [#emoji.checkmark.box],
    [烟花爆炸的辉光特效], [], [#emoji.checkmark.box],
  ),
  kind: table
)

项目已上传至#link("https://github.com/Le-Poisson/version1")[#text(blue, "GitHub")]。

//************************************************************//

= 实现过程

本项目中，重要的类(或着色器)及其功能如下所示：

#figure(
  tablex(
    columns: 3,
    align: center + horizon,
    auto-vlines: false,
    //header-rows: 2,

    [类名/着色器], [], [功能],
    [```CPP class Camera```], [], [摄像机的相关操作],
    [```CPP class Shader```], [], [着色器的集中管理],
    [```CPP class Laucher```], [], [烟花的创建与更新（包含烟花粒子结构体的定义）],
    [```CPP class PointLigth```], [], [点光源的相关操作],
    [```CPP terrain.frag```], [], [地面的片段着色器],
    [```CPP gaussian_blur.frag```], [], [高斯模糊的片段着色器],
  ),
  kind: table
)


== 普通烟花的实现

=== 烟花粒子的构建 @LearnOpenGLParticles

首先构建粒子的结构体：

```CPP
struct Particle {
	enum Type { LAUNCHING, SPARKLE, TRAIL, FOUNTAIN, DEAD };

	glm::vec3 pos, speed;
	unsigned char r, g, b, a;
	float size, life, trailTime, cameraDst;
	Type type;

	bool operator<(const Particle& right) const {
		return this->cameraDst > right.cameraDst;
	}
};
```

粒子需要和烟花类型绑定，所以使用Type枚举来区分。这些类型分别指示了粒子的性质：

#figure(
  tablex(
    columns: 3,
    align: center + horizon,
    auto-vlines: false,
    //header-rows: 2,

    [枚举类型], [], [解释],
    [```CPP LAUNCHING```], [], [会爆炸的烟花所持有的粒子],
    [```CPP SPARKLE```], [], [烟花爆炸后产生的火花粒子],
    [```CPP TRAIL```], [], [烟花上升过程的拖尾粒子],
    [```CPP FOUNTAIN```], [], [喷泉型烟花持有的粒子],
    [```CPP DEAD```], [], [未使用或已死亡的粒子],
  ),
  kind: table
)

而粒子应具有的基本属性有：位置、速度、大小、生命，以及上述所提的类型。当然，颜色也很重要，这里使用的直接是分立的`r, g, b, a`值，而不是封装到`glm::vec4`中。

`cameraDst`用于优化渲染。在渲染前会按照这个属性对所有粒子排序，越近的粒子越早渲染。

=== 烟花类```CPP class Laucher```的相关逻辑

接下来直接看烟花类```CPP class Laucher```中的成员函数：

```CPP
class Launcher
{
public:
	Launcher();
	Launcher(glm::vec3 position);
	Launcher(std::shared_ptr<Shader> shader);
	Launcher(glm::vec3 position, std::shared_ptr<Shader> shader);
	//~Launcher();

	void renderTrails(Particle& p, float deltaTime);
	void spawnParticle(glm::vec3 position, glm::vec3 speed, glm::vec4 color, float size, float life, Particle::Type type);
	void explode(Particle& p);
	void launchFirework();
	void launchFountain();

	void simulate(Camera &camera, GLfloat* particle_position, GLubyte* particle_color);
	void update(Camera &camera, GLfloat* particle_position, GLubyte* particle_color);

	void sortParticles();
	int findUnusedParticle();
}
```

这其中最重要的函数是```CPP update(...)```和```CPP simulate(...)```。首先说明这些函数的调用关系：

$ "update"->cases(
  "launchFirework"->"spawnParticle", 
  "launchFountain"->"spawnParticle",
  "simulate"-> cases(
    "renderTrails"->"spawnParticle",
    "explode"->"spawnParticle"
  ),
  "sortParticles"
) $

其中```CPP spawnParticle(...)```中会调用```CPP findUnusedParticle(...)```，为避免繁琐就不在上面关系图中标出。

```CPP update(...)```应在每一帧渲染时由外部创建者(即主函数)调用，用于更新类中持有的两种类型烟花发射器，所有粒子(和点光源，这会在 @PointLight 中说明)。

更新所有粒子的功能由```CPP simulate(...)```实现。例如，正在上升的粒子受重力影响，速度会减小；某些粒子生命耗尽死亡在空中爆炸。而爆炸效果由```CPP explode(...)```实现。在这个函数中，会在爆炸中心创建点光源(在 @PointLight 中说明)，并生成爆炸产生的粒子(即由大部分函数都会调用的```CPP spawnParticle(...)```实现)。
粒子死亡后，手动将`type`设为`DEAD`。

=== 烟花爆炸效果的实现

烟花的爆炸本质上是一个粒子的消失和更多新的粒子的出现：

```cpp
// 处理爆炸
void Launcher::explode2(Particle& p)
{
	int randomSound = getRandomNumber(1, 6); // 随机选择爆炸声音
	soundEngine->play2D(explosionSounds[randomSound - 1]); // 播放爆炸声音

	// 添加点光源 Add: YuZhuZhi
	auto pointLight = std::make_shared<PointLight>(
		Color(p.r, p.g, p.b), // 光的颜色
		p.pos,                   // 光的位置
		Attenuation(1.0f, 0.014f, 0.007f), // 光的衰减参数
		sparkleLife + 0.75
	);
	auto it = std::find(pointLights.begin(), pointLights.end(), nullptr);
	if (it != pointLights.end()) { // 添加到集中管理
		*it = pointLight;
		pointLight->addToShader(shader, it - pointLights.begin());
	}

	float randSize = getRandomNumber(0, explosionSpread); // 随机扩散范围
	for (int i = 0; i < sparklesPerExplosion; i++)
	{
		// 随机生成火花位置和速度
		float randX = getRandomNumber(-1, 2);
		float randY = getRandomNumber(-1, 2);
		float randZ = getRandomNumber(-1, 2);
		float randSpread = getRandomNumber(0, explosionSpread);
		float randLife = getRandomNumber(0, 1.5f);

		// 生成火花粒子
		spawnParticle(
			p.pos,
			glm::normalize(glm::vec3(randX, randY, randZ)) * (explosionSize - randSpread + randSize),
			glm::vec4(p.r, p.g, p.b, p.a),
			sparkleSize,
			sparkleLife + randLife,
			Particle::Type::SPARKLE
		);

	}
}
```

在这个函数中，我们首先调用音频库产生爆炸音效，然后在爆炸所在的地方放置一个点光源（程序优化的需要，如果将每个粒子作为点光源，整个屏幕中包含几百个粒子，对于每个粒子都需要通过光线追踪计算地面的颜色，程序将无法流畅运行）。接下来产生 `sparklesPerExplosion` 个粒子，为了模拟真实的烟花爆炸效果，爆炸产生的粒子具有随机的初始速度向量。

== 烟花发射和爆炸音频的导入

我们在网上找到了 13 个烟花发射和 5 个烟花爆炸的音频：

```cpp
static const char* launchSounds[13] = {
	"version1/sounds/launch1.wav",
	"version1/sounds/launch2.wav",
	"version1/sounds/launch3.wav",
	"version1/sounds/launch4.wav",
	"version1/sounds/launch5.wav",
	"version1/sounds/launch6.wav",
	"version1/sounds/launch7.wav",
	"version1/sounds/launch8.wav",
	"version1/sounds/launch9.wav",
	"version1/sounds/launch10.wav",
	"version1/sounds/launch11.wav",
	"version1/sounds/launch12.wav",
	"version1/sounds/launch13.wav"
};
static const char* explosionSounds[5] = { 
	"version1/sounds/explosion1.wav", 
	"version1/sounds/explosion2.wav", 
	"version1/sounds/explosion3.wav", 
	"version1/sounds/explosion4.wav", 
	"version1/sounds/explosion5.wav" 
};
```

在 `launcher.cpp` 中，当烟花发射时，在 10 个音频中随机选择一个播放（为了防止烟花发射的声音掩盖掉爆炸的声音，我们将发射的音量调小了）：

```cpp
// 发射烟花
void Launcher::launchFirework()
{
	int randomSound = getRandomNumber(1, 13); // 随机选择爆炸声音
	float volume = 0.1f; // 设置音量大小，范围是 0.0 到 1.0

	// 播放声音时设置音量
	ISound* sound = soundEngine->play2D(launchSounds[randomSound - 1], false, false, true);
	if (sound) {
		sound->setVolume(volume); // 调整音量
	}
   //...处理烟花发射
}
```

同理，当烟花爆炸时，在导入的 5 个音频中随机选择一个播放。

```cpp
// 创建声音引擎
ISoundEngine* soundEngine = createIrrKlangDevice();

void Launcher::explode2(Particle& p)
{
	int randomSound = getRandomNumber(1, 6); // 随机选择爆炸声音
	soundEngine->play2D(explosionSounds[randomSound - 1]); // 播放爆炸声音
   //...处理爆炸
}
```

== 地面和天空盒的实现


=== 地面的实现

地面主要由*法线贴图*和*高度贴图*构成，其中，法线贴图主要用于模拟细节和影响光照，高度贴图用于定义物体表面的高度变化，两者结合使用可以显著提高渲染效果和真实感。

我们使用的法线贴图如下：

#figure(
  image("image/NormalMap.png"),
  caption: [地面法线贴图]
)

高度贴图如下：

#figure(
  image("image/HeightMap.png"),
  caption: [地面法线贴图]
)

接下来是地形的渲染：

```cpp
// 渲染高度图
Transform heightMapTransform;
heightMapTransform.scale = glm::vec3(2.0f); // 调整高度图大小
_terrainShader.use();
_terrainShader.SetMat4("projection", projection);
_terrainShader.SetMat4("view", view);
_terrainShader.SetMat4("model", heightMapTransform.to_mat4());
_terrainShader.SetVec3("viewPos", glm::vec3(camera->to_mat4()[3])); // 设置视点位置
_terrainShader.SetVec3("lightPos", lightPos); // 光源位置
_terrainShader.SetVec3("lightColor", lightColor); // 光源颜色
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, _normalMapTexture.ID); // 绑定法线贴图
glBindVertexArray(_heightMap.vao);
glDrawElements(GL_TRIANGLE_STRIP, _heightMap.indexCount, GL_UNSIGNED_INT, 0); // 绘制高度图
glBindVertexArray(0);
glClear(GL_DEPTH_BUFFER_BIT); // 清除深度缓冲
```

其中，`glBindTexture` 用于绑定法线贴图，使得着色器根据这个贴图进行光照的计算。`glBindVertexArray` 用于绑定高度贴图的 `VAO`，使用高度贴图定义的顶点进行绘制形状。


由于本项目并非软光栅化渲染器，所以与光线计算相关的逻辑都在`GLSL`着色器中(而不是在`.cpp`文件中)。地面对全局点光源的反射计算相关逻辑，在片段着色器`terrain.frag`中（地形着色器，以下是`terrain.frag`的初版本）：


```GLSL
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
    vec3 diffuse = ndotl * terrainColor * lightColor; // 漫反射 Add * lightColor: YuZhuZhi
    vec3 specular = specularStrength * spec * lightColor * terrainColor; // 高光 Add * terrainColor: YuZhuZhi

    vec3 lighting = ambient + diffuse + specular; // 总光照

    // 雾效
    float d = distance(viewPos, WorldPos);
    float alpha = FogFactor(d);
    vec3 FogColor = vec3(0.09, 0.11, 0.09); // 雾色

    // 最终颜色
    FragColor.rgb = mix(lighting, FogColor, alpha);
    FragColor.a = 1.0;
}
```

注意！这里要将法线从 $[0, 1]$ 空间转换到 $[-1, 1]$ 空间。法线贴图通常存储在 $[0, 1]$ 范围内，其中 $(0, 0, 0)$ 表示向下的法线，而 $(1, 1, 1)$ 表示向上的法线。通过将这些值转换到 $[-1, 1]$ 范围，才能够正确地映射出法线方向。

=== 天空盒的实现

天空盒可以看作是无限远的背景，即贴图会跟着相机一起移动。实现天空盒，可以看作实现一个立方体，只需要 6 张贴图，分别放在上下左右前后6个位置。

#figure(
  image("image/天空盒贴图.png"),
  caption: [天空盒贴图]
)

接下来是天空盒渲染的片段着色器：

```GLSL
#version 420 core

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform samplerCube cubeMap;

in vec3 TexCoords;

// na tutorialu kaze samo uzmem mapu i ove koordinate i mogu da gadjam koji deo skyboxa gledam
void main() {		
    FragColor.rgb = texture(cubeMap, TexCoords).rgb;
    FragColor.a = 1.0;
}
```

这里使用了 ```cpp texture(cubeMap, TexCoords)``` 函数从立方体贴图中采样颜色。并将 alpha 通道设置为 1.0，表示不透明。

在主循环中，我们应用这个着色器并进行渲染：

```cpp
// 绑定天空盒着色器并设置矩阵
_skyboxShader.use();
_skyboxShader.SetMat4("projection", projection);
_skyboxShader.SetMat4("view", view);
_skyboxShader.SetMat4("model", skyBoxTransform.to_mat4());
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture.ID); // 绑定天空盒纹理
glBindVertexArray(_cubeVao); // 绑定立方体VAO
glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // 绘制天空盒
glClear(GL_DEPTH_BUFFER_BIT); // 清除深度缓冲
```


注意！渲染顺序十分重要，必须按照[天空盒$->$地面$->$烟花]的顺序进行渲染，否则先渲染的部分会被后渲染的部分给覆盖掉。


== 烟花光照 <PointLight>


为了实现烟花的光效，我们首先构造了点光源类`PointLight`：

```CPP
using Color = glm::vec3;
using Position = glm::vec3;
using Attenuation = glm::vec3;
using Direction = glm::vec3;

class PointLight
{
public:
	PointLight();
	PointLight(const Color& color, const Position& position, const Attenuation& attenuation, const float life);
	~PointLight() = default;

	#pragma region Get&Set
	Color getColor() { return _color; }
	Position getPosition() { return _position; }
	Attenuation getAttenuation() { return _attenuation; }
	float getLife() { return _life; }
	// Unable to Set. 
	#pragma endregion

	float distance(const Position& fragment); // Calculate DISTANCE between light & fragment.
	Color calcAddColor(const Position& fragment, const Direction& normal); // Calculate COLOR should be ADDED on the fragment.
	bool updateLife(const float decrease);

	bool addToShader(const std::shared_ptr<Shader> shader, const int index);
	bool deleteFromShader(const std::shared_ptr<Shader> shader, const int index);

public:
	Color _color;
	Position _position;
	Attenuation _attenuation;
	float _life;

};
```


=== `terrain.frag`的修改


这个类中最重要的两个函数是```CPP addToShader(...)```和```CPP deleteFromShader(...)```。如同之前所述：由于本项目并非软光栅化渲染器，所以与光线计算相关的逻辑都在着色器中，因此创建点光源后应当将其添加到着色器中。为此，首先需要修改`terrain.frag`着色器，使之能够存储点光源相关信息。在头部添加结构体与变量：

```GLSL
struct PointLight {
    vec3 position;
    vec3 color;
    vec3 attenuation; // 衰减参数 (constant, linear, quadratic)
};

#define MAX_LIGHTS 10 // 支持的最大点光源数量
uniform PointLight pointLights[MAX_LIGHTS];
```

在这里，为了节省计算时间，我们固定支持的最大点光源数量为$10$个。然后，仿照原先代码中的Blinn-Phong光照实现，也计算每个片段对烟花点光源的反应：

```GLSL
vec3 calcPointLightLighting(PointLight light, vec3 fragColor, vec3 fragPos, vec3 normal, vec3 viewPos) {
    // 计算光源到片段的方向距离
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    vec3 viewDir = normalize(viewPos - fragPos); // 计算视角方向
    vec3 halfDir = normalize(lightDir + viewDir); // 半程向量

    // 计算光源的衰减(这个参数计算结果有问题，可能根本没有设置到pointLights数组)
    float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * distance * distance);

    vec3 ambient = light.color * fragColor;

    // 漫反射计算
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.color * fragColor; // 漫反射

    // 高光计算
    float specularStrength = 0.5; // 高光强度
    float shininess = 32.0; // 粗糙度
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess); // 高光部分
    vec3 specular = specularStrength * spec * light.color * fragColor;

    // 返回计算的漫反射和高光部分的总和
    return (ambient + diffuse + specular) * attenuation;
}
```

这样，在着色器的主函数中，就可以对`pointLigths`中每一个点光源遍历计算并累加，所得结果再与之前实现的全局点光源相加，就能得到地面最终的颜色：

```GLSL
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < MAX_LIGHTS; i++) {
        pointLighting += calcPointLightLighting(pointLights[i], terrainColor, WorldPos, normal, viewPos);
    }

    vec3 lighting = ambient + diffuse + specular + pointLighting; // 总光照
```

现在我们可以结合在`C++`中实现的点光源类了。在创建点光源之后，应当手动将其添加到着色器中，否则着色器不能知道这个点光源的存在：

```CPP
bool PointLight::addToShader(const std::shared_ptr<Shader> shader, const int index)
{
    if (index >= MAX_LIGHTS || index < 0) return false;
    shader->use();

    shader->SetVec3("pointLights[" + std::to_string(index) + "].position", _position); // 设置位置
    shader->SetVec3("pointLights[" + std::to_string(index) + "].color", _color); // 设置颜色
    shader->SetVec3("pointLights[" + std::to_string(index) + "].attenuation", _attenuation); // 设置衰减

    return true;
}
```

应当注意到，`C++`的点光源类中具有属性`_life`，而着色器中的是没有的。因此点光源的生命控制应在`C++`中完成。当点光源的生命耗尽，需要手动调用```CPP deleteFromShader(...)```：

```CPP
bool PointLight::deleteFromShader(const std::shared_ptr<Shader> shader, const int index)
{
    if (index < 0) return false;
    shader->use();

    shader->SetVec3("pointLights[" + std::to_string(index) + "].color", glm::vec3(0.0f)); // 设置颜色

    return true;
}
```


=== 嵌入到```CPP class Laucher```中


烟花爆炸时生成点光源、消散时移除点光源，因此控制点光源的生命的任务自然要交给烟花类```CPP class Laucher```。首先在烟花类的成员变量中添加一个与着色器中对应的数组，以及一个指向相关着色器的指针：

```CPP
std::array<std::shared_ptr<PointLight>, MAX_LIGHTS> pointLights;
std::shared_ptr<Shader> shader;
```

烟花类中有一个```CPP explode(...)```函数，自然就在这个函数中创建点光源：

```CPP
void Launcher::explode2(Particle& p)
{
	int randomSound = getRandomNumber(1, 6); // 随机选择爆炸声音
	soundEngine->play2D(explosionSounds[randomSound - 1]); // 播放爆炸声音

	// 添加点光源 Add: YuZhuZhi
	auto pointLight = std::make_shared<PointLight>(
		Color(p.r, p.g, p.b), // 光的颜色
		p.pos,                   // 光的位置
		Attenuation(1.0f, 0.014f, 0.007f), // 光的衰减参数
		sparkleLife + 0.75
	);
	auto it = std::find(pointLights.begin(), pointLights.end(), nullptr);
	if (it != pointLights.end()) { // 添加到集中管理
		*it = pointLight;
		pointLight->addToShader(shader, it - pointLights.begin());
	}

	......
}
```

同时，烟花类中还有一个逐帧更新的函数```CPP update(...)```，它原先控制了粒子的生命的衰减，现在也要控制点光源的生命了：

```CPP
void Launcher::update(Camera& camera, GLfloat* particle_position, GLubyte* particle_color)
{
	float deltaTime = Camera::getDeltaTime(); // 获取时间增量
  ......
	simulate(camera, particle_position, particle_color); // 模拟粒子

	for (auto light = pointLights.begin(); light != pointLights.end(); light++) { // Add: YuZhuZhi
		if (light->get() && !(light->get()->updateLife(deltaTime))) { // 删除点光源
			int index = light - pointLights.begin();
			light->get()->deleteFromShader(shader, index);
			pointLights[index] = nullptr;
		}
	}

	sortParticles(); // 排序粒子
}
```

以上就基本完成了点光源相关的逻辑。

#figure(
  image("image/地面反射烟花.png"),
  caption: [地面对烟花点光源的反射 \ 左中右分别为不同时间的截图]
)



== 高斯模糊


由于在本项目中，烟花爆炸时产生的点光源并不具有材质，因此基于HDR的泛光并没有使用的前提条件。所以我们直接使用后处理来实现，即直接对输出的像素应用高斯模糊。为此，我们创建了高斯模糊的片段着色器`gaussian_blur.frag`：

```GLSL
#version 460
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 texOffset[24]; // 使用 24 个偏移量 (4*5 - 1)

void main()
{
    vec3 result = texture(screenTexture, TexCoords).rgb * 0.427027; // 中心像素权重

    // 5x5 核心采样偏移
    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216); // 权重
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float weight = weights[abs(x)] * weights[abs(y)];
            result += texture(screenTexture, TexCoords + vec2(x, y) * texOffset[0]).rgb * weight;
        }
    }

    FragColor = vec4(result, 1.0);
}
```
这里使用了$5 times 5$的高斯模糊核来完成高斯模糊，因为如果使用$3 times 3$的核，则模糊不明显。

我们通过创建帧缓冲对象 @LearnOpenGLFramebuffers 来对整个屏幕的画面进行高斯模糊：

```GLSL
GLuint fbo;
GLuint textureColorBuffer;
glGenFramebuffers(1, &fbo);
glBindFramebuffer(GL_FRAMEBUFFER, fbo);

// 创建一个纹理来保存渲染结果
glGenTextures(1, &textureColorBuffer);
glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_FLOAT, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

// 检查FBO是否完整
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "帧缓冲对象不完整!" << std::endl;
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

然后在主循环中，先清除缓冲区，再绑定帧缓冲对象，在完成所有图像的渲染之后，计算出纹理像素相对周围$5 times 5$方格的位置偏移量。接着使用高斯模糊片段着色器，再使用两个三角形（即将整个长方形屏幕分为两个三角形）对这一帧中的所有画面进行渲染。

```GLSL
while (!glfwWindowShouldClose(window))
{
    //...帧计数
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清除缓冲区
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); //绑定帧缓冲对象，必须在清除缓冲区之后

    // ...实现地面、天空盒、粒子的渲染

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 应用高斯模糊
    Shader blurShader("version1/shaders/gaussian_blur.vert", "version1/shaders/gaussian_blur.frag");

    // 计算纹理偏移量
    glm::vec2 texOffset[25]; // 24 个偏移量
    float offset = 1.0f; // 偏移量

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            texOffset[(x + 2) * 5 + (y + 2)] = glm::vec2(x * offset / SCREEN_W, y * offset / SCREEN_H);
        }
    }

    glClear(GL_COLOR_BUFFER_BIT); // 清除屏幕颜色缓冲

    blurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glUniform1i(glGetUniformLocation(blurShader.id, "screenTexture"), 0);
    glUniform2fv(glGetUniformLocation(blurShader.id, "texOffset"), 4, &texOffset[0][0]);

    // 绑定四边形 VAO 并绘制
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // 绘制四边形
    glBindVertexArray(0); // 解绑 VAO

    glfwSwapBuffers(window); // 交换缓冲区
    glfwPollEvents(); // 处理事件
}
```

这里的 `quadVAO` 是一个四边形的 `VAO`，在进入主循环之前创建（流程和之前一样）：

```GLSL
// 创建 quadVAO
GLuint quadVAO, quadVBO;
float quadVertices[] = {
    // positions         // texture coords
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, // Top-left
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f, // Top-right
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f, // Bottom-right
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f  // Bottom-left
};

glGenVertexArrays(1, &quadVAO);
glGenBuffers(1, &quadVBO);
glBindVertexArray(quadVAO);

glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

// 设置顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // 位置
glEnableVertexAttribArray(0);
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // 纹理坐标
glEnableVertexAttribArray(1);

// 解绑 VAO
glBindVertexArray(0);
```

最终效果如下：

#figure(
  image("image/gaussian_blur.png"),
  caption: [使用高斯模糊对画面进行处理的结果]
)

看起来画面确实变得模糊了，烟花粒子更具有真实感。

至此，我们完成了作业任务的所有要求，接下来是是对项目的扩展，我们将实现一个爆炸后产生图案的烟花效果。

== 特殊烟花效果

=== 实现"A"形烟花

在我们生活中遇到的烟花，有一种特殊的烟花——“图案烟花”，我们认为这种烟花十分有趣，如果能在普通烟花中出现特殊的图案烟花，想必效果一定十分美丽。

实现不同形状的烟花需要我们在```cpp expolode(...)```函数中修改爆炸时烟花粒子的分布：

```cpp
void Launcher::explode(Particle& p)
{
	int countA = 600;  // 设置A烟花的数量
	int randomSound = getRandomNumber(1, 6); // 随机选择爆炸声音
	soundEngine->play2D(explosionSounds[randomSound - 1]); // 播放爆炸声音

	// 添加点光源
	auto pointLight = std::make_shared<PointLight>(
		Color(p.r, p.g, p.b), // 光的颜色
		p.pos,                   // 光的位置
		Attenuation(1.0f, 0.014f, 0.007f), // 光的衰减参数
		sparkleLife + 0.75
	);
	auto it = std::find(pointLights.begin(), pointLights.end(), nullptr);
	if (it != pointLights.end()) { // 添加到集中管理
		*it = pointLight;
		pointLight->addToShader(shader, it - pointLights.begin());
	}

	// 定义 "A" 字母的火花位置
	float size = 10.0f; // 图案大小
	glm::vec3 basePos = p.pos; // 基础位置

	std::vector<glm::vec3> pattern = {
   //这里放置构成图像的点位置信息
   	};

	// 定义随机爆炸烟花的火花位置
	float randSize = getRandomNumber(0, explosionSpread); // 随机扩散范围

	// 生成 "A" 类型的火花粒子
	for (int i = 0; i < countA; i++)
	{
		const auto& pos = pattern[i % pattern.size()]; // 循环使用位置
		spawnParticle(
			pos,
			glm::vec3(getRandomNumber(-1.0f, 1.0f), getRandomNumber(-1.0f, 1.0f), 0), // 随机速度
			glm::vec4(p.r, p.g, p.b, p.a),
			sparkleSize,
			sparkleLife * 5,
			Particle::Type::SPARKLE
		);
	}
}

```
其中，`size`用于控制烟花大小，`countA`用于控制烟花数量，`pattern`用于设置点集图案，根据原本位置加上偏移来计算各个粒子的位置。

注意到，`pattern`的图案点集需要我们把我们的目标图案分解成点集信息，故使用`Python`实现对目标图案的点集分解。

```Python
import cv2
import numpy as np
import os

# 读取图像
image_path = 'B.png'
image = cv2.imread(image_path)

# 检查图像是否成功读取
if image is None:
    print(f"无法读取图像: {image_path}")
    exit()

# 提取图像的高度和宽度
height, width, _ = image.shape

# 定义基位置和大小
basePos = "basePos"  # 根据需要替换为实际位置
size = 0.1  # 根据需要设置大小

# 存储提取的白色像素位置
positions = []
gap = 10
# 遍历图像，按照 10 像素的间隔提取白色像素点
for y in range(0, height, gap):
    for x in range(0, width, gap):
        # 获取当前像素的 BGR 值
        pixel = image[y, x]
        # 检查是否为白色像素
        if not np.array_equal(pixel, [0, 0, 0]):  # OpenCV 中白色是 [255, 255, 255]
            # 添加格式化的坐标到 positions 列表
            positions.append(f"{basePos} + size*glm::vec3({x * 0.1}, {(height - 1 - y) * 0.1}, 0)")  # Y轴翻转

# 创建文件名，使用图片的名称
output_filename = os.path.splitext(image_path)[0] + '.txt'

# 保存结果到文本文件
with open(output_filename, 'w') as f:
    f.write("std::vector<glm::vec3> pattern = {\n")
    for pos in positions:
        f.write(f"\t{pos},\n")  # 输出位置
    f.write("};\n")

print(f"提取结果已保存到: {output_filename}")
```

其中，`size`用于控制图像大小，`gap`用于控制提取点集在图像上的间隔，如果要图像更清晰，则缩小`gap`，但粒子数也会更多，影响代码性能。

该代码的实现效果是把提取一副黑白图片的白色点集，并转化为OpenGL中的目标语言。

#figure(
  image("image/convert.png"),
  caption: [A形烟花的部分代码]
)

上述代码执行结果如下：

#figure(
  image("image/A_firework.png"),
  caption: [A形烟花]
)

可以看到，通过转化图片点集设置粒子位置来实现字母“A”的图案烟花。

=== 实现中山大学校徽形状的烟花

有了上述`Python`提取图案方法，我们也可以找一找黑白的中山大学校徽图案来提取点集，这里我们设置间距`gap`为10提取点集，执行代码可以得到如下烟花：

#figure(
  image("image/校徽烟花.png"),
  caption: [校徽形烟花]
)

可以看到，我们这里出现了中山大学校徽图案的烟花。

但是这里只有一种图案烟花，我们想要原本的普通烟花和图案烟花同时出现，于是我们增加一个```CPP explode2(...)```函数。

```CPP
void Launcher::explode2(Particle& p)
{
  
...

	// 定义随机爆炸烟花的火花位置
	float randSize = getRandomNumber(0, explosionSpread); // 随机扩散范围
	
	// 生成随机类型的火花粒子
	for (int i = 1; i < sparklesPerExplosion; i++)
	{
		// 随机生成火花位置和速度
		float randX = getRandomNumber(-1, 2);
		float randY = getRandomNumber(-1, 2);
		float randZ = getRandomNumber(-1, 2);
		float randSpread = getRandomNumber(0, explosionSpread);
		float randLife = getRandomNumber(0, 1.5f);

		// 生成火花粒子
		spawnParticle(
			p.pos,
			glm::normalize(glm::vec3(randX, randY, randZ)) * (explosionSize - randSpread + randSize),
			glm::vec4(p.r, p.g, p.b, p.a),
			sparkleSize,
			sparkleLife + randLife,
			Particle::Type::SPARKLE
		);

	}
}

// 发射烟花
void Launcher::launchFirework()
{
	float randX = getRandomNumber(-launchSpread, launchSpread); // 随机X偏移
	float randZ = getRandomNumber(-launchSpread, launchSpread); // 随机Z偏移
	float rand = getRandomNumber(-150.0f, 150.0f); // 随机Y偏移

	// 生成火箭粒子
	spawnParticle(
		position + glm::vec3(rand, 50.0, rand), // 发射位置
		glm::vec3(randX, getRandomNumber(launchSpeed, launchSpeed + 25), randZ), // 发射速度
		getRandomBrightColor(), // 随机颜色
		rocketSize, // 火箭大小
		rocketLife, // 火箭生命期
		Particle::Type::LAUNCHING // 类型为发射中
	);
}
```

添加普通烟花的爆炸代码，然后设置任意比例的两种烟花的数量，即可是实现多种烟花的爆炸。

```CPP
int counter = 0; //每 5 次烟花爆炸出现一次图案

// 模拟粒子的运动
void Launcher::simulate(Camera& camera, GLfloat* particle_position, GLubyte* particle_color)
{
	float deltaTime = Camera::getDeltaTime(); // 获取时间增量
	particlesCount = 0; // 重置粒子计数

	for (int i = 0; i < maxParticles; i++)
	{
		//...模拟烟花粒子

		// 粒子刚刚死亡
		if (p.type == Particle::Type::LAUNCHING){
			counter++;
			if(counter==5){
				counter = 0;
				explode(p); // 处理爆炸
			}
			else {
				explode2(p);
			}
		}

		p.type = Particle::Type::DEAD; // 设置为死亡

		p.cameraDst = -1.0f; // 重置与相机的距离
	}
}
```

这里我们设置每 5 次爆炸产生一个校徽形状的烟花。

#figure(
  image("image/混合烟花.png"),
  caption: [同时生成普通烟花和校徽烟花]
)

=== 实现图案烟花的渐变增大

事实上，在我们预想中的图案烟花因该是由范围较小较为密集的形态逐渐扩大为完整的图案并持续展示一段时间，而之前只实现了后者，故我们需要继续修改代码以实现图案烟花的渐变增大。

这里我们将 `pattern` 中的元素修改为相对坐标而不是每个点的绝对坐标，然后将相对坐标直接作为烟花的初始速度向量，这样，烟花就能够按照校徽的形状扩散开。

```CPP
std::vector<glm::vec3> pattern = {
  size* glm::vec3(20.0, 41.400000000000006, 0),
  size* glm::vec3(21.0, 41.400000000000006, 0),
    //...点坐标，构成校徽的形状
}

for (int i = 0; i < countA; i++)
{
  const auto& pos = pattern[i % pattern.size()]; // 循环使用位置

  spawnParticle(
    p.pos,
    pos,
    glm::vec4(p.r, p.g, p.b, p.a),
    sparkleSize,
    sparkleLife * 5,
    Particle::Type::SPARKLE
  );
}
```

#figure(
  image("image/校徽烟花扩散.png"),
  caption: [校徽烟花在烟花爆炸中心扩散]
)

//************************************************************//

= 实验中遇到的困难总结

== 环境配置

问题：为了使用烟花的音频，我们使用 `irrKlang` 音频库，配置环境的过程中一直导入失败

解决方案：最终将 `irrKlang` 中的三个 `dll` 文件放入项目/x64/Debug/ 下，问题解决。 

#figure(
  image("image/irrKlang.png"),
  caption: [导入 irrKlang 音频库的关键步骤]
)

== 添加地面和天空盒

- 问题：渲染地面后，出现一部分全黑和一部分全白的不自然着色

解决方案：最终发现我们没有将法线从 $[0, 1]$ 空间转换到 $[-1, 1]$ 空间。法线贴图通常存储在 $[0, 1]$ 范围内，其中 $(0, 0, 0)$ 表示向下的法线，而 $(1, 1, 1)$ 表示向上的法线。通过将这些值转换到 $[-1, 1]$ 范围，才能够正确地映射出法线方向。按照这个原理进行修改后，地面的颜色渲染变得正常。

- 问题：将已经完成烟花渲染的代码加入地面和天空盒的渲染之后，烟花消失不见

解决方案：必须先渲染地面和天空盒，然后再渲染烟花，否则烟花会被覆盖。

== 烟花光照

原先我们希望每一个爆炸的粒子都创建一个点光源，这就要求烟花类```CPP class Launcher```中对点光源的管理的数据结构是动态数组```CPP std::vector```。但是，`GLSL`中并没有动态数组，只有静态裸数组。因此，为了与着色器对应，我们只能也在烟花类中用相似的数据结构，即`std::array`，并且限定相同的大小。

在原先的设想中，当一个点光源生命耗尽，只需要将它从动态数组中移除即可。但这显然带来一个问题：移除之后，`C++`中的点光源的下标就不与着色器中的对应了。因此这也是我们不得不选用`std::array`的理由。

同样的道理，即便使用静态数组，被删去的点光源可能在一串连续存在数组中的点光源的中间。而`GLSL`并没有指针类型，因此下次渲染时无法通过判断空指针来跳过这一被删去的点光源。

我们最终的解决方法是：考虑颜色为```CPP glm::vec3(0.0f)```的光。这种光(向量)无论和谁相乘，都得到零向量或者零标量，即不影响最终结果。这就是为什么在```CPP deleteFromShader(...)```中只是简单地将`color`设为```CPP glm::vec3(0.0f)```的原因。

当然，这一问题也有更加聪明的解决方案，那就是在着色器的结构体`PointLight`中附设一个是否有效的属性。如果无效，就跳过不渲染。但由于前一种方法需要附带修改的地方不多，所以实现的是前一种方法。

== 高斯模糊

问题：在高斯模糊一节，创建帧缓冲对象后，使用两个三角形对屏幕进行渲染时，最终只渲染了一个三角形。

解决方案：最终发现绑定帧缓冲对象必须在清除缓冲区之后，问题解决。

//************************************************************//

= 小组分工

#figure(
  tablex(
    columns: 3,
    align: center + horizon,
    auto-vlines: false,
    //header-rows: 2,

    [姓名], [], [完成内容],
    [陈德建], [], [烟花和场景的代码缝合 \ 校徽烟花扩散 \ 高斯模糊],
    [曹越], [], [普通烟花效果 \ 地面和天空盒 \ 校徽形状烟花的创建],
    [王俊亚], [], [点光源类 \ 烟花的点光源相关逻辑 \ README撰写与PPT制作],
    [张晋], [], [烟花音效 \ PPT制作 \ BUG测试]
  ),
  kind: table
)

//***************参考文献********************//

#set text(font: "Times New Roman", lang: "en", region: "en")

#show bibliography: set heading(numbering: "I")

#bibliography("reference.bib", style: "ieee", title: "参考文献")
