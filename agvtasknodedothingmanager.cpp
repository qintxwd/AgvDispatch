#include "agvtasknodedothingmanager.h"

#ifdef WIN32
#include <windows.h>
#else

#endif


AgvTaskNodeDoThingManager* AgvTaskNodeDoThingManager::p = new AgvTaskNodeDoThingManager();

AgvTaskNodeDoThingManager::AgvTaskNodeDoThingManager()
{

}

std::vector<std::string> AgvTaskNodeDoThingManager::listDynamicLib(){

    using namespace std;
    std::vector<std::string> result;
#ifdef WIN32
    HANDLE hFind;
    WIN32_FIND_DATA findData;
    LARGE_INTEGER size;
    char dirNew[100];

    hFind = FindFirstFile(".//", &findData);
    do
    {
        // 是否是文件夹，并且名称不为"."或".."
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY != 0
                && strcmp(findData.cFileName, ".") != 0
                && strcmp(findData.cFileName, "..") != 0
                )
        {
        }
        else
        {
            size.LowPart = findData.nFileSizeLow;
            size.HighPart = findData.nFileSizeHigh;
            result.push_back(std::string(findData.cFileName));
            cout << findData.cFileName << "\t" << size.QuadPart << " bytes\n";
        }
    } while (FindNextFile(hFind, &findData));

    FindClose(hFind);
#else



#endif
}

void AgvTaskNodeDoThingManager::init()
{
    //加载目录下 HRG_NODE_THING_*.so/dll
    //获取目录下的所有的so/dll




    void * handle = dlopen(".so", RTLD_LAZY);

    if( !handle )
    {
        std::cerr << dlerror() << std::endl;
        exit(1);
    }

}
