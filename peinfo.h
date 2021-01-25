//+------------------------------------------------------------------+
//|                                                          PE Info |
//|                                             Ilias Sharapov, 2020 |
//+------------------------------------------------------------------+
#pragma once

#include "file.h"
//+------------------------------------------------------------------+
//| PE Info                                                          |
//+------------------------------------------------------------------+
class CPEInfo
  {
private:

   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   LPCSTR GetString(UINT offset)
     {
      if(offset>=m_size)
         return(NULL);

      return(reinterpret_cast<LPCSTR>(m_image+offset));
     }

   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   void DumpResourceDirectory(const IMAGE_RESOURCE_DIRECTORY *resource_dir,const UINT resource_base,const UINT level)
     {
      const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry=reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY_ENTRY *>(resource_dir+1);
      const UINT count=resource_dir->NumberOfIdEntries + resource_dir->NumberOfNamedEntries;

      for(UINT i=0; i<count; i++, entry++)
        {
         printf("\n");

         if(entry->NameIsString)
           {
            const IMAGE_RESOURCE_DIR_STRING_U *name=reinterpret_cast<const IMAGE_RESOURCE_DIR_STRING_U *>(m_image+resource_base+entry->NameOffset);
            printf("%*c%.*S",level*3,' ',name->Length,name->NameString);
           }
         else
           {
            printf("%*c%04x",level*3,' ',entry->Id);
           }

         if(entry->DataIsDirectory)
           {
            const IMAGE_RESOURCE_DIRECTORY *dir=reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY *>(m_image+resource_base+entry->OffsetToDirectory);
            DumpResourceDirectory(dir,resource_base,level+1);
           }
         else
           {
            const IMAGE_RESOURCE_DATA_ENTRY *data=reinterpret_cast<const IMAGE_RESOURCE_DATA_ENTRY *>(m_image+resource_base+entry->OffsetToData);
            printf(" %x %u %x",data->CodePage,data->Size,data->OffsetToData);
           }
        }
     }

   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   void DumpResources(UINT offset,UINT size)
     {
      printf("\nIMAGE_DIRECTORY_ENTRY_RESOURCE");

      if(size)
         DumpResourceDirectory(reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY *>(m_image+offset),offset,1);

      printf("\n");
     }

   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   template<typename TThunkData>
   void DumpImportTable(UINT offset,UINT size)
     {
      printf("\nIMAGE_DIRECTORY_ENTRY_IMPORT\n");

      if(!size)
         return;

      for(const IMAGE_IMPORT_DESCRIPTOR *desc=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(m_image+offset); desc->Name; desc++)
        {
         printf("   %s\n",GetString(desc->Name));

         const TThunkData *thunk=reinterpret_cast<const TThunkData *>(m_image+desc->OriginalFirstThunk);

         for(; thunk->u1.Function; thunk++)
           {
            if(thunk->u1.Function & (decltype(thunk->u1.Function)(1)<<(sizeof(thunk->u1.Function)*8-1)))
              {
               printf("      %04x: <IMPORT BY ORDINAL NUMBER>\n",(WORD)(thunk->u1.Function & ~(decltype(thunk->u1.Function)(1)<<(sizeof(thunk->u1.Function)*8-1))));
              }
            else
              {
               const IMAGE_IMPORT_BY_NAME *imp=reinterpret_cast<const IMAGE_IMPORT_BY_NAME *>(m_image+thunk->u1.Function);

               printf("      %04x: %s\n",imp->Hint,imp->Name);
              }
           }
        }

      printf("\n");
     }

   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   void DumpExportTable(UINT offset,UINT size)
     {
      printf("\nIMAGE_EXPORT_DIRECTORY\n");

      if(!size)
         return;

      const IMAGE_EXPORT_DIRECTORY *header=reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(m_image+offset);

      printf("   Characteristics      : %u\n"
             "   TimeDateStamp        : %u\n"
             "   MajorVersion         : %u\n"
             "   MinorVersion         : %u\n"
             "   Name                 : %s\n"
             "   Base                 : %u\n"
             "   NumberOfFunctions    : %u\n"
             "   NumberOfNames        : %u\n"
             "   AddressOfFunctions   : %u\n"
             "   AddressOfNames       : %u\n"
             "   AddressOfNameOrdinals: %u\n",

              header->Characteristics,
              header->TimeDateStamp,
              header->MajorVersion,
              header->MinorVersion,
              GetString(header->Name),
              header->Base,
              header->NumberOfFunctions,
              header->NumberOfNames,
              header->AddressOfFunctions,
              header->AddressOfNames,
              header->AddressOfNameOrdinals
            );

      //---
      printf("\n   export function names:\n");

      const UINT *names  =reinterpret_cast<UINT *>(m_image+header->AddressOfNames);
      const WORD *ordinal=reinterpret_cast<WORD *>(m_image+header->AddressOfNameOrdinals);
      const UINT *address=reinterpret_cast<UINT *>(m_image+header->AddressOfFunctions);

      for(UINT n=0; n<header->NumberOfNames; n++)
        {
         if(address[ordinal[n]]<offset || address[ordinal[n]]>=(offset+size))
            printf("      %u: %s -> 0x%p\n",ordinal[n]+header->Base,names[n] ? GetString(names[n]) : "<NA>",m_image+address[ordinal[n]]);
         else
            printf("      %u: %s -> %s\n",ordinal[n]+header->Base,names[n] ? GetString(names[n]) : "<NA>",GetString(address[ordinal[n]]));
        }

      printf("\n");
     }

   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   template<typename PlatformInfo>
   bool Parse(const CFile &file)
     {
      decltype(PlatformInfo::THeader::OptionalHeader) header;

      if(!file.Read(&header,sizeof(header)))
         return(false);
      //---
      if(!(m_image=new(std::nothrow) BYTE[header.SizeOfImage]))
         return(false);

      if(!file.Seek(0,CFile::FromBeginning))
         return(false);

      if(!file.Read(m_image,header.SizeOfHeaders))
         return(false);
      //---
      const IMAGE_DOS_HEADER        *dos_header =reinterpret_cast<IMAGE_DOS_HEADER *>(m_image);
      const PlatformInfo::THeader   *nt_header  =reinterpret_cast<typename PlatformInfo::THeader *>(m_image+dos_header->e_lfanew);
      const IMAGE_SECTION_HEADER    *sec_header =reinterpret_cast<IMAGE_SECTION_HEADER *>(m_image+dos_header->e_lfanew+sizeof(PlatformInfo::THeader));
      //---
      if(!LoadSections(file,sec_header,nt_header->FileHeader.NumberOfSections))
         return(false);

      m_size=header.SizeOfImage;

      DumpExportTable(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
      DumpImportTable<PlatformInfo::TThunkData>(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
      DumpResources(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress,nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);

      //---
      return(true);
     }
   //+------------------------------------------------------------------+
   //|                                                                  |
   //+------------------------------------------------------------------+
   bool LoadSections(const CFile &file,const IMAGE_SECTION_HEADER *sec,const UINT count)
     {
      for(const IMAGE_SECTION_HEADER *end=sec+count; sec<end; sec++)
        {
         //--- Normalize Virtual & Raw sizes
         UINT v_size=sec->Misc.VirtualSize;
         UINT r_size=sec->SizeOfRawData;

         if(!v_size)
            v_size=r_size;

         if(!sec->PointerToRawData)
            r_size=0;
         else
            if(r_size>v_size)
               r_size=v_size;

         //--- read data if needed
         if(r_size)
           {
            if(!file.Seek(sec->PointerToRawData,CFile::FromBeginning))
               return(false);
            if(!file.Read(m_image+sec->VirtualAddress,r_size))
               return(false);
           }

         //--- zero padding to virtual size
         if(r_size<v_size)
            memset(m_image+sec->VirtualAddress+r_size,0,v_size-r_size);
        }

      return(true);
     }

protected:
   BYTE             *m_image=nullptr;
   size_t            m_size=0;

public:
   //+------------------------------------------------------------------+
   //| Destructor                                                       |
   //+------------------------------------------------------------------+
   ~CPEInfo(void)
     {
      Close();
     }

   //+------------------------------------------------------------------+
   //| Open PE file                                                     |
   //+------------------------------------------------------------------+
   bool Open(LPCWSTR path)
     {
      //--- close previous before open new 
      Close();
      //--- load file into the buffer
      if(!LoadFromFile(path))
         return(false);

      Close();
      return(false);
     }

   //+------------------------------------------------------------------+
   //| Close                                                            |
   //+------------------------------------------------------------------+
   void Close(void)
     {
      if(m_image)
        {
         delete[] m_image;
         m_image=nullptr;
        }
     }

private:

   struct Platform32
     {
      typedef IMAGE_NT_HEADERS32 THeader;
      typedef IMAGE_THUNK_DATA32 TThunkData;
     };

   struct Platform64
     {
      typedef IMAGE_NT_HEADERS64 THeader;
      typedef IMAGE_THUNK_DATA64 TThunkData;
     };

   //+------------------------------------------------------------------+
   //| Read file                                                        |
   //+------------------------------------------------------------------+
   bool LoadFromFile(LPCWSTR path)
     {
      CFile file;

      if(file.OpenRead(path))
        {
         IMAGE_DOS_HEADER dos_header;

         if(file.Read(&dos_header,sizeof(dos_header)))
           {
            if(dos_header.e_magic==IMAGE_DOS_SIGNATURE)
              {
               if(file.Seek(dos_header.e_lfanew,CFile::FromBeginning))
                 {
                  UINT pe_signature;

                  if(file.Read(&pe_signature,sizeof(pe_signature)))
                    {
                     if(pe_signature==IMAGE_NT_SIGNATURE)
                       {
                        IMAGE_FILE_HEADER file_header;

                        if(file.Read(&file_header,sizeof(file_header)))
                          {
                           if(file_header.Machine==IMAGE_FILE_MACHINE_I386)
                              return(Parse<Platform32>(file));
                           if(file_header.Machine==IMAGE_FILE_MACHINE_AMD64)
                              return(Parse<Platform64>(file));
                          }
                       }
                    }
                 }
              }
           }
        }
      //---
      return(false);
     }
  };
//+------------------------------------------------------------------+
