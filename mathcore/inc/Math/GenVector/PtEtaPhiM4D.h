// @(#)root/mathcore:$Name:  $:$Id: PtEtaPhiM4D.h,v 1.3 2005/12/06 17:17:48 moneta Exp $
// Authors: W. Brown, M. Fischler, L. Moneta    2005  

/**********************************************************************
*                                                                    *
* Copyright (c) 2005 , LCG ROOT FNAL MathLib Team                    *
*                                                                    *
*                                                                    *
**********************************************************************/

// Header file for class PtEtaPhiM4D
// 
// Created by: fischler at Wed Jul 21 2005
//   Similar to PtEtaPhiMSystem by moneta
// 
// Last update: $Id: PtEtaPhiM4D.h,v 1.3 2005/12/06 17:17:48 moneta Exp $
// 
#ifndef ROOT_Math_GenVector_PtEtaPhiM4D 
#define ROOT_Math_GenVector_PtEtaPhiM4D  1

#include "Math/GenVector/etaMax.h"
#include "Math/GenVector/GenVector_exception.h"


#include <cmath>

//#define TRACE_CE
#ifdef TRACE_CE
#include <iostream>
#endif

namespace ROOT {   
namespace Math { 
       
/** 
   Class describing a 4D cylindrical coordinate system
   using Pt , Phi, Eta and M (mass)  
   The metric used is (-,-,-,+). 

   @ingroup GenVector
*/ 

template <class ScalarType> 
class PtEtaPhiM4D { 

public : 

  typedef ScalarType Scalar;

  // --------- Constructors ---------------

  /**
  Default constructor gives zero 4-vector (with zero mass)  
   */
  PtEtaPhiM4D() : fPt(0), fEta(0), fPhi(0), fM(0) { }

  /**
    Constructor  from pt, eta, phi, mass values
   */
  PtEtaPhiM4D(Scalar pt, Scalar eta, Scalar phi, Scalar mass) : 
      			fPt(pt), fEta(eta), fPhi(phi), fM(mass) { }

  /**
    Generic constructor from any 4D coordinate system implementing 
    Pt(), Eta(), Phi() and M()  
   */ 
  template <class CoordSystem > 
    explicit PtEtaPhiM4D(const CoordSystem & c) : 
    fPt(c.Pt()), fEta(c.Eta()), fPhi(c.Phi()), fM(c.M())  { }

  // no need for customized copy constructor and destructor 

  /**
    Set internal data based on an array of 4 Scalar numbers
   */ 
  void SetCoordinates( const Scalar src[] ) 
    { fPt=src[0]; fEta=src[1]; fPhi=src[2]; fM=src[3]; }

  /**
    get internal data into an array of 4 Scalar numbers
   */ 
  void GetCoordinates( Scalar dest[] ) const 
    { dest[0] = fPt; dest[1] = fEta; dest[2] = fPhi; dest[3] = fM; }

  /**
    Set internal data based on 4 Scalar numbers
   */ 
  void SetCoordinates(Scalar pt, Scalar eta, Scalar phi, Scalar mass) 
    { fPt=pt; fEta = eta; fPhi = phi; fM = mass; }

  /**
    get internal data into 4 Scalar numbers
   */ 
  void 
  GetCoordinates(Scalar& pt, Scalar & eta, Scalar & phi, Scalar& mass) const 
    { pt=fPt; eta=fEta; phi = fPhi; mass = fM; }

  // --------- Coordinates and Coordinate-like Scalar properties -------------

  // 4-D Cylindrical eta coordinate accessors  

  Scalar Pt()  const { return fPt;  }
  Scalar Eta() const { return fEta; }
  Scalar Phi() const { return fPhi; }
  /** 
    M() is the invariant mass; 
    in this coordinate system it can be negagative if set that way. 
   */
  Scalar M()   const { return fM;   }
  Scalar Mag() const    { return M(); }

  Scalar Perp()const { return Pt(); }
  Scalar Rho() const { return Pt(); }
  
  // other coordinate representation

  Scalar Px() const { return fPt*cos(fPhi);}
  Scalar X () const { return Px();         }
  Scalar Py() const { return fPt*sin(fPhi);}
  Scalar Y () const { return Py();         }
  Scalar Pz() const {
    return fPt >   0 ? fPt*std::sinh(fEta)     : 
           fEta == 0 ? 0                       :
           fEta >  0 ? fEta - etaMax<Scalar>() :
                       fEta + etaMax<Scalar>() ; 
  }
  Scalar Z () const { return Pz(); }

  /** 
     magnitude of momentum
   */
  Scalar P() const { 
    return  fPt  > 0                 ?  fPt*std::cosh(fEta)       :
  	    fEta >  etaMax<Scalar>() ?  fEta - etaMax<Scalar>()   :
 	    fEta < -etaMax<Scalar>() ? -fEta - etaMax<Scalar>()   :
			                0                         ; 
  }
  Scalar R() const { return P(); }

  /** 
    squared magnitude of spatial components (momentum squared)
   */
  Scalar P2() const { Scalar p = P(); return p*p; }

  /** 
     Energy (timelike component of momentum-energy 4-vector)
     Will be negative if mass is set to a negative quantity.
   */
  Scalar E()   const { 
    Scalar e = std::sqrt(fM*fM + P()*P()); 
    return fM>=0? e : -e;
  }
  Scalar T()   const { return E();  }

  /**
    vector magnitude squared (or mass squared)
   */
  Scalar M2()   const { return fM*fM; }
  Scalar Mag2() const { return M2();  } 

