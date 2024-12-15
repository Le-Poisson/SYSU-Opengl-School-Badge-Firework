#include "launcher.h"
#include <irrKlang\include/irrKlang.h>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace irrklang;

// 创建声音引擎
ISoundEngine* soundEngine = createIrrKlangDevice();

// 获取指定范围内的随机数
float getRandomNumber(float min, float max);
// 渐变颜色
glm::vec3 fadeColor(glm::vec3 startColor, glm::vec3 destColor, float ratio);
// 获取随机明亮颜色
glm::vec4 getRandomBrightColor();

// 定义重力和无力的向量
auto gravityForce = glm::vec3(0.0f, -GRAVITY, 0.0f);
auto noForce = glm::vec3(1.0f, 1.0f, 1.0f);

// 发射器构造函数，初始化位置为原点
Launcher::Launcher()
	: Launcher(glm::vec3(0.0f, 20.0f, 0.0f))
{}

// 带位置的发射器构造函数
Launcher::Launcher(glm::vec3 position)
	: position(position)
{
	for (int i = 0; i < maxParticles; i++)
	{
		// 初始化粒子的默认值
		particles[i].life = -1.0f; // 生命期为负，表示未使用
		particles[i].cameraDst = -1.0f; // 与相机的距离为负
		particles[i].trailTime = trailDelay; // 初始化拖尾时间
		particles[i].type = Particle::Type::DEAD; // 初始化为死亡类型
	}
}

Launcher::Launcher(std::shared_ptr<Shader> shader)
	: Launcher()
{
	this->shader = shader;
	auto zeroPointLight = PointLight();
	for (int i = 0; i < MAX_LIGHTS; i++) {
		zeroPointLight.addToShader(shader, i);
	}
}

Launcher::Launcher(glm::vec3 position, std::shared_ptr<Shader> shader)
	: Launcher(position)
{
	this->shader = shader;
	auto zeroPointLight = PointLight();
	for (int i = 0; i < MAX_LIGHTS; i++) {
		zeroPointLight.addToShader(shader, i);
	}
}


// Add: YuZhuZhi
//Launcher::~Launcher()
//{
//	for (auto& light : pointLights) delete light;
//	pointLights.clear();
//}
int counter = 0; //每 5 次烟花爆炸出现一次图案

