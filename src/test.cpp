
#include "movie.h"

#include <vector>
#include <cv.h>
#include <dirent.h>
#include <iostream>
using namespace std;


void find_file(vector<string> &filename,string path)
{
    DIR* dir = opendir(path.c_str());//打开指定目录
    dirent* p = NULL;//定义遍历指针
    while((p = readdir(dir)) != NULL)//开始逐个遍历
    {
        //cout<< p->d_name<<" "<<strcmp(p->d_name+strlen(p->d_name)-4,".bmp")<<endl;
        //这里需要注意，linux平台下一个目录中有"."和".."隐藏文件，需要过滤掉
        if(p->d_name[0] != '.'&& strcmp(p->d_name+strlen(p->d_name)-4,".bmp")==0)//d_name是一个char数组，存放当前遍历到的文件名
        {
            filename.push_back(path + p->d_name);
        }
    }
    closedir(dir);//关闭指定目录
}

int main()
{
#if 0
    int height=960;
    int width=1280;
    cv::Mat img,gray;

    string input_path="/home/yangh/Desktop/bmp/";
    string output_path="/home/yangh/Desktop/bmp/test";
    vector<string> file_name;
    find_file(file_name,input_path);

    MovieWriter writerRGB;
    MovieWriter writerIR;
    writerRGB.init(input_path+"1/",height,width,3);
    writerRGB.start();
    writerIR.init(input_path+"2/",height,width,1);
    writerIR.start();

    int count=0;
    for(int i=0;i<file_name.size();i++)  //file_name.size()
    {
        img = cv::imread(file_name[i]);
        image im;
        memcpy(im.data,img.data,HEIGHT*WIDTH*3);
        writerRGB.addImage(im);
        cv::cvtColor(img,gray,CV_BGR2GRAY);
        memcpy(im.data,gray.data,HEIGHT*WIDTH*1);
        writerIR.addImage(im);
        //cv::imwrite("/home/yangh/Desktop/gray/"+to_string(i)+".bmp",gray);
        if(i%100==-1)
        {
            writerRGB.done=TRUE;
            writerIR.done=TRUE;
            writerRGB.join();
            writerIR.join();
            writerRGB.stop();
            writerIR.stop();
            writerRGB.start();
            writerIR.start();
        }
        usleep(30000);
//        bool ret = writer.addImage(im);
//        if(!ret)
//        {
//            count++;
//            printf("fail to push image!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
//        }
//        else
//            printf("success push image\n");
    }
    printf("count: %d\n",count);
    writerRGB.done=TRUE;
    writerIR.done=TRUE;
    writerRGB.join();
    writerIR.join();
    writerRGB.stop();
    writerIR.stop();
#else
    int height=960;
    int width=1280;
    string inputfile="/home/yangh/Desktop/bmp/2/1.mp4";
    string save_path="/home/yangh/Desktop/2/";
    MovieReader reader(inputfile, width, height);
    const int channels=1;
    reader.getFrame(save_path,channels,100);
#endif
    return 0;
}


