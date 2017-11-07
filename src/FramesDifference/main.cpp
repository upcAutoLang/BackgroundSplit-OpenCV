/*=================================================
 * Version:
 * v1.0: 原版程序由IplImage转换为Mat
===================================================
*/

#include "highgui.h"
#include "cv.h"
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char *argv[])
{
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

    // 用于遍历capture中的帧，通道数为3，需要转化为单通道才可以处理
    Mat tmpFrame, tmpFrameF;
    // 当前帧，单通道，uchar / Float
    Mat currentFrame, currentFrameF;
    // 上一帧，单通道，uchar / Float
    Mat previousFrame, previousFrameF;

    int frameNum = 0;

    capture >> tmpFrame;
    while(!tmpFrame.empty())
    {
        capture >> tmpFrame;
        //tmpFrame=cvQueryFrame(capture);
        frameNum++;
        if(frameNum == 1)
        {
            // 第一帧先初始化各个结构，为它们分配空间
            previousFrame.create(tmpFrame.size(), CV_8UC1);
            currentFrame.create(tmpFrame.size(), CV_8UC1);
            currentFrameF.create(tmpFrame.size(), CV_32FC1);
            previousFrameF.create(tmpFrame.size(), CV_32FC1);
            tmpFrameF.create(tmpFrame.size(), CV_32FC1);
        }

        if(frameNum >= 2)
        {
            // 转化为单通道灰度图，此时currentFrame已经存了tmpFrame的内容
            cvtColor(tmpFrame, currentFrame, CV_BGR2GRAY);
            currentFrame.convertTo(tmpFrameF, CV_32FC1);
            previousFrame.convertTo(previousFrameF, CV_32FC1);

            // 做差求绝对值
            absdiff(tmpFrameF, previousFrameF, currentFrameF);
            currentFrameF.convertTo(currentFrame, CV_8UC1);
            /*
            在currentFrameMat中找大于20（阈值）的像素点，把currentFrame中对应的点设为255
            此处阈值可以帮助把车辆的阴影消除掉
            */
//            threshold(currentFrameF, currentFrame, 20, 255.0, CV_THRESH_BINARY);
            threshold(currentFrame, currentFrame, 30, 255, CV_THRESH_BINARY);

            int g_nStructElementSize = 3; //结构元素(内核矩阵)的尺寸
            // 获取自定义核
            Mat element = getStructuringElement(MORPH_RECT,
                                                Size(2 * g_nStructElementSize + 1, 2 * g_nStructElementSize + 1),
                                                Point( g_nStructElementSize, g_nStructElementSize ));
            // 膨胀
            dilate(currentFrame, currentFrame, element);
            // 腐蚀
            erode(currentFrame, currentFrame, element);
        }

        //把当前帧保存作为下一次处理的前一帧
        cvtColor(tmpFrame, previousFrame, CV_BGR2GRAY);

        // 显示图像
        imshow("Camera", tmpFrame);
        imshow("Moving Area", currentFrame);
        waitKey(25);
    }
}
