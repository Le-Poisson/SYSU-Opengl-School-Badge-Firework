#include "launcher.h"
#include <irrKlang\include/irrKlang.h>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace irrklang;

// ������������
ISoundEngine* soundEngine = createIrrKlangDevice();

// ��ȡָ����Χ�ڵ������
float getRandomNumber(float min, float max);
// ������ɫ
glm::vec3 fadeColor(glm::vec3 startColor, glm::vec3 destColor, float ratio);
// ��ȡ���������ɫ
glm::vec4 getRandomBrightColor();

// ��������������������
auto gravityForce = glm::vec3(0.0f, -GRAVITY, 0.0f);
auto noForce = glm::vec3(1.0f, 1.0f, 1.0f);

// ���������캯������ʼ��λ��Ϊԭ��
Launcher::Launcher()
	: Launcher(glm::vec3(0.0f, 20.0f, 0.0f))
{}

// ��λ�õķ��������캯��
Launcher::Launcher(glm::vec3 position)
	: position(position)
{
	for (int i = 0; i < maxParticles; i++)
	{
		// ��ʼ�����ӵ�Ĭ��ֵ
		particles[i].life = -1.0f; // ������Ϊ������ʾδʹ��
		particles[i].cameraDst = -1.0f; // ������ľ���Ϊ��
		particles[i].trailTime = trailDelay; // ��ʼ����βʱ��
		particles[i].type = Particle::Type::DEAD; // ��ʼ��Ϊ��������
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
int counter = 0; //ÿ 5 ���̻���ը����һ��ͼ��

// ģ�����ӵ��˶�
void Launcher::simulate(Camera& camera, GLfloat* particle_position, GLubyte* particle_color)
{
	float deltaTime = Camera::getDeltaTime(); // ��ȡʱ������
	particlesCount = 0; // �������Ӽ���

	

	for (int i = 0; i < maxParticles; i++)
	{
		// ģ��ÿ������
		Particle& p = particles[i];

		if (p.life > 0.0f)
		{
			// ������Ȼ���
			p.speed += gravityForce * deltaTime; // ������Ӱ��
			p.pos += p.speed * deltaTime; // ����λ��
			p.cameraDst = glm::distance(p.pos, camera.getPosition()); // ����������ľ���

			renderTrails(p, deltaTime); // ��Ⱦ��β

			// ���GPU������
			particle_position[4 * particlesCount + 0] = p.pos.x;
			particle_position[4 * particlesCount + 1] = p.pos.y;
			particle_position[4 * particlesCount + 2] = p.pos.z;

			float lifeRatio = 1.0f; // ��������
			auto newColor = glm::vec3(p.r, p.g, p.b); // ��ǰ��ɫ
			switch (p.type)
			{
			case Particle::Type::SPARKLE:
				lifeRatio = p.life / sparkleLife; // �𻨵���������
				break;
			case Particle::Type::TRAIL:
				lifeRatio = p.life / trailLife; // ��β����������
				newColor = fadeColor(newColor, trailFade, lifeRatio); // ������ɫ
				break;
			case Particle::Type::FOUNTAIN:
				lifeRatio = p.life / fountainLife; // ��Ȫ����������
				newColor = fadeColor(fountainColor, fountainFade, lifeRatio); // ������ɫ
				break;
			}
			particle_position[4 * particlesCount + 3] = p.size * lifeRatio; // ���Ӵ�С

			particle_color[4 * particlesCount + 0] = newColor.r; // ��ɫ����
			particle_color[4 * particlesCount + 1] = newColor.g; // ��ɫ����
			particle_color[4 * particlesCount + 2] = newColor.b; // ��ɫ����
			particle_color[4 * particlesCount + 3] = p.life; // ����ֵ

			particlesCount++; // �������Ӽ���
			p.life -= deltaTime; // ��������ֵ
			continue;
		}

		// ���Ӹո�����
		if (p.type == Particle::Type::LAUNCHING){
			counter++;
			if(counter==5){
				counter = 0;
				explode(p); // ����ը
			}
			else {
				explode2(p);
			}
		}

		p.type = Particle::Type::DEAD; // ����Ϊ����

		p.cameraDst = -1.0f; // ����������ľ���
	}
}

// ��Ⱦ���ӵ���β
void Launcher::renderTrails(Particle& p, float deltaTime)
{
	if (p.trailTime <= 0 && (p.type == Particle::Type::SPARKLE || p.type == Particle::Type::LAUNCHING))
	{
		// ������β����
		spawnParticle(
			p.pos,
			glm::vec3(0.0f), // ��β���ӳ��ٶ�Ϊ0
			glm::vec4(p.r, p.g, p.b, p.a), // ��β��ɫ
			trailSize, // ��β��С
			trailLife, // ��β������
			Particle::Type::TRAIL // ����Ϊ��β
		);
		p.trailTime = trailDelay; // ������βʱ��
	}
	p.trailTime -= deltaTime; // ������βʱ��
}

// ��������
void Launcher::spawnParticle(glm::vec3 position, glm::vec3 speed, glm::vec4 color, float size, float life, Particle::Type type)
{
	int idx = findUnusedParticle(); // ����δʹ�õ�����

	// �������ӵ�����
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


// ����ը
void Launcher::explode2(Particle& p)
{
	int randomSound = getRandomNumber(1, 6); // ���ѡ��ը����
	soundEngine->play2D(explosionSounds[randomSound - 1]); // ���ű�ը����

	// ��ӵ��Դ Add: YuZhuZhi
	auto pointLight = std::make_shared<PointLight>(
		Color(p.r, p.g, p.b), // �����ɫ
		p.pos,                   // ���λ��
		Attenuation(1.0f, 0.014f, 0.007f), // ���˥������
		sparkleLife + 0.75
	);
	auto it = std::find(pointLights.begin(), pointLights.end(), nullptr);
	if (it != pointLights.end()) { // ��ӵ����й���
		*it = pointLight;
		pointLight->addToShader(shader, it - pointLights.begin());
	}

	float randSize = getRandomNumber(0, explosionSpread); // �����ɢ��Χ
	for (int i = 0; i < sparklesPerExplosion; i++)
	{
		// ������ɻ�λ�ú��ٶ�
		float randX = getRandomNumber(-1, 2);
		float randY = getRandomNumber(-1, 2);
		float randZ = getRandomNumber(-1, 2);
		float randSpread = getRandomNumber(0, explosionSpread);
		float randLife = getRandomNumber(0, 1.5f);

		// ���ɻ�����
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
	int countA = 600;  // ����A�̻�������
	int randomSound = getRandomNumber(1, 6); // ���ѡ��ը����
	soundEngine->play2D(explosionSounds[randomSound - 1]); // ���ű�ը����

	// ��ӵ��Դ
	auto pointLight = std::make_shared<PointLight>(
		Color(p.r, p.g, p.b), // �����ɫ
		p.pos,                   // ���λ��
		Attenuation(1.0f, 0.014f, 0.007f), // ���˥������
		sparkleLife + 0.75
	);
	auto it = std::find(pointLights.begin(), pointLights.end(), nullptr);
	if (it != pointLights.end()) { // ��ӵ����й���
		*it = pointLight;
		pointLight->addToShader(shader, it - pointLights.begin());
	}

	// ���� "A" ��ĸ�Ļ�λ��
	float size = 1.0f; // ͼ����С
	glm::vec3 basePos = p.pos; // ����λ��

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


	// ���������ը�̻��Ļ�λ��
	//float randSize = getRandomNumber(0, explosionSpread); // �����ɢ��Χ

	// ���� "A" ���͵Ļ�����
	for (int i = 0; i < countA; i++)
	{
		const auto& pos = pattern[i % pattern.size()]; // ѭ��ʹ��λ��

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
	// ����������͵Ļ�����
	for (int i = countA; i < sparklesPerExplosion; i++)
	{
		// ������ɻ�λ�ú��ٶ�
		float randX = getRandomNumber(-1, 2);
		float randY = getRandomNumber(-1, 2);
		float randZ = getRandomNumber(-1, 2);
		float randSpread = getRandomNumber(0, explosionSpread);
		float randLife = getRandomNumber(0, 1.5f);

		// ���ɻ�����
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


// �����̻�
void Launcher::launchFirework()
{
	float randX = getRandomNumber(-launchSpread, launchSpread); // ���Xƫ��
	float randZ = getRandomNumber(-launchSpread, launchSpread); // ���Zƫ��
	float rand = getRandomNumber(-150.0f, 150.0f); // ���Yƫ��

	// ���ɻ������
	spawnParticle(
		position + glm::vec3(rand, 50.0, rand), // ����λ��
		glm::vec3(randX, getRandomNumber(launchSpeed, launchSpeed + 25), randZ), // �����ٶ�
		getRandomBrightColor(), // �����ɫ
		rocketSize, // �����С
		rocketLife, // ���������
		Particle::Type::LAUNCHING // ����Ϊ������
	);
}

// ������Ȫ
void Launcher::launchFountain()
{
	for (int i = 0; i < fountainDensity; i++)
	{
		// ���������Ȫ���ӵ��ٶȺ�λ��
		float randX = getRandomNumber(-fountainSpread, fountainSpread);
		float randZ = getRandomNumber(-fountainSpread, fountainSpread);
		float randY = getRandomNumber(-10.0f, 10.0f);

		// ������Ȫ����
		spawnParticle(
			position,
			glm::vec3(randX, fountainSpeed + randY, randZ), // �ٶ�
			glm::vec4(fountainColor, 1.0f), // ��ɫ
			fountainSize, // ��С
			fountainLife, // ������
			Particle::Type::FOUNTAIN // ����Ϊ��Ȫ
		);
	}
}

// ���·�����״̬
void Launcher::update(Camera& camera, GLfloat* particle_position, GLubyte* particle_color)
{
	float deltaTime = Camera::getDeltaTime(); // ��ȡʱ������

	launchTime -= deltaTime; // ���ٷ���ʱ��
	if (launchTime <= 0)
	{
		launchFirework(); // �����̻�
		launchTime = launchDelay; // ���÷����ӳ�
	}

	fountainTime -= deltaTime; // ������Ȫʱ��
	if (fountainTime <= 0)
	{
		launchFountain(); // ������Ȫ
		fountainTime = fountainDelay; // ������Ȫ�ӳ�
	}

	simulate(camera, particle_position, particle_color); // ģ������

	for (auto light = pointLights.begin(); light != pointLights.end(); light++) { // Add: YuZhuZhi
		if (light->get() && !(light->get()->updateLife(deltaTime))) { // ɾ�����Դ
			int index = light - pointLights.begin();
			light->get()->deleteFromShader(shader, index);
			pointLights[index] = nullptr;
		}
	}

	sortParticles(); // ��������
}

// ��������
void Launcher::sortParticles() {
	std::sort(&particles[0], &particles[maxParticles]); // ��������ľ�������
}

// ����δʹ�õ�����
int Launcher::findUnusedParticle() {
	for (int i = lastUsedId; i < maxParticles; i++) {
		if (particles[i].life < 0) { // ���������С��0����ʾδʹ��
			lastUsedId = i; // ������һ��ʹ�õ�����ID
			return i; // ���ظ����ӵ�����
		}
	}
	//�Ż�����lastUsedId������û�ù��ģ�����֮ǰ����ġ�
	for (int i = 0; i < lastUsedId; i++) {
		if (particles[i].life < 0) { // ���������С��0����ʾδʹ��
			lastUsedId = i; // ������һ��ʹ�õ�����ID
			return i; // ���ظ����ӵ�����
		}
	}

	return 0; // Ĭ�Ϸ���0
}

// ������ɫ
glm::vec3 fadeColor(glm::vec3 startColor, glm::vec3 destColor, float ratio)
{
	return {
		(ratio * startColor.r + (1.0f - ratio) * destColor.r) / 1.0f, // ��ɫ��������
		(ratio * startColor.g + (1.0f - ratio) * destColor.g) / 1.0f, // ��ɫ��������
		(ratio * startColor.b + (1.0f - ratio) * destColor.b) / 1.0f // ��ɫ��������
	};
}

// ��ȡָ����Χ�ڵ������
float getRandomNumber(float min, float max)
{
	return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min; // ���������
}

// ��ȡ���������ɫ
glm::vec4 getRandomBrightColor()
{
	auto rgb = glm::vec3(0.0f, 0.0f, 0.0f); // ��ʼ��RGB
	unsigned char region, remainder, p, q, t;

	float h = getRandomNumber(0, 256); // ���ɫ��
	unsigned char s = 255; // ���Ͷ�
	unsigned char v = 255; // ����

	region = h / 43; // ȷ��ɫ������
	remainder = (h - (region * 43)) * 6; // ��������

	// ����RGB����
	p = (v * (255 - s)) >> 8;
	q = (v * (255 - ((s * remainder) >> 8))) >> 8;
	t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

	// ��������������ɫ
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

	return glm::vec4(rgb, 1.0f); // ���ش���͸���ȵ���ɫ
}