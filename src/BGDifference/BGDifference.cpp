/*=================================================================
 * Calculate Background Model of a list of Frames(Normally a video stream) in the
 * method of Background Difference Method & OTSU Algorithm By OpenCV.
 *
 * Copyright (C) 2017 Chandler Geng. All rights reserved.
 *
 *     This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *     You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
===================================================================
*/
#include "BGDifference.h"

/*===================================================================
 * 函数名：BackgroundDiff
 * 说明：背景差分算法；
 * 参数：
 *   Mat src:  源图像
 *   Mat& imgForeground: 前景图像
 *   Mat& imgBackground: 背景图像
 *   int nFrmNum: 当前帧数
 *   int threshold_method: 阈值方法
 *      - CV_THRESH_OTSU:       使用OpenCV自带OTUS方法
 *      - CV_THRESH_BINARY:   使用该类中的OTSU方法
 *   double updateSpeed: 背景更新速度
 * 返回值：void
 *------------------------------------------------------------------
 * Function: BackgroundDiff
 *
 * Summary:
 *   Background Difference Algorithm;
 *
 * Arguments:
 *   Mat src - source image
 *   Mat& imgForeground - Foreground Image
 *   Mat& imgBackground - Background Image
 *   int nFrmNum - Current Frame Number
 *   int threshold_method - the method of getting Threshold Value
 *      - CV_THRESH_OTSU:       Using OpenCV's OTSU method
 *      - CV_THRESH_BINARY:   Using this class's OTSU method
 *   double updateSpeed - the Speed of Background Update
 *
 * Returns:
 *   void
=====================================================================
*/
void BGDiff::BackgroundDiff(Mat src, Mat &imgForeground, Mat& imgBackground, int nFrmNum, int threshold_method, double updateSpeed)
{
    // 源图像的灰度图像
    // Gray Image(uchar) of Source Image
    Mat src_gray;

    // 单通道浮点图像，用于背景建模
    // Gray Image(float) of Source Image, and will be used to model background
    Mat src_grayf;

    // 前景、背景的浮点图像
    // Gray Image(float) of Foreground & Background Images
    Mat imgForegroundf, imgBackgroundf;

    // 前景图像缓存
    // temp Image of ForeGround Image
    Mat imgForeground_temp;

    // 初始化浮点图像
    // Init float Images
    imgBackgroundf.create(src.size(), CV_32FC1);
    imgForegroundf.create(src.size(), CV_32FC1);
    src_grayf.create(src.size(), CV_32FC1);

    // 视频流第一帧，前景与背景都初始化为第一帧的灰度图
    // if it is in the first frame of Video stream, Foreground & Background Image will be inited as Gray Image of First Frame
    if(nFrmNum == 1)
    {
        src_gray.create(src.size(), CV_8UC1);
        imgForeground_temp.create(src.size(), CV_8UC1);

        // 用第一帧图像的灰度图初始化前景与背景图像
        // Gray Image of First Frame init as Foreground & Background Image
        cvtColor(src, imgBackground, CV_BGR2GRAY);
        cvtColor(src, imgForeground, CV_BGR2GRAY);
        imgBackground.convertTo(imgBackgroundf, CV_32FC1);
        imgForeground.convertTo(imgForegroundf, CV_32FC1);
    }
    // 视频流其余帧，根据当前帧图像更新前景与背景图像
    // if it's not the First Frame of Video stream, it will update Fore & Back ground According to Current Frame Image
    else
    {
        // 获得当前帧图像灰度图，并转换为浮点数格式
        // get Gray Image of Source Image and Convert it to float Format.
        cvtColor(src, src_gray, CV_BGR2GRAY);
        src_gray.convertTo(src_grayf, CV_32FC1);

        // 获得背景图像灰度图，并转换为浮点数格式
        // get Gray Image of Background Image and Convert it to float Format.
        imgBackground.convertTo(imgBackgroundf, CV_32FC1);

        // 当前帧跟背景图相减
        // get Gray Image of Foreground. Formula is:
        //     Foreground = Source - Background
        absdiff(src_grayf, imgBackgroundf, imgForegroundf);

        // 复制前景图像
        // Copy Foreground Image
        imgForegroundf.convertTo(imgForeground_temp, CV_32FC1);

        // 使用OpenCV自带的OTSU方法
        // Using OpenCV's OTSU method
        if(threshold_method == CV_THRESH_OTSU)
        {
            // 浮点转化为整点
            // Convert Foreground Image's Format from float to uchar
            imgForeground_temp.convertTo(imgForeground_temp, CV_8UC1);
            // 对比自适应阈值化
            threshold(imgForeground_temp, imgForeground, 0, 255, CV_THRESH_OTSU);
        }
        // 使用该类中的OTSU方法
        // Using this class's OTSU method
        else
        {
            // 二值化前景图
            int threshold_otsu = 0;
            Otsu(imgForeground_temp, threshold_otsu);
            // 浮点转化为整点
            // Convert Foreground Image's Format from float to uchar
            imgForeground_temp.convertTo(imgForeground_temp, CV_8UC1);
            threshold(imgForeground_temp, imgForeground, threshold_otsu, 255, CV_THRESH_BINARY);
        }

        /*===================================================================
         * 说明：
         *      更新背景，其中accumulateWeighted()函数是背景移动平均函数；
         * 函数原型如下：
         *      void accumulateWeighted( InputArray src, InputOutputArray dst, double alpha, InputArray mask=noArray() )
         * 参数：
         *     InputArray src: 输入图像
         *     InputOutputArray dst: 输入图像，并作为两幅输入图像累加后的图像输出
         *     double alpha: 输入图像的权重
         *     InputArray mask: 可选蒙版
         * 计算公式如下：
         *     dst = (1 - alptha) * dst + alpha * src
         * 注：此处两个输入图像是浮点格式，因为计算过程中会有小数出现；
         *------------------------------------------------------------------
         * Summary:
         *   Background Difference Algorithm;
         *
         * Function's Prototype:
         *      void accumulateWeighted( InputArray src, InputOutputArray dst, double alpha, InputArray mask=noArray() )
         *
         * Arguments:
         *      InputArray src - Input Image
         *      InputOutputArray dst - Input & Output Image(Output as Input Images's(src, dst) accumulate result)
         *      double alpha - Weight of the input image
         *      InputArray mask - Optional operation mask
         *
         * Formula:
         *      dst = (1 - alptha) * dst + alpha * src
         *
         * P.S:
         *      All of Input Images must be float format, because there will be decimal number during the process of calculating.
        =====================================================================
        */
        accumulateWeighted(src_grayf, imgBackgroundf, updateSpeed);

        // 浮点转化为整点
        // Convert Foreground Image's Format from float to uchar
        imgBackgroundf.convertTo(imgBackground, CV_8UC1);
    }
}

