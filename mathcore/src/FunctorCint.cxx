// implentation of functor for interpreted functions

#if !defined(__sun)

#define MAKE_CINT_FUNCTOR

#include <Math/Functor.h>
#include <iostream>

#include "TClass.h"
#include "TMethodCall.h"
#include "TError.h"
#include "G__ci.h"

namespace ROOT { 
namespace Math { 

//#if defined(__MAKECINT__) || defined(G__DICTIONARY) 

//Functor handler for Cint functions

template<class ParentFunctor>
class FunctorCintHandler : public ParentFunctor::Impl
{ 
public:
   typedef typename ParentFunctor::Impl ImplFunc; 
   typedef typename ImplFunc::BaseFunc BaseFunc; 

   // for 1D functor1d
   FunctorCintHandler(void * p, const char * className , const char * methodName, const char * derivName = 0 );

   // for GradFunctor1D
   FunctorCintHandler(void * p1, void * p2 );

   // for Functor
   FunctorCintHandler(void * p, unsigned int dim, const char * className , const char * methodName, const char * derivName = 0 );

   // for GradFunctor
   FunctorCintHandler(void * p1, void * p2, unsigned int dim );

   //copy ctor (need for cloning)
   FunctorCintHandler(const FunctorCintHandler<Functor> & rhs) :
      BaseFunc(),
      fDim(rhs.fDim),
      fPtr(rhs.fPtr), 
      fPtr2(0),
      fMethodCall(rhs.fMethodCall),
      fMethodCall2(0)
   {}
   FunctorCintHandler(const FunctorCintHandler<GradFunctor> & rhs) :
      BaseFunc(),
      ImplFunc(),
      fDim(rhs.fDim),
      fPtr(rhs.fPtr), 
      fPtr2(rhs.fPtr2),
      fMethodCall(rhs.fMethodCall),
      fMethodCall2(rhs.fMethodCall2)
   {}
   FunctorCintHandler(const FunctorCintHandler<Functor1D> & rhs) :
      BaseFunc(),
      fDim(1),
      fPtr(rhs.fPtr), 
      fPtr2(0),
      fMethodCall(rhs.fMethodCall),
      fMethodCall2(0)
   {}
   FunctorCintHandler(const FunctorCintHandler<GradFunctor1D> & rhs) :
      BaseFunc(),
      ImplFunc(),
      fDim(1),
      fPtr(rhs.fPtr), 
      fPtr2(rhs.fPtr2),
      fMethodCall(rhs.fMethodCall),
      fMethodCall2(rhs.fMethodCall2)
   {}

   ~FunctorCintHandler() { //no op (keep pointer to TMethodCall)
   }
   BaseFunc  * Clone() const {  return new FunctorCintHandler(*this);  } 

   unsigned int NDim() const { 
      return fDim;
   } 


private:

   unsigned int fDim;

   void * fPtr; // pointer to callable object
   void * fPtr2; // pointer to callable object

   // function required by interface
   inline double DoEval (double ) const; 
   inline double DoDerivative (double ) const; 
   inline double DoDerivative (const double *,unsigned int  ) const; 
   inline double DoEval (const double * x) const; 


   mutable TMethodCall *fMethodCall; // pointer to method call
   mutable TMethodCall *fMethodCall2; // pointer to second method call (for deriv)

