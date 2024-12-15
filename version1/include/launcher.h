#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <array>
#include <memory>

#include "camera.h"
#include "Constant.h"
#include "PointLight.h"

static const unsigned int maxParticles = 70000;
static const float GRAVITY = 9.81f;

static const char* explosionSounds[5] = { 
	"version1/sounds/explosion1.wav", 
	"version1/sounds/explosion2.wav", 
	"version1/sounds/explosion3.wav", 
	"version1/sounds/explosion4.wav", 
	"version1/sounds/explosion5.wav" 
};

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
	void explode2(Particle& p);
	void launchFirework();
	void launchFountain();

	void simulate(Camera &camera, GLfloat* particle_position, GLubyte* particle_color);
	void update(Camera &camera, GLfloat* particle_position, GLubyte* particle_color);

	void sortParticles();
	int findUnusedParticle();

	static unsigned int particlesCount;

private:
	float launchDelay = 0.25f;       /* delay between each launch (seconds) */
	float trailDelay = 0.04f;        /* delay between each trail spawn (seconds) */
	float fountainDelay = 0.05f;

	int sparklesPerExplosion = 300; /* Number of sparkles after explosion */
	float sparkleLife = 1.2f;       /* lifetime of a sparkle (seconds) */
	float sparkleSize = 1.3f;

	float trailSize = 1.6f;         /* size of a trail */
	float trailLife = 0.6f;         /* lifetime of a trail (seconds) */

	float rocketSize = 0.6f;        /* size of a rocket */
	float rocketLife = 5.5f;        /* lifetime of a rocket before exploding (seconds) 8.5f */

	int fountainDensity = 25;
	float fountainSize = 1.5f;
	float fountainLife = 8.0f;
	float fountainSpeed = 50.0f;
	float fountainSpread = 5.0f;

	/* Colors */
	glm::vec3 fountainColor = glm::vec3(255.0f, 247.0f, 0.0f);
	glm::vec3 fountainFade = glm::vec3(204.0f, 51.0f, 0.0f);
	glm::vec3 trailFade = glm::vec3(0.0f, 0.0f, 0.0f);

	/* Spread radius */
	float explosionSize = 30.0f;    /* size of explosion blow */
	float explosionSpread = 10.0f;   /* spread of explosion blow */
	float launchSpread = 40.0f;     /* spread of launch direction */
	float launchSpeed = 160.0f;      /* speed of launch direction */

	/* PointLights: YuZhuZhi */
	std::array<std::shared_ptr<PointLight>, MAX_LIGHTS> pointLights;
	std::shared_ptr<Shader> shader;

	/* Do not touch */
	std::unique_ptr<Particle[]> particles{ new Particle[maxParticles] };
	glm::vec3 position;
	int lastUsedId = 0;
	float launchTime = 0.0f, fountainTime = 0.0f;
	float trailTime = 0.0f;
};
