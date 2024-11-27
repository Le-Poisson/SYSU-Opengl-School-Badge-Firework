#include "image-loader.h"
#include <fstream>



// ���캯��
ImageLoader::ImageLoader() {}

// �Զ������ BMP ͼ��
GLuint ImageLoader::loadBMP_custom(const char* imagepath)
{	
	// �� BMP �ļ�
	FILE* file;
	fopen_s(&file, imagepath, "rb");
	if (!file) { printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); return 0; } // ������� BMP �ļ�

	// ��ȡ BMP �ļ�ͷ��54 �ֽڣ�
	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		return 0;
	}

	// ����ļ�ͷ��ʶ
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}

	dataPos = *(int*)&(header[0x0A]); // ������ʼλ��
	imageSize = *(int*)&(header[0x22]); // ͼ�����ݴ�С
	width = *(int*)&(header[0x12]); // ͼ����
	height = *(int*)&(header[0x16]); // ͼ��߶�

	// ���ͼ���СΪ0�������С
	if (imageSize == 0)
		imageSize = width * height * 3; // ÿ������ 3 �ֽڣ�RGB��
	
	// �������λ��Ϊ0��������Ϊ54
	if (dataPos == 0)
		dataPos = 54;

	// �����ڴ��Դ洢ͼ������
	data = new unsigned char[imageSize];
	fread(data, 1, imageSize, file);
	fclose(file);

	// �������� ID
	GLuint textureId;
	glGenTextures(1, &textureId); // ��������
	glBindTexture(GL_TEXTURE_2D, textureId); // ������
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data); // ��������ͼ��
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // ���÷Ŵ����
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // ������С����

	delete[] data; // �ͷ�ͼ�������ڴ�

	return textureId; // �������� ID
}

// ���� JPG ͼ��
GLuint ImageLoader::loadJPG(const char* imagepath) {
	unsigned char* imgData = stbi_load(imagepath, &width, &height, nullptr, 3);
	if (!imgData) {
		std::cerr << "Failed to load JPG image: " << imagepath << std::endl;
		return 0;
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imgData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	stbi_image_free(imgData); // �ͷ�ͼ���ڴ�
	return textureId; // �������� ID
}

// ���� PNG ͼ��
GLuint ImageLoader::loadPNG(const char* imagepath) {
	unsigned char* imgData = stbi_load(imagepath, &width, &height, nullptr, 4);
	if (!imgData) {
		std::cerr << "Failed to load PNG image: " << imagepath << std::endl;
		return 0;
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	stbi_image_free(imgData); // �ͷ�ͼ���ڴ�
	return textureId; // �������� ID
}


// ��ȡͼ����
unsigned int ImageLoader::getWidth() const 
{
	return width; // ���ؿ��
}

// ��ȡͼ��߶�
unsigned int ImageLoader::getHeight() const
{
	return height; // ���ظ߶�
}

// ��ȡͼ������
unsigned char* ImageLoader::getData() const
{
	return data;// ����ͼ������
}