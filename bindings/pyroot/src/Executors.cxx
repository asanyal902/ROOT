// @(#)root/pyroot:$Id$
// Author: Wim Lavrijsen, Jan 2005

// Bindings
#include "PyROOT.h"
#include "PyStrings.h"
#include "Executors.h"
#include "ObjectProxy.h"
#include "TPyBufferFactory.h"
#include "RootWrapper.h"
#include "Utility.h"

// ROOT
#include "TClass.h"
#include "TClassEdit.h"
#include "TInterpreter.h"

// Standard
#include <utility>
#include <sstream>


//- data ______________________________________________________________________
PyROOT::ExecFactories_t PyROOT::gExecFactories;


//- helpers -------------------------------------------------------------------
static inline
void PRCallFuncExec( CallFunc_t* func, void* self, Bool_t release_gil ) {
   if ( release_gil ) {
      Py_BEGIN_ALLOW_THREADS
      gInterpreter->CallFunc_Exec( func, self );
      Py_END_ALLOW_THREADS
   } else
      gInterpreter->CallFunc_ExecInt( func, self );
}

static inline
Long_t PRCallFuncExecInt( CallFunc_t* func, void* self, Bool_t release_gil ) {
   Long_t result;
   if ( release_gil ) {
      Py_BEGIN_ALLOW_THREADS
         result = (Long_t)gInterpreter->CallFunc_ExecInt( func, self );
      Py_END_ALLOW_THREADS
   } else
      result = (Long_t)gInterpreter->CallFunc_ExecInt( func, self );
   return result;
}

static inline
Double_t PRCallFuncExecDouble( CallFunc_t* func, void* self, Bool_t release_gil ) {
   Double_t result;
   if ( release_gil ) {
      Py_BEGIN_ALLOW_THREADS
      result = (Double_t)gInterpreter->CallFunc_ExecDouble( func, self );
      Py_END_ALLOW_THREADS
   } else
      result = (Double_t)gInterpreter->CallFunc_ExecDouble( func, self );
   return result;
}


//- executors for built-ins ---------------------------------------------------
PyObject* PyROOT::TBoolExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python bool return value
   PyObject* result =
      (Bool_t)PRCallFuncExecInt( func, self, release_gil ) ? Py_True : Py_False;
   Py_INCREF( result );
   return result;
}

PyObject* PyROOT::TLongExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python long return value
   return PyLong_FromLong( (Long_t)PRCallFuncExecInt( func, self, release_gil ) );
}

//____________________________________________________________________________
PyObject* PyROOT::TCharExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python string return value
   return PyROOT_PyUnicode_FromFormat( "%c", (Int_t)PRCallFuncExecInt( func, self, release_gil ) );
}

//____________________________________________________________________________
PyObject* PyROOT::TIntExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python int return value
   return PyInt_FromLong( (Long_t)PRCallFuncExecInt( func, self, release_gil ) );
}

//____________________________________________________________________________
PyObject* PyROOT::TULongExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python unsigned long return value
   return PyLong_FromUnsignedLong( (ULong_t)PRCallFuncExecInt( func, self, release_gil ) );
}

//____________________________________________________________________________
PyObject* PyROOT::TLongLongExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python long long return value
// (CLING) TODO: this was returning a G__value
   Long64_t result;
   if ( release_gil ) {
      Py_BEGIN_ALLOW_THREADS
      result = (Long64_t)gInterpreter->CallFunc_ExecInt64( func, self );
      Py_END_ALLOW_THREADS
   } else
      result = (Long64_t)gInterpreter->CallFunc_ExecInt64( func, self );

   return PyLong_FromLongLong( result );
}

//____________________________________________________________________________
PyObject* PyROOT::TULongLongExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python unsigned long long return value
// (CLING) TODO: this was returning a G__value
   ULong64_t result;
   if ( release_gil ) {
      Py_BEGIN_ALLOW_THREADS
      result = (ULong64_t)gInterpreter->CallFunc_ExecInt64( func, self );
      Py_END_ALLOW_THREADS
   } else
      result = (ULong64_t)gInterpreter->CallFunc_ExecInt64( func, self );

   return PyLong_FromUnsignedLongLong( result );
}

//____________________________________________________________________________
PyObject* PyROOT::TDoubleExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python float return value
   return PyFloat_FromDouble( (Double_t)PRCallFuncExecDouble( func, self, release_gil ) );
}

