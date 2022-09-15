// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME libsupix_dict
#define R__NO_DEPRECATION

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "RConfig.h"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// The generated code does not explicitly qualifies STL entities
namespace std {} using namespace std;

// Header files passed as explicit arguments
#include "RunInfo.h"
#include "SupixAnly.h"
#include "SupixTree.h"

// Header files passed via #pragma extra_include

namespace ROOT {
   static void *new_RunInfo(void *p = 0);
   static void *newArray_RunInfo(Long_t size, void *p);
   static void delete_RunInfo(void *p);
   static void deleteArray_RunInfo(void *p);
   static void destruct_RunInfo(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::RunInfo*)
   {
      ::RunInfo *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::RunInfo >(0);
      static ::ROOT::TGenericClassInfo 
         instance("RunInfo", ::RunInfo::Class_Version(), "RunInfo.h", 19,
                  typeid(::RunInfo), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::RunInfo::Dictionary, isa_proxy, 4,
                  sizeof(::RunInfo) );
      instance.SetNew(&new_RunInfo);
      instance.SetNewArray(&newArray_RunInfo);
      instance.SetDelete(&delete_RunInfo);
      instance.SetDeleteArray(&deleteArray_RunInfo);
      instance.SetDestructor(&destruct_RunInfo);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::RunInfo*)
   {
      return GenerateInitInstanceLocal((::RunInfo*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal((const ::RunInfo*)0x0); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

namespace ROOT {
   static TClass *SupixTree_Dictionary();
   static void SupixTree_TClassManip(TClass*);
   static void *new_SupixTree(void *p = 0);
   static void *newArray_SupixTree(Long_t size, void *p);
   static void delete_SupixTree(void *p);
   static void deleteArray_SupixTree(void *p);
   static void destruct_SupixTree(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::SupixTree*)
   {
      ::SupixTree *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(::SupixTree));
      static ::ROOT::TGenericClassInfo 
         instance("SupixTree", "SupixTree.h", 17,
                  typeid(::SupixTree), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &SupixTree_Dictionary, isa_proxy, 0,
                  sizeof(::SupixTree) );
      instance.SetNew(&new_SupixTree);
      instance.SetNewArray(&newArray_SupixTree);
      instance.SetDelete(&delete_SupixTree);
      instance.SetDeleteArray(&deleteArray_SupixTree);
      instance.SetDestructor(&destruct_SupixTree);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::SupixTree*)
   {
      return GenerateInitInstanceLocal((::SupixTree*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal((const ::SupixTree*)0x0); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *SupixTree_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal((const ::SupixTree*)0x0)->GetClass();
      SupixTree_TClassManip(theClass);
   return theClass;
   }

   static void SupixTree_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   static TClass *SupixAnly_Dictionary();
   static void SupixAnly_TClassManip(TClass*);
   static void *new_SupixAnly(void *p = 0);
   static void *newArray_SupixAnly(Long_t size, void *p);
   static void delete_SupixAnly(void *p);
   static void deleteArray_SupixAnly(void *p);
   static void destruct_SupixAnly(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::SupixAnly*)
   {
      ::SupixAnly *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(::SupixAnly));
      static ::ROOT::TGenericClassInfo 
         instance("SupixAnly", "SupixAnly.h", 56,
                  typeid(::SupixAnly), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &SupixAnly_Dictionary, isa_proxy, 0,
                  sizeof(::SupixAnly) );
      instance.SetNew(&new_SupixAnly);
      instance.SetNewArray(&newArray_SupixAnly);
      instance.SetDelete(&delete_SupixAnly);
      instance.SetDeleteArray(&deleteArray_SupixAnly);
      instance.SetDestructor(&destruct_SupixAnly);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::SupixAnly*)
   {
      return GenerateInitInstanceLocal((::SupixAnly*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal((const ::SupixAnly*)0x0); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *SupixAnly_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal((const ::SupixAnly*)0x0)->GetClass();
      SupixAnly_TClassManip(theClass);
   return theClass;
   }

   static void SupixAnly_TClassManip(TClass* ){
   }

} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr RunInfo::fgIsA(0);  // static to hold class pointer

//______________________________________________________________________________
const char *RunInfo::Class_Name()
{
   return "RunInfo";
}

//______________________________________________________________________________
const char *RunInfo::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)0x0)->GetImplFileName();
}

//______________________________________________________________________________
int RunInfo::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)0x0)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *RunInfo::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)0x0)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *RunInfo::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)0x0)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void RunInfo::Streamer(TBuffer &R__b)
{
   // Stream an object of class RunInfo.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(RunInfo::Class(),this);
   } else {
      R__b.WriteClassBuffer(RunInfo::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_RunInfo(void *p) {
      return  p ? new(p) ::RunInfo : new ::RunInfo;
   }
   static void *newArray_RunInfo(Long_t nElements, void *p) {
      return p ? new(p) ::RunInfo[nElements] : new ::RunInfo[nElements];
   }
   // Wrapper around operator delete
   static void delete_RunInfo(void *p) {
      delete ((::RunInfo*)p);
   }
   static void deleteArray_RunInfo(void *p) {
      delete [] ((::RunInfo*)p);
   }
   static void destruct_RunInfo(void *p) {
      typedef ::RunInfo current_t;
      ((current_t*)p)->~current_t();
   }
} // end of namespace ROOT for class ::RunInfo

namespace ROOT {
   // Wrappers around operator new
   static void *new_SupixTree(void *p) {
      return  p ? new(p) ::SupixTree : new ::SupixTree;
   }
   static void *newArray_SupixTree(Long_t nElements, void *p) {
      return p ? new(p) ::SupixTree[nElements] : new ::SupixTree[nElements];
   }
   // Wrapper around operator delete
   static void delete_SupixTree(void *p) {
      delete ((::SupixTree*)p);
   }
   static void deleteArray_SupixTree(void *p) {
      delete [] ((::SupixTree*)p);
   }
   static void destruct_SupixTree(void *p) {
      typedef ::SupixTree current_t;
      ((current_t*)p)->~current_t();
   }
} // end of namespace ROOT for class ::SupixTree

namespace ROOT {
   // Wrappers around operator new
   static void *new_SupixAnly(void *p) {
      return  p ? new(p) ::SupixAnly : new ::SupixAnly;
   }
   static void *newArray_SupixAnly(Long_t nElements, void *p) {
      return p ? new(p) ::SupixAnly[nElements] : new ::SupixAnly[nElements];
   }
   // Wrapper around operator delete
   static void delete_SupixAnly(void *p) {
      delete ((::SupixAnly*)p);
   }
   static void deleteArray_SupixAnly(void *p) {
      delete [] ((::SupixAnly*)p);
   }
   static void destruct_SupixAnly(void *p) {
      typedef ::SupixAnly current_t;
      ((current_t*)p)->~current_t();
   }
} // end of namespace ROOT for class ::SupixAnly

namespace {
  void TriggerDictionaryInitialization_libsupix_dict_Impl() {
    static const char* headers[] = {
"RunInfo.h",
"SupixAnly.h",
"SupixTree.h",
0
    };
    static const char* includePaths[] = {
"/opt/local/libexec/root6/include/root",
"/Users/lilong/workarea/Supix/Supix-1/supix_anal/supix/",
0
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "libsupix_dict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
class __attribute__((annotate("$clingAutoload$RunInfo.h")))  RunInfo;
class __attribute__((annotate("$clingAutoload$SupixTree.h")))  __attribute__((annotate("$clingAutoload$SupixAnly.h")))  SupixTree;
class __attribute__((annotate("$clingAutoload$SupixAnly.h")))  SupixAnly;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "libsupix_dict dictionary payload"


#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "RunInfo.h"
#include "SupixAnly.h"
#include "SupixTree.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"RunInfo", payloadCode, "@",
"SupixAnly", payloadCode, "@",
"SupixTree", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("libsupix_dict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_libsupix_dict_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_libsupix_dict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_libsupix_dict() {
  TriggerDictionaryInitialization_libsupix_dict_Impl();
}
