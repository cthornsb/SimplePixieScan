// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME dIhomedIcorydIoptdISimplePixieScandIdictdIsourcedI

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

// Since CINT ignores the std namespace, we need to do so in this file.
namespace std {} using namespace std;

// Header files passed as explicit arguments
#include "/home/cory/opt/SimplePixieScan/dict/include/Structures.h"

// Header files passed via #pragma extra_include

namespace ROOT {
   static void *new_Structure(void *p = 0);
   static void *newArray_Structure(Long_t size, void *p);
   static void delete_Structure(void *p);
   static void deleteArray_Structure(void *p);
   static void destruct_Structure(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::Structure*)
   {
      ::Structure *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::Structure >(0);
      static ::ROOT::TGenericClassInfo 
         instance("Structure", ::Structure::Class_Version(), "invalid", 24,
                  typeid(::Structure), DefineBehavior(ptr, ptr),
                  &::Structure::Dictionary, isa_proxy, 4,
                  sizeof(::Structure) );
      instance.SetNew(&new_Structure);
      instance.SetNewArray(&newArray_Structure);
      instance.SetDelete(&delete_Structure);
      instance.SetDeleteArray(&deleteArray_Structure);
      instance.SetDestructor(&destruct_Structure);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::Structure*)
   {
      return GenerateInitInstanceLocal((::Structure*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_(Init) = GenerateInitInstanceLocal((const ::Structure*)0x0); R__UseDummy(_R__UNIQUE_(Init));
} // end of namespace ROOT

namespace ROOT {
   static void *new_Trace(void *p = 0);
   static void *newArray_Trace(Long_t size, void *p);
   static void delete_Trace(void *p);
   static void deleteArray_Trace(void *p);
   static void destruct_Trace(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::Trace*)
   {
      ::Trace *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::Trace >(0);
      static ::ROOT::TGenericClassInfo 
         instance("Trace", ::Trace::Class_Version(), "invalid", 44,
                  typeid(::Trace), DefineBehavior(ptr, ptr),
                  &::Trace::Dictionary, isa_proxy, 4,
                  sizeof(::Trace) );
      instance.SetNew(&new_Trace);
      instance.SetNewArray(&newArray_Trace);
      instance.SetDelete(&delete_Trace);
      instance.SetDeleteArray(&deleteArray_Trace);
      instance.SetDestructor(&destruct_Trace);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::Trace*)
   {
      return GenerateInitInstanceLocal((::Trace*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_(Init) = GenerateInitInstanceLocal((const ::Trace*)0x0); R__UseDummy(_R__UNIQUE_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr Structure::fgIsA(0);  // static to hold class pointer

//______________________________________________________________________________
const char *Structure::Class_Name()
{
   return "Structure";
}

//______________________________________________________________________________
const char *Structure::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Structure*)0x0)->GetImplFileName();
}

//______________________________________________________________________________
int Structure::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Structure*)0x0)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *Structure::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Structure*)0x0)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *Structure::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD2(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Structure*)0x0)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
atomic_TClass_ptr Trace::fgIsA(0);  // static to hold class pointer

//______________________________________________________________________________
const char *Trace::Class_Name()
{
   return "Trace";
}

//______________________________________________________________________________
const char *Trace::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Trace*)0x0)->GetImplFileName();
}

