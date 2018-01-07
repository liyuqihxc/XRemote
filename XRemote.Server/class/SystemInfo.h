#pragma once

#ifndef _GETSYSINFO_H_
#define _GETSYSINFO_H_

namespace hxc
{
    class SystemInfo
    {
    private:
        SystemInfo(void) {}

    public:
        /********获取操作系统版本，Service pack版本、系统类型************/
        static const std::wstring& get__OSName();
        static BOOL get__IsWow64();//判断是否为64位操作系统

        /***********获取网卡数目和名字***********/
        static int  get__NetworkInterFaces();

        /***获取物理内存和虚拟内存大小***/
        void GetMemoryInfo(std::wstring &dwTotalPhys, std::wstring &dwTotalVirtual);

        /****获取CPU名称、内核数目、主频*******/
        void GetCpuInfo(std::wstring &chProcessorName, std::wstring &chProcessorType, DWORD &dwNum, DWORD &dwMaxClockSpeed);

        /****获取硬盘信息****/
        void GetDiskInfo(DWORD &dwNum, std::wstring chDriveInfo[]);

        /****获取显卡信息*****/
        void GetDisplayCardInfo(DWORD &dwNum, std::wstring chCardName[]);
    private:
        static std::wstring _OSName;
        //std::vector<std::wstring> Interfaces;		                  //保存所有网卡的名字
        //CList < DWORD, DWORD &>		Bandwidths;	  //各网卡的带宽
        //CList < DWORD, DWORD &>		TotalTraffics;    //各网卡的总流量
    };
}//namespace hxc

#endif //if !defined _GETSYSINFO_H_