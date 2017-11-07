该仓库中内容是笔者研究了几种背景提取算法后，根据其原理调用了OpenCV库写出的C++代码。
笔者研究的几种背景提取算法有帧间差分法、背景差分法、ViBe背景提取算法、ViBe+ 背景提取算法。

# 一、原理解释
对几种背景提取算法的研究，笔者写了博客。地址如下：  
[《背景提取算法——帧间差分法、背景差分法、ViBe背景提取算法》](http://blog.csdn.net/ajianyingxiaoqinghan/article/details/72628402)  
[《论文翻译：ViBe+算法（ViBe算法的改进版本）》](http://blog.csdn.net/ajianyingxiaoqinghan/article/details/72782685)  
ViBe+ 算法原论文地址：  
[《Background Subtraction: Experiments and Improvements for ViBe》](http://orbi.ulg.ac.be/bitstream/2268/117561/1/VanDroogenbroeck2012Background.pdf)  

# 二、文件说明

- src：源代码所在路径
	- FramesDifference：帧间差分法源码
	- BGDifference：背景差分法源码
	- ViBe：ViBe 背景提取算法源码
	- ViBe+: ViBe+ 背景提取算法源码
- Image： 测试截图
- Video：测试使用视频
- CMakeLists.txt：该工程的CMake文件

# 三、工程生成教程
## 1. 笔者的工作环境：

- 操作系统：Ubuntu 14.04 LTS
- 编译条件：
	- 已编译且安装OpenCV
	- 已安装CMake

关于Ubuntu 14.04下OpenCV的安装，笔者写的教程如下：  
CSDN：http://blog.csdn.net/ajianyingxiaoqinghan/article/details/62424132   
GitHub：https://github.com/upcAutoLang/Blog/issues/1  

## 2. CMake该项目
进入终端，进入GLCM_OpenCV路径，输入以下指令：  
```bash
cmake ./
make
```
即可编译该工程。  
生成文件路径：/GLCM_OpenCV/bin  
生成库文件路径：/GLCM_OpenCV/lib  

# 四、测试效果
分别用三种算法对/BackgroundSplit-OpenCV/Video/Camera Road 01.avi做测试：  
帧间差分法结果：
![](./Image/FrameDifference.png)
背景差分法结果：
![](./Image/GaussBG_Difference.png)
ViBe算法结果：
![](./Image/ViBe.png)
ViBe+ 算法结果：
![](./Image/ViBe+.jpg)

**注：**  
**1. ViBe算法的效率：**  
Debug版本下，测试程序中计算出的该算法效率输出如下：  
```cpp
Time of Update ViBe Background: 15.5914ms
Time of Update ViBe Background: 15.7827ms
Time of Update ViBe Background: 15.2309ms
Time of Update ViBe Background: 15.3791ms
Time of Update ViBe Background: 16.5063ms
Time of Update ViBe Background: 16.0289ms
```
Release版本下，测试程序中计算出的该算法效率输出如下：  
```cpp
Time of Update ViBe Background: 3.88142ms
Time of Update ViBe Background: 3.71257ms
Time of Update ViBe Background: 3.59945ms
Time of Update ViBe Background: 3.35824ms
Time of Update ViBe Background: 3.57153ms
Time of Update ViBe Background: 3.44415ms
```

**2. ViBe+算法的效率：**  
Debug版本下，测试程序中计算出的该算法效率输出如下：  
```cpp
Time of Update ViBe+ Background: 224.118ms
Time of Update ViBe+ Background: 222.495ms
Time of Update ViBe+ Background: 223.623ms
Time of Update ViBe+ Background: 243.826ms
Time of Update ViBe+ Background: 224.687ms
Time of Update ViBe+ Background: 223.875ms
```
Release版本下，测试程序中计算出的该算法效率输出如下：  
```cpp
Time of Update ViBe+ Background: 66.9405ms
Time of Update ViBe+ Background: 67.1447ms
Time of Update ViBe+ Background: 69.6733ms
Time of Update ViBe+ Background: 68.3159ms
Time of Update ViBe+ Background: 67.0427ms
Time of Update ViBe+ Background: 75.1574ms
Time of Update ViBe+ Background: 68.5131ms
```
对比可知，添加了算法复杂度之后会使得计算量明显增多，计算效率降低。