//______________________________________________________________________________
int Trace::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Trace*)0x0)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *Trace::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Trace*)0x0)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *Trace::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD2(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Trace*)0x0)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void Structure::Streamer(TBuffer &R__b)
{
   // Stream an object of class Structure.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(Structure::Class(),this);
   } else {
      R__b.WriteClassBuffer(Structure::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_Structure(void *p) {
      return  p ? new(p) ::Structure : new ::Structure;
   }
   static void *newArray_Structure(Long_t nElements, void *p) {
      return p ? new(p) ::Structure[nElements] : new ::Structure[nElements];
   }
   // Wrapper around operator delete
   static void delete_Structure(void *p) {
      delete ((::Structure*)p);
   }
   static void deleteArray_Structure(void *p) {
      delete [] ((::Structure*)p);
   }
   static void destruct_Structure(void *p) {
      typedef ::Structure current_t;
      ((current_t*)p)->~current_t();
   }
} // end of namespace ROOT for class ::Structure

//______________________________________________________________________________
void Trace::Streamer(TBuffer &R__b)
{
   // Stream an object of class Trace.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(Trace::Class(),this);
   } else {
      R__b.WriteClassBuffer(Trace::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_Trace(void *p) {
      return  p ? new(p) ::Trace : new ::Trace;
   }
   static void *newArray_Trace(Long_t nElements, void *p) {
      return p ? new(p) ::Trace[nElements] : new ::Trace[nElements];
   }
   // Wrapper around operator delete
   static void delete_Trace(void *p) {
      delete ((::Trace*)p);
   }
   static void deleteArray_Trace(void *p) {
      delete [] ((::Trace*)p);
   }
   static void destruct_Trace(void *p) {
      typedef ::Trace current_t;
      ((current_t*)p)->~current_t();
   }
} // end of namespace ROOT for class ::Trace

namespace ROOT {
   static TClass *vectorlEintgR_Dictionary();
   static void vectorlEintgR_TClassManip(TClass*);
   static void *new_vectorlEintgR(void *p = 0);
   static void *newArray_vectorlEintgR(Long_t size, void *p);
   static void delete_vectorlEintgR(void *p);
   static void deleteArray_vectorlEintgR(void *p);
   static void destruct_vectorlEintgR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const vector<int>*)
   {
      vector<int> *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(vector<int>));
      static ::ROOT::TGenericClassInfo 
         instance("vector<int>", -2, "vector", 210,
                  typeid(vector<int>), DefineBehavior(ptr, ptr),
                  &vectorlEintgR_Dictionary, isa_proxy, 0,
                  sizeof(vector<int>) );
      instance.SetNew(&new_vectorlEintgR);
      instance.SetNewArray(&newArray_vectorlEintgR);
      instance.SetDelete(&delete_vectorlEintgR);
      instance.SetDeleteArray(&deleteArray_vectorlEintgR);
      instance.SetDestructor(&destruct_vectorlEintgR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::Pushback< vector<int> >()));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_(Init) = GenerateInitInstanceLocal((const vector<int>*)0x0); R__UseDummy(_R__UNIQUE_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *vectorlEintgR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal((const vector<int>*)0x0)->GetClass();
      vectorlEintgR_TClassManip(theClass);
   return theClass;
   }

   static void vectorlEintgR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_vectorlEintgR(void *p) {
      return  p ? ::new((::ROOT::TOperatorNewHelper*)p) vector<int> : new vector<int>;
   }
   static void *newArray_vectorlEintgR(Long_t nElements, void *p) {
      return p ? ::new((::ROOT::TOperatorNewHelper*)p) vector<int>[nElements] : new vector<int>[nElements];
   }
   // Wrapper around operator delete
   static void delete_vectorlEintgR(void *p) {
      delete ((vector<int>*)p);
   }
   static void deleteArray_vectorlEintgR(void *p) {
      delete [] ((vector<int>*)p);
   }
   static void destruct_vectorlEintgR(void *p) {
      typedef vector<int> current_t;
      ((current_t*)p)->~current_t();
   }
} // end of namespace ROOT for class vector<int>

namespace {
  void TriggerDictionaryInitialization__Impl() {
    static const char* headers[] = {
"/home/cory/opt/SimplePixieScan/dict/include/Structures.h",
0
    };
    static const char* includePaths[] = {
"/opt/root/root_v6.04.06/include",
"/home/cory/opt/SimplePixieScan/build/dict/source/",
0
    };
    static const char* fwdDeclCode = 
R"DICTFWDDCLS(
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_Autoloading_Map;
class __attribute__((annotate(R"ATTRDUMP(Structure)ATTRDUMP"))) __attribute__((annotate(R"ATTRDUMP(Structure)ATTRDUMP"))) __attribute__((annotate("$clingAutoload$/home/cory/opt/SimplePixieScan/dict/include/Structures.h")))  Structure;
class __attribute__((annotate(R"ATTRDUMP(Trace)ATTRDUMP"))) __attribute__((annotate(R"ATTRDUMP(Trace)ATTRDUMP"))) __attribute__((annotate("$clingAutoload$/home/cory/opt/SimplePixieScan/dict/include/Structures.h")))  Trace;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(

#ifndef G__VECTOR_HAS_CLASS_ITERATOR
  #define G__VECTOR_HAS_CLASS_ITERATOR 1
#endif

#define _BACKWARD_BACKWARD_WARNING_H
#include "/home/cory/opt/SimplePixieScan/dict/include/Structures.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[]={
"Structure", payloadCode, "@",
"Trace", payloadCode, "@",
nullptr};

    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization__Impl, {}, classesHeaders);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization__Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_() {
  TriggerDictionaryInitialization__Impl();
}
