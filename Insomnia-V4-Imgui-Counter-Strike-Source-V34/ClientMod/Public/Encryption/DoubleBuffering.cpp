//DoubleBuffering.cpp Source File   
   
#include "DoubleBuffering.h"   
#include <cassert>   
#include <stdexcept>
#include <exception>   
   
using namespace std;   
   
CDoubleBuffering::CDoubleBuffering(FILE* in, char* pcBuff, int iSize, int iDataLen) : m_rin(in),   // r47: was ifstream&
    m_iDataLen(iDataLen), m_bEOF(false), m_iSize(iSize), m_iSize2(iSize >> 1), m_pcBuff(pcBuff)   
{   
    //m_iSize should be even   
    if(m_iSize%2 != 0)   
        throw runtime_error("CDoubleBuffering: m_iSize should be Even Number!");   
    //Check file   
    if(!in)   // r47: was is_open()/bad()
        throw runtime_error("CDoubleBuffering: Referenced File not Opened or in Bad State!");   
    //Check construction data   
    if(m_iDataLen<1 || m_iSize2<m_iDataLen)   
        throw runtime_error("CDoubleBuffering: Illegal Construction Data!");   
    m_iEnd = (int)fread( m_pcBuff, 1, m_iSize2, in );   // r47: was in.read/gcount

    m_iCurPos = 0;   
    m_iBuf = 0;   
}   
   
int CDoubleBuffering::GetData(char* pszDataBuf, int iDataLen)   
{   
    if(-1 == iDataLen)   
        iDataLen = m_iDataLen;   
    if(iDataLen<1 || m_iSize2<iDataLen)   
        throw runtime_error("CDoubleBuffering::GetData(): Illegal iDataLen!");   
    if(true == m_bEOF)   
        return 0;   
    //Estimate the next position   
    int iCurPos = m_iCurPos + iDataLen;   
    if(0 == m_iBuf) //First Buffer   
    {   
        if(iCurPos >= m_iEnd)   
        {   
            //Read the next buffer   
            if(feof(m_rin))   
            {   
                m_bEOF = true;   
                //Take everything remained   
                int iRead = m_iEnd-m_iCurPos;   
                memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iRead);   
                return iRead;   
            }   
            else   
            {   
                m_iEnd += (int)fread( m_pcBuff + m_iEnd, 1, m_iSize2, m_rin );   // r47

                if(iCurPos > m_iEnd) //Still greater, then EOF attained   
                {   
                    assert(feof(m_rin));   
                    m_bEOF = true;   
                    //Take everything remained   
                    int iRead = m_iEnd-m_iCurPos;   
                    memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iRead);   
                    return iRead;   
                }   
                else   
                {   
                    memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iDataLen);   
                    m_iCurPos = iCurPos;   
                    assert(m_iCurPos >= m_iSize2);   
                    m_iBuf = 1;   
                    return iDataLen;   
                }   
            }   
        }   
        else   
        {   
            memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iDataLen);   
            m_iCurPos = iCurPos;   
            return iDataLen;   
        }   
    }   
    else //1 == m_iBuf, Second Buffer   
    {   
        if(iCurPos >= m_iEnd)   
        {   
            //Read the next buffer   
            if(feof(m_rin))   
            {   
                m_bEOF = true;   
                //Take everything remained   
                int iRead = m_iEnd-m_iCurPos;   
                memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iRead);   
                return iRead;   
            }   
            else   
            {   
                m_iEnd = (int)fread( m_pcBuff, 1, m_iSize2, m_rin );   // r47
                iCurPos %= m_iSize;   
                if(iCurPos > m_iEnd) //Still greater, then EOF attained   
                {   
                    assert(feof(m_rin));   
                    m_bEOF = true;   
                    //Take everything remained   
                    int iRead = m_iSize-m_iCurPos;   
                    memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iRead);   
                    memcpy(pszDataBuf+iRead, m_pcBuff, m_iEnd);   
                    return iRead + m_iEnd;   
                }   
                else   
                {   
                    int iRead = m_iSize-m_iCurPos;   
                    memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iRead);   
                    memcpy(pszDataBuf+iRead, m_pcBuff, iDataLen-iRead);   
                    m_iCurPos = iCurPos;   
                    assert(m_iCurPos < m_iSize2);   
                    m_iBuf = 0;   
                    return iDataLen;   
                }   
            }   
        }   
        else   
        {   
            memcpy(pszDataBuf, m_pcBuff+m_iCurPos, iDataLen);   
            m_iCurPos = iCurPos;   
            return iDataLen;   
        }   
    }   
}   