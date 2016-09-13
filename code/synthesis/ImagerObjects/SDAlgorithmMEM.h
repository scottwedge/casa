//# SDAlgorithmMEM.h: Definition for SDAlgorithmMEM
//# Copyright (C) 1996,1997,1998,1999,2000,2002
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be adressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//#
//# $Id$

#ifndef SYNTHESIS_SDALGORITHMMEM_H
#define SYNTHESIS_SDALGORITHMMEM_H

#include <ms/MeasurementSets/MeasurementSet.h>
//#include <synthesis/MeasurementComponents/SkyModel.h>
#include <casa/Arrays/Matrix.h>
#include <images/Images/ImageInterface.h>
#include <images/Images/PagedImage.h>
#include <images/Images/TempImage.h>
#include <casa/Logging/LogMessage.h>
#include <casa/Logging/LogSink.h>

#include<synthesis/ImagerObjects/SDAlgorithmBase.h>

#include <synthesis/MeasurementEquations/Entropy.h>

namespace casa { //# NAMESPACE CASA - BEGIN

  /* Forware Declaration */
  class SIMinorCycleController;


  class SDAlgorithmMEM : public SDAlgorithmBase 
  {
  public:
    
    // Empty constructor
    SDAlgorithmMEM(String entropy);
    virtual  ~SDAlgorithmMEM();
    
  protected:
    
    // Local functions to be overloaded by various algorithm deconvolvers.
    void takeOneStep( Float loopgain, Int cycleNiter, Float cycleThreshold, Float &peakresidual, Float &modelflux, Int &iterdone );
    //    virtual void initializeDeconvolver( Float &peakresidual, Float &modelflux );
    void initializeDeconvolver();
    void finalizeDeconvolver();

    Array<Float> itsMatResidual, itsMatModel, itsMatPsf, itsMatMask;
    Array<Float> itsMatDeltaModel;

    CountedPtr<Entropy> itsEnt;


  };

} //# NAMESPACE CASA - END

#endif