//____________________________________________________________________________
Bool_t PyROOT::TRefExecutor::SetAssignable( PyObject* pyobject )
{
// prepare "buffer" for by-ref returns, used with __setitem__
   if ( pyobject != 0 ) {
      Py_INCREF( pyobject );
      fAssignable = pyobject;
      return kTRUE;
   }

   fAssignable = 0;
   return kFALSE;
}

//____________________________________________________________________________
#define PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( name, type, stype, F1, F2, CF )  \
PyObject* PyROOT::T##name##RefExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )\
{                                                                            \
   type* ref = (type*)PRCallFuncExecInt( func, self, release_gil );          \
   if ( ! fAssignable )                                                      \
      return F1( (stype)*ref );                                              \
   else {                                                                    \
      *ref = (type)F2( fAssignable );                                        \
      Py_DECREF( fAssignable );                                              \
      fAssignable = 0;                                                       \
      Py_INCREF( Py_None );                                                  \
      return Py_None;                                                        \
   }                                                                         \
}

PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( Short,  Short_t,  Long_t,   PyInt_FromLong,     PyLong_AsLong,    PRCallFuncExecInt )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( UShort, UShort_t, ULong_t,  PyInt_FromLong,     PyLongOrInt_AsULong, PRCallFuncExecInt )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( Int,    Int_t,    Long_t,   PyInt_FromLong,     PyLong_AsLong,    PRCallFuncExecInt )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( UInt,   UInt_t,   ULong_t,  PyLong_FromUnsignedLong, PyLongOrInt_AsULong, PRCallFuncExecInt )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( Long,   Long_t,   Long_t,   PyLong_FromLong,    PyLong_AsLong,    PRCallFuncExecInt )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( ULong,  ULong_t,  ULong_t,  PyLong_FromUnsignedLong, PyLongOrInt_AsULong, PRCallFuncExecInt )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( Float,  Float_t,  Double_t, PyFloat_FromDouble, PyFloat_AsDouble, PRCallFuncExecDouble )
PYROOT_IMPLEMENT_BASIC_REFEXECUTOR( Double, Double_t, Double_t, PyFloat_FromDouble, PyFloat_AsDouble, PRCallFuncExecDouble )

//____________________________________________________________________________
PyObject* PyROOT::TSTLStringRefExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, return python string return value
   if ( ! fAssignable ) {
      std::string* result = (std::string*)PRCallFuncExecInt( func, self, release_gil );
      return PyROOT_PyUnicode_FromStringAndSize( result->c_str(), result->size() );
   } else {
      std::string* result = (std::string*)PRCallFuncExecInt( func, self, release_gil );
      *result = std::string(
         PyROOT_PyUnicode_AsString( fAssignable ), PyROOT_PyUnicode_GET_SIZE( fAssignable ) );

      Py_DECREF( fAssignable );
      fAssignable = 0;

      Py_INCREF( Py_None );
      return Py_None;
   }
}

//____________________________________________________________________________
PyObject* PyROOT::TVoidExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, return None
   PRCallFuncExec( func, self, release_gil );
   Py_INCREF( Py_None );
   return Py_None;
}

//____________________________________________________________________________
PyObject* PyROOT::TCStringExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python string return value
   char* result = (char*)PRCallFuncExecInt( func, self, release_gil );
   if ( ! result ) {
      Py_INCREF( PyStrings::gEmptyString );
      return PyStrings::gEmptyString;
   }

   return PyROOT_PyUnicode_FromString( result );
}


//- pointer/array executors ---------------------------------------------------
PyObject* PyROOT::TVoidArrayExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python long return value
   return BufFac_t::Instance()->PyBuffer_FromMemory( (Long_t*)PRCallFuncExecInt( func, self, release_gil ), 1 );
}

//____________________________________________________________________________
#define PYROOT_IMPLEMENT_ARRAY_EXECUTOR( name, type )                        \
PyObject* PyROOT::T##name##ArrayExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )\
{                                                                            \
   return BufFac_t::Instance()->PyBuffer_FromMemory( (type*)PRCallFuncExecInt( func, self, release_gil ) );\
}

