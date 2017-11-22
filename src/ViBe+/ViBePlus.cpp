/*=================================================================
 * Extract Background & Foreground Model by ViBe+ Algorithm using OpenCV Library.
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

/*=================================================================
 * Version:
 *  v1.0:  以 ViBe 工程 V1.4 版本起步，进行 ViBe+ 的开发；
 *  v1.1:  处理 ViBe+ 宏定义部分；
 *  v1.2:  将 ViBe+ 类进一步封装，在类中加了 Frame, Gray 成员；
 *  v1.3:  在 ViBe+ 类中加入了 samples_Frame 样本库，存储了灰度样本 samples
 *      对应位置的 BGR 通道值，用于后续的颜色矫正计算；
 *  v1.4:
 *      (1) 在 ViBe+ 判断匹配情况部分，加入了自适应阈值的计算与颜色畸变的计算，
 *      并以此基础进行判断；
 *      (2) 将背景模型 FGModel 改名为 SegModel；
 *  v1.5:  将 Run() 拆分为提取背景函数 ExtractBG() 与更新背景函数 Update()；
 *  v1.6:
 *      (1) 加入了样本集平均值、方差矩阵 samples_ave[][], samples_stddev[][]，
 *      提高了计算自适应阈值的效率；
 *      (2) 部分完善注释；
 *      (3) 将样本库所需的补充信息，从 ***samples 数组中抽出，使得 ***samples
 *      单纯的存储样本灰度值；
 *  v1.7:
 *      (1) 填充更新蒙版前景空洞区域；
 *      (2) 从更新蒙版中移除闪烁等级过高的像素；
 *      (3) 填充分割蒙版前景斑点区域；
 *      (4) 更新模板 UpdateModel 的前景像素点不被用来更新样本库；
 *      (5) 根据当前点最大梯度 maxGrad，跳出该次循环实现抑制传播；
 *  v1.8: 完善所有注释；
 *=================================================================
 */

#include "ViBePlus.h"

/*===================================================================
 * 构造函数：ViBePlus
 * 说明：初始化ViBe+算法部分参数；
 * 参数：
 *   int num_sam:  每个像素点的样本个数
 *   int min_match:  #min指数
 *   int r:  Sqthere半径
 *   int rand_sam:  子采样概率
 *   int count:  当前算法处理的视频帧数
 *------------------------------------------------------------------
 * Constructed Function: ViBePlus
 *
 * Summary:
 *   Init several arguments of ViBe+ Algorithm.
 *
 * Arguments:
 *   int num_sam - Number of pixel's samples
 *   int min_match - Match Number of make pixel as Background
 *   int r - Radius of pixel value
 *   int rand_sam - the probability of random sample
 *   int count - the Video's Frame Number Processed by Current Algorithm
=====================================================================
*/
ViBePlus::ViBePlus(int num_sam, int min_match, int r, int rand_sam)
{
    num_samples = num_sam;
    num_min_matches = min_match;
    radius = r;
    random_sample = rand_sam;
    count = 0;
}

/*===================================================================
 * 析构函数：~ViBePlus
 * 说明：释放样本库内存；
 *------------------------------------------------------------------
 * Destructor Function: ~ViBePlus
 *
 * Summary:
 *   Release the memory of Sample Library.
=====================================================================
*/
ViBePlus::~ViBePlus(void)
{
    deleteSamples();
}

