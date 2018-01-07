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
        /********��ȡ����ϵͳ�汾��Service pack�汾��ϵͳ����************/
        static const std::wstring& get__OSName();
        static BOOL get__IsWow64();//�ж��Ƿ�Ϊ64λ����ϵͳ

        /***********��ȡ������Ŀ������***********/
        static int  get__NetworkInterFaces();

        /***��ȡ�����ڴ�������ڴ��С***/
        void GetMemoryInfo(std::wstring &dwTotalPhys, std::wstring &dwTotalVirtual);

        /****��ȡCPU���ơ��ں���Ŀ����Ƶ*******/
        void GetCpuInfo(std::wstring &chProcessorName, std::wstring &chProcessorType, DWORD &dwNum, DWORD &dwMaxClockSpeed);

        /****��ȡӲ����Ϣ****/
        void GetDiskInfo(DWORD &dwNum, std::wstring chDriveInfo[]);

        /****��ȡ�Կ���Ϣ*****/
        void GetDisplayCardInfo(DWORD &dwNum, std::wstring chCardName[]);
    private:
        static std::wstring _OSName;
        //std::vector<std::wstring> Interfaces;		                  //������������������
        //CList < DWORD, DWORD &>		Bandwidths;	  //�������Ĵ���
        //CList < DWORD, DWORD &>		TotalTraffics;    //��������������
    };
}//namespace hxc

#endif //if !defined _GETSYSINFO_H_