// 模拟粒子的运动
void Launcher::simulate(Camera& camera, GLfloat* particle_position, GLubyte* particle_color)
{
	float deltaTime = Camera::getDeltaTime(); // 获取时间增量
	particlesCount = 0; // 重置粒子计数

	

	for (int i = 0; i < maxParticles; i++)
	{
		// 模拟每个粒子
		Particle& p = particles[i];

		if (p.life > 0.0f)
		{
			// 粒子仍然存活
			p.speed += gravityForce * deltaTime; // 受重力影响
			p.pos += p.speed * deltaTime; // 更新位置
			p.cameraDst = glm::distance(p.pos, camera.getPosition()); // 更新与相机的距离

			renderTrails(p, deltaTime); // 渲染拖尾

			// 填充GPU缓冲区
			particle_position[4 * particlesCount + 0] = p.pos.x;
			particle_position[4 * particlesCount + 1] = p.pos.y;
			particle_position[4 * particlesCount + 2] = p.pos.z;

			float lifeRatio = 1.0f; // 生命比例
			auto newColor = glm::vec3(p.r, p.g, p.b); // 当前颜色
			switch (p.type)
			{
			case Particle::Type::SPARKLE:
				lifeRatio = p.life / sparkleLife; // 火花的生命比例
				break;
			case Particle::Type::TRAIL:
				lifeRatio = p.life / trailLife; // 拖尾的生命比例
				newColor = fadeColor(newColor, trailFade, lifeRatio); // 渐变颜色
				break;
			case Particle::Type::FOUNTAIN:
				lifeRatio = p.life / fountainLife; // 喷泉的生命比例
				newColor = fadeColor(fountainColor, fountainFade, lifeRatio); // 渐变颜色
				break;
			}
			particle_position[4 * particlesCount + 3] = p.size * lifeRatio; // 粒子大小

			particle_color[4 * particlesCount + 0] = newColor.r; // 红色分量
			particle_color[4 * particlesCount + 1] = newColor.g; // 绿色分量
			particle_color[4 * particlesCount + 2] = newColor.b; // 蓝色分量
			particle_color[4 * particlesCount + 3] = p.life; // 生命值

			particlesCount++; // 增加粒子计数
			p.life -= deltaTime; // 减少生命值
			continue;
		}

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

// 渲染粒子的拖尾
void Launcher::renderTrails(Particle& p, float deltaTime)
{
	if (p.trailTime <= 0 && (p.type == Particle::Type::SPARKLE || p.type == Particle::Type::LAUNCHING))
	{
		// 生成拖尾粒子
		spawnParticle(
			p.pos,
			glm::vec3(0.0f), // 拖尾粒子初速度为0
			glm::vec4(p.r, p.g, p.b, p.a), // 拖尾颜色
			trailSize, // 拖尾大小
			trailLife, // 拖尾生命期
			Particle::Type::TRAIL // 类型为拖尾
		);
		p.trailTime = trailDelay; // 重置拖尾时间
	}
	p.trailTime -= deltaTime; // 减少拖尾时间
}

// 生成粒子
void Launcher::spawnParticle(glm::vec3 position, glm::vec3 speed, glm::vec4 color, float size, float life, Particle::Type type)
{
	int idx = findUnusedParticle(); // 查找未使用的粒子

	// 设置粒子的属性
	particles[idx].pos = position;
	particles[idx].speed = speed;
	particles[idx].r = color.r;
	particles[idx].g = color.g;
	particles[idx].b = color.b;
	particles[idx].a = color.a;
	particles[idx].size = size;
	particles[idx].life = life;
	particles[idx].type = type;
}


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
	float size = 1.0f; // 图案大小
	glm::vec3 basePos = p.pos; // 基础位置

	std::vector<glm::vec3> pattern = {
		/*size * glm::vec3(10, 10, 0),
		size * glm::vec3(10, -10, 0),
		size * glm::vec3(-10, 10, 0),
		size * glm::vec3(-10, -10, 0),
		size * glm::vec3(0, 10, 0),
		size * glm::vec3(0, -10, 0),
		size * glm::vec3(10, 0, 0),
		size * glm::vec3(-10, 0, 0),
		size* glm::vec3(20, 0, 0),
		size* glm::vec3(-20, 0, 0),
		size* glm::vec3(0, 20, 0),
		size* glm::vec3(0, -20, 0),*/
		size* glm::vec3(20.0, 41.400000000000006, 0),
		size* glm::vec3(21.0, 41.400000000000006, 0),
		size* glm::vec3(22.0, 41.400000000000006, 0),
		size* glm::vec3(23.0, 41.400000000000006, 0),
		size* glm::vec3(24.0, 41.400000000000006, 0),
		size* glm::vec3(25.0, 41.400000000000006, 0),
		size* glm::vec3(16.0, 40.400000000000006, 0),
		size* glm::vec3(17.0, 40.400000000000006, 0),
		size* glm::vec3(18.0, 40.400000000000006, 0),
		size* glm::vec3(19.0, 40.400000000000006, 0),
		size* glm::vec3(20.0, 40.400000000000006, 0),
		size* glm::vec3(21.0, 40.400000000000006, 0),
		size* glm::vec3(22.0, 40.400000000000006, 0),
		size* glm::vec3(23.0, 40.400000000000006, 0),
		size* glm::vec3(24.0, 40.400000000000006, 0),
		size* glm::vec3(25.0, 40.400000000000006, 0),
		size* glm::vec3(26.0, 40.400000000000006, 0),
		size* glm::vec3(27.0, 40.400000000000006, 0),
		size* glm::vec3(28.0, 40.400000000000006, 0),
		size* glm::vec3(29.0, 40.400000000000006, 0),
		size* glm::vec3(14.0, 39.400000000000006, 0),
		size* glm::vec3(15.0, 39.400000000000006, 0),
		size* glm::vec3(16.0, 39.400000000000006, 0),
		size* glm::vec3(17.0, 39.400000000000006, 0),
		size* glm::vec3(20.0, 39.400000000000006, 0),
		size* glm::vec3(21.0, 39.400000000000006, 0),
		size* glm::vec3(22.0, 39.400000000000006, 0),
		size* glm::vec3(23.0, 39.400000000000006, 0),
		size* glm::vec3(24.0, 39.400000000000006, 0),
		size* glm::vec3(25.0, 39.400000000000006, 0),
		size* glm::vec3(26.0, 39.400000000000006, 0),
		size* glm::vec3(28.0, 39.400000000000006, 0),
		size* glm::vec3(29.0, 39.400000000000006, 0),
		size* glm::vec3(30.0, 39.400000000000006, 0),
		size* glm::vec3(31.0, 39.400000000000006, 0),
		size* glm::vec3(32.0, 39.400000000000006, 0),
		size* glm::vec3(12.0, 38.400000000000006, 0),
		size* glm::vec3(13.0, 38.400000000000006, 0),
		size* glm::vec3(14.0, 38.400000000000006, 0),
		size* glm::vec3(15.0, 38.400000000000006, 0),
		size* glm::vec3(16.0, 38.400000000000006, 0),
		size* glm::vec3(17.0, 38.400000000000006, 0),
		size* glm::vec3(28.0, 38.400000000000006, 0),
		size* glm::vec3(29.0, 38.400000000000006, 0),
		size* glm::vec3(31.0, 38.400000000000006, 0),
		size* glm::vec3(32.0, 38.400000000000006, 0),
		size* glm::vec3(33.0, 38.400000000000006, 0),
		size* glm::vec3(11.0, 37.4, 0),
		size* glm::vec3(12.0, 37.4, 0),
		size* glm::vec3(13.0, 37.4, 0),
		size* glm::vec3(14.0, 37.4, 0),
		size* glm::vec3(15.0, 37.4, 0),
		size* glm::vec3(31.0, 37.4, 0),
		size* glm::vec3(33.0, 37.4, 0),
		size* glm::vec3(34.0, 37.4, 0),
		size* glm::vec3(35.0, 37.4, 0),
		size* glm::vec3(10.0, 36.4, 0),
		size* glm::vec3(11.0, 36.4, 0),
		size* glm::vec3(13.0, 36.4, 0),
		size* glm::vec3(18.0, 36.4, 0),
		size* glm::vec3(26.0, 36.4, 0),
		size* glm::vec3(27.0, 36.4, 0),
		size* glm::vec3(33.0, 36.4, 0),
		size* glm::vec3(34.0, 36.4, 0),
		size* glm::vec3(35.0, 36.4, 0),
		size* glm::vec3(36.0, 36.4, 0),
		size* glm::vec3(9.0, 35.4, 0),
		size* glm::vec3(10.0, 35.4, 0),
		size* glm::vec3(11.0, 35.4, 0),
		size* glm::vec3(18.0, 35.4, 0),
		size* glm::vec3(19.0, 35.4, 0),
		size* glm::vec3(20.0, 35.4, 0),
		size* glm::vec3(25.0, 35.4, 0),
		size* glm::vec3(26.0, 35.4, 0),
		size* glm::vec3(27.0, 35.4, 0),
		size* glm::vec3(28.0, 35.4, 0),
		size* glm::vec3(34.0, 35.4, 0),
		size* glm::vec3(36.0, 35.4, 0),
		size* glm::vec3(37.0, 35.4, 0),
		size* glm::vec3(8.0, 34.4, 0),
		size* glm::vec3(9.0, 34.4, 0),
		size* glm::vec3(10.0, 34.4, 0),
		size* glm::vec3(19.0, 34.4, 0),
		size* glm::vec3(20.0, 34.4, 0),
		size* glm::vec3(21.0, 34.4, 0),
		size* glm::vec3(25.0, 34.4, 0),
		size* glm::vec3(26.0, 34.4, 0),
		size* glm::vec3(35.0, 34.4, 0),
		size* glm::vec3(37.0, 34.4, 0),
		size* glm::vec3(38.0, 34.4, 0),
		size* glm::vec3(7.0, 33.4, 0),
		size* glm::vec3(8.0, 33.4, 0),
		size* glm::vec3(9.0, 33.4, 0),
		size* glm::vec3(12.0, 33.4, 0),
		size* glm::vec3(13.0, 33.4, 0),
		size* glm::vec3(14.0, 33.4, 0),
		size* glm::vec3(18.0, 33.4, 0),
		size* glm::vec3(19.0, 33.4, 0),
		size* glm::vec3(21.0, 33.4, 0),
		size* glm::vec3(24.0, 33.4, 0),
		size* glm::vec3(25.0, 33.4, 0),
		size* glm::vec3(27.0, 33.4, 0),
		size* glm::vec3(32.0, 33.4, 0),
		size* glm::vec3(33.0, 33.4, 0),
		size* glm::vec3(36.0, 33.4, 0),
		size* glm::vec3(38.0, 33.4, 0),
		size* glm::vec3(39.0, 33.4, 0),
		size* glm::vec3(6.0, 32.4, 0),
		size* glm::vec3(7.0, 32.4, 0),
		size* glm::vec3(12.0, 32.4, 0),
		size* glm::vec3(13.0, 32.4, 0),
		size* glm::vec3(14.0, 32.4, 0),
		size* glm::vec3(18.0, 32.4, 0),
		size* glm::vec3(27.0, 32.4, 0),
		size* glm::vec3(28.0, 32.4, 0),
		size* glm::vec3(32.0, 32.4, 0),
		size* glm::vec3(33.0, 32.4, 0),
		size* glm::vec3(34.0, 32.4, 0),
		size* glm::vec3(37.0, 32.4, 0),
		size* glm::vec3(38.0, 32.4, 0),
		size* glm::vec3(39.0, 32.4, 0),
		size* glm::vec3(6.0, 31.400000000000002, 0),
		size* glm::vec3(7.0, 31.400000000000002, 0),
		size* glm::vec3(8.0, 31.400000000000002, 0),
		size* glm::vec3(12.0, 31.400000000000002, 0),
		size* glm::vec3(13.0, 31.400000000000002, 0),
		size* glm::vec3(14.0, 31.400000000000002, 0),
		size* glm::vec3(30.0, 31.400000000000002, 0),
		size* glm::vec3(31.0, 31.400000000000002, 0),
		size* glm::vec3(32.0, 31.400000000000002, 0),
		size* glm::vec3(33.0, 31.400000000000002, 0),
		size* glm::vec3(34.0, 31.400000000000002, 0),
		size* glm::vec3(35.0, 31.400000000000002, 0),
		size* glm::vec3(38.0, 31.400000000000002, 0),
		size* glm::vec3(39.0, 31.400000000000002, 0),
		size* glm::vec3(40.0, 31.400000000000002, 0),
		size* glm::vec3(5.0, 30.400000000000002, 0),
		size* glm::vec3(6.0, 30.400000000000002, 0),
		size* glm::vec3(7.0, 30.400000000000002, 0),
		size* glm::vec3(12.0, 30.400000000000002, 0),
		size* glm::vec3(13.0, 30.400000000000002, 0),
		size* glm::vec3(14.0, 30.400000000000002, 0),
		size* glm::vec3(15.0, 30.400000000000002, 0),
		size* glm::vec3(30.0, 30.400000000000002, 0),
		size* glm::vec3(32.0, 30.400000000000002, 0),
		size* glm::vec3(33.0, 30.400000000000002, 0),
		size* glm::vec3(34.0, 30.400000000000002, 0),
		size* glm::vec3(40.0, 30.400000000000002, 0),
		size* glm::vec3(41.0, 30.400000000000002, 0),
		size* glm::vec3(5.0, 29.400000000000002, 0),
		size* glm::vec3(6.0, 29.400000000000002, 0),
		size* glm::vec3(12.0, 29.400000000000002, 0),
		size* glm::vec3(13.0, 29.400000000000002, 0),
		size* glm::vec3(15.0, 29.400000000000002, 0),
		size* glm::vec3(21.0, 29.400000000000002, 0),
		size* glm::vec3(22.0, 29.400000000000002, 0),
		size* glm::vec3(23.0, 29.400000000000002, 0),
		size* glm::vec3(24.0, 29.400000000000002, 0),
		size* glm::vec3(25.0, 29.400000000000002, 0),
		size* glm::vec3(30.0, 29.400000000000002, 0),
		size* glm::vec3(31.0, 29.400000000000002, 0),
		size* glm::vec3(32.0, 29.400000000000002, 0),
		size* glm::vec3(34.0, 29.400000000000002, 0),
		size* glm::vec3(39.0, 29.400000000000002, 0),
		size* glm::vec3(40.0, 29.400000000000002, 0),
		size* glm::vec3(41.0, 29.400000000000002, 0),
		size* glm::vec3(4.0, 28.400000000000002, 0),
		size* glm::vec3(5.0, 28.400000000000002, 0),
		size* glm::vec3(6.0, 28.400000000000002, 0),
		size* glm::vec3(16.0, 28.400000000000002, 0),
		size* glm::vec3(20.0, 28.400000000000002, 0),
		size* glm::vec3(21.0, 28.400000000000002, 0),
		size* glm::vec3(22.0, 28.400000000000002, 0),
		size* glm::vec3(23.0, 28.400000000000002, 0),
		size* glm::vec3(24.0, 28.400000000000002, 0),
		size* glm::vec3(25.0, 28.400000000000002, 0),
		size* glm::vec3(26.0, 28.400000000000002, 0),
		size* glm::vec3(30.0, 28.400000000000002, 0),
		size* glm::vec3(31.0, 28.400000000000002, 0),
		size* glm::vec3(41.0, 28.400000000000002, 0),
		size* glm::vec3(4.0, 27.400000000000002, 0),
		size* glm::vec3(5.0, 27.400000000000002, 0),
		size* glm::vec3(6.0, 27.400000000000002, 0),
		size* glm::vec3(19.0, 27.400000000000002, 0),
		size* glm::vec3(20.0, 27.400000000000002, 0),
		size* glm::vec3(25.0, 27.400000000000002, 0),
		size* glm::vec3(26.0, 27.400000000000002, 0),
		size* glm::vec3(27.0, 27.400000000000002, 0),
		size* glm::vec3(40.0, 27.400000000000002, 0),
		size* glm::vec3(41.0, 27.400000000000002, 0),
		size* glm::vec3(42.0, 27.400000000000002, 0),
		size* glm::vec3(4.0, 26.400000000000002, 0),
		size* glm::vec3(8.0, 26.400000000000002, 0),
		size* glm::vec3(10.0, 26.400000000000002, 0),
		size* glm::vec3(17.0, 26.400000000000002, 0),
		size* glm::vec3(18.0, 26.400000000000002, 0),
		size* glm::vec3(19.0, 26.400000000000002, 0),
		size* glm::vec3(20.0, 26.400000000000002, 0),
		size* glm::vec3(23.0, 26.400000000000002, 0),
		size* glm::vec3(26.0, 26.400000000000002, 0),
		size* glm::vec3(27.0, 26.400000000000002, 0),
		size* glm::vec3(28.0, 26.400000000000002, 0),
		size* glm::vec3(29.0, 26.400000000000002, 0),
		size* glm::vec3(35.0, 26.400000000000002, 0),
		size* glm::vec3(36.0, 26.400000000000002, 0),
		size* glm::vec3(37.0, 26.400000000000002, 0),
		size* glm::vec3(38.0, 26.400000000000002, 0),
		size* glm::vec3(40.0, 26.400000000000002, 0),
		size* glm::vec3(41.0, 26.400000000000002, 0),
		size* glm::vec3(42.0, 26.400000000000002, 0),
		size* glm::vec3(3.0, 25.400000000000002, 0),
		size* glm::vec3(4.0, 25.400000000000002, 0),
		size* glm::vec3(5.0, 25.400000000000002, 0),
		size* glm::vec3(8.0, 25.400000000000002, 0),
		size* glm::vec3(9.0, 25.400000000000002, 0),
		size* glm::vec3(11.0, 25.400000000000002, 0),
		size* glm::vec3(15.0, 25.400000000000002, 0),
		size* glm::vec3(16.0, 25.400000000000002, 0),
		size* glm::vec3(18.0, 25.400000000000002, 0),
		size* glm::vec3(19.0, 25.400000000000002, 0),
		size* glm::vec3(22.0, 25.400000000000002, 0),
		size* glm::vec3(23.0, 25.400000000000002, 0),
		size* glm::vec3(24.0, 25.400000000000002, 0),
		size* glm::vec3(26.0, 25.400000000000002, 0),
		size* glm::vec3(27.0, 25.400000000000002, 0),
		size* glm::vec3(28.0, 25.400000000000002, 0),
		size* glm::vec3(29.0, 25.400000000000002, 0),
		size* glm::vec3(30.0, 25.400000000000002, 0),
		size* glm::vec3(36.0, 25.400000000000002, 0),
		size* glm::vec3(37.0, 25.400000000000002, 0),
		size* glm::vec3(42.0, 25.400000000000002, 0),
		size* glm::vec3(3.0, 24.400000000000002, 0),
		size* glm::vec3(4.0, 24.400000000000002, 0),
		size* glm::vec3(5.0, 24.400000000000002, 0),
		size* glm::vec3(10.0, 24.400000000000002, 0),
		size* glm::vec3(14.0, 24.400000000000002, 0),
		size* glm::vec3(15.0, 24.400000000000002, 0),
		size* glm::vec3(16.0, 24.400000000000002, 0),
		size* glm::vec3(30.0, 24.400000000000002, 0),
		size* glm::vec3(31.0, 24.400000000000002, 0),
		size* glm::vec3(36.0, 24.400000000000002, 0),
		size* glm::vec3(41.0, 24.400000000000002, 0),
		size* glm::vec3(42.0, 24.400000000000002, 0),
		size* glm::vec3(3.0, 23.400000000000002, 0),
		size* glm::vec3(4.0, 23.400000000000002, 0),
		size* glm::vec3(5.0, 23.400000000000002, 0),
		size* glm::vec3(8.0, 23.400000000000002, 0),
		size* glm::vec3(9.0, 23.400000000000002, 0),
		size* glm::vec3(10.0, 23.400000000000002, 0),
		size* glm::vec3(14.0, 23.400000000000002, 0),
		size* glm::vec3(15.0, 23.400000000000002, 0),
		size* glm::vec3(22.0, 23.400000000000002, 0),
		size* glm::vec3(23.0, 23.400000000000002, 0),
		size* glm::vec3(24.0, 23.400000000000002, 0),
		size* glm::vec3(31.0, 23.400000000000002, 0),
		size* glm::vec3(32.0, 23.400000000000002, 0),
		size* glm::vec3(36.0, 23.400000000000002, 0),
		size* glm::vec3(41.0, 23.400000000000002, 0),
		size* glm::vec3(42.0, 23.400000000000002, 0),
		size* glm::vec3(43.0, 23.400000000000002, 0),
		size* glm::vec3(3.0, 22.400000000000002, 0),
		size* glm::vec3(4.0, 22.400000000000002, 0),
		size* glm::vec3(5.0, 22.400000000000002, 0),
		size* glm::vec3(13.0, 22.400000000000002, 0),
		size* glm::vec3(15.0, 22.400000000000002, 0),
		size* glm::vec3(22.0, 22.400000000000002, 0),
		size* glm::vec3(23.0, 22.400000000000002, 0),
		size* glm::vec3(24.0, 22.400000000000002, 0),
		size* glm::vec3(31.0, 22.400000000000002, 0),
		size* glm::vec3(32.0, 22.400000000000002, 0),
		size* glm::vec3(36.0, 22.400000000000002, 0),
		size* glm::vec3(37.0, 22.400000000000002, 0),
		size* glm::vec3(38.0, 22.400000000000002, 0),
		size* glm::vec3(39.0, 22.400000000000002, 0),
		size* glm::vec3(41.0, 22.400000000000002, 0),
		size* glm::vec3(42.0, 22.400000000000002, 0),
		size* glm::vec3(43.0, 22.400000000000002, 0),
		size* glm::vec3(3.0, 21.400000000000002, 0),
		size* glm::vec3(4.0, 21.400000000000002, 0),
		size* glm::vec3(5.0, 21.400000000000002, 0),
		size* glm::vec3(7.0, 21.400000000000002, 0),
		size* glm::vec3(10.0, 21.400000000000002, 0),
		size* glm::vec3(13.0, 21.400000000000002, 0),
		size* glm::vec3(14.0, 21.400000000000002, 0),
		size* glm::vec3(15.0, 21.400000000000002, 0),
		size* glm::vec3(16.0, 21.400000000000002, 0),
		size* glm::vec3(17.0, 21.400000000000002, 0),
		size* glm::vec3(22.0, 21.400000000000002, 0),
		size* glm::vec3(23.0, 21.400000000000002, 0),
		size* glm::vec3(24.0, 21.400000000000002, 0),
		size* glm::vec3(29.0, 21.400000000000002, 0),
		size* glm::vec3(30.0, 21.400000000000002, 0),
		size* glm::vec3(31.0, 21.400000000000002, 0),
		size* glm::vec3(33.0, 21.400000000000002, 0),
		size* glm::vec3(36.0, 21.400000000000002, 0),
		size* glm::vec3(41.0, 21.400000000000002, 0),
		size* glm::vec3(42.0, 21.400000000000002, 0),
		size* glm::vec3(43.0, 21.400000000000002, 0),
		size* glm::vec3(3.0, 20.400000000000002, 0),
		size* glm::vec3(4.0, 20.400000000000002, 0),
		size* glm::vec3(5.0, 20.400000000000002, 0),
		size* glm::vec3(7.0, 20.400000000000002, 0),
		size* glm::vec3(9.0, 20.400000000000002, 0),
		size* glm::vec3(10.0, 20.400000000000002, 0),
		size* glm::vec3(13.0, 20.400000000000002, 0),
		size* glm::vec3(14.0, 20.400000000000002, 0),
		size* glm::vec3(15.0, 20.400000000000002, 0),
		size* glm::vec3(16.0, 20.400000000000002, 0),
		size* glm::vec3(17.0, 20.400000000000002, 0),
		size* glm::vec3(18.0, 20.400000000000002, 0),
		size* glm::vec3(19.0, 20.400000000000002, 0),
		size* glm::vec3(20.0, 20.400000000000002, 0),
		size* glm::vec3(21.0, 20.400000000000002, 0),
		size* glm::vec3(22.0, 20.400000000000002, 0),
		size* glm::vec3(23.0, 20.400000000000002, 0),
		size* glm::vec3(24.0, 20.400000000000002, 0),
		size* glm::vec3(25.0, 20.400000000000002, 0),
		size* glm::vec3(26.0, 20.400000000000002, 0),
		size* glm::vec3(27.0, 20.400000000000002, 0),
		size* glm::vec3(28.0, 20.400000000000002, 0),
		size* glm::vec3(29.0, 20.400000000000002, 0),
		size* glm::vec3(30.0, 20.400000000000002, 0),
		size* glm::vec3(31.0, 20.400000000000002, 0),
		size* glm::vec3(33.0, 20.400000000000002, 0),
		size* glm::vec3(36.0, 20.400000000000002, 0),
		size* glm::vec3(37.0, 20.400000000000002, 0),
		size* glm::vec3(38.0, 20.400000000000002, 0),
		size* glm::vec3(39.0, 20.400000000000002, 0),
		size* glm::vec3(41.0, 20.400000000000002, 0),
		size* glm::vec3(42.0, 20.400000000000002, 0),
		size* glm::vec3(43.0, 20.400000000000002, 0),
		size* glm::vec3(3.0, 19.400000000000002, 0),
		size* glm::vec3(4.0, 19.400000000000002, 0),
		size* glm::vec3(5.0, 19.400000000000002, 0),
		size* glm::vec3(8.0, 19.400000000000002, 0),
		size* glm::vec3(9.0, 19.400000000000002, 0),
		size* glm::vec3(10.0, 19.400000000000002, 0),
		size* glm::vec3(13.0, 19.400000000000002, 0),
		size* glm::vec3(14.0, 19.400000000000002, 0),
		size* glm::vec3(15.0, 19.400000000000002, 0),
		size* glm::vec3(16.0, 19.400000000000002, 0),
		size* glm::vec3(17.0, 19.400000000000002, 0),
		size* glm::vec3(18.0, 19.400000000000002, 0),
		size* glm::vec3(19.0, 19.400000000000002, 0),
		size* glm::vec3(20.0, 19.400000000000002, 0),
		size* glm::vec3(22.0, 19.400000000000002, 0),
		size* glm::vec3(24.0, 19.400000000000002, 0),
		size* glm::vec3(26.0, 19.400000000000002, 0),
		size* glm::vec3(28.0, 19.400000000000002, 0),
		size* glm::vec3(29.0, 19.400000000000002, 0),
		size* glm::vec3(30.0, 19.400000000000002, 0),
		size* glm::vec3(31.0, 19.400000000000002, 0),
		size* glm::vec3(32.0, 19.400000000000002, 0),
		size* glm::vec3(36.0, 19.400000000000002, 0),
		size* glm::vec3(39.0, 19.400000000000002, 0),
		size* glm::vec3(41.0, 19.400000000000002, 0),
		size* glm::vec3(42.0, 19.400000000000002, 0),
		size* glm::vec3(43.0, 19.400000000000002, 0),
		size* glm::vec3(3.0, 18.400000000000002, 0),
		size* glm::vec3(4.0, 18.400000000000002, 0),
		size* glm::vec3(5.0, 18.400000000000002, 0),
		size* glm::vec3(13.0, 18.400000000000002, 0),
		size* glm::vec3(14.0, 18.400000000000002, 0),
		size* glm::vec3(15.0, 18.400000000000002, 0),
		size* glm::vec3(16.0, 18.400000000000002, 0),
		size* glm::vec3(17.0, 18.400000000000002, 0),
		size* glm::vec3(18.0, 18.400000000000002, 0),
		size* glm::vec3(19.0, 18.400000000000002, 0),
		size* glm::vec3(22.0, 18.400000000000002, 0),
		size* glm::vec3(23.0, 18.400000000000002, 0),
		size* glm::vec3(24.0, 18.400000000000002, 0),
		size* glm::vec3(25.0, 18.400000000000002, 0),
		size* glm::vec3(27.0, 18.400000000000002, 0),
		size* glm::vec3(28.0, 18.400000000000002, 0),
		size* glm::vec3(29.0, 18.400000000000002, 0),
		size* glm::vec3(30.0, 18.400000000000002, 0),
		size* glm::vec3(31.0, 18.400000000000002, 0),
		size* glm::vec3(32.0, 18.400000000000002, 0),
		size* glm::vec3(37.0, 18.400000000000002, 0),
		size* glm::vec3(38.0, 18.400000000000002, 0),
		size* glm::vec3(41.0, 18.400000000000002, 0),
		size* glm::vec3(42.0, 18.400000000000002, 0),
		size* glm::vec3(3.0, 17.400000000000002, 0),
		size* glm::vec3(4.0, 17.400000000000002, 0),
		size* glm::vec3(5.0, 17.400000000000002, 0),
		size* glm::vec3(14.0, 17.400000000000002, 0),
		size* glm::vec3(16.0, 17.400000000000002, 0),
		size* glm::vec3(17.0, 17.400000000000002, 0),
		size* glm::vec3(18.0, 17.400000000000002, 0),
		size* glm::vec3(19.0, 17.400000000000002, 0),
		size* glm::vec3(20.0, 17.400000000000002, 0),
		size* glm::vec3(22.0, 17.400000000000002, 0),
		size* glm::vec3(24.0, 17.400000000000002, 0),
		size* glm::vec3(25.0, 17.400000000000002, 0),
		size* glm::vec3(26.0, 17.400000000000002, 0),
		size* glm::vec3(27.0, 17.400000000000002, 0),
		size* glm::vec3(28.0, 17.400000000000002, 0),
		size* glm::vec3(29.0, 17.400000000000002, 0),
		size* glm::vec3(30.0, 17.400000000000002, 0),
		size* glm::vec3(31.0, 17.400000000000002, 0),
		size* glm::vec3(32.0, 17.400000000000002, 0),
		size* glm::vec3(36.0, 17.400000000000002, 0),
		size* glm::vec3(37.0, 17.400000000000002, 0),
		size* glm::vec3(42.0, 17.400000000000002, 0),
		size* glm::vec3(4.0, 16.400000000000002, 0),
		size* glm::vec3(10.0, 16.400000000000002, 0),
		size* glm::vec3(15.0, 16.400000000000002, 0),
		size* glm::vec3(17.0, 16.400000000000002, 0),
		size* glm::vec3(18.0, 16.400000000000002, 0),
		size* glm::vec3(19.0, 16.400000000000002, 0),
		size* glm::vec3(20.0, 16.400000000000002, 0),
		size* glm::vec3(22.0, 16.400000000000002, 0),
		size* glm::vec3(24.0, 16.400000000000002, 0),
		size* glm::vec3(25.0, 16.400000000000002, 0),
		size* glm::vec3(26.0, 16.400000000000002, 0),
		size* glm::vec3(27.0, 16.400000000000002, 0),
		size* glm::vec3(28.0, 16.400000000000002, 0),
		size* glm::vec3(29.0, 16.400000000000002, 0),
		size* glm::vec3(30.0, 16.400000000000002, 0),
		size* glm::vec3(31.0, 16.400000000000002, 0),
		size* glm::vec3(36.0, 16.400000000000002, 0),
		size* glm::vec3(40.0, 16.400000000000002, 0),
		size* glm::vec3(41.0, 16.400000000000002, 0),
		size* glm::vec3(42.0, 16.400000000000002, 0),
		size* glm::vec3(4.0, 15.4, 0),
		size* glm::vec3(5.0, 15.4, 0),
		size* glm::vec3(6.0, 15.4, 0),
		size* glm::vec3(8.0, 15.4, 0),
		size* glm::vec3(9.0, 15.4, 0),
		size* glm::vec3(10.0, 15.4, 0),
		size* glm::vec3(16.0, 15.4, 0),
		size* glm::vec3(17.0, 15.4, 0),
		size* glm::vec3(18.0, 15.4, 0),
		size* glm::vec3(19.0, 15.4, 0),
		size* glm::vec3(20.0, 15.4, 0),
		size* glm::vec3(21.0, 15.4, 0),
		size* glm::vec3(22.0, 15.4, 0),
		size* glm::vec3(23.0, 15.4, 0),
		size* glm::vec3(24.0, 15.4, 0),
		size* glm::vec3(25.0, 15.4, 0),
		size* glm::vec3(26.0, 15.4, 0),
		size* glm::vec3(27.0, 15.4, 0),
		size* glm::vec3(28.0, 15.4, 0),
		size* glm::vec3(29.0, 15.4, 0),
		size* glm::vec3(30.0, 15.4, 0),
		size* glm::vec3(35.0, 15.4, 0),
		size* glm::vec3(37.0, 15.4, 0),
		size* glm::vec3(38.0, 15.4, 0),
		size* glm::vec3(40.0, 15.4, 0),
		size* glm::vec3(41.0, 15.4, 0),
		size* glm::vec3(42.0, 15.4, 0),
		size* glm::vec3(4.0, 14.4, 0),
		size* glm::vec3(5.0, 14.4, 0),
		size* glm::vec3(6.0, 14.4, 0),
		size* glm::vec3(19.0, 14.4, 0),
		size* glm::vec3(20.0, 14.4, 0),
		size* glm::vec3(22.0, 14.4, 0),
		size* glm::vec3(23.0, 14.4, 0),
		size* glm::vec3(24.0, 14.4, 0),
		size* glm::vec3(26.0, 14.4, 0),
		size* glm::vec3(27.0, 14.4, 0),
		size* glm::vec3(35.0, 14.4, 0),
		size* glm::vec3(36.0, 14.4, 0),
		size* glm::vec3(37.0, 14.4, 0),
		size* glm::vec3(41.0, 14.4, 0),
		size* glm::vec3(5.0, 13.4, 0),
		size* glm::vec3(9.0, 13.4, 0),
		size* glm::vec3(10.0, 13.4, 0),
		size* glm::vec3(11.0, 13.4, 0),
		size* glm::vec3(19.0, 13.4, 0),
		size* glm::vec3(20.0, 13.4, 0),
		size* glm::vec3(21.0, 13.4, 0),
		size* glm::vec3(25.0, 13.4, 0),
		size* glm::vec3(26.0, 13.4, 0),
		size* glm::vec3(39.0, 13.4, 0),
		size* glm::vec3(40.0, 13.4, 0),
		size* glm::vec3(41.0, 13.4, 0),
		size* glm::vec3(5.0, 12.4, 0),
		size* glm::vec3(6.0, 12.4, 0),
		size* glm::vec3(7.0, 12.4, 0),
		size* glm::vec3(10.0, 12.4, 0),
		size* glm::vec3(11.0, 12.4, 0),
		size* glm::vec3(13.0, 12.4, 0),
		size* glm::vec3(20.0, 12.4, 0),
		size* glm::vec3(21.0, 12.4, 0),
		size* glm::vec3(22.0, 12.4, 0),
		size* glm::vec3(24.0, 12.4, 0),
		size* glm::vec3(25.0, 12.4, 0),
		size* glm::vec3(26.0, 12.4, 0),
		size* glm::vec3(33.0, 12.4, 0),
		size* glm::vec3(34.0, 12.4, 0),
		size* glm::vec3(35.0, 12.4, 0),
		size* glm::vec3(40.0, 12.4, 0),
		size* glm::vec3(41.0, 12.4, 0),
		size* glm::vec3(6.0, 11.4, 0),
		size* glm::vec3(7.0, 11.4, 0),
		size* glm::vec3(8.0, 11.4, 0),
		size* glm::vec3(10.0, 11.4, 0),
		size* glm::vec3(12.0, 11.4, 0),
		size* glm::vec3(13.0, 11.4, 0),
		size* glm::vec3(14.0, 11.4, 0),
		size* glm::vec3(22.0, 11.4, 0),
		size* glm::vec3(23.0, 11.4, 0),
		size* glm::vec3(24.0, 11.4, 0),
		size* glm::vec3(33.0, 11.4, 0),
		size* glm::vec3(35.0, 11.4, 0),
		size* glm::vec3(36.0, 11.4, 0),
		size* glm::vec3(38.0, 11.4, 0),
		size* glm::vec3(39.0, 11.4, 0),
		size* glm::vec3(40.0, 11.4, 0),
		size* glm::vec3(6.0, 10.4, 0),
		size* glm::vec3(7.0, 10.4, 0),
		size* glm::vec3(11.0, 10.4, 0),
		size* glm::vec3(12.0, 10.4, 0),
		size* glm::vec3(15.0, 10.4, 0),
		size* glm::vec3(16.0, 10.4, 0),
		size* glm::vec3(31.0, 10.4, 0),
		size* glm::vec3(32.0, 10.4, 0),
		size* glm::vec3(33.0, 10.4, 0),
		size* glm::vec3(37.0, 10.4, 0),
		size* glm::vec3(38.0, 10.4, 0),
		size* glm::vec3(39.0, 10.4, 0),
		size* glm::vec3(7.0, 9.4, 0),
		size* glm::vec3(8.0, 9.4, 0),
		size* glm::vec3(9.0, 9.4, 0),
		size* glm::vec3(14.0, 9.4, 0),
		size* glm::vec3(15.0, 9.4, 0),
		size* glm::vec3(17.0, 9.4, 0),
		size* glm::vec3(18.0, 9.4, 0),
		size* glm::vec3(29.0, 9.4, 0),
		size* glm::vec3(30.0, 9.4, 0),
		size* glm::vec3(33.0, 9.4, 0),
		size* glm::vec3(36.0, 9.4, 0),
		size* glm::vec3(38.0, 9.4, 0),
		size* glm::vec3(39.0, 9.4, 0),
		size* glm::vec3(8.0, 8.4, 0),
		size* glm::vec3(9.0, 8.4, 0),
		size* glm::vec3(10.0, 8.4, 0),
		size* glm::vec3(14.0, 8.4, 0),
		size* glm::vec3(16.0, 8.4, 0),
		size* glm::vec3(17.0, 8.4, 0),
		size* glm::vec3(19.0, 8.4, 0),
		size* glm::vec3(20.0, 8.4, 0),
		size* glm::vec3(24.0, 8.4, 0),
		size* glm::vec3(26.0, 8.4, 0),
		size* glm::vec3(27.0, 8.4, 0),
		size* glm::vec3(28.0, 8.4, 0),
		size* glm::vec3(29.0, 8.4, 0),
		size* glm::vec3(31.0, 8.4, 0),
		size* glm::vec3(35.0, 8.4, 0),
		size* glm::vec3(37.0, 8.4, 0),
		size* glm::vec3(38.0, 8.4, 0),
		size* glm::vec3(9.0, 7.4, 0),
		size* glm::vec3(10.0, 7.4, 0),
		size* glm::vec3(11.0, 7.4, 0),
		size* glm::vec3(15.0, 7.4, 0),
		size* glm::vec3(17.0, 7.4, 0),
		size* glm::vec3(18.0, 7.4, 0),
		size* glm::vec3(20.0, 7.4, 0),
		size* glm::vec3(21.0, 7.4, 0),
		size* glm::vec3(24.0, 7.4, 0),
		size* glm::vec3(26.0, 7.4, 0),
		size* glm::vec3(27.0, 7.4, 0),
		size* glm::vec3(28.0, 7.4, 0),
		size* glm::vec3(29.0, 7.4, 0),
		size* glm::vec3(31.0, 7.4, 0),
		size* glm::vec3(34.0, 7.4, 0),
		size* glm::vec3(36.0, 7.4, 0),
		size* glm::vec3(37.0, 7.4, 0),
		size* glm::vec3(10.0, 6.4, 0),
		size* glm::vec3(11.0, 6.4, 0),
		size* glm::vec3(13.0, 6.4, 0),
		size* glm::vec3(17.0, 6.4, 0),
		size* glm::vec3(18.0, 6.4, 0),
		size* glm::vec3(21.0, 6.4, 0),
		size* glm::vec3(24.0, 6.4, 0),
		size* glm::vec3(26.0, 6.4, 0),
		size* glm::vec3(33.0, 6.4, 0),
		size* glm::vec3(34.0, 6.4, 0),
		size* glm::vec3(35.0, 6.4, 0),
		size* glm::vec3(36.0, 6.4, 0),
		size* glm::vec3(11.0, 5.4, 0),
		size* glm::vec3(12.0, 5.4, 0),
		size* glm::vec3(13.0, 5.4, 0),
		size* glm::vec3(14.0, 5.4, 0),
		size* glm::vec3(15.0, 5.4, 0),
		size* glm::vec3(25.0, 5.4, 0),
		size* glm::vec3(26.0, 5.4, 0),
		size* glm::vec3(31.0, 5.4, 0),
		size* glm::vec3(33.0, 5.4, 0),
		size* glm::vec3(34.0, 5.4, 0),
		size* glm::vec3(35.0, 5.4, 0),
		size* glm::vec3(12.0, 4.4, 0),
		size* glm::vec3(13.0, 4.4, 0),
		size* glm::vec3(14.0, 4.4, 0),
		size* glm::vec3(15.0, 4.4, 0),
		size* glm::vec3(16.0, 4.4, 0),
		size* glm::vec3(17.0, 4.4, 0),
		size* glm::vec3(29.0, 4.4, 0),
		size* glm::vec3(31.0, 4.4, 0),
		size* glm::vec3(32.0, 4.4, 0),
		size* glm::vec3(33.0, 4.4, 0),
		size* glm::vec3(14.0, 3.4000000000000004, 0),
		size* glm::vec3(15.0, 3.4000000000000004, 0),
		size* glm::vec3(16.0, 3.4000000000000004, 0),
		size* glm::vec3(17.0, 3.4000000000000004, 0),
		size* glm::vec3(20.0, 3.4000000000000004, 0),
		size* glm::vec3(21.0, 3.4000000000000004, 0),
		size* glm::vec3(22.0, 3.4000000000000004, 0),
		size* glm::vec3(23.0, 3.4000000000000004, 0),
		size* glm::vec3(24.0, 3.4000000000000004, 0),
		size* glm::vec3(25.0, 3.4000000000000004, 0),
		size* glm::vec3(26.0, 3.4000000000000004, 0),
		size* glm::vec3(28.0, 3.4000000000000004, 0),
		size* glm::vec3(29.0, 3.4000000000000004, 0),
		size* glm::vec3(30.0, 3.4000000000000004, 0),
		size* glm::vec3(31.0, 3.4000000000000004, 0),
		size* glm::vec3(32.0, 3.4000000000000004, 0),
		size* glm::vec3(16.0, 2.4000000000000004, 0),
		size* glm::vec3(17.0, 2.4000000000000004, 0),
		size* glm::vec3(18.0, 2.4000000000000004, 0),
		size* glm::vec3(19.0, 2.4000000000000004, 0),
		size* glm::vec3(20.0, 2.4000000000000004, 0),
		size* glm::vec3(21.0, 2.4000000000000004, 0),
		size* glm::vec3(22.0, 2.4000000000000004, 0),
		size* glm::vec3(23.0, 2.4000000000000004, 0),
		size* glm::vec3(24.0, 2.4000000000000004, 0),
		size* glm::vec3(25.0, 2.4000000000000004, 0),
		size* glm::vec3(26.0, 2.4000000000000004, 0),
		size* glm::vec3(27.0, 2.4000000000000004, 0),
		size* glm::vec3(28.0, 2.4000000000000004, 0),
		size* glm::vec3(29.0, 2.4000000000000004, 0),
		size* glm::vec3(20.0, 1.4000000000000001, 0),
		size* glm::vec3(21.0, 1.4000000000000001, 0),
		size* glm::vec3(22.0, 1.4000000000000001, 0),
		size* glm::vec3(23.0, 1.4000000000000001, 0),
		size* glm::vec3(24.0, 1.4000000000000001, 0),
		size* glm::vec3(25.0, 1.4000000000000001, 0),
	};


	// 定义随机爆炸烟花的火花位置
	//float randSize = getRandomNumber(0, explosionSpread); // 随机扩散范围

	// 生成 "A" 类型的火花粒子
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
	/*
	// 生成随机类型的火花粒子
	for (int i = countA; i < sparklesPerExplosion; i++)
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

	}*/
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

// 发射喷泉
void Launcher::launchFountain()
{
	for (int i = 0; i < fountainDensity; i++)
	{
		// 随机生成喷泉粒子的速度和位置
		float randX = getRandomNumber(-fountainSpread, fountainSpread);
		float randZ = getRandomNumber(-fountainSpread, fountainSpread);
		float randY = getRandomNumber(-10.0f, 10.0f);

		// 生成喷泉粒子
		spawnParticle(
			position,
			glm::vec3(randX, fountainSpeed + randY, randZ), // 速度
			glm::vec4(fountainColor, 1.0f), // 颜色
			fountainSize, // 大小
			fountainLife, // 生命期
			Particle::Type::FOUNTAIN // 类型为喷泉
		);
	}
}

// 更新发射器状态
void Launcher::update(Camera& camera, GLfloat* particle_position, GLubyte* particle_color)
{
	float deltaTime = Camera::getDeltaTime(); // 获取时间增量

	launchTime -= deltaTime; // 减少发射时间
	if (launchTime <= 0)
	{
		launchFirework(); // 发射烟花
		launchTime = launchDelay; // 重置发射延迟
	}

	fountainTime -= deltaTime; // 减少喷泉时间
	if (fountainTime <= 0)
	{
		launchFountain(); // 发射喷泉
		fountainTime = fountainDelay; // 重置喷泉延迟
	}

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

// 排序粒子
void Launcher::sortParticles() {
	std::sort(&particles[0], &particles[maxParticles]); // 按与相机的距离排序
}

// 查找未使用的粒子
int Launcher::findUnusedParticle() {
	for (int i = lastUsedId; i < maxParticles; i++) {
		if (particles[i].life < 0) { // 如果生命期小于0，表示未使用
			lastUsedId = i; // 更新上一个使用的粒子ID
			return i; // 返回该粒子的索引
		}
	}
	//优化，从lastUsedId往后找没用过的，再找之前用完的。
	for (int i = 0; i < lastUsedId; i++) {
		if (particles[i].life < 0) { // 如果生命期小于0，表示未使用
			lastUsedId = i; // 更新上一个使用的粒子ID
			return i; // 返回该粒子的索引
		}
	}

	return 0; // 默认返回0
}

// 渐变颜色
glm::vec3 fadeColor(glm::vec3 startColor, glm::vec3 destColor, float ratio)
{
	return {
		(ratio * startColor.r + (1.0f - ratio) * destColor.r) / 1.0f, // 红色分量渐变
		(ratio * startColor.g + (1.0f - ratio) * destColor.g) / 1.0f, // 绿色分量渐变
		(ratio * startColor.b + (1.0f - ratio) * destColor.b) / 1.0f // 蓝色分量渐变
	};
}

// 获取指定范围内的随机数
float getRandomNumber(float min, float max)
{
	return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min; // 生成随机数
}

// 获取随机明亮颜色
glm::vec4 getRandomBrightColor()
{
	auto rgb = glm::vec3(0.0f, 0.0f, 0.0f); // 初始化RGB
	unsigned char region, remainder, p, q, t;

	float h = getRandomNumber(0, 256); // 随机色相
	unsigned char s = 255; // 饱和度
	unsigned char v = 255; // 明度

	region = h / 43; // 确定色相区域
	remainder = (h - (region * 43)) * 6; // 计算余数

	// 计算RGB分量
	p = (v * (255 - s)) >> 8;
	q = (v * (255 - ((s * remainder) >> 8))) >> 8;
	t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

	// 根据区域设置颜色
	switch (region)
	{
	case 0:
		rgb.r = v; rgb.g = t; rgb.b = p;
		break;
	case 1:
		rgb.r = q; rgb.g = v; rgb.b = p;
		break;
	case 2:
		rgb.r = p; rgb.g = v; rgb.b = t;
		break;
	case 3:
		rgb.r = p; rgb.g = q; rgb.b = v;
		break;
	case 4:
		rgb.r = t; rgb.g = p; rgb.b = v;
		break;
	default:
		rgb.r = v; rgb.g = p; rgb.b = q;
		break;
	}

	return glm::vec4(rgb, 1.0f); // 返回带有透明度的颜色
}