/*===================================================================
 * 函数名：FrameCapture
 * 说明：捕获一帧图像且根据捕获图像的格式分别存储（支持 RGB 与灰度模式）；；
 *
 * 参数：
 *   Mat img:  源图像
 * 返回值：void
 *------------------------------------------------------------------
 * Function: FrameCapture
 *
 * Summary:
 *   Capture One Frame From Video, and Save according to Image's Format
 * (Support RGB & Gray)
 *
 * Arguments:
 *   Mat img - source image
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::FrameCapture(Mat img)
{
    img.copyTo(Frame);
    if(img.channels() == 3)
    {
        cvtColor(Frame, Gray, CV_BGR2GRAY);
        Channels = 3;
    }
    else
    {
        img.copyTo(Gray);
        Channels = 1;
    }
}

/*===================================================================
 * 函数名：init
 * 说明：背景模型初始化；
 *    为样本库及相关信息分配空间；
 *
 * 返回值：void
 *------------------------------------------------------------------
 * Function: init
 *
 * Summary:
 *   Init Background Model.
 *   Assign space for sample library.
 *   Read the first frame of video query as background model, then select pixel's
 * neighbourhood pixels randomly and fill the sample library.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::init()
{
    if(Gray.empty())
    {
        cout<<"ERROR: Init Error, No Gray Image."<<endl;
        return ;
    }

    // 动态分配三维数组，samples[][][num_samples]存储前景被连续检测的次数
    // Dynamic Assign 3-D Array.
    // sample[img.rows][img.cols][num_samples] is a 3-D Array which includes all pixels' samples.
    samples = new unsigned char **[Gray.rows];

    // 为 BGR 通道样本库动态分配数组
    // Dynamic Assign Array for BGR Channels of Samples
    samples_Frame = new unsigned char ***[Frame.rows];

    // 为样本集平均值、方差动态分配数组
    // Dynamic Assign Array for Average Values and Variance Values of Samples
    samples_sumsqr = new double *[Gray.rows];
    samples_ave = new double *[Gray.rows];

    // 为样本集相关信息矩阵初始化，动态分配数组
    // Dynamic Assign Array for Other Relative Information of Samples
    samples_ForeNum = new int *[Gray.rows];
    samples_BGInner = new bool *[Gray.rows];
    samples_InnerState = new int *[Gray.rows];
    samples_BlinkLevel = new int *[Gray.rows];
    samples_MaxInnerGrad = new int *[Gray.rows];

    for (int i = 0; i < Gray.rows; i++)
    {
        samples[i] = new uchar *[Gray.cols];
        samples_Frame[i] = new uchar **[Frame.cols];
        samples_sumsqr[i] = new double [Gray.cols];
        samples_ave[i] = new double [Gray.cols];
        samples_ForeNum[i] = new int [Gray.cols];
        samples_BGInner[i] = new bool [Gray.cols];
        samples_InnerState[i] = new int [Gray.cols];
        samples_BlinkLevel[i] = new int [Gray.cols];
        samples_MaxInnerGrad[i] = new int [Gray.cols];

        for (int j = 0; j < Gray.cols; j++)
        {
            samples[i][j] =new uchar [num_samples];
            samples_Frame[i][j] = new uchar *[num_samples];
            samples_sumsqr[i][j] = 0;
            samples_ave[i][j] = 0;
            samples_ForeNum[i][j] = 0;
            samples_BGInner[i][j] = false;
            samples_InnerState[i][j] = 0;
            samples_BlinkLevel[i][j] = 0;
            samples_MaxInnerGrad[i][j] = 0;

            for (int k = 0; k < num_samples + 1; k++)
            {
                // 创建样本库时，所有样本全部初始化为0
                // All Samples init as 0 When Creating Sample Library.
                samples[i][j][k] = 0;

                // 创建样本库时，样本库RGB通道全部初始化为0
                // All RGB Channels of Samples Init as 0 When Creating Sample Library.
                samples_Frame[i][j][k] = new uchar [3];
                for(int m = 0; m < 3; m++)
                    samples_Frame[i][j][k][m] = 0;
            }
        }
    }

    SegModel = Mat::zeros(Gray.size(),CV_8UC1);
    UpdateModel = Mat::zeros(Gray.size(),CV_8UC1);
}

/*===================================================================
 * 函数名：ProcessFirstFrame
 * 说明：处理第一帧图像；
 *    读取视频序列第一帧，并随机选取像素点邻域内像素填充样本库，初始化背景模型，
 * 并计算样本库的数学参数；
 *
 * 返回值：void
 *------------------------------------------------------------------
 * Function: ProcessFirstFrame
 *
 * Summary:
 *   Process First Frame of Video Query, then select pixel's neighbourhood pixels
 * randomly, fill the sample library, init Background Model, and Calculate Math
 * Arguments of Sample Library.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::ProcessFirstFrame()
{
    if(Gray.empty())
    {
        cout<<"ERROR: Process First Frame Error, No Gray Image."<<endl;
        return ;
    }

    RNG rng;
    int row, col;

    for(int i = 0; i < Gray.rows; i++)
    {
        for(int j = 0; j < Gray.cols; j++)
        {
            for(int k = 0 ; k < num_samples; k++)
            {
                // 随机选择num_samples个邻域像素点，构建背景模型
                // Random pick up num_samples pixel in neighbourhood to construct the model
                int random;
                random = rng.uniform(0, 9); row = i + c_yoff[random];
                random = rng.uniform(0, 9); col = j + c_xoff[random];

                // 防止选取的像素点越界
                // Protect Pixel from Crossing the border
                if (row < 0)  row = 0;
                if (row >= Gray.rows)  row = Gray.rows - 1;
                if (col < 0)  col = 0;
                if (col >= Gray.cols)   col = Gray.cols - 1;

                // 为样本库赋随机值
                // Set random pixel's Value for Sample Library
                samples[i][j][k] = Gray.at<uchar>(row, col);

                // 为RGB通道样本库赋随机值
                // Set random pixel's Value for Sample Libraries' RGB Channels
                for(int m = 0; m < 3; m++)
                    samples_Frame[i][j][k][m] = Frame.at<Vec3b>(row, col)[m];

                // 累加当前像素样本集灰度值
                // Accumulate Current Pixel's Sample Library's Gray Values
                samples_ave[i][j] += samples[i][j][k];
            }

            // 首次计算当前像素点样本集平均值灰度
            // Calculate Current Pixel's Sample Library's Gray Average Value Firstly
            samples_ave[i][j] /= num_samples;

            //========================================
            //        计算方差值  |  Calculate the Variance
            //========================================
            for(int k = 0 ; k < num_samples; k++)
            {
                // 累加与均值差值的平方
                // Accumulate Current Pixel's Square Value of the Difference between Current Value and Average.
                samples_sumsqr[i][j] += pow(samples[i][j][k] - samples_ave[i][j], 2);
            }
            // 首次获得样本方差值
            // Calculate Current Pixel's Sample Library's Variance Value Firstly
            samples_sumsqr[i][j] /= num_samples;
        }
    }
}

/*===================================================================
 * 函数名：Run
 * 说明：运行 ViBe 算法
 *
 * 返回值：void
 *------------------------------------------------------------------
 * Function: Run
 *
 * Summary:
 *   Run the ViBe Algorithm.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::Run()
{
    if(Frame.empty() || Gray.empty())
    {
        cout<<"ERROR: Run Error, No Image."<<endl;
        return ;
    }

    //=============================================
    //       零、建立首帧模型
    //--------------------------------------------------------
    //   Step 0 : Build First Frame's Model (Just Run for One Time)
    //=============================================
    if(count <= 0)
    {
        init();
        ProcessFirstFrame();
        cout<<"Training ViBe+ Success."<<endl;
        count++;
        return ;
    }

    //=============================================
    //       一、提取分割模板
    //--------------------------------------------------------
    //   Step 1 : Extract the Segment Model
    //=============================================
    ExtractBG();

    //=============================================
    //       二、根据已经得到的分割模板，计算更新模板；
    //--------------------------------------------------------
    //   Step 2 : Calculate Update Model from Segment Model
    //=============================================
    CalcuUpdateModel();

    //=============================================
    //        三、更新背景模板；
    //--------------------------------------------------------
    //   Step 3 : Update the Update Model
    //=============================================
    Update();

    // 帧数累计；
    // Count the Frame Number
    count++;
}

/*===================================================================
 * 函数名：ExtractBG
 * 说明：提取分割模板
 *
 * 返回值：void
 *------------------------------------------------------------------
 * Function: ExtractBG
 *
 * Summary:
 *   Run the ViBe Algorithm.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::ExtractBG()
{
    RNG rng;
    int k = 0, dist = 0, matches = 0;
    for(int i = 0; i < Gray.rows; i++)
    {
        for(int j = 0; j < Gray.cols; j++)
        {
            //==============================================
            //        计算自适应阈值  |  Calculate Adaptive Threshold
            //==============================================
            // 距离的自适应阈值
            // Adaptive Threshold of Distance
            double AdaThreshold = 20, sigma = 0;

            // 根据方差计算标准差
            // Calculate Standard Deviation According to Variance Value
            sigma = sqrt(samples_sumsqr[i][j]);

            // 计算得到的距离自适应阈值
            // Calculate Adaptive Threshold
            AdaThreshold = sigma * AMP_MULTIFACTOR;

            // 调整自适应阈值范围
            // Adjust Range of Adaptive Threshold
            if(AdaThreshold < 20)   AdaThreshold = 20;
            if(AdaThreshold > 40)   AdaThreshold = 40;

            //=============================================
            //        计算颜色畸变与匹配情况
            //--------------------------------------------------------
            //    Calculate Color Distortion & Sample Match Number
            //=============================================
            // 当前帧在 (i, j) 点的 RGB 通道值
            // (i, j) Pixel of Current Frame's RGB Channels Values
            int R, G, B;
            B = Frame.at<Vec3b>(i, j)[0]; G = Frame.at<Vec3b>(i, j)[1]; R = Frame.at<Vec3b>(i, j)[2];
            for(k = 0, matches = 0; matches < num_min_matches && k < num_samples; k++)
            {
                // 当前帧在 (i, j) 点第 k 个样本的 RGB 通道值
                // Number k Sample in (i, j) Pixel of Current Frame's RGB Channels Values
                int R_sam, G_sam, B_sam;
                B_sam = samples_Frame[i][j][k][0]; G_sam = samples_Frame[i][j][k][1]; R_sam = samples_Frame[i][j][k][2];

                // 计算颜色畸变
                // Calculate Color Distortion
                double colordist, RGB_Norm2, RGBSam_Norm2, RGB_Vec, p2;
                RGB_Norm2 = pow(B, 2) + pow(G, 2) + pow(R, 2);
                RGBSam_Norm2 = pow(B_sam, 2) + pow(G_sam, 2) + pow(R_sam, 2);
                RGB_Vec = R_sam * R + G_sam * G + B_sam * B; RGB_Vec = pow(RGB_Vec, 2);
                p2 = RGB_Vec / RGBSam_Norm2;
                colordist = RGB_Norm2 > p2 ? sqrt(RGB_Norm2 - p2) : 0;

                //=============================================
                // 若当前值与样本值之差小于自适应阈值，且颜色畸变值小于20，满足匹配条件；
                // If: (1) the Difference of Current Value and Sample's Value is less than Adaptive Threshold;
                //      (2) Color Distortion is less than 20;
                // Then:  Sample Match.
                //=============================================
                dist = abs(samples[i][j][k] - Gray.at<uchar>(i, j));
                if (dist < AdaThreshold && colordist < 20)
                    matches++;
            }

            /*===================================================================
             * 说明：
             *      当前像素值与样本库中值匹配次数较高，则认为是背景像素点；
             *      此时更新前景统计次数、更新前景模型、更新该像素模型样本值、更新该像素点邻域像素点的模型样本值
             *------------------------------------------------------------------
             * Summary:
             *   the Match Times of current pixel value and samples in library is large enough to
             * regard current pixel as a Background pixel.
             *   Then it needs to be done:
             *   - Run the times of Foreground Statistic
             *   - Run Foreground Model
             *   - Run model sample library of this pixel probably
             *   - Run model sample library of this pixel's neighborhood pixel probably
            =====================================================================
            */
            //========================================
            //        前景提取   |   Extract Foreground Areas
            //========================================
            if (matches >= num_min_matches)
            {
                // 已经认为是背景像素，故该像素的前景统计次数置0
                // This pixel has regard as a background pixel, so the count of this pixel's foreground statistic set as 0
                samples_ForeNum[i][j]=0;

                // 该像素点被的前景模型像素值置0
                // Set Foreground Model's pixel as 0
                SegModel.at<uchar>(i, j) = 0;
            }
            /*===================================================================
             * 说明：
             *      当前像素值与样本库中值匹配次数较低，则认为是前景像素点；
             *      此时需要更新前景统计次数、判断更新前景模型；
             *------------------------------------------------------------------
             * Summary:
             *   the Match Times of current pixel value and samples in library is small enough to
             * regard current pixel as a Foreground pixel.
             *   Then it needs to be done:
             *   - Run the times of Foreground Statistic
             *   - Judge and Run Foreground Model
            =====================================================================
            */
            else
            {
                // 已经认为是前景像素，故该像素的前景统计次数+1
                // This pixel has regard as a foreground pixel, so the count of this pixel's foreground statistic plus 1
                samples_ForeNum[i][j]++;

                // 该像素点被的前景模型像素值置255
                // Set Foreground Model's pixel as 255
                SegModel.at<uchar>(i, j) = 255;

                // 如果某个像素点连续50次被检测为前景，则认为一块静止区域被误判为运动，将其更新为背景点
                // if this pixel is regarded as foreground for more than 50 times, then we regard this static area as dynamic area by mistake, and Run this pixel as background one.
                if(samples_ForeNum[i][j] > 50)
                {
                    int random = rng.uniform(0, num_samples);
                    samples[i][j][random] = Gray.at<uchar>(i, j);

                    // 同时更新RGB通道样本库
                    // Update RGB Channels' Values of Sample Libraries
                    for(int m = 0; m < 3; m++)
                        samples_Frame[i][j][random][m] = Frame.at<Vec3b>(i, j)[m];
                }
            }
        }
    }
}

