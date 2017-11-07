#ifndef VIBEPLUSMACRO_H
#define VIBEPLUSMACRO_H

// 每个像素点的样本个数默认值
// the Default Number of pixel's samples
#define DEFAULT_NUM_SAMPLES  20

// #min指数默认值
// the Default Match Number of make pixel as Background
#define DEFAULT_MIN_MATCHES  2

// Sqthere半径默认值
// the Default Radius of pixel value
#define DEFAULT_RADIUS 20

// 子采样概率默认值
// the Default the probability of random sample
#define DEFAULT_RANDOM_SAMPLE 5

// 振幅乘数因子
#define AMP_MULTIFACTOR  0.5

// 连续记为前景次数
#define ID_FORENUM  20

// 是否为背景内边缘?  0:  NO;  1:  YES
#define ID_BGINNER  21

// 在背景内边缘状况下，八邻域状态位
#define ID_INNER_STATE  22

// 闪烁等级
#define ID_BLINK_LEVEL  23

// 邻域梯度最大值
#define ID_MAX_INNERGRAD  24

#endif // VIBEPLUSMACRO_H
