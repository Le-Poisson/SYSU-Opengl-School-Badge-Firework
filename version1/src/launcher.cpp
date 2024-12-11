#include "launcher.h"
#include "irrKlang/include/irrKlang.h"

#include <glm/gtx/string_cast.hpp>
#include <algorithm>
#include <iostream>

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
		if (p.type == Particle::Type::LAUNCHING)
			explode(p); // ����ը

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
void Launcher::explode(Particle& p)
{
	int randomSound = getRandomNumber(1, 6); // ���ѡ��ը����
	soundEngine->play2D(explosionSounds[randomSound - 1]); // ���ű�ը����

	// ��ӵ��Դ Add: YuZhuZhi
	auto pointLight = std::make_shared<PointLight>(
		Color(p.r, p.g, p.b), // �����ɫ
		p.pos,                   // ���λ��
		Attenuation(1.0f, 0.007f, 0.0002f), // ���˥������
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