/*===================================================================
 * 函数名：Otsu
 * 说明：大津法；
 * 参数：
 *   Mat src:  源图像
 *   int& thresholdValue: 输出计算得到的阈值
 *   bool ToShowValue: 是否在终端输出计算得到的阈值
 * 返回值：void
 *------------------------------------------------------------------
 * Function: Otsu
 *
 * Summary:
 *   OTSU Algorithm;
 *
 * Arguments:
 *   Mat src - source image
 *   int& thresholdValue - get the threshold value after calculating
 *   bool ToShowValue - if output threshold value result on terminal or not
 *
 * Returns:
 *   void
=====================================================================
*/

void BGDiff::Otsu(Mat src, int& thresholdValue, bool ToShowValue)
{
    // 输入图像是否为灰度图的标志位
    // the Flag Bit of source image is gray image or not
    uchar grayflag =1;

    // 原图像的灰度图
    // Gray Image of Source Image
    Mat gray;

    // 检查源图像是否为灰度图像
    // Check Source image that is Gray image or not
    if(src.channels()  != 1)
    {
        gray.create(src.size(), CV_8UC1);
        cvtColor(src, gray, CV_BGR2GRAY);
        grayflag = 0;
    }
    else
        gray = src;

    // 阈值缓存变量
    // threshold Temp Value
    int thresholdValue_temp = 1;

    // 图像直方图,256个点
    // the Histogram of Image (including 256 points)
    int ihist[256];

    // 对直方图置零
    // set all points of histogram as 0
    memset(ihist, 0, sizeof(ihist));

    // n: 非零像素个数, n1: 前景像素个数, n2: 背景像素个数
    // n - number of pixels whose value isn't equal to 0
    // n1 - number of Foreground's pixels
    // n2 - number of Background's pixels
    int n, n1, n2;

    // m1: 前景灰度均值, m2: 背景灰度均值
    // m1 - the Average of Foreground Pixels' sum
    // m2 - the Average of Background Pixels' sum
    double m1, m2;


    double sum, csum, fmax, sb;

    //*************
    // 生成直方图
    //*************
    int nr = src.rows;
    int nc = src.cols * src.channels();

    for(int j = 0; j < nr; j++)
    {
        uchar* ImgData = src.ptr<uchar>(j);

        for(int i = 0; i < nc; i++)
        {
            //灰度统计 '&255'防止指针溢出
            ihist[( (int)(ImgData[i]) ) & 255]++;
        }
    }

    // set up everything
    sum = csum = 0.0;
    n = 0;
    for(int i = 0; i < 255; i++)
    {
        // x*f(x)质量矩
        sum += (double)i * (double)ihist[i];
        // f(x)质量 像素总数
        n += ihist[i];
    }

    // n为0，即图像全黑，输出警告
    if (!n)
    {
        fprintf (stderr, "NOT NORMAL thresholdValue=160\n");
    }

    // OTSU算法
    fmax = -1.0;
    n1 = 0;
    for (int i = 0; i < 255; i++)
    {
        n1 += ihist[i];
        if (n1 == 0) {continue;}
        n2 = n - n1;
        if (n2 == 0) {break;}
        csum += (double)i * ihist[i];
        m1 = csum / n1;
        m2 = (sum - csum) / n2;

        // 计算类间方差，公式已简化
        // Calculate interclass variance (Simplified the formula)
        sb = (double)n1 * (double)n2 * (m1 - m2) * (m1 - m2);

        if (sb > fmax)
        {
            fmax = sb;
            // 找到使类间方差最大的灰度值i
            // Find the Gray Value which make interclass variance as max value
            thresholdValue_temp = i;
        }
    }

    // 设定阈值最小值
    // set Minimum of threshold value
    if(thresholdValue_temp < 20)
        thresholdValue = 20;
    else
        thresholdValue = thresholdValue_temp;

    // 是否显示计算得到的阈值
    // Show the threshold value or not
    if(ToShowValue)
    {
        cout << "OTSU thresholdValue = " << thresholdValue_temp<<", Returned thresholdValue = " << thresholdValue<<'\n'<<endl;
    }
}