PYROOT_IMPLEMENT_ARRAY_EXECUTOR( Bool,   Bool_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( Short,  Short_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( UShort, UShort_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( Int,    Int_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( UInt,   UInt_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( Long,   Long_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( ULong,  ULong_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( Float,  Float_t )
PYROOT_IMPLEMENT_ARRAY_EXECUTOR( Double, Double_t )


//- special cases ------------------------------------------------------------
PyObject* PyROOT::TSTLStringExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// TODO: Cling can not handle return by value for now
   PyErr_SetString(PyExc_NotImplementedError, "CLING DOES NOT SUPPORT RETURN BY VALUE!" );
   return 0;

// execute <func> with argument <self>, construct python string return value
   std::string* result = (std::string*)PRCallFuncExecInt( func, self, release_gil );
   if ( ! result ) {
      Py_INCREF( PyStrings::gEmptyString );
      return PyStrings::gEmptyString;
   }

   PyObject* pyresult =
      PyROOT_PyUnicode_FromStringAndSize( result->c_str(), result->size() );

// TODO: take over ownership from Cling somehow (old:  G__pop_tempobject_nodel(); delete result;)

   return pyresult;
}

//____________________________________________________________________________
PyObject* PyROOT::TTGlobalExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python ROOT object return value
   return BindRootGlobal( (TGlobal*)PRCallFuncExecInt( func, self, release_gil ) );
}

//____________________________________________________________________________
PyObject* PyROOT::TRootObjectExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python ROOT object return value
   return BindRootObject( (void*)PRCallFuncExecInt( func, self, release_gil ), fClass );
}

//____________________________________________________________________________
PyObject* PyROOT::TRootObjectByValueExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// TODO: Cling can not handle return by value for now
   PyErr_SetString(PyExc_NotImplementedError, "CLING DOES NOT SUPPORT RETURN BY VALUE!" );
   return 0;

// execution will bring a temporary in existence
   void* result = (void*)PRCallFuncExecInt( func, self, release_gil );
   if ( ! result ) {
      if ( ! PyErr_Occurred() )         // callee may have set a python error itself
         PyErr_SetString( PyExc_ValueError, "NULL result where temporary expected" );
      return 0;
   }

// TODO: resolve possible ownership issues (old: G__pop_tempobject_nodel();)
// Can copy over the element, then gInterpreter->ClearStack(); will erase it

// the result can then be bound
   ObjectProxy* pyobj = (ObjectProxy*)BindRootObjectNoCast( result, fClass );
   if ( ! pyobj )
      return 0;

// python ref counting will now control this object's life span
   pyobj->fFlags |= ObjectProxy::kIsOwner;
   return (PyObject*)pyobj;
}

//____________________________________________________________________________
PyObject* PyROOT::TRootObjectRefExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// executor binds the result to the left-hand side, overwriting if an old object
   PyObject* result = BindRootObject( (void*)PRCallFuncExecInt( func, self, release_gil ), fClass );
   if ( ! result || ! fAssignable )
      return result;
   else {
   // this generic code is quite slow compared to its C++ equivalent ...
      PyObject* res2 = PyObject_CallMethod( result,
         const_cast< char* >( "__assign__" ), const_cast< char* >( "O" ), fAssignable );

      Py_DECREF( result );
      Py_DECREF( fAssignable );
      fAssignable = 0;

      if ( res2 ) {
         Py_DECREF( res2 );             // typically, *this from operator=()
         Py_INCREF( Py_None );
         return Py_None;
      }

      return 0;
   }
}

//____________________________________________________________________________
PyObject* PyROOT::TRootObjectPtrExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, construct python ROOT object return ptr value
   return BindRootObject( (void*)PRCallFuncExecInt( func, self, release_gil ), fClass, kTRUE );
}

//____________________________________________________________________________
PyObject* PyROOT::TConstructorExecutor::Execute( CallFunc_t* func, void* klass, Bool_t release_gil )
{
// package return address in PyObject* for caller to handle appropriately
   return (PyObject*)PRCallFuncExecInt( func, klass, release_gil );
}

//____________________________________________________________________________
PyObject* PyROOT::TPyObjectExecutor::Execute( CallFunc_t* func, void* self, Bool_t release_gil )
{
// execute <func> with argument <self>, return python object
   return (PyObject*)PRCallFuncExecInt( func, self, release_gil );
}


//- factories -----------------------------------------------------------------
PyROOT::TExecutor* PyROOT::CreateExecutor( const std::string& fullType )
{
// The matching of the fulltype to an executor factory goes through up to 4 levels:
//   1) full, qualified match
//   2) drop '&' as by ref/full type is often pretty much the same python-wise
//   3) ROOT classes, either by ref/ptr or by value
//   4) additional special case for enums
//
// If all fails, void is used, which will cause the return type to be ignored on use

// an exactly matching executor is best
   ExecFactories_t::iterator h = gExecFactories.find( fullType );
   if ( h != gExecFactories.end() )
      return (h->second)();

// resolve typedefs etc., and collect qualifiers
   std::string resolvedType = TClassEdit::ResolveTypedef( fullType.c_str(), true );

// a full, qualified matching executor is preferred
   h = gExecFactories.find( resolvedType );
   if ( h != gExecFactories.end() )
      return (h->second)();

//-- nothing? ok, collect information about the type and possible qualifiers/decorators
   const std::string& cpd = Utility::Compound( resolvedType );
   std::string realType = TClassEdit::ShortType( resolvedType.c_str(), 1 );

// const-ness (dropped by TClassEdit::ShortType) is in general irrelevant
   h = gExecFactories.find( realType + cpd );
   if ( h != gExecFactories.end() )
      return (h->second)();

// ROOT classes and special cases (enum)
   TExecutor* result = 0;
   if ( TClass* klass = TClass::GetClass( realType.c_str() ) ) {
      if ( cpd == "" )
         result = new TRootObjectByValueExecutor( klass );
      else if ( cpd == "&" )
         result = new TRootObjectRefExecutor( klass );
      else if ( cpd == "**" || cpd == "*&" || cpd == "&*" )
         result = new TRootObjectPtrExecutor( klass );
      else
         result = new TRootObjectExecutor( klass );
   } else if ( gInterpreter->ClassInfo_IsEnum( realType.c_str() ) ) {
   // could still be an enum ...
      h = gExecFactories.find( "UInt_t" );
   } else {
   // handle (with warning) unknown types
      std::stringstream s;
      s << "creating executor for unknown type \"" << fullType << "\"" << std::ends;
      PyErr_Warn( PyExc_RuntimeWarning, (char*)s.str().c_str() );
   // void* may work ("user knows best"), void will fail on use of return value
      h = (cpd == "") ? gExecFactories.find( "void" ) : gExecFactories.find( "void*" );
   }

   if ( ! result && h != gExecFactories.end() )
   // executor factory available, use it to create executor
      result = (h->second)();

   return result;                  // may still be null
}

//____________________________________________________________________________
#define PYROOT_EXECUTOR_FACTORY( name )                \
TExecutor* Create##name##Executor()                    \
{                                                      \
   return new T##name##Executor;                       \
}

namespace {

   using namespace PyROOT;

// use macro rather than template for portability ...
   PYROOT_EXECUTOR_FACTORY( Bool )
   PYROOT_EXECUTOR_FACTORY( Char )
   PYROOT_EXECUTOR_FACTORY( ShortRef )
   PYROOT_EXECUTOR_FACTORY( UShortRef )
   PYROOT_EXECUTOR_FACTORY( Int )
   PYROOT_EXECUTOR_FACTORY( IntRef )
   PYROOT_EXECUTOR_FACTORY( UIntRef )
   PYROOT_EXECUTOR_FACTORY( ULong )
   PYROOT_EXECUTOR_FACTORY( ULongRef )
   PYROOT_EXECUTOR_FACTORY( Long )
   PYROOT_EXECUTOR_FACTORY( LongRef )
   PYROOT_EXECUTOR_FACTORY( FloatRef )
   PYROOT_EXECUTOR_FACTORY( Double )
   PYROOT_EXECUTOR_FACTORY( DoubleRef )
   PYROOT_EXECUTOR_FACTORY( Void )
   PYROOT_EXECUTOR_FACTORY( LongLong )
   PYROOT_EXECUTOR_FACTORY( ULongLong )
   PYROOT_EXECUTOR_FACTORY( CString )
   PYROOT_EXECUTOR_FACTORY( VoidArray )
   PYROOT_EXECUTOR_FACTORY( BoolArray )
   PYROOT_EXECUTOR_FACTORY( ShortArray )
   PYROOT_EXECUTOR_FACTORY( UShortArray )
   PYROOT_EXECUTOR_FACTORY( IntArray )
   PYROOT_EXECUTOR_FACTORY( UIntArray )
   PYROOT_EXECUTOR_FACTORY( LongArray )
   PYROOT_EXECUTOR_FACTORY( ULongArray )
   PYROOT_EXECUTOR_FACTORY( FloatArray )
   PYROOT_EXECUTOR_FACTORY( DoubleArray )
   PYROOT_EXECUTOR_FACTORY( STLString )
   PYROOT_EXECUTOR_FACTORY( STLStringRef )
   PYROOT_EXECUTOR_FACTORY( TGlobal )
   PYROOT_EXECUTOR_FACTORY( Constructor )
   PYROOT_EXECUTOR_FACTORY( PyObject )

// executor factories for ROOT types
   typedef std::pair< const char*, ExecutorFactory_t > NFp_t;

   NFp_t factories_[] = {
   // factories for built-ins
      NFp_t( "char",               &CreateCharExecutor                ),
      NFp_t( "unsigned char",      &CreateCharExecutor                ),
      NFp_t( "short",              &CreateIntExecutor                 ),
      NFp_t( "short&",             &CreateShortRefExecutor            ),
      NFp_t( "unsigned short",     &CreateIntExecutor                 ),
      NFp_t( "unsigned short&",    &CreateUShortRefExecutor           ),
      NFp_t( "int",                &CreateIntExecutor                 ),
      NFp_t( "int&",               &CreateIntRefExecutor              ),
      NFp_t( "unsigned int",       &CreateULongExecutor               ),
      NFp_t( "unsigned int&",      &CreateUIntRefExecutor             ),
      NFp_t( "UInt_t", /* enum */  &CreateULongExecutor               ),
      NFp_t( "long",               &CreateLongExecutor                ),
      NFp_t( "long&",              &CreateLongRefExecutor             ),
      NFp_t( "unsigned long",      &CreateULongExecutor               ),
      NFp_t( "unsigned long&",     &CreateULongRefExecutor            ),
      NFp_t( "long long",          &CreateLongLongExecutor            ),
      NFp_t( "Long64_t",           &CreateLongLongExecutor            ),
      NFp_t( "unsigned long long", &CreateULongLongExecutor           ),
      NFp_t( "ULong64_t",          &CreateULongLongExecutor           ),
      NFp_t( "float",              &CreateDoubleExecutor              ),
      NFp_t( "float&",             &CreateFloatRefExecutor            ),
      NFp_t( "double",             &CreateDoubleExecutor              ),
      NFp_t( "double&",            &CreateDoubleRefExecutor           ),
      NFp_t( "void",               &CreateVoidExecutor                ),
      NFp_t( "bool",               &CreateBoolExecutor                ),
      NFp_t( "const char*",        &CreateCStringExecutor             ),
      NFp_t( "char*",              &CreateCStringExecutor             ),

   // pointer/array factories
      NFp_t( "void*",              &CreateVoidArrayExecutor           ),
      NFp_t( "bool*",              &CreateBoolArrayExecutor           ),
      NFp_t( "short*",             &CreateShortArrayExecutor          ),
      NFp_t( "unsigned short*",    &CreateUShortArrayExecutor         ),
      NFp_t( "int*",               &CreateIntArrayExecutor            ),
      NFp_t( "unsigned int*",      &CreateUIntArrayExecutor           ),
      NFp_t( "long*",              &CreateLongArrayExecutor           ),
      NFp_t( "unsigned long*",     &CreateULongArrayExecutor          ),
      NFp_t( "float*",             &CreateFloatArrayExecutor          ),
      NFp_t( "double*",            &CreateDoubleArrayExecutor         ),

   // factories for special cases
      NFp_t( "std::string",        &CreateSTLStringExecutor           ),
      NFp_t( "string",             &CreateSTLStringExecutor           ),
      NFp_t( "std::string&",       &CreateSTLStringRefExecutor        ),
      NFp_t( "string&",            &CreateSTLStringRefExecutor        ),
      NFp_t( "TGlobal*",           &CreateTGlobalExecutor             ),
      NFp_t( "__init__",           &CreateConstructorExecutor         ),
      NFp_t( "PyObject*",          &CreatePyObjectExecutor            ),
      NFp_t( "_object*",           &CreatePyObjectExecutor            ),
      NFp_t( "FILE*",              &CreateVoidArrayExecutor           )
   };

   struct InitExecFactories_t {
   public:
      InitExecFactories_t()
      {
      // load all executor factories in the global map 'gExecFactories'
         int nf = sizeof( factories_ ) / sizeof( factories_[ 0 ] );
         for ( int i = 0; i < nf; ++i ) {
            gExecFactories[ factories_[ i ].first ] = factories_[ i ].second;
         }
      }
   } initExecvFactories_;

} // unnamed namespace
