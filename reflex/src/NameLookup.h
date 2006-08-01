// @(#)root/reflex:$Name:  $:$Id: NameLookup.h,v 1.6 2006/07/24 14:57:40 axel Exp $
// Author: Stefan Roiser 2006

// Copyright CERN, CH-1211 Geneva 23, 2004-2006, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#ifndef ROOT_Reflex_NameLookup
#define ROOT_Reflex_NameLookup

// Include files
#include <string>
#include <vector>
#include <set>


namespace ROOT {
   namespace Reflex {
    
      // forward declarations
      class Type;
      class Scope;
      class Member;
      
      /*
       * point of declaration (3.3.1 [basic.scope.pdecl]) is not taken into account 
       */

      struct NameLookup {

         // 1. Lookup
         static const Type & LookupType( const std::string & nam,
                                 const Scope & current );

         
         static const Type & LookupTypeQualified( const std::string & nam );

         
         static const Type & LookupTypeUnqualified( const std::string & nam,
                                            const Scope & current );

         static Type LookupTypeInScope( const std::string & nam, 
                                        const Scope & current,
                                        bool &partial_success,
                                        std::set<Scope> & lookedAtUsingDir,
                                        size_t pos_subscope = 0,
                                        size_t pos_scope_end = std::string::npos );
  
           
         static Type LookupTypeInUnknownScope( const std::string & nam,
                                               const Scope & current );

         static const Scope & LookupScope( const std::string & nam,
                                   const Scope & current );


         static const Scope & LookupScopeQualified( const std::string & nam );


         static const Scope & LookupScopeUnqualified( const std::string & nam,
                                              const Scope & current );


         static const Member & LookupMember( const std::string & nam,
                                     const Scope & current );


         static const Member & LookupMemberQualified( const std::string & nam );

         
         static const Member & LookupMemberUnqualified( const std::string & nam,
                                                const Scope & current );

         


         // 2. OverloadResolution
         static const Member & OverloadResultion( const std::string & nam,
                                                  const std::vector< Member > & funcs );
                                   

         // 3. AccessControl
         static bool AccessControl( const Type & typ,
                                    const Scope & current );

         private:

      }; // struct  NameLookup

   } //namespace Reflex
} //namespace ROOT


#endif // ROOT_Reflex_NameLookup
