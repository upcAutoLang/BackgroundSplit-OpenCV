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

int main(int argc, char* argv[])
{
    Mat frame, gray, FGModel;
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

    capture.set(CAP_PROP_FRAME_WIDTH,160);
    capture.set(CAP_PROP_FRAME_HEIGHT,120);
    if (!capture.isOpened())
    {
        cout<<"No camera or video input!"<<endl;
        return -1;
    }

    // 程序运行时间统计变量
    // the Time Statistical Variable of Program Running Time
    double time;
    double start;

    ViBe vibe;
    bool count = true;

    while (1)
    {
        capture >> frame;
        if (frame.empty())
            continue;

        cvtColor(frame, gray, cv::COLOR_RGB2GRAY);
        if (count)
        {
            vibe.init(gray);
            vibe.ProcessFirstFrame(gray);
            cout<<"Training ViBe Success."<<endl;
            count=false;
        }
        else
        {
            start = static_cast<double>(getTickCount());
            vibe.Run(gray);
            time = ((double)getTickCount() - start) / getTickFrequency() * 1000;

            cout << "Time of Update ViBe Background: " << time << "ms" <<endl<<endl;

            FGModel = vibe.getFGModel();
//			morphologyEx(FGModel, FGModel, MORPH_OPEN, Mat());
            imshow("FGModel", FGModel);
        }

        imshow("input", frame);

        if ( waitKey(25) == 27 )
            break;
    }

    return 0;
}
