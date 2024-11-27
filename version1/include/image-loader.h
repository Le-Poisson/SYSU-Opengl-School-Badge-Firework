#pragma once

#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <fstream>
#include <iostream>

#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>


class ImageLoader
{
public:
	ImageLoader();
	GLuint loadBMP_custom(const char* imagepath);
	GLuint loadPNG(const char* imagepath);
	GLuint loadJPG(const char* imagepath);

	unsigned int getWidth() const;
	unsigned int getHeight() const;
	unsigned char* getData() const;

private:
	unsigned char header[54];
	unsigned int dataPos;
	int width, height;
	unsigned int imageSize;
	unsigned char* data;
};


struct Transform {
	// https://www.youtube.com/watch?v=G25aT8VFsNI
	glm::vec3 position = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);
	glm::vec3 scale = glm::vec3(1);
	glm::mat4 to_mat4() {
		// pravim translate matricu 4x4 sa identity matricom
		glm::mat4 m = glm::translate(glm::mat4(1), position);
		// print(m)
		// quaternion matrix odradi sve sto bi uradile x,y,z rotatcione matrice ali radi sa uglovima (zato imam jedan vec3 rotation)
		// prakticno orientation matrix
		m *= glm::mat4_cast(glm::quat(rotation));
		// scale klasika
		m = glm::scale(m, scale);
		return m;
	};

	// transformatio matrix
};


struct Texture {

	unsigned int ID = 0;
	int width = 0;
	int height = 0;

	void Load(std::string filepath) {
		stbi_set_flip_vertically_on_load(false);
		int channelCount = 0;
		unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channelCount, 0);
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_2D, ID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		GLint format = GL_RGB;
		if (channelCount == 4)
			format = GL_RGBA;
		if (channelCount == 1)
			format = GL_RED;
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
			std::cout << "Failed to load texture: " << filepath << "\n";
		stbi_image_free(data);
	}
};



struct CubemapTexutre {

	GLuint ID;

	void Create(std::vector<std::string>& textures) {

		// pravimo id za texturu i bindujemo GL_TEXTURE_CUBE_MAP na taj id - sve ide na ovaj texture u buducnosti
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
		int width, height, nrChannels;
		// idemo kroz svih 6 slika
		for (unsigned int i = 0; i < textures.size(); i++) {
			// loadujemo datu
			unsigned char* data = stbi_load(textures[i].c_str(), &width, &height, &nrChannels, 0);
			// odredjujemo format u kom je (rgb, rgba, gray)
			if (data) {
				GLint format = GL_RGB;
				if (nrChannels == 4)
					format = GL_RGBA;
				if (nrChannels == 1)
					format = GL_RED;
				// samo svaku sliku incrementujem za 1 (definovani su sa razlikom od 1, tkd to mi super odgovara)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
				// kad ga loadujemo mozemo da freeujemo memoriju
				stbi_image_free(data);
			}
			else {
				std::cout << "Failed to load cubemap\n";
				stbi_image_free(data);
			}
		}
		// interpuje na edgevima izmedju preklapanja slika, da nema gapova
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		// kad smanjujemo / povecavamo sliku linearnu iterpolaciju koristimo
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// samo ovde x, y, z clampujemo da bi imali lepse edgeve oko ivica (da se ne ponavlja slika oko ivica...)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
};

struct HeightMap {

	GLuint vao = 0;
	GLuint indexCount = 0;

	void Load(std::string filepath) {

		// ovde zelimo za height mapu da odradimo vertices i indices(da bi imali manje verticesa)
		std::vector<glm::vec3> vertices;
		std::vector<unsigned int> indices;
		unsigned char* data = nullptr;
		int channelCount = 0;
		int width = 0;
		int height = 0;

		stbi_set_flip_vertically_on_load(false);
		// nasetujemo height, width, channel count, na ovaj od date
		data = stbi_load(filepath.c_str(), &width, &height, &channelCount, 0);
		// scale i shift isti kao na guide-u
		// yscale normalizuje da ide od 0 do 64 zato delimo sa 4 (normalizacija + scale na 64)
		// shift samo kaze +-  range scale-a
		float yScale = 64.0f / 256.0f, yShift = 16.0f;
		int rez = 1;
		unsigned bytePerPixel = channelCount;
		// broj kanal rgb(4)/ grascale(1) itd, ali nama treba samo prva vrednost za y
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				unsigned char* pixelOffset = data + (j + width * i) * bytePerPixel;
				unsigned char y = pixelOffset[0];
				glm::vec3 vertex;
				// ide u x,z osi od x =  -width/2 i z = -height/2
				vertex.x = (-height / 2.0f + height * i / (float)height);
				vertex.y = ((int)y * yScale - yShift);
				vertex.z = (-width / 2.0f + width * j / (float)width);
				vertices.push_back(vertex);
			}
		}

		// kad izbildujemo ove mozemo da free upujemo datu
		stbi_image_free(data);

		// ovde prolazimo kroz sve idx i stavljamo u ovaj vektor
		for (unsigned i = 0; i < height - 1; i += rez) {
			for (unsigned j = 0; j < width; j += rez) {
				for (unsigned k = 0; k < 2; k++) {
					indices.push_back(j + width * (i + k * rez));
				}
			}
		}
		indexCount = indices.size();

		GLuint vbo;
		GLuint ebo;

		// dajemo mu id
		glGenVertexArrays(1, &vao);
		// pravimo buffere za vert i idx info
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);
		// bindujemo (sve dalje ide na ovaj vao, vbo i ebo su povezani sa ovim vao ciji smo id uzeli)
		glBindVertexArray(vao);
		//vbo mi je array buffer i u njega posle stavljam sve vertixe (po 3 za svaki trougao)
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		// ebo su indexi (svaka 3 su jedan trougao ali idu redom kao element array)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		// layout 0
		glEnableVertexAttribArray(0);
		// ovo kaze kako da ga cita 3 kordinate i sizeof je vec3 jer samo storujemo position (bez normalize(vec3) i uv(vec2))kao sto smo videli gore 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	}
};