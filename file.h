//+------------------------------------------------------------------+
//|                                                          PE Info |
//|                                             Ilias Sharapov, 2020 |
//+------------------------------------------------------------------+
#pragma once
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
class CFile
  {
public:
   enum ESeekMethod
     {
      FromBeginning  =SEEK_SET,
      FromCurrent    =SEEK_CUR,
      FromEnd        =SEEK_END
     };

protected:
   HANDLE            m_handle=INVALID_HANDLE_VALUE;

public:
                    ~CFile(void) { Close(); }
   bool              IsValid(void) const { return(m_handle!=INVALID_HANDLE_VALUE); }
   bool              OpenRead(LPCWSTR path);
   void              Close(void);
   bool              Read(void *buffer,UINT size) const;
   bool              Seek(INT64 distance,ESeekMethod method) const;
  };
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
inline bool CFile::OpenRead(LPCWSTR path)
  {
   Close();
//---
   m_handle=::CreateFile(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
//---
   return(IsValid());
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
inline void CFile::Close(void)
  {
   if(m_handle!=INVALID_HANDLE_VALUE)
     {
      ::CloseHandle(m_handle);
      m_handle=INVALID_HANDLE_VALUE;
     }
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
inline bool CFile::Seek(INT64 distance,ESeekMethod method) const
  {
   return(IsValid() && ::SetFilePointerEx(m_handle,(LARGE_INTEGER &)distance,nullptr,method)!=FALSE);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
inline bool CFile::Read(void *buffer,const UINT size) const
  {
   DWORD nbr=0;
   return(IsValid() && ::ReadFile(m_handle,buffer,size,&nbr,nullptr)!=FALSE && size==nbr);
  }
//+------------------------------------------------------------------+