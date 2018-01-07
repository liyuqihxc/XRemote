#pragma once

#if !defined _DELEGATE_H_
#define _DELEGATE_H_

#define DECLARE_EVENT(varname,argtype) std::function<void(argtype*)> varname;

#define EVENT_HANDLER(name,argtype) void name(argtype* e)

namespace hxc
{
    template<typename Handler, typename Arg>
    class Event
    {
    public:
        Event()
        {
            m_HandlerArray->resize(5);
        }

        ~Event()
        {

        }

        void operator+=(Handler value)
        {
            m_HandlerArray.Lock();
            m_HandlerArray->push_back(value);
            m_HandlerArray.Release();
        }

        void operator-=(Handler value)
        {
            m_HandlerArray.Lock();
            for (auto Iterator = m_HandlerArray->begin(); Iterator != m_HandlerArray->end(); Iterator++)
            {
                if (*Iterator == value)
                {
                    m_HandlerArray->erase(Iterator);
                    break;
                }
            }
            m_HandlerArray.Release();
        }

        void operator()(Arg* pe)
        {
            m_HandlerArray.Lock();
            for (auto Iterator = m_HandlerArray->begin(); Iterator != m_HandlerArray->end(); Iterator++)
            {
                (*Iterator)(pe);
            }
            m_HandlerArray.Release();
        }
    private:
        Critical_Section<std::vector<Handler>> m_HandlerArray;
    };

};//namespace hxc
#endif//if !defined _DELEGATE_H_