  /** 
    transverse spatial component squared  
    */
  Scalar Pt2()   const { return fPt*fPt;}
  Scalar Perp2() const { return Pt2();  }

  /** 
    transverse mass squared
    */
  Scalar Mt2() const { return fM*fM  + fPt*fPt; } 

  /**
    transverse mass - will be negative if mass is negative
   */
  Scalar Mt() const { 
    Scalar mt2 = Mt2();
    return fM >= 0 ? std::sqrt(mt2) : -std::sqrt(mt2);
  } 

  /** 
    transverse energy squared
    */
  Scalar Et2() const { return fM*fM/std::cosh(fEta) + fPt*fPt; }
  					// a bit faster than Et()*Et()

  /**
    transverse energy
   */
  Scalar Et() const { 
    return E() / std::cosh(fEta); 
  }

private:
  inline static double pi() { return 3.14159265358979323; } 
public:

  /**
    polar angle
   */
  Scalar Theta() const {
    if (fPt  >  0) return 2* std::atan( exp( - fEta ) );
    if (fEta >= 0) return 0;
    return pi();
  }

  // --------- Set Coordinates of this system  ---------------

  /**
    set Pt value 
   */
  void SetPt( Scalar  pt) { 
    fPt = pt; 
  }
  /**
    set eta value 
   */
  void SetEta( Scalar  eta) { 
    fEta = eta; 
  }
  /**
    set phi value 
   */
  void SetPhi( Scalar  phi) { 
    fPhi = phi; 
  }
  /**
    set M value 
   */
  void SetM( Scalar  mass) { 
    fM = mass; 
  }

  // ------ Manipulations -------------

  /**
     negate the 4-vector -- Note that the mass becomes negative 
   */
  void Negate( ) { fPhi = - fPhi; fEta = - fEta; fM = - fM; }

  /**
    Scale coordinate values by a scalar quantity a
   */
  void Scale( Scalar a) { 
    if (a < 0) {
	Negate(); a = -a;
    }
    fPt *= a; 
    fM  *= a; 
  }

  /**
    Assignment from a generic coordinate system implementing 
    Pt(), Eta(), Phi() and M()  
   */ 
  template <class CoordSystem > 
    PtEtaPhiM4D & operator = (const CoordSystem & c) { 
      fPt  = c.Pt(); 
      fEta = c.Eta();
      fPhi = c.Phi(); 
      fM   = c.M(); 
      return *this;
    }

  /**
    Exact equality
   */  
  bool operator == (const PtEtaPhiM4D & rhs) const {
    return fPt == rhs.fPt && fEta == rhs.fEta 
    			  && fPhi == rhs.fPhi && fM == rhs.fM;
  }
  bool operator != (const PtEtaPhiM4D & rhs) const {return !(operator==(rhs));}

    // ============= Compatibility secition ==================

  // The following make this coordinate system look enough like a CLHEP
  // vector that an assignment member template can work with either
  Scalar x() const { return X(); }
  Scalar y() const { return Y(); }
  Scalar z() const { return Z(); } 
  Scalar t() const { return E(); } 



#if defined(__MAKECINT__) || defined(G__DICTIONARY) 

  // ====== Set member functions for coordinates in other systems =======

  void SetPx(Scalar px);  

  void SetPy(Scalar py);

  void SetPz(Scalar pz);  

  void SetE(Scalar t);  

#endif

private:

  Scalar fPt;
  Scalar fEta;
  Scalar fPhi;
  Scalar fM; 

};    
    
    
} // end namespace Math  
} // end namespace ROOT




#if defined(__MAKECINT__) || defined(G__DICTIONARY) 
// move implementations here to avoid circle dependencies

#include "Math/GenVector/PxPyPzE4D.h"


namespace ROOT { 

namespace Math { 

     
  // ====== Set member functions for coordinates in other systems =======

template <class ScalarType>  
void PtEtaPhiM4D<ScalarType>::SetPx(Scalar px) {  
    GenVector_exception e("PtEtaPhiM4D::SetPx() is not supposed to be called");
    Throw(e);
    PxPyPzE4D<Scalar> v(*this); v.SetPx(px); *this = PtEtaPhiM4D<Scalar>(v);
}
template <class ScalarType>  
void PtEtaPhiM4D<ScalarType>::SetPy(Scalar py) {  
    GenVector_exception e("PtEtaPhiM4D::SetPx() is not supposed to be called");
    Throw(e);
    PxPyPzE4D<Scalar> v(*this); v.SetPy(py); *this = PtEtaPhiM4D<Scalar>(v);
}
template <class ScalarType>  
void PtEtaPhiM4D<ScalarType>::SetPz(Scalar pz) {  
    GenVector_exception e("PtEtaPhiM4D::SetPx() is not supposed to be called");
    Throw(e);
    PxPyPzE4D<Scalar> v(*this); v.SetPz(pz); *this = PtEtaPhiM4D<Scalar>(v);
}
template <class ScalarType>  
void PtEtaPhiM4D<ScalarType>::SetE(Scalar t) {  
    GenVector_exception e("PtEtaPhiM4D::SetE() is not supposed to be called");
    Throw(e);
    PxPyPzE4D<Scalar> v(*this); v.SetE(t);   *this = PtEtaPhiM4D<Scalar>(v);
}

} // end namespace Math

} // end namespace ROOT

#endif  // endif __MAKE__CINT || G__DICTIONARY


#endif // ROOT_Math_GenVector_PtEtaPhiM4D 

