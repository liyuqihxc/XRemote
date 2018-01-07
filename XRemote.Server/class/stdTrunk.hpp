#pragma once
#ifndef _STD_TRUNK_H_
#define _STD_TRUNK_H_

namespace hxc
{
    class StdCallThunk
    {
#if defined _M_IX86
#pragma pack(push,1)
        typedef struct _tagThunk
        {
            BYTE        m_pop_eax;           //pop eax
            BYTE        m_push_this;          // push pThis
            DWORD   m_this;
            BYTE        m_push_eax;         //push eax
            BYTE    m_jmp;          // jmp WndProc
            DWORD   m_relproc;      // relative jmp
        }THUNK, *PTHUNK;
#pragma pack(pop)
#elif defined _M_X64
#pragma pack(push,1)
        typedef struct _tagThunk
        {
            //mov rax,qword ptr[rsp]    返回地址
            //sub rsp 8h                            堆栈指针+1
            //mov qword ptr[rsp],rax    返回地址入栈顶 rsp[0]
            //mov qword ptr[rsp+28h], r9            原第四个参数入栈顶-5 rsp[5]
            //mov r9,r8                             原第三个参数到原第四个参数的位置
            //mov r8,rdx                          原第二个参数到原第三个参数的位置
            //mov rdx,rcx                        原第一个参数到原第二个参数的位置
            //mov rcx,this                        this指针现在是第一个参数
            //mov rax,proc                      
            //jmp rax
            //add rsp 8h                        平衡栈

            //mov rax,qword ptr[rsp]
            //sub rsp,10h
            //mov qword ptr[rsp+28h], r9
            //mov r9,r8
            //mov r8,rdx
            //mov rdx,rcx
            //mov rcx,123456789h
            //mov qword ptr[rsp],rcx
            //mov rcx,123456789fch
            //mov qword ptr[rsp+30h],rax
            //mov rax,123456789ebh
            //jmp rax
            //add rsp,10h
            //jmp qword ptr[rsp+30h]

            DWORD Mov_Rax__Qword_Ptr_Rsp;
            DWORD Sub_Rsp__10;
            BYTE Dummy1;
            DWORD Mov_Qword_Ptr_Rsp_plus_28__R9;
            BYTE Dummy2;
            USHORT Mov_R9__R8;
            BYTE Dummy3;
            USHORT Mov_R8__Rdx;
            BYTE Dummy4;
            USHORT Mov_Rdx__Rcx;
            USHORT Mov_Rcx__fakeRetAddr;
            ULONG64 fakeRetAddr;
            DWORD Mov_Qword_Ptr_Rsp__Rcx;
            USHORT Mov_Rcx__pThis;
            ULONG64 pThis;
            BYTE Dummy5;
            DWORD Mov_Qword_Ptr_Rsp_plus_30__Rax;
            USHORT Mov_Rax;
            ULONG64 proc;
            USHORT Jmp_Rax;
            DWORD Add_Rsp__10;
            DWORD Jmp_Qword_Ptr_Rsp_plus_10;
        }THUNK, *PTHUNK;
#pragma pack(pop)
#endif
    public:
        template<typename T>
        StdCallThunk(void* pThis, T proc)
        {
            if (sm_hExecutableHeap == NULL)
                sm_hExecutableHeap = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, sizeof(_tagThunk) * 10, 0);
            InterlockedIncrement(&_RefCount);

            union
            {
                T From;
                DWORD_PTR To;
            }union_cast;
            union_cast.From = proc;