   mutable Long_t fArgs[2]; // for the address
};

//implementation of Functor methods
Functor::Functor(void * p, unsigned int dim, const char * className , const char * methodName ) :
   fImpl(new FunctorCintHandler<Functor>(p,dim,className,methodName) )
{}


Functor1D::Functor1D(void * p, const char * className , const char * methodName ) :
   fImpl(new FunctorCintHandler<Functor1D>(p,className,methodName) )
{}

GradFunctor1D::GradFunctor1D(void * p, const char * className , const char * methodName, const char * derivName ) :
   fImpl(new FunctorCintHandler<GradFunctor1D>(p,className,methodName,derivName) )
{}

GradFunctor1D::GradFunctor1D(void * p1, void * p2 ) :
   fImpl(new FunctorCintHandler<GradFunctor1D>(p1,p2) )
{}

GradFunctor::GradFunctor(void * p, unsigned int dim, const char * className , const char * methodName, const char * derivName ) :
   fImpl(new FunctorCintHandler<GradFunctor>(p,dim,className,methodName,derivName) )
{}

GradFunctor::GradFunctor(void * p1, void * p2, unsigned int dim ) :
   fImpl(new FunctorCintHandler<GradFunctor>(p1,p2,dim) )
{}


template<class PF>
FunctorCintHandler<PF>::FunctorCintHandler(void * p, const char * className , const char * methodName, const char * derivMethodName ):    fDim(1)  {

   //constructor for non-grad 1D functions
   fPtr = p; 
   fMethodCall2 = 0;
   fPtr2 = 0;

   //std::cout << "creating Cint functor" << std::endl;
   fMethodCall = new TMethodCall();
   
   if (className == 0) { 
      char *funcname = G__p2f2funcname((void *) fPtr);

      if (funcname) 
         fMethodCall->InitWithPrototype(funcname,"double");
   }
   else {

      TClass *cl = TClass::GetClass(className);

      if (cl) {

         if (methodName)
            fMethodCall->InitWithPrototype(cl,methodName,"double");
         else {
            fMethodCall->InitWithPrototype(cl,"operator()","double");
         }
         if (derivMethodName) {
            fMethodCall2 = new TMethodCall();
            fMethodCall2->InitWithPrototype(cl,derivMethodName,"double");
         }
         
         if (! fMethodCall->IsValid() ) {
            if (methodName)
               Error("ROOT::Math::FunctorCintHandler","No function found in class %s with the signature %s(double ) ",className,methodName);
            else
               Error("ROOT::Math::FunctorCintHandler","No function found in class %s with the signature operator() ( double ) ",className);
         }

         if (fMethodCall2 && ! fMethodCall2->IsValid() ) {
               Error("ROOT::Math::FunctorCintHandler","No function found in class %s with the signature %s(double ) ",className,derivMethodName);
         }
      } else {
         Error("ROOT::Math::FunctorCintHandler","can not find any class with name %s at the address 0x%x",className,fPtr);
      }
   }
}


template<class PF>
FunctorCintHandler<PF>::FunctorCintHandler(void * p1, void * p2 ) :    fDim(1) {
   //constructor for grad 1D functions
   fPtr = p1; 
   fPtr2 = p2;

   //std::cout << "creating Grad Cint functor" << std::endl;
   fMethodCall = new TMethodCall();
   fMethodCall2 = new TMethodCall();
   
   char *funcname = G__p2f2funcname((void *) fPtr);
   
   if (funcname) 
      fMethodCall->InitWithPrototype(funcname,"double");
   
   char *funcname2 = G__p2f2funcname((void *) fPtr2);
   
   if (funcname2) 
      fMethodCall2->InitWithPrototype(funcname2,"double");

   if (! fMethodCall->IsValid() ) {
      Error("ROOT::Math::FunctorCintHandler","No function %s found with the signature double () ( double ) at the address 0x%x",funcname,fPtr);
   }
   if (! fMethodCall2->IsValid() ) {
      Error("ROOT::Math::FunctorCintHandler","No function %s found with the signature double () ( double ) at the address 0x%x",funcname2,fPtr2);
   }
}

template<class PF>
FunctorCintHandler<PF>::FunctorCintHandler(void * p, unsigned int ndim, const char * className , const char * methodName, const char * derivMethodName ) :    fDim(ndim) {
   // for multi-dim functions
   fPtr = p; 
   fMethodCall2 = 0;
   fPtr2 = 0;

   //std::cout << "creating Cint functor" << std::endl;
   fMethodCall = new TMethodCall();
   
   if (className == 0) { 
      char *funcname = G__p2f2funcname((void *) fPtr);

      if (funcname) 
         fMethodCall->InitWithPrototype(funcname,"const double*");
   }
   else {

      TClass *cl = TClass::GetClass(className);

      if (cl) {

         if (methodName)
            fMethodCall->InitWithPrototype(cl,methodName,"const double*");
         else {
            fMethodCall->InitWithPrototype(cl,"operator()","const double*");
         }
         if (derivMethodName) {
            fMethodCall2 = new TMethodCall();
            fMethodCall2->InitWithPrototype(cl,derivMethodName,"const double*,unsigned int");
         }
         
         if (! fMethodCall->IsValid() ) {
            if (methodName)
               Error("ROOT::Math::FunctorCintHandler","No function found in class %s with the signature %s(const double *) ",className,methodName);
            else
               Error("ROOT::Math::FunctorCintHandler","No function found in class %s with the signature operator() (const double * ) ",className);
         }

         if (fMethodCall2 && ! fMethodCall2->IsValid() ) {
               Error("ROOT::Math::FunctorCintHandler","No function found in class %s with the signature %s(const double *, unsigned int ) ",className,derivMethodName);
         }
      } else {
         Error("ROOT::Math::FunctorCintHandler","can not find any class with name %s at the address 0x%x",className,fPtr);
      }
   }
}



template<class PF>
FunctorCintHandler<PF>::FunctorCintHandler(void * p1, void * p2, unsigned int dim ) :    fDim(dim) {
   //constructor for grad 1D functions

   fPtr = p1; 
   fPtr2 = p2;

   //std::cout << "creating Grad Cint functor" << std::endl;
   fMethodCall = new TMethodCall();
   fMethodCall2 = new TMethodCall();
   
   char *funcname = G__p2f2funcname((void *) fPtr);
   
   if (funcname) 
      fMethodCall->InitWithPrototype(funcname,"const double *");
   
   char *funcname2 = G__p2f2funcname((void *) fPtr2);
   
   if (funcname2) 
      fMethodCall2->InitWithPrototype(funcname2,"const double *,UInt_t");

   if (! fMethodCall->IsValid() ) {
      Error("ROOT::Math::FunctorCintHandler","No function %s found with the signature double () (const double * ) at the address 0x%x",funcname,fPtr);
   }
   if (! fMethodCall2->IsValid() ) {
      Error("ROOT::Math::FunctorCintHandler","No function %s found with the signature double () (const double *, unsigned int) at the address 0x%x",funcname2,fPtr2);
   }
}

template<class PF>
inline double FunctorCintHandler<PF>::DoEval (double x) const { 
   //fArgs[0] = (Long_t)&x; 
   fMethodCall->ResetParam();
   fMethodCall->SetParam(x);
   double result = 0; 
   fMethodCall->Execute(fPtr,result);
   //std::cout << "execute doeval for x = " << x << " on " << fPtr << "  result " << result << std::endl;
   return result;    
}
template<class PF>
inline double FunctorCintHandler<PF>::DoDerivative (double x) const { 
   if (!fMethodCall2) return 0;

   fMethodCall2->ResetParam();
   fMethodCall2->SetParam(x);
   double result = 0;
   
   if (fPtr2) { 
      fMethodCall2->Execute(fPtr2,result);
      //std::cout << "execute doDerivative for x = " << x << " on " << fPtr2 << "  result " << result << std::endl;
   }
   else {
      fMethodCall2->Execute(fPtr,result);
   }
   return result;    
}

template<class PF>
inline double FunctorCintHandler<PF>::DoEval (const double *x) const { 
   // for multi-dim functions
   //fArgs[0] = (Long_t)&x; 
   fMethodCall->ResetParam();
   fArgs[0] = (Long_t)x;
   fMethodCall->SetParamPtrs(fArgs);
   double result = 0; 
   fMethodCall->Execute(fPtr,result);
   //std::cout << "execute doeval for x = " << x << " on " << fPtr << "  result " << result << std::endl;
   return result;    
}

template<class PF>
inline double FunctorCintHandler<PF>::DoDerivative (const double *x, unsigned int ipar) const { 
   // derivative for multi-dim functions
   fMethodCall2->ResetParam();
   fArgs[0] = (Long_t)&x;
   fArgs[1] = (Long_t)&ipar;
   fMethodCall2->SetParamPtrs(fArgs);

   double result = 0; 

   if (fPtr2) { 
      fMethodCall2->Execute(fPtr2,result);
      std::cout << "execute doDerivative for x = " << *x << " on " << fPtr2 << "  result " << result << std::endl;
   }
   else {
      fMethodCall2->Execute(fPtr,result);
   }
   return result;    
}

} //end namespace Math

} //end namespace ROOT

#undef MAKE_CINT_FUNCTOR
#endif // if not sun is defined
