#include "PacketParsePool.h"

CPacketParsePool::CPacketParsePool()
{
}

CPacketParsePool::~CPacketParsePool()
{
    OUR_DEBUG((LM_INFO, "[CPacketParsePool::~CPacketParsePool].\n"));
    Close();
    OUR_DEBUG((LM_INFO, "[CPacketParsePool::~CPacketParsePool] End.\n"));
}

void CPacketParsePool::Init(uint32 u4PacketCount)
{
    Close();

    //初始化HashTable
    m_objPacketParseList.Init((int)u4PacketCount);

    for(int i = 0; i < (int)u4PacketCount; i++)
    {
        CPacketParse* pPacket = new CPacketParse();

        if(NULL != pPacket)
        {
            //设置默认包头长度
            pPacket->SetPacket_Mode(App_MainConfig::instance()->GetPacketParseInfo()->m_u1Type);
            pPacket->SetPacket_Head_Src_Length( App_MainConfig::instance()->GetPacketParseInfo()->m_u4OrgLength);
            //添加到HashPool里面
            char szPacketID[10] = {'\0'};
            sprintf_safe(szPacketID, 10, "%d", i);
            int nHashPos = m_objPacketParseList.Add_Hash_Data(szPacketID, pPacket);

            if(-1 != nHashPos)
            {
                pPacket->SetHashID(nHashPos);
            }
        }
    }
}

void CPacketParsePool::Close()
{
    //清理所有已存在的指针
    vector<CPacketParse*> vecPacketParse;
    m_objPacketParseList.Get_All_Used(vecPacketParse);

    for(int i = 0; i < (int)vecPacketParse.size(); i++)
    {
        CPacketParse* pPacket = vecPacketParse[i];
        SAFE_DELETE(pPacket);
    }

    m_objPacketParseList.Close();
}

int CPacketParsePool::GetUsedCount()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    return m_objPacketParseList.Get_Count() - m_objPacketParseList.Get_Used_Count();
}

int CPacketParsePool::GetFreeCount()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    return  m_objPacketParseList.Get_Used_Count();
}

CPacketParse* CPacketParsePool::Create()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    //如果free池中已经没有了，则添加到free池中。
    CPacketParse* pPacket = NULL;

    //在Hash表中弹出一个已使用的数据
    pPacket = m_objPacketParseList.Pop();

    return pPacket;
}

bool CPacketParsePool::Delete(CPacketParse* pPacketParse)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    CPacketParse* pBuff = (CPacketParse* )pPacketParse;

    if(NULL == pBuff)
    {
        return false;
    }

    pPacketParse->Clear();

    char szHashID[10] = {'\0'};
    sprintf_safe(szHashID, 10, "%d", pPacketParse->GetHashID());
    bool blState = m_objPacketParseList.Push(szHashID, pPacketParse);

    if(false == blState)
    {
        OUR_DEBUG((LM_INFO, "[CPacketParsePool::Delete]HashID=%s(0x%08x).\n", szHashID, pPacketParse));
    }
    else
    {
        //OUR_DEBUG((LM_INFO, "[CSendMessagePool::Delete]HashID=%s(0x%08x) nPos=%d.\n", szHashID, pObject, nPos));
    }

    return true;
}
