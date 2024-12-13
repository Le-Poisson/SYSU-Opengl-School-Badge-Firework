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

# 遍历图像，按照 10 像素的间隔提取白色像素点
for y in range(0, height, 10):
    for x in range(0, width, 10):
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