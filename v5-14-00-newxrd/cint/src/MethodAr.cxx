/* /% C++ %/ */
/***********************************************************************
 * cint (C/C++ interpreter)
 ************************************************************************
 * Source file MethodAr.cxx
 ************************************************************************
 * Description:
 *  Extended Run Time Type Identification API
 ************************************************************************
 * Author                  Masaharu Goto 
 * Copyright(c) 1995~1999  Masaharu Goto 
 *
 * For the licensing terms see the file COPYING
 *
 ************************************************************************/

#include "Api.h"
#include "common.h"

/*********************************************************************
* class G__MethodArgInfo
*
* 
*********************************************************************/
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodArgInfo::Init(class G__MethodInfo &a)
{
  if(a.IsValid()) {
    belongingmethod = &a;
    argn = -1;
  }
  else {
    belongingmethod=(G__MethodInfo*)NULL;
  }
}
///////////////////////////////////////////////////////////////////////////
const char* Cint::G__MethodArgInfo::Name()
{
  if(IsValid()) {
    struct G__ifunc_table *ifunc;
    /* long property=0; */
    ifunc = (struct G__ifunc_table*)belongingmethod->handle;
    return(ifunc->para_name[belongingmethod->index][argn]); 
  }
  else {
    return((char*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
long Cint::G__MethodArgInfo::Property()
{
  if(IsValid()) {
    struct G__ifunc_table *ifunc;
    long property=0;
    ifunc = (struct G__ifunc_table*)belongingmethod->handle;
    if(isupper(ifunc->para_type[belongingmethod->index][argn])) 
      property|=G__BIT_ISPOINTER;
    if(ifunc->para_default[belongingmethod->index][argn]) 
      property|=G__BIT_ISDEFAULT;
    if(ifunc->para_reftype[belongingmethod->index][argn]) 
      property|=G__BIT_ISREFERENCE;
    if(ifunc->para_isconst[belongingmethod->index][argn]&G__CONSTVAR) 
      property|=G__BIT_ISCONSTANT;
    if(ifunc->para_isconst[belongingmethod->index][argn]&G__PCONSTVAR) 
      property|=G__BIT_ISPCONSTANT;
    return(property);
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
char* Cint::G__MethodArgInfo::DefaultValue()
{
  if(IsValid()) {
    struct G__ifunc_table *ifunc;
    /* long property=0; */
    ifunc = (struct G__ifunc_table*)belongingmethod->handle;
    return(ifunc->para_def[belongingmethod->index][argn]); 
  }
  else {
    return((char*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodArgInfo::IsValid()
{
  if(belongingmethod && belongingmethod->IsValid()) {
    if(0<=argn&&argn<belongingmethod->NArg()) {
      return(1);
    }
    else {
      return(0);
    }
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodArgInfo::Next()
{
  ++argn;
  if(IsValid()) {
    struct G__ifunc_table *ifunc;
    ifunc = (struct G__ifunc_table*)belongingmethod->handle;
    type.type = ifunc->para_type[belongingmethod->index][argn];
    type.tagnum=ifunc->para_p_tagtable[belongingmethod->index][argn];
    type.typenum =ifunc->para_p_typetable[belongingmethod->index][argn];
    type.reftype = ifunc->para_reftype[belongingmethod->index][argn];
    type.class_property=0;
    type.isconst = ifunc->para_isconst[belongingmethod->index][argn];
    return(1);
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
