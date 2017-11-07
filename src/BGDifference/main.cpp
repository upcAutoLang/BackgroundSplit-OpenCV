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

/*=================================================
 * Version:
 * v1.0: 原版程序由IplImage转换为Mat；
 * v1.1: 背景差分法封装成类: BGDiff；
 * v1.2: 补充注释；
 * v1.3: 该方法与高斯混合背景模型不同，命名有误，改为背景差分法；
===================================================
*/

#include "BGDifference.h"

int main( int argc, char** argv )
{
    // 原图像
    Mat pFrame;
    // 原始OTSU算法输出图像
    Mat pFroundImg;
    // 背景图像
    Mat pBackgroundImg;
    // 改进的OTSU算法输出图像
    Mat pFroundImg_c;
    Mat pBackgroundImg_c;

    //视频控制全局变量,
   // 's' 画面stop
   // 'q' 退出播放
   // 'p' 打印OTSU算法中找到的阈值
   char ctrl = NULL;

   BGDiff BGDif;

   VideoCapture capture;
   capture = VideoCapture("./Video/Camera Road 01.avi");
   if(!capture.isOpened())
   {
       capture = VideoCapture("../Video/Camera Road 01.avi");
       if(!capture.isOpened())
       {
           capture = VideoCapture("../../Video/Camera Road 01.avi");
           if(!capture.isOpened())
           {
               cout<<"ERROR: Did't find this video!"<<endl;
               return 0;
           }
       }
   }

    int nFrmNum = 0;

    // 逐帧读取视频
    capture >> pFrame;
    while(!pFrame.empty())
    {
        capture >> pFrame;
        nFrmNum++;

        // 视频控制
        if( (ctrl = cvWaitKey(1000 / 25)) == 's' )
            waitKey(0);
        else if( ctrl == 'p')
            cout << "Current Frame = " << nFrmNum << endl;
        else if( ctrl == 'q')
            break;

        // OpenCV自带OTSU
        BGDif.BackgroundDiff(pFrame, pFroundImg, pBackgroundImg, nFrmNum, CV_THRESH_OTSU);
        // 阈值筛选后的OTSU
        BGDif.BackgroundDiff(pFrame, pFroundImg_c, pBackgroundImg_c, nFrmNum, CV_THRESH_BINARY);

        // 显示图像
        imshow("Source Video", pFrame);
        imshow("Background", pBackgroundImg);
        imshow("OTSU ForeGround", pFroundImg);
        imshow("Advanced OTSU ForeGround", pFroundImg_c);
    }
    return 0;
}

