// @(#)root/reflex:$Id$
// Author: Stefan Roiser 2004

// Copyright CERN, CH-1211 Geneva 23, 2004-2006, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#ifndef REFLEX_BUILD
#define REFLEX_BUILD
#endif

#include "Reflex/internal/TypeName.h"

#include "Reflex/Type.h"
#include "Reflex/internal/OwnedMember.h"

#include "stl_hash.h"
#include <vector>


//-------------------------------------------------------------------------------
typedef __gnu_cxx::hash_map<const std::string *, Reflex::Internal::TypeName * > Name2Type_t;
typedef __gnu_cxx::hash_map<const char *, Reflex::Internal::TypeName * > TypeId2Type_t;
typedef std::vector< Reflex::Type > TypeVec_t;


//-------------------------------------------------------------------------------
static Name2Type_t & sTypes() {
//-------------------------------------------------------------------------------
// Static wrapper for type map.
   static Name2Type_t m;
   return m;
}


//-------------------------------------------------------------------------------
static TypeId2Type_t & sTypeInfos() {
//-------------------------------------------------------------------------------
// Static wrapper for type map (type_infos).
   static TypeId2Type_t m;
   return m;
}


//-------------------------------------------------------------------------------
static TypeVec_t & sTypeVec() {
//-------------------------------------------------------------------------------
// Static wrapper for type vector.
   static TypeVec_t m;
   return m;
}


//-------------------------------------------------------------------------------
Reflex::Internal::TypeName::TypeName( const char * nam,
                                  TypeBase * typeBas,
                                  const std::type_info * ti )
   : fName( nam ),
     fTypeBase( typeBas ) {
//-------------------------------------------------------------------------------
// Construct a type name.
   fThisType = new Type(this);
   sTypes() [ &fName ] = this;
   sTypeVec().push_back(*fThisType);
   if ( ti ) sTypeInfos() [ ti->name() ] = this;
}


//-------------------------------------------------------------------------------
Reflex::Internal::TypeName::~TypeName() {
//-------------------------------------------------------------------------------
// Destructor.
}


//-------------------------------------------------------------------------------
void Reflex::Internal::TypeName::CleanUp() {
//-------------------------------------------------------------------------------
   // Cleanup memory allocations for types.
   for ( TypeVec_t::iterator it = sTypeVec().begin(); it != sTypeVec().end(); ++it ) {
      TypeName * tn = (TypeName*)it->Id();
      Type * t = tn->fThisType;
      if ( *t ) t->Unload();
      delete t;
      delete tn;
   }
}


//-------------------------------------------------------------------------------
void Reflex::Internal::TypeName::DeleteType() const {
//-------------------------------------------------------------------------------
// Delete the type base information.
   delete fTypeBase;
   fTypeBase = 0;
}


//-------------------------------------------------------------------------------
void Reflex::Internal::TypeName::SetTypeId( const std::type_info & ti ) const {
//-------------------------------------------------------------------------------
// Add a type_info to the map.
   sTypeInfos() [ ti.name() ] = const_cast<TypeName*>(this);
}


//-------------------------------------------------------------------------------
Reflex::Type
Reflex::Internal::TypeName::ByName( const std::string & key ) {
//-------------------------------------------------------------------------------
// Lookup a type by name.
   size_t pos =  key.substr(0,2) == "::" ?  2 : 0;
   const std::string & k = key.substr(pos);
   Name2Type_t::iterator it = sTypes().find(&k);
   if( it != sTypes().end() ) return it->second->ThisType();
   else                       return Dummy::Type();
}


//-------------------------------------------------------------------------------
Reflex::Type
Reflex::Internal::TypeName::ByTypeInfo( const std::type_info & ti ) {
//-------------------------------------------------------------------------------
// Lookup a type by type_info.
   TypeId2Type_t::iterator it = sTypeInfos().find(ti.name());
   if( it != sTypeInfos().end() ) return it->second->ThisType();
   else                           return Dummy::Type();
}


//-------------------------------------------------------------------------------
void Reflex::Internal::TypeName::HideName() {
//-------------------------------------------------------------------------------
// Append the string " @HIDDEN@" to a type name.
   if ( fName.length() == 0 || fName[fName.length()-1] != '@' ) {
      sTypes().erase(&fName);
      fName += " @HIDDEN@";
      sTypes()[&fName] = this;
   }
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::Internal::TypeName::ThisType() const {
//-------------------------------------------------------------------------------
// Return Type of this TypeName.
   return *fThisType;
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::Internal::TypeName::TypeAt( size_t nth ) {
//-------------------------------------------------------------------------------
// Return nth type in Reflex.
   if ( nth < sTypeVec().size()) return sTypeVec()[nth];
   return Dummy::Type();
}


//-------------------------------------------------------------------------------
size_t Reflex::Internal::TypeName::TypeSize() {
//-------------------------------------------------------------------------------
// Return number of types in Reflex.
   return sTypeVec().size();
}


//-------------------------------------------------------------------------------
Reflex::Type_Iterator Reflex::Internal::TypeName::Type_Begin() {
//-------------------------------------------------------------------------------
// Return begin iterator of the type container.
   return sTypeVec().begin();
}


//-------------------------------------------------------------------------------
Reflex::Type_Iterator Reflex::Internal::TypeName::Type_End() {
//-------------------------------------------------------------------------------
// Return end iterator of the type container.
   return sTypeVec().end();
}


//-------------------------------------------------------------------------------
Reflex::Reverse_Type_Iterator Reflex::Internal::TypeName::Type_RBegin() {
//-------------------------------------------------------------------------------
// Return rbegin iterator of the type container.
   return ((const std::vector<Type>&)sTypeVec()).rbegin();
}


//-------------------------------------------------------------------------------
Reflex::Reverse_Type_Iterator Reflex::Internal::TypeName::Type_REnd() {
//-------------------------------------------------------------------------------
// Return rend iterator of the type container.
   return ((const std::vector<Type>&)sTypeVec()).rend();
}


