// @(#)root/mathmore:$Id: SpecFunc.h,v 1.1 2005/09/18 17:33:47 brun Exp $
// Authors: L. Moneta, A. Zsenei   08/2005

 /**********************************************************************
  *                                                                    *
  * Copyright (c) 2004 ROOT Foundation,  CERN/PH-SFT                   *
  *                                                                    *
  * This library is free software; you can redistribute it and/or      *
  * modify it under the terms of the GNU General Public License        *
  * as published by the Free Software Foundation; either version 2     *
  * of the License, or (at your option) any later version.             *
  *                                                                    *
  * This library is distributed in the hope that it will be useful,    *
  * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU   *
  * General Public License for more details.                           *
  *                                                                    *
  * You should have received a copy of the GNU General Public License  *
  * along with this library (see file COPYING); if not, write          *
  * to the Free Software Foundation, Inc., 59 Temple Place, Suite      *
  * 330, Boston, MA 02111-1307 USA, or contact the author.             *
  *                                                                    *
  **********************************************************************/


//#ifndef STANDALONEMATH
#include "config.h"
//#endif


//#if defined(HASMATHMORE)

#ifdef R__HAS_MATHMORE

#include "Math/SpecFuncMathCore.h"
#include "Math/SpecFuncMathMore.h"


#else

#include "Math/SpecFuncMathCore.h"

#endif