            void* pVoid = __AllocStdCallThunk();
            m_pRef = (long*)pVoid;
            InterlockedIncrement(m_pRef);
            m_pThunk = (PTHUNK)((DWORD_PTR)pVoid + sizeof(long));
#if defined _M_IX86
            m_pThunk->m_pop_eax = 0x58;
            m_pThunk->m_push_this = 0x68;
            m_pThunk->m_this = PtrToUlong(pThis);
            m_pThunk->m_push_eax = 0x50;
            m_pThunk->m_jmp = 0xe9;
            m_pThunk->m_relproc = DWORD((INT_PTR)union_cast.To - ((INT_PTR)m_pThunk + sizeof(THUNK)));
#elif defined _M_X64
            m_pThunk->Mov_Rax__Qword_Ptr_Rsp = 0x24048B48;
            m_pThunk->Sub_Rsp__10 = 0x10EC8348;
            m_pThunk->Dummy1 = 0x4C;
            m_pThunk->Mov_Qword_Ptr_Rsp_plus_28__R9 = 0x28244C89;
            m_pThunk->Dummy2 = 0x4D;
            m_pThunk->Mov_R9__R8 = 0xC88B;
            m_pThunk->Dummy3 = 0x4C;
            m_pThunk->Mov_R8__Rdx = 0xC28B;
            m_pThunk->Dummy4 = 0x48;
            m_pThunk->Mov_Rdx__Rcx = 0xD18B;
            m_pThunk->Mov_Rcx__fakeRetAddr = 0xb948;
            m_pThunk->fakeRetAddr = (ULONG64)&m_pThunk->Add_Rsp__10;
            m_pThunk->Mov_Qword_Ptr_Rsp__Rcx = 0x240C8948;
            m_pThunk->Mov_Rcx__pThis = 0xb948;
            m_pThunk->pThis = (ULONG64)pThis;
            m_pThunk->Dummy5 = 0x48;
            m_pThunk->Mov_Qword_Ptr_Rsp_plus_30__Rax = 0x30244489;
            m_pThunk->Mov_Rax = 0xB848;
            m_pThunk->proc = (ULONG64)union_cast.To;
            m_pThunk->Jmp_Rax = 0xE0FF;
            m_pThunk->Add_Rsp__10 = 0x10C48348;
            m_pThunk->Jmp_Qword_Ptr_Rsp_plus_10 = 0x102464FF;
#endif
            FlushInstructionCache(GetCurrentProcess(), m_pThunk, sizeof(THUNK));
        }

        ~StdCallThunk()
        {
            if (m_pRef != nullptr && InterlockedDecrement(m_pRef) == 0)
                __FreeStdCallThunk((void*)m_pRef);

            if (InterlockedDecrement(&_RefCount) == 0)
                HeapDestroy(sm_hExecutableHeap);
        }

        StdCallThunk(const StdCallThunk& value)
        {
            m_pRef = value.m_pRef;
            InterlockedIncrement(m_pRef);
            m_pThunk = value.m_pThunk;
        }

        StdCallThunk& operator=(const StdCallThunk& value)
        {
            m_pRef = value.m_pRef;
            InterlockedIncrement(m_pRef);
            m_pThunk = value.m_pThunk;
        }

        template<typename T>
        T get__FunctionPtr()
        {
            return (T)m_pThunk;
        }

    private:
        static void* __stdcall __AllocStdCallThunk(void)
        {
            if (sm_hExecutableHeap != NULL)
                return ::HeapAlloc(sm_hExecutableHeap, HEAP_ZERO_MEMORY, sizeof(THUNK) + sizeof(long));
            return nullptr;
        }

        static void __stdcall __FreeStdCallThunk(void* pRef)
        {
            if (sm_hExecutableHeap != NULL)
            {
                ::HeapFree(sm_hExecutableHeap, 0, pRef);
                DWORD err = GetLastError();
            }
        }
    private:
        PTHUNK m_pThunk;
        volatile long * m_pRef;
        static HANDLE sm_hExecutableHeap;
        static volatile long _RefCount;
    };

    __declspec(selectany) volatile long StdCallThunk::_RefCount = 0;
    __declspec(selectany) HANDLE StdCallThunk::sm_hExecutableHeap = NULL;

}//namespace hxc

#endif // ! _STD_TRUNK_H_
