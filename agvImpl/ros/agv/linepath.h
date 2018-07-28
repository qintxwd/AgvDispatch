#ifndef LINEPATH_H
#define LINEPATH_H

#include "../../../common.h"
#include <iostream>
#include <fstream>

using namespace std;


struct Pose2D
{
  double x;
  double y;
  double theta;
}; // struct Pose2D


class LinePath{
public:

    //the point(x,y) unit is meter(m)
    static std::vector<Pose2D>  getLinePath_mm(int start_x,int start_y,int end_x,int end_y,double interval=0.05)
    {
        return getLinePath((double)start_x/1000.0, (double)start_y/1000.0, (double)end_x/1000.0, (double)end_y/1000.0, interval);
    }

    //the point(x,y) unit is meter(m)
    static std::vector<Pose2D>  getLinePath(double start_x,double start_y,double end_x,double end_y,double interval=0.05)
    {
        std::vector<Pose2D> path;
        Pose2D pose2d;

        double length=sqrt((end_x-start_x)*(end_x-start_x)+(end_y-start_y)*(end_y-start_y));
        double direction=atan2(end_y-start_y,end_x-start_x);

        pose2d.theta=direction;

        double sinth=sin(direction);
        double costh=cos(direction);

        int num_pathpts=ceil(length/interval)+1;
        double residue=length-(num_pathpts-2)*interval;
        path.clear();
        pose2d.x=start_x;
        pose2d.y=start_y;
        path.push_back(pose2d);

        pose2d.x+=costh*residue;
        pose2d.y+=sinth*residue;
        path.push_back(pose2d);
        for(int i=0;i<num_pathpts-1;i++){
            pose2d.x+=costh*interval;
            pose2d.y+=sinth*interval;
            path.push_back(pose2d);
        }

        return path;
    }

    static bool writeToText(string filename, std::vector<Pose2D> path, bool use_quaternion){
        ofstream ofs(filename.c_str(),ofstream::out);
        if(!ofs.is_open())
            return false;
        for(Pose2D pose:path){
            ofs<<pose.x<<" "<<pose.y<<" ";
            if(use_quaternion)
                ofs<<sin(pose.theta/2)<<" "<<cos(pose.theta/2);
            else
                ofs<<pose.theta;
            ofs<<endl;
        }
        ofs.close();
        return true;
    }

};

#endif // LINEPATH_H
