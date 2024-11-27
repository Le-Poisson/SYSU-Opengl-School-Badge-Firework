#include "image-loader.h"
#include <fstream>



// 构造函数
ImageLoader::ImageLoader() {}

// 自定义加载 BMP 图像
GLuint ImageLoader::loadBMP_custom(const char* imagepath)
{	
	// 打开 BMP 文件
	FILE* file;
	fopen_s(&file, imagepath, "rb");
	if (!file) { printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); return 0; } // 如果不是 BMP 文件

	// 读取 BMP 文件头（54 字节）
	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		return 0;
	}

	// 检查文件头标识
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}

	dataPos = *(int*)&(header[0x0A]); // 数据起始位置
	imageSize = *(int*)&(header[0x22]); // 图像数据大小
	width = *(int*)&(header[0x12]); // 图像宽度
	height = *(int*)&(header[0x16]); // 图像高度

	// 如果图像大小为0，计算大小
	if (imageSize == 0)
		imageSize = width * height * 3; // 每个像素 3 字节（RGB）
	
	// 如果数据位置为0，则设置为54
	if (dataPos == 0)
		dataPos = 54;

	// 分配内存以存储图像数据
	data = new unsigned char[imageSize];
	fread(data, 1, imageSize, file);
	fclose(file);

	// 生成纹理 ID
	GLuint textureId;
	glGenTextures(1, &textureId); // 生成纹理
	glBindTexture(GL_TEXTURE_2D, textureId); // 绑定纹理
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data); // 设置纹理图像
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // 设置放大过滤
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // 设置缩小过滤

	delete[] data; // 释放图像数据内存

	return textureId; // 返回纹理 ID
}

// 加载 JPG 图像
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

	stbi_image_free(imgData); // 释放图像内存
	return textureId; // 返回纹理 ID
}

// 加载 PNG 图像
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

	stbi_image_free(imgData); // 释放图像内存
	return textureId; // 返回纹理 ID
}


// 获取图像宽度
unsigned int ImageLoader::getWidth() const 
{
	return width; // 返回宽度
}

// 获取图像高度
unsigned int ImageLoader::getHeight() const
{
	return height; // 返回高度
}

// 获取图像数据
unsigned char* ImageLoader::getData() const
{
	return data;// 返回图像数据
}