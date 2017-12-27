/*=================================================================
 * Extract Background & Foreground Model by ViBe Algorithm using OpenCV Library.
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

#include "Vibe.h"

/*===================================================================
 * 构造函数：ViBe
 * 说明：初始化ViBe算法部分参数；
 * 参数：
 *   int num_sam:  每个像素点的样本个数
 *   int min_match:  #min指数
 *   int r:  Sqthere半径
 *   int rand_sam:  子采样概率
 *------------------------------------------------------------------
 * Constructed Function: ViBe
 *
 * Summary:
 *   Init several arguments of ViBe Algorithm.
 *
 * Arguments:
 *   int num_sam - Number of pixel's samples
 *   int min_match - Match Number of make pixel as Background
 *   int r - Radius of pixel value
 *   int rand_sam - the probability of random sample
=====================================================================
*/
ViBe::ViBe(int num_sam, int min_match, int r, int rand_sam)
{
    num_samples = num_sam;
    num_min_matches = min_match;
    radius = r;
    random_sample = rand_sam;
    int c_off[9] = {-1, 0, 1, -1, 1, -1, 0, 1, 0};
    for(int i = 0; i < 9; i++){
        c_xoff[i] = c_yoff[i] = c_off[i];
}

/*===================================================================
 * 析构函数：~ViBe
 * 说明：释放样本库内存；
 *------------------------------------------------------------------
 * Destructor Function: ~ViBe
 *
 * Summary:
 *   Release the memory of Sample Library.
=====================================================================
*/
ViBe::~ViBe(void)
{
    deleteSamples();
}

/*===================================================================
 * 函数名：init
 * 说明：背景模型初始化；
 *    为样本库分配空间；
 * 参数：
 *   Mat img:  源图像
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
 * Arguments:
 *   Mat img - source image
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBe::init(Mat img)
{
    // 动态分配三维数组，samples[][][num_samples]存储前景被连续检测的次数
    // Dynamic Assign 3-D Array.
    // sample[img.rows][img.cols][num_samples] is a 3-D Array which includes all pixels' samples.
    samples = new unsigned char **[img.rows];
    for (int i = 0; i < img.rows; i++)
	{
        samples[i] = new uchar *[img.cols];
        for (int j = 0; j < img.cols; j++)
		{
            // 数组中，在num_samples之外多增的一个值，用于统计该像素点连续成为前景的次数；
            // the '+ 1' in 'num_samples + 1', it's used to count times of this pixel regarded as foreground pixel.
            samples[i][j] =new uchar [num_samples + 1];
            for (int k = 0; k < num_samples + 1; k++)
			{
                // 创建样本库时，所有样本全部初始化为0
                // All Samples init as 0 When Creating Sample Library.
                samples[i][j][k] = 0;
			}
		}
	}

    FGModel = Mat::zeros(img.size(),CV_8UC1);
}

/*===================================================================
 * 函数名：ProcessFirstFrame
 * 说明：处理第一帧图像；
 *    读取视频序列第一帧，并随机选取像素点邻域内像素填充样本库，初始化背景模型；
 * 参数：
 *   Mat img:  源图像
 * 返回值：void
 *------------------------------------------------------------------
 * Function: ProcessFirstFrame
 *
 * Summary:
 *   Process First Frame of Video Query, then select pixel's neighbourhood pixels
 * randomly and fill the sample library, and init Background Model.
 *
 * Arguments:
 *   Mat img - source image
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBe::ProcessFirstFrame(Mat img)
{
	RNG rng;
	int row, col;

    for(int i = 0; i < img.rows; i++)
	{
        for(int j = 0; j < img.cols; j++)
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
                if (row >= img.rows)  row = img.rows - 1;
                if (col < 0)  col = 0;
                if (col >= img.cols)   col = img.cols - 1;

                // 为样本库赋随机值
                // Set random pixel's Value for Sample Library
                samples[i][j][k]=img.at<uchar>(row, col);
			}
		}
	}
}

/*===================================================================
 * 函数名：Run
 * 说明：运行 ViBe 算法，提取前景区域并更新背景模型样本库；
 * 参数：
 *   Mat img:  源图像
 * 返回值：void
 *------------------------------------------------------------------
 * Function: Run
 *
 * Summary:
 *   Run the ViBe Algorithm: Extract Foreground Areas & Update Background Model Sample Library.
 *
 * Arguments:
 *   Mat img - source image
 *
 * Returns:
 *   void
=====================================================================
*/
void ViBe::Run(Mat img)
{
    RNG rng;
    int k = 0, dist = 0, matches = 0;
    for(int i = 0; i < img.rows; i++)
	{
        for(int j = 0; j < img.cols; j++)
        {
            //========================================
            //        前景提取   |   Extract Foreground Areas
            //========================================
            /*===================================================================
             * 说明：计算当前像素值与样本库的匹配情况；
             * 参数：
             *   int matches: 当前像素值与样本库中值之差小于阈值范围RADIUS的个数；
             *   int count: 遍历样本库的缓存变量；
             *------------------------------------------------------------------
             * Summary:
             *   Count how many samples in library can match to current pixel value.
             *
             * Argumen:
             *   int matches - the Number of samples whose value subtract current pixel's value
             *          is less than RADIUS.
             *   int count - the temp variance for going through sample library.
            =====================================================================
            */
            for(k = 0, matches = 0; matches < num_min_matches && k < num_samples; k++)
            {
                dist = abs(samples[i][j][k] - img.at<uchar>(i, j));
                if (dist < radius)
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
            if (matches >= num_min_matches)
            {
                // 已经认为是背景像素，故该像素的前景统计次数置0
                // This pixel has regard as a background pixel, so the count of this pixel's foreground statistic set as 0
                samples[i][j][num_samples]=0;

                // 该像素点被的前景模型像素值置0
                // Set Foreground Model's pixel as 0
                FGModel.at<uchar>(i, j) = 0;
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
                samples[i][j][num_samples]++;

                // 该像素点被的前景模型像素值置255
                // Set Foreground Model's pixel as 255
                FGModel.at<uchar>(i, j) = 255;

                // 如果某个像素点连续50次被检测为前景，则认为一块静止区域被误判为运动，将其更新为背景点
                // if this pixel is regarded as foreground for more than 50 times, then we regard this static area as dynamic area by mistake, and Run this pixel as background one.
                if(samples[i][j][num_samples] > 50)
                {
                    int random = rng.uniform(0, num_samples);
                    samples[i][j][random]=img.at<uchar>(i, j);
                }
            }

            //================================================================
            //        更新模型样本库    |    Update Background Model Sample Library
            //================================================================
            if (matches >= num_min_matches)
            {
                // 已经认为该像素是背景像素，那么它有 1 / φ 的概率去更新自己的模型样本值
                // This pixel is already regarded as Background Pixel, then it has possibility of 1/φ to Run its model sample's value.
                int random = rng.uniform(0, random_sample);
                if (random == 0)
                {
                    random = rng.uniform(0, num_samples);
                    samples[i][j][random]=img.at<uchar>(i, j);
                }

                // 同时也有 1 / φ 的概率去更新它的邻居点的模型样本值
                // At the same time, it has possibility of 1/φ to Run its neighborhood point's sample value.
                random = rng.uniform(0, random_sample);
                if (random == 0)
                {
                    int row, col;
                    random = rng.uniform(0, 9); row = i + c_yoff[random];
                    random = rng.uniform(0, 9); col = j + c_xoff[random];

                    // 防止选取的像素点越界
                    // Protect Pixel from Crossing the border
                    if (row < 0) row = 0;
                    if (row >= img.rows)  row = img.rows - 1;
                    if (col < 0) col = 0;
                    if (col >= img.cols) col = img.cols - 1;

                    // 为样本库赋随机值
                    // Set random pixel's Value for Sample Library
                    random = rng.uniform(0, num_samples);
                    samples[row][col][random]=img.at<uchar>(i, j);
                }
            }
        }
    }
}

/*===================================================================
 * 函数名：getFGModel
 * 说明：获取前景模型二值图像；
 * 返回值：Mat
 *------------------------------------------------------------------
 * Function: getFGModel
 *
 * Summary:
 *   get Foreground Model Binary Image.
 *
 * Returns:
 *   Mat
=====================================================================
*/
Mat ViBe::getFGModel()
{
    return FGModel;
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
void ViBe::deleteSamples()
{
    delete samples;
}