/*===================================================================
 * 函数名：CalcuUpdateModel
 * 说明：根据已经得到的分割模板，计算更新模板
 *
 * 返回值：void
 *------------------------------------------------------------------
 * Function: CalcuUpdateModel
 *
 * Summary:
 *   Calculate Update Model from Segment Model.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::CalcuUpdateModel()
{
    //========================================================
    //    更新蒙版的计算，填充更新蒙版前景空洞区域
    //-----------------------------------------------
    //   Calculate Update Model, and Fill Foreground Hole Areas of Update Model
    //========================================================
    SegModel.copyTo(UpdateModel);
    Mat imgtmp;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    UpdateModel.copyTo(imgtmp);

    // 提取轮廓
    // Extract Contours
    findContours(imgtmp, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
    for(int i = 0; i < contours.size(); i++)
    {
        // 一级父轮廓
        // Level 1 of Father Contour
        int father = hierarchy[i][3];
        // 二级父轮廓
        // Level 2 of Father Contour
        int grandpa;
        if(father >= 0)
            grandpa = hierarchy[father][3];
        else
            grandpa = -1;

        //===================================================================
        // 有父轮廓，无两级父轮廓，说明该轮廓是等级为 1 的轮廓，即我们需要的前景空洞区域；
        //------------------------------------------------------------------
        // If: (1) Have Level 1 of Father Contour;
        //      (2) No Level 2 of Father Contour;
        // Then:  It means this Contour is Level 1 Contour, and it's Foreground Hole Areas we need.
        //====================================================================
        if(father >= 0 && grandpa == -1)
        {
            // 填充面积 <= 50 的前景空洞区域
            // Fill Foreground Hole Areas whose Area is less than 50
            if(contourArea(contours[i]) <= 50)
                drawContours(UpdateModel, contours, i, Scalar(255), -1);
        }
    }

    for(int i = 1; i < Gray.rows - 1; i++)
    {
        for(int j = 1; j < Gray.cols - 1; j++)
        {
            // 邻域总状态位
            // Neighbor Area State Bits
            int state = 0, maxGrad = 0;
            //=============================
            //     判断背景内边缘
            //-----------------------------------------------
            //   Judge Background Inner Edge
            //=============================
            if(SegModel.at<uchar>(i, j) <= 0)
            {
                // 定义邻域范围
                // Range of Neighbor Area
                int i_min, i_max, j_min, j_max;
                i_min = i - 1; i_max = i + 1; j_min = j - 1; j_max = j + 1;

                //==================================
                //     计算邻域状态，邻域灰度最大梯度；
                //----------------------------------------------
                //   Calculate State & Max Gray Gradient of Neighbor Area
                //==================================
                for(int i_tmp = i_min; i_tmp <= i_max; i_tmp++)
                {
                    for(int j_tmp = j_min; j_tmp <= j_max; j_tmp++)
                    {
                        if( !(i_tmp == i && j_tmp == j) )
                        {
                            // 邻域内单点状态：前景为1，背景为0
                            // One Pixel's State of Neighbor Area
                            // 0 -- Background;  1 -- Foreground
                            int bitstate = (SegModel.at<uchar>(i_tmp, j_tmp) == 255) ? 1 : 0;
                            state = (state << 1) + bitstate;

                            // 计算最大梯度
                            // Calculate Max Gradient
                            int tmpGrad = Gray.at<uchar>(i, j) - Gray.at<uchar>(i_tmp, j_tmp); tmpGrad = abs(tmpGrad);
                            if(tmpGrad >= maxGrad)  maxGrad = tmpGrad;
                        }
                    }
                }
                // 当前状态位限定在8bit，即 0-255 之间
                // Set State Bits in 8Bits (0 -- 255)
                state = state & 255;

                // 状态位大于 0，则说明邻域有前景，即当前点为背景内边缘；
                // If State Bits is larger than 0, it means there is Foreground Point in Neighbor Area, and Current Point Belongs to Background Inner Edge.
                if(state > 0)
                    samples_BGInner[i][j] = true;
                else
                    samples_BGInner[i][j] = false;
            }
            else
            {
                samples_BGInner[i][j] = false;
                samples_InnerState[i][j] = 0;
            }

            //==================================
            //         计算闪烁等级
            //----------------------------------------------
            //   Calculate Blink Level
            //==================================
            if(samples_BGInner[i][j])
            {
                // 当前邻域状态与上一帧邻域状态相同，则说明当前点不闪烁；
                // If Neighbor Area State of Current Frame is same as Previous Frame's, it means Current Pixel is not Blinking.
                if(state == samples_InnerState[i][j])
                    samples_BlinkLevel[i][j] = max(samples_BlinkLevel[i][j] - 1, 0);
                // 当前邻域状态与上一帧邻域状态不同，则说明当前点闪烁；
                // If Neighbor Area State of Current Frame is not same as Previous Frame's, it means Current Pixel is Blinking.
                else
                {
                    samples_BlinkLevel[i][j] += 15;
                    samples_BlinkLevel[i][j] = min(samples_BlinkLevel[i][j], 150);
                }
            }
            else
            {
                samples_BlinkLevel[i][j] = max(samples_BlinkLevel[i][j] - 1, 0);
            }

            //==================================
            //         更新状态位
            //----------------------------------------------
            //   Update State Bits
            //==================================
            samples_InnerState[i][j] = state;
            samples_MaxInnerGrad[i][j] = maxGrad;

            //==================================
            //      闪烁等级 > 30，从更新蒙版 UpdateModel 中移除
            //----------------------------------------------
            //   If Blink Level is Larger than 30, Then remove this Pixel from Update Model.
            //==================================
            if(samples_BlinkLevel[i][j] > 30)
            {
                UpdateModel.at<uchar>(i, j) = 255;
//                SegModel.at<uchar>(i, j) = 0;
            }
        }
    }

    //==================================
    //         处理分割蒙版前景区域
    //----------------------------------------------
    //   Process Foreground Areas of Segment Areas
    //==================================
    SegModel.copyTo(imgtmp);
    findContours(imgtmp, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

    for(int i = 0; i < contours.size(); i++) {
        // 一级父轮廓
        // Level 1 of Father Contour
        int father = hierarchy[i][3];
        // 二级父轮廓
        // Level 2 of Father Contour
        int grandpa;
        if(father >= 0)
            grandpa = hierarchy[father][3];
        else
            grandpa = -1;
        //===================================================================
        // 有一级父轮廓，无两级父轮廓，说明该轮廓是等级为 1 的轮廓，即需要的前景空洞区域；
        //------------------------------------------------------------------
        // If: (1) Have Level 1 of Father Contour;
        //      (2) No Level 2 of Father Contour;
        // Then:  It means this Contour is Level 1 Contour, and it's Foreground Hole Areas we need.
        //====================================================================
        if(father >= 0 && grandpa == -1) {
            // 填充面积 <= 20 的前景空洞区域
            // Fill Foreground Hole Areas whose Area is less than 20
            double area = contourArea(contours[i]);
            if(area <= 20)
                drawContours(SegModel, contours, i, Scalar(255), -1);
        }

        //===================================================================
        // 无父轮廓，说明该轮廓是最外围轮廓，即等级为 0 的前景斑点区域；
        //------------------------------------------------------------------
        // If:        No Level 1 of Father Contour;
        // Then:  It means this Contour is Level 0 Contour, and it's Foreground Blob Areas we need.
        //====================================================================
        if(father == -1) {
            // 填充面积 <= 10 的前景斑点区域
            // Fill Foreground Blob Areas whose Area is less than 10
            double area = contourArea(contours[i]);
            if(area < 10)
                drawContours(SegModel, contours, i, Scalar(0), -1);
        }
    }
}

/*===================================================================
 * 函数名：Update
 * 说明：更新背景模板
 *
 * 返回值：void
 *------------------------------------------------------------------
 * Function: Update
 *
 * Summary:
 *   Update the Update Model.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::Update()
{
    RNG rng;
    for(int i = 0; i < Gray.rows; i++)
    {
        for(int j = 0; j < Gray.cols; j++)
        {
            //===================================================================
            // 更新模板 UpdateModel 的前景像素点不被用来更新样本库
            //------------------------------------------------------------------
            // Foreground Pixels in UpdateModel won't be used to Update Sample Libraries.
            //====================================================================
            if (UpdateModel.at<uchar>(i, j) <= 0)
            {
                // 已经认为该像素是背景像素，那么它有 1 / φ 的概率去更新自己的模型样本值
                // This pixel is already regarded as Background Pixel, then it has possibility of 1/φ to Run its model sample's value.
                int random = rng.uniform(0, random_sample);
                if (random == 0)
                {
                    uchar newVal = Gray.at<uchar>(i, j);
                    random = rng.uniform(0, num_samples);
                    // 更新样本集灰度方差
                    // Update Variance of Gray Value Sample Library
                    UpdatePixSampleSumSquare(i, j, random, newVal);
                    samples[i][j][random] = newVal;

                    // 同时更新RGB通道样本库
                    // Update RGB Channels' Values of Sample Library
                    for(int m = 0; m < 3; m++)
                        samples_Frame[i][j][random][m] = Frame.at<Vec3b>(i, j)[m];
                }

                // 同时也有 1 / φ 的概率去更新它的邻居点的模型样本值
                // At the same time, it has possibility of 1/φ to Run its neighborhood point's sample value.
                random = rng.uniform(0, random_sample);
                if (random == 0)
                {
                    //===================================================================
                    //   根据当前点最大梯度 maxGrad，跳出该次循环，便抑制传播
                    //------------------------------------------------------------------
                    //  Jump out of this Loop for Inhibiting Diffusion According to Gray Value Max Gradient of Current Pixel.
                    //====================================================================
                    if(samples_MaxInnerGrad[i][j] > 50)     continue;

                    int row, col;
                    uchar newVal = Gray.at<uchar>(i, j);
                    random = rng.uniform(0, 9); row = i + c_yoff[random];
                    random = rng.uniform(0, 9); col = j + c_xoff[random];

                    // 防止选取的像素点越界
                    // Protect Pixel from Crossing the border
                    if (row < 0) row = 0;
                    if (row >= Gray.rows)  row = Gray.rows - 1;
                    if (col < 0) col = 0;
                    if (col >= Gray.cols) col = Gray.cols - 1;

                    // 为样本库赋随机值
                    // Set random pixel's Value for Sample Library
                    random = rng.uniform(0, num_samples);
                    UpdatePixSampleSumSquare(row, col, random, newVal);
                    samples[row][col][random] = newVal;

                    // 同时更新RGB通道样本库
                    // Update RGB Channels' Values of Sample Libraries
                    for(int m = 0; m < 3; m++)
                        samples_Frame[row][col][random][m] = Frame.at<Vec3b>(i, j)[m];
                }
            }
        }
    }
}

/*===================================================================
 * 函数名：UpdatePixSampleSumSquare
 * 说明：更新当前像素点样本集方差
 * 参数：
 *      int i：行数
 *      int j：列数
 *      int k：样本编号
 *      int val：更新的样本值
 * 返回值：void
 *------------------------------------------------------------------
 * Function: UpdatePixSampleSumSquare
 *
 * Summary:
 *   Update Variance of Sample Set of Current Pixel whose location is (i, j)
 *
 * Arguments:
 *   int i - Number of Line
 *   int j - Number of Column
 *   int k - ID in Sample Library
 *   int val - Sample Value Updated
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::UpdatePixSampleSumSquare(int i, int j, int k, int val)
{
    double res = 0;
    // 更新当前像素点样本集平均值
    // Update Average Value of Current Pixel's Sample Library
    UpdatePixSampleAve(i, j, k, val);
    // 计算样本集平均值更新后的样本集灰度总平方和
    // Calculate Sum of Gray Value's Square After Average Value Updated.
    for(int m = 0; m < num_samples; m++)
        res += pow(samples[i][j][k] - samples_ave[i][j], 2);
    res /= num_samples;
    samples_sumsqr[i][j] = res;
}

/*===================================================================
 * 函数名：UpdatePixSampleAve
 * 说明：更新当前像素点样本集平均值
 * 参数：
 *      int i：行数
 *      int j：列数
 *      int k：样本编号
 *      int val：更新的样本值
 * 返回值：void
 *------------------------------------------------------------------
 * Function: UpdatePixSampleAve
 *
 * Summary:
 *   Update Average Value of Sample Set of Current Pixel whose location is (i, j)
 *
 * Arguments:
 *   int i - Number of Line
 *   int j - Number of Column
 *   int k - ID in Sample Library
 *   int val - Sample Value Updated
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::UpdatePixSampleAve(int i, int j, int k, int val)
{
    double res;
    // 计算原样本集灰度总值
    // Calculate the sum of Previous Sample Library's Gray Values
    res = samples_ave[i][j] * num_samples;
    // 计算更新后的样本集灰度平均值
    // Calculate the Gray Value Average of Sample Library After Updating
    res = res - samples[i][j][k] + val;
    res /= num_samples;
    samples_ave[i][j] = res;
}

/*===================================================================
 * 函数名：getSegModel
 * 说明：获取前景模型二值图像；
 * 返回值：Mat
 *------------------------------------------------------------------
 * Function: getSegModel
 *
 * Summary:
 *   get Foreground Model Binary Image.
 *
 * Returns:
 *   Mat
=====================================================================
*/
Mat ViBePlus::getSegModel()
{
    return SegModel;
}

/*===================================================================
 * 函数名：getUpdateModel
 * 说明：获取更新模型二值图像；
 * 返回值：Mat
 *------------------------------------------------------------------
 * Function: getUpdateModel
 *
 * Summary:
 *   get Foreground Model Binary Image.
 *
 * Returns:
 *   Mat
=====================================================================
*/
Mat ViBePlus::getUpdateModel()
{
    return UpdateModel;
}


/*===================================================================
 * 函数名：deleteSamples
 * 说明：删除样本库；
 * 返回值：void
 *------------------------------------------------------------------
 * Function: deleteSamples
 *
 * Summary:
 *   Delete Sample Library.
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBePlus::deleteSamples()
{
    delete samples;
    delete samples_Frame;
    delete samples_ave;
    delete samples_sumsqr;
    delete samples_ForeNum;
    delete samples_BGInner;
    delete samples_InnerState;
    delete samples_BlinkLevel;
    delete samples_MaxInnerGrad;
}

