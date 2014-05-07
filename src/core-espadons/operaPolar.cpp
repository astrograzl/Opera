/********************************************************************
 ****                  MODULE FOR OPERA v1.0                     ****
 ********************************************************************
 Module name: operaPolar
 Version: 1.0
 Description: This module calculates the polarization of light.
 Author(s): CFHT OPERA team
 Affiliation: Canada France Hawaii Telescope 
 Location: Hawaii USA
 Date: Jan/2011
 Contact: teeple@cfht.hawaii.edu
 
 Copyright (C) 2011  Opera Pipeline team, Canada France Hawaii Telescope
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see:
 http://software.cfht.hawaii.edu/licenses
 -or-
 http://www.gnu.org/licenses/gpl-3.0.html
 ********************************************************************/

// $Date$
// $Id$
// $Revision$
// $Locker$
// $Log$

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <fitsio.h>
#include <getopt.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "globaldefines.h"
#include "operaError.h"
#include "libraries/operaException.h"
#include "libraries/operaSpectralElements.h"
#include "libraries/operaSpectralOrderVector.h"
#include "libraries/operaSpectralOrder.h"
#include "libraries/operaFluxVector.h"
#include "libraries/operaStokesVector.h"
#include "libraries/operaPolarimetry.h"             // for method_t and operaPolarimetry
#include "libraries/operaStats.h"
#include "libraries/operaLibCommon.h"               // for anglecoord_t and timecoord_t

#include "core-espadons/operaPolar.h"

#define NOTPROVIDED -999

/*!
 * \file operaPolar.cpp
 * \brief Calculate polarimetry.
 * \details This file holds the implementation for the calculation of polarimetry.
 */

using namespace std;

/*!
 * \author Andre Venne
 * \brief Calculate polarimetry.
 * \param argc
 * \param argv
 * \note By default, mandatory inputs are --input1=... --input2=... --input3=... --input4=... --output=... --stokesparameter=...
 * \throws operaException cfitsio error code
 * \throws operaException operaErrorNoInput
 * \throws operaException operaErrorNoOuput
 * \return EXIT_STATUS
 * \sa class operaFluxVector, class operaStokesVector, class operaPolarimetry
 * \sa class operaSpectralElements, class operaSpectralOrder, class operaSpectralOrderVector
 
 This module calculates the polarimetry measurements. It describes the polarization using the 4 Stokes parameters.
 
 For each Stokes parameter, there are 4 exposures, numbered 1,2,3,4.
 There is an option to use only 2 exposures, in which case there can be no null polarization spectrum calculated.
 
 For each exposure, there is a "perpendicular" beam (E) and a "parallel"
 beam (A) within each order. The left beam is (E) and the right beam is (A).
 
 Exposure #1 has in its left  beam an intensity i1E, and in its right beam, i1A.
 etc.
 
 Those intensities are the 1D extracted spectra.
 
 From the Bagnulo et al. 2009 paper
 (link to the paper)
 and the Donati et al. 1997 paper
 (https://info.cfht.hawaii.edu/download/attachments/30966138/1997MNRAS_291__658D.pdf?version=1&modificationDate=1311131714953 ),
 here are the formulas to calculate a Stokes parameter (also called "polarization"):
 
 ## Difference method
 
 ### STEP 1 - calculate the quantity Gn (Eq #12-14 on page 997 of Bagnulo et al. 2009 paper), n being the pair of exposures
 
  G1 = (i1E - i1A)/(i1E + i1A)
  G2 = (i2E - i2A)/(i2E + i2A)
  G3 = (i3E - i3A)/(i3E + i3A)
  G4 = (i4E - i4A)/(i4E + i4A)
	
 ### STEP 2 - calculate the quantity Dm (Eq #18 on page 997 of Bagnulo et al. 2009 paper) and the quantity Dms with exposure 2 and 4 swapped, m being the pair of exposures
 
  D1 = G1 - G2
  D2 = G3 - G4
  
  D1s = G1 - G4
  D2s = G3 - G2
 
 ### STEP 3 - calculate the degree of Stokes parameter (Eq #19 on page 997 of Bagnulo et al. 2009 paper), aka "the degree of polarization" P
 
  P/I = (D1 + D2) / (2.0 * PairOfExposures)
 
 ### STEP 4 - calculate the first NULL spectrum (Eq #20 on page 997 of Bagnulo et al. 2009 paper)
 
  N1 = (D1 - D2) / (2.0 * PairOfExposures)
 
 ### STEP 5 - calculate the second NULL spectrum (Eq #20 on page 997 of Bagnulo et al. 2009 paper) with exposure 2 and 4 swapped
 
  N2 = (D1s - D2s) / (2.0 * PairOfExposures)
 
 ## Ratio method
 
 ### STEP 1 - calculate ratio of beams for each exposure (Eq #12 on page 997 of Bagnulo et al. 2009 paper)
 
 r1 = i1E / i1A
 r2 = i2E / i2A
 r3 = i3E / i3A
 r4 = i4E / i4A
 
 ### STEP 2 - calculate the quantity Rm (Eq #23 on page 998 of Bagnulo et al. 2009 paper) and the quantity Rms with exposure 2 and 4 swapped, m being the pair of exposures
 
  R1 = r1 / r2
  R2 = r3 / r4
  
  R1s = r1 / r4
  R2s = r3 / r2
 
 ### STEP 3 - calculate the quantity R (Part of Eq #24 on page 998 of Bagnulo et al. 2009 paper)
 
  R = Pow( R1 * R2 , 1.0/(2.0 * PairOfExposures) )
 
 ### STEP 4 - calculate the degree of Stokes parameter (Simplification with STEP 2 of Eq #24 on page 998 of Bagnulo et al. 2009 paper), aka "the degree of polarization" P
 
  P/I = (R - 1.0) / (R + 1.0)
 
 ### STEP 5 - calculate the quantity RN1 (Part of Eq #25-26 on page 998 of Bagnulo et al. 2009 paper)
 
  RN1 = Pow( R1 / R2 , 1.0/(2.0 * PairOfExposures) )
 
 ### STEP 6 - calculate the first NULL spectrum (Simplification with STEP 4 of Eq #25-26 on page 998 of Bagnulo et al. 2009 paper)
 
  N1 = (RN1 - 1.0) / (RN1 + 1.0)
 
 ### STEP 7 - calculate the quantity RN2 (Part of Eq #25-26 on page 998 of Bagnulo et al. 2009 paper) with exposure 2 and 4 swapped
 
  RN2 = Pow( R1s / R2s , 1.0/(2.0 * PairOfExposures) )
 
 ### STEP 8 - calculate the second NULL spectrum (Simplification with STEP 4 of Eq #25-26 on page 998 of Bagnulo et al. 2009 paper) with exposure 2 and 4 swapped
 
  N2 = (RN2 - 1.0) / (RN2 + 1.0)
 
 ## Difference method with beam swapped
 
 ### STEP 1 - calculate the quantity Gn (Eq #12-14 on page 997 of Bagnulo et al. 2009 paper) with beam i1A and i2E, and i3A and i4E swapped, n being the pair of exposures
 
 G1 = (i1E - i2E)/(i1E + i2E)
 G2 = (i1A - i2A)/(i1A + i2A)
 G3 = (i3E - i4E)/(i3E + i4E)
 G4 = (i3A - i4A)/(i3A + i4A)
 
 ### STEP 2 - calculate the quantity Dm (Eq #18 on page 997 of Bagnulo et al. 2009 paper) and the quantity Dms with exposure 2 and 4 swapped, m being the pair of exposures
 
 D1 = G1 - G2
 D2 = G3 - G4
 
 D1s = G1 - G4
 D2s = G3 - G2
 
 ### STEP 3 - calculate the degree of Stokes parameter (Eq #19 on page 997 of Bagnulo et al. 2009 paper), aka "the degree of polarization" P
 
 P/I = (D1 + D2) / (2.0 * PairOfExposures)
 
 ### STEP 4 - calculate the first NULL spectrum (Eq #20 on page 997 of Bagnulo et al. 2009 paper)
 
 N1 = (D1 - D2) / (2.0 * PairOfExposures)
 
 ### STEP 5 - calculate the second NULL spectrum (Eq #20 on page 997 of Bagnulo et al. 2009 paper) with exposure 2 and 4 swapped
 
 N2 = (D1s - D2s) / (2.0 * PairOfExposures)
 
 _______________________________________
 
 ## To modify the code:
 
 Take note that the algorithm is duplicated for the 2 and 4 exposures mode. Any changes made to the algorithm should be applied to both versions. 
 Futhermore, take note that the inputs 3 and 4 are swapped to be consistent with the algorithm in the Bagnulo et al. 2009 paper.
 
 ### To add a calculation method for the polarization:
 
 These are the steps that should be followed to add a calculation method:
 
  1. Add the method name in the enumeration \em method_t defined at the beggining of the \em main in the module operaPolar
  2. Add the method in the condition to verify if the chosen method is valid at the beggining of the \em try in the \em main of the module operaPolar
  3. Implement the new method in operaPolar for both number of exposures options in the same way as the other methods
  4. Store the degree of polarization in the variable PoverI, and the 2 null spectra in N1 and N2 variables (if applicable)
  5. If needed, write the value of the intermediate steps to the data file. Don't forget to update the header of the data file.
  6. Update the function \em printUsageSyntax so that the help lists the new method.
 
 One should also apply the same steps to modify operaPolarTest if simulations are needed for that method.
 
 #### Don't forget to document the new method.
 
 */
int main(int argc, char *argv[])
{
	int opt;
	
	int plot=0, verbose=0, debug=0, trace=0;
	
	string input[4];
	string outputfilename;
	string inputWaveFile;
    
    string inputFlatFluxCalibration;
    
    /* Method enumeration definition */
    method_t method = Ratio;
    
    stokes_parameter_t StokesParameter = StokesI;
	
	unsigned NumberOfExposures = 4;
    	
	int ordernumber = NOTPROVIDED;
    
    int minorder = 22;
    bool minorderprovided = false;
    int maxorder = 62;    
    bool maxorderprovided = false; 
    
	bool interactive = false;
	
    string plotfilename;	
	string datafilename;	
	string scriptfilename;
    
    bool generate3DPlot = FALSE;
    bool plotContinuum = FALSE;
        
	struct option longopts[] = {
		{"input1",				1,			NULL, '1'},
		{"input2",				1,			NULL, '2'},
		{"input3",				1,			NULL, '3'},
		{"input4",				1,			NULL, '4'},
		{"output",				1,			NULL, 'o'},
        {"stokesparameter",		1,			NULL, 's'},
		{"method",				1,			NULL, 'm'},
		{"numberofexposures",	1,			NULL, 'c'},
		{"inputWaveFile",		1,			NULL, 'w'},	// wavelength calibration file (.wcal or .tell)

		{"ordernumber",			1,			NULL, 'O'},
		{"minorder",			1,			NULL, 'M'},
		{"maxorder",			1,			NULL, 'X'},
		
        {"inputFlatFluxCalibration",		1,			NULL, 'f'},	// flat field file (.fcal)

		{"plotfilename",		1,			NULL, 'P'},
		{"datafilename",		1,			NULL, 'F'},
        {"scriptfilename",		1,			NULL, 'S'},
        
        {"generate3DPlot",      1,			NULL, 'E'},
        {"plotContinuum",       1,			NULL, 'C'},
		
		{"plot",				optional_argument,	NULL, 'p'},
        {"interactive",			optional_argument,	NULL, 'I'},
		{"verbose",				optional_argument,	NULL, 'v'},
		{"debug",				optional_argument,	NULL, 'd'},
		{"trace",				optional_argument,	NULL, 't'},
		{"help",				no_argument,		NULL, 'h'},
		{0,0,0,0}
    };
	
	while((opt = getopt_long(argc, argv, "1:2:3:4:o:s:c:m:w:O:M:X:f:P:F:S:I:E:C:p::I::v::d::t::h", 
							 longopts, NULL))  != -1)
	{
		switch(opt) 
		{
			case '1':
				input[0] = optarg;
				break;
			case '2':
				input[1] = optarg;
				break;
			case '3':
				input[2] = optarg;
				break;
			case '4':
				input[3] = optarg;
				break;
			case 'o':
				outputfilename = optarg;
				break;
            case 's':
				StokesParameter = (stokes_parameter_t)atoi(optarg);
				break;
			case 'm':
				method = (method_t)atoi(optarg);
				break;
			case 'c':
				NumberOfExposures = atoi(optarg);
				break;  
			case 'w':
				inputWaveFile = optarg;
				break;  
				
			case 'O':
				ordernumber = atoi(optarg);
				break;				
			case 'M':
				minorder = atoi(optarg);
                minorderprovided = true;
				break;  
			case 'X':
				maxorder = atoi(optarg);
                maxorderprovided = true;
				break;

			case 'f':		// flat field fcal
				inputFlatFluxCalibration = optarg;
				break;

			case 'E':
				generate3DPlot = (atoi(optarg)?true:false);
				break;
			case 'C':
				plotContinuum = (atoi(optarg)?true:false);
				break;
			case 'P':
				plotfilename = optarg;
				plot = 1;
				break;
            case 'F':
				datafilename = optarg;
				break;
            case 'S':
				scriptfilename = optarg;
				break;
			case 'p':
				plot = 1;
				break;
            case 'I':
				interactive = (atoi(optarg)?true:false);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'd':
				debug = 1;
				break;
			case 't':
				trace = 1;
				break;
			case 'h':
				printUsageSyntax(argv[0]);
				exit(EXIT_SUCCESS);
				break;
			case '?':
				printUsageSyntax(argv[0]);
				exit(EXIT_SUCCESS);
				break;
		}
	}
	try {
        /* Stokes parameter check */
		if (StokesParameter != StokesQ && StokesParameter != StokesU && StokesParameter != StokesV) {
			throw operaException("operaPolar: ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);	
		}
        /* Method check Difference=1, Ratio, DifferenceWithBeamSwapped */
        if (method != Difference && method != Ratio && method != DifferenceWithBeamSwapped) {
            throw operaException("operaPolar: ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);    
        }
        /* Number of exposures check */
        if (NumberOfExposures != 2 && NumberOfExposures != 4) {
            throw operaException("operaPolar: ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);    
        }
        /* Output file name check */
		if (outputfilename.empty()) {
			throw operaException("operaPolar: ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);	
		}
        
        ofstream *fdata = NULL ;
        
        if (!datafilename.empty()) {
            fdata = new ofstream();
            fdata->open(datafilename.c_str());  
        }
        
		if (verbose) {
            cout << "operaPolar: NumberOfExposures = " << NumberOfExposures << endl;

            for(unsigned i=0;i<NumberOfExposures;i++) {
                cout << "operaPolar: input " << i+1 << " = " << input[i] << endl;
            }
			cout << "operaPolar: outputfilename = " << outputfilename << endl;
			cout << "operaPolar: StokesParameter = " << inputWaveFile << endl;
			cout << "operaPolar: StokesParameter = " << StokesParameter << endl; 
			cout << "operaPolar: method = " << method << endl;
            cout << "operaPolar: inputFlatFluxCalibration = " << inputFlatFluxCalibration << endl;
            
            if(ordernumber != NOTPROVIDED) {
                cout << "operaPolar: ordernumber = " << ordernumber << endl;            
            }             

            cout << "operaPolar: plotfilename = " << plotfilename << endl;
            cout << "operaPolar: scriptfilename = " << scriptfilename << endl;
            if(interactive) {
                cout << "operaPolar: interactive = YES" << endl;
            } else {
                cout << "operaPolar: interactive = NO" << endl;
            }
            
		}
        
        /* Create output spectral order vector based on base spectrum (i=0)*/
        operaSpectralOrderVector outputorderVector(input[0]);
        if (!inputFlatFluxCalibration.empty()) {
            outputorderVector.ReadSpectralOrders(inputFlatFluxCalibration);
        }
        if (!inputWaveFile.empty()) {
            outputorderVector.ReadSpectralOrders(inputWaveFile);
        }
        
        if(!minorderprovided) {
            minorder = outputorderVector.getMinorder();
        }
        if(!maxorderprovided) {
            maxorder = outputorderVector.getMaxorder();
        }
		
		if(ordernumber != NOTPROVIDED) {
			minorder = ordernumber;
			maxorder = ordernumber;
		}
		
		if (verbose) {
			cerr << "operaPolar: minorder = " << minorder << " maxorder = " << maxorder << endl;
		}
        
        /* Create the spectral order vector based on inputs */
        operaSpectralOrderVector *spectralOrdervector[4];
        for(unsigned i=0;i<NumberOfExposures;i++) {
            /* Inputs file name check */
            if (input[i].empty()) {
                throw operaException("operaPolar: ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);
            }
            /* Create the spectral order vector based on inputs */
            spectralOrdervector[i] = new operaSpectralOrderVector(input[i]);
            if (!inputWaveFile.empty()) {
                spectralOrdervector[i]->ReadSpectralOrders(inputWaveFile);
            }
            
            /* Correct flat-field */
            if (!inputFlatFluxCalibration.empty()) {
                spectralOrdervector[i]->correctFlatField(inputFlatFluxCalibration, minorder, maxorder, false);
            }
        }
        
        
        operaSpectralOrder *spectralOrder[4];
        operaSpectralElements *spectralElements[4];
        operaWavelength *wavelength[4];

		/*
         * Take note that the algorithm is duplicated for the 2 and 4 exposures mode. Any changes made to the algorithm should be applied to both versions.
         */
        for (int order=minorder; order<=maxorder; order++) {
            if (verbose)
                cout << "operaPolar: Processing order number: " << order << endl;
            unsigned spectralElementsTest = 0;
            
            for(unsigned i=0;i<NumberOfExposures;i++) {
                spectralOrder[i] = spectralOrdervector[i]->GetSpectralOrder(order);
                
                if(spectralOrder[i]->gethasSpectralElements()) {
                    spectralElements[i] = spectralOrder[i]->getSpectralElements();
                    spectralElementsTest++;
					if (spectralOrder[i]->gethasWavelength()) {
						wavelength[i] = spectralOrder[i]->getWavelength();
						spectralElements[i]->setwavelengthsFromCalibration(wavelength[i]);
					}
                }
            }
            
            // Below it will skip order if not all exposures have spectral elements.
            if (spectralElementsTest == NumberOfExposures) {
                
                /* Get length of base spectrum */
                unsigned length = spectralOrder[0]->getBeamElements(0)->getFluxVector()->getlength();
                
                operaSpectralOrder *outputspectralOrder = outputorderVector.GetSpectralOrder(order);
                if (!inputFlatFluxCalibration.empty() && outputspectralOrder->gethasSpectralEnergyDistribution()) {
                    outputspectralOrder->divideSpectralElementsBySEDElements(true, NULL, false);
                }
                operaSpectralElements *outputspectralElements = outputspectralOrder->getSpectralElements();
                operaWavelength *outputwavelength = NULL;
                
                if (outputspectralOrder->gethasWavelength()) {
                    outputwavelength = outputspectralOrder->getWavelength();
                    outputspectralElements->setwavelengthsFromCalibration(outputwavelength);
                }
                
                /* Create Polarimetry for output vector */
                outputspectralOrder->deletePolarimetry();
                outputspectralOrder->createPolarimetry(length);
                operaPolarimetry *Polarimetry = outputspectralOrder->getPolarimetry();
                
                /* update output cross-correlation and Stokes I including all input spectra */
                
                for(unsigned indexElem=0;indexElem < outputspectralElements->getnSpectralElements(); indexElem++) {
                    
                    double outputXCorrelation = 0;
                    for(unsigned i=0;i<NumberOfExposures;i++) {
                        outputXCorrelation += spectralElements[i]->getXCorrelation(indexElem)/(double)NumberOfExposures;
                    }
                    outputspectralElements->setXCorrelation(outputXCorrelation, indexElem);
                    
                }
                
                operaFluxVector *iE[4], *iA[4];
                /* Populate vectors with the E/A data */
                for(unsigned i=0;i<NumberOfExposures;i++) {
					// May 28 2013 DT moved the swap to the harness...
#if 0                    
					iE[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(0)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(0)->getFluxVector()->getvariances(), length);
					iA[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(1)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(1)->getFluxVector()->getvariances(), length);
#else
                    // To swap images in the second pair of images --  E. Martioli May 28 2013
                    if(i==0 || i==1) {
                        iE[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(0)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(0)->getFluxVector()->getvariances(), length);
                        iA[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(1)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(1)->getFluxVector()->getvariances(), length);
                    } else if (i==2) {
                        iE[3] = new operaFluxVector(spectralOrder[i]->getBeamElements(0)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(0)->getFluxVector()->getvariances(), length);
                        iA[3] = new operaFluxVector(spectralOrder[i]->getBeamElements(1)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(1)->getFluxVector()->getvariances(), length);
                    } else if (i==3) {
                        iE[2] = new operaFluxVector(spectralOrder[i]->getBeamElements(0)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(0)->getFluxVector()->getvariances(), length);
                        iA[2] = new operaFluxVector(spectralOrder[i]->getBeamElements(1)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(1)->getFluxVector()->getvariances(), length);
                    }
#endif
#if 0
                    // To swap beams in the second pair of images --  E. Martioli May 28 2013
                    if(i==0 || i==1) {
                        iE[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(0)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(0)->getFluxVector()->getvariances(), length);
                        iA[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(1)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(1)->getFluxVector()->getvariances(), length);
                    } else if (i==2 || i==3) {
                       iA[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(0)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(0)->getFluxVector()->getvariances(), length);
                       iE[i] = new operaFluxVector(spectralOrder[i]->getBeamElements(1)->getFluxVector()->getfluxes(), spectralOrder[i]->getBeamElements(1)->getFluxVector()->getvariances(), length);
                    }
#endif
                }
                
                Polarimetry->setmethod(method);
                
				// This is called by calculateStokesParameter -- DT Apr 26 2013
                //Polarimetry->calculateDegreeOfPolarization(StokesParameter,iE,iA,NumberOfExposures);
                
                Polarimetry->calculateStokesParameter(StokesParameter,iE,iA,NumberOfExposures);
                
                outputspectralOrder->sethasPolarimetry(true);
                
                /* Writting to data file for plot */
                if (fdata != NULL) {
                    fdata->precision(6);
                    *fdata << fixed;
                    *fdata << "# operaPolar: <index> <Degree of Polarization> <Intensity> <i1E> <i1A> <i2E> <i2A> <i3E> <i3A> <i4E> <i4A> <r1 = i1E / i1A> <r2 = i2E / i2A> <r3 = i3E / i3A> <r4 = i4E / i4A> <R1 = r1 / r2> <R2 = r3 / r4> <First Null Flux> <Second Null Flux>\n";
                    for (unsigned index = 0 ; index < length ; index++) {
                        *fdata << 0 << '\t' << order << '\t'
                        << outputspectralElements->getdistd(index) << '\t'
                        << outputspectralElements->getwavelength(index) << '\t'
                        << Polarimetry->getStokesParameter(StokesI)->getflux(index) << '\t'
                        << Polarimetry->getStokesParameter(StokesParameter)->getflux(index) << '\t'
                        << Polarimetry->getDegreeOfPolarization(StokesParameter)->getflux(index) << '\t'
                        << Polarimetry->getFirstNullPolarization(StokesParameter)->getflux(index) << '\t'
                        << Polarimetry->getSecondNullPolarization(StokesParameter)->getflux(index) << '\t';
                        for(unsigned i=0;i<NumberOfExposures;i++) {
                            *fdata << iE[i]->getflux(index) << '\t'
                            << iA[i]->getflux(index) << '\t';
                        }
                        *fdata << endl;
                    }
                    *fdata << endl;
                    if(generate3DPlot) {
                        for (unsigned index = 0 ; index < length ; index++) {
                            *fdata << 1 << '\t' << order << '\t'
                            << outputspectralElements->getdistd(index) << '\t'
                            << outputspectralElements->getwavelength(index) << '\t'
                            << Polarimetry->getStokesParameter(StokesI)->getflux(index) << '\t'
                            << Polarimetry->getStokesParameter(StokesParameter)->getflux(index) << '\t'
                            << Polarimetry->getDegreeOfPolarization(StokesParameter)->getflux(index) << '\t'
                            << Polarimetry->getFirstNullPolarization(StokesParameter)->getflux(index) << '\t'
                            << Polarimetry->getSecondNullPolarization(StokesParameter)->getflux(index) << '\t';
                            for(unsigned i=0;i<NumberOfExposures;i++) {
                                *fdata << iE[i]->getflux(index) << '\t'
                                << iA[i]->getflux(index) << '\t';
                            }
                            *fdata << endl;   
                        }
                    }
                    *fdata << endl;                
                    *fdata << endl;
                }
                /* Delete vectors with the E/A data */
                for(unsigned i=0;i<NumberOfExposures;i++) {
                    delete iE[i];
                    delete iA[i];
                }
            } else { // if (spectralElementsTest == NumberOfExposures) {
                if (verbose)
					cerr << "operaPolar: NOT all input spectra have spectralElements, skipping order " << order << "." << endl;
            }
        }

		outputorderVector.WriteSpectralOrders(outputfilename, Polarimetry);
        
        if (fdata != NULL) {
            fdata->close();
            if (!scriptfilename.empty()) {
                if(generate3DPlot) {
                    GeneratePolarization3DPlot(scriptfilename,plotfilename,datafilename, plotContinuum, interactive,StokesParameter);
                } else {
                    GeneratePolarimetryPlot(scriptfilename,plotfilename,datafilename,interactive,minorder,maxorder,StokesParameter);
                }
            }
        }
	}
	catch (operaException e) {
		cerr << "operaPolar: " << e.getFormattedMessage() << endl;
		return EXIT_FAILURE;
	}
	catch (...) {
		cerr << "operaPolar: " << operaStrError(errno) << endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
} 

/*!
 * \brief Print out the proper program usage syntax.
 * \param modulename Name of the module, which is operaPolar
 */
static void printUsageSyntax(char * modulename) {
	cout <<
	"\n"
	" Usage: "+string(modulename)+"  [-pDvdth]"
    " --input1=<FILE_NAME>"
    " --input2=<FILE_NAME>"
    " --input3=<FILE_NAME>"
    " --input4=<FILE_NAME>"
    " --output=<FILE_NAME>"
    " --stokesparameter=<STOKES_PARAMETER>"
    " --method=<UNS_VALUE>"
    " --numberofexposures=<UNS_VALUE>"
    " --ordernumber=<INT_VALUE>"
    " --numberofamplifiers=<UNS_VALUE>"
	" --generate3DPlot=<BOOL>"
	" --plotContinuum=<BOOL>"
    " --plotfilename=<FILE_NAME>"
    " --datafilename=<FILE_NAME>"
    " --scriptfilename=<FILE_NAME> \n\n"
	" Example: "+string(modulename)+" --output=o.txt --input1=001.e --input2=002.e --input3=003.e --input4=004.e --stokesparameter=1 --method=2 --numberofexposures=4 --ordernumber=34 --plotfilename=plot.eps --datafilename=data.dat --scriptfilename=script.gnu -v -p \n\n"
    "  -1, --input1=<FILE_NAME>,  First exposure input file name \n"
    "  -2, --input2=<FILE_NAME>,  Second exposure input file name \n"
    "  -3, --input3=<FILE_NAME>,  Third exposure input file name \n"
    "  -4, --input4=<FILE_NAME>,  Fourth exposure input file name \n"
    "  -o, --output=<FILE_NAME>,  Output file name \n"
    "  -s, --stokesparameter=<UNS_VALUE>, Which Stokes parameter the module is calculating \n"
    "                              Available options are = 0, 1, 2 or 3, where: \n"
    "                              0. Stokes I (default)\n"
    "                              1. Stokes Q \n"
    "                              2. Stokes U \n"
    "                              3. Stokes V \n"
    "  -m, --method=<UNS_VALUE>, Method for calculation of polarisation \n"
    "                              Available options are = 1, 2, where: \n"
    "                              1. Difference \n"
    "                              2. Ratio (default) \n"
    "  -c, --numberofexposures=<UNS_VALUE>, Number of input file to use \n"
    "                              Available options are = 2, 4 \n"
    "                              2. Use the first 2 input files \n"
    "                              4. Use all 4 input files (default) \n"
    "  -O, --ordernumber=<INT_VALUE>, Absolute order number to extract (default=all) \n"
	"  -E, --generate3DPlot=<BOOL>, Switch to generate 3D or 2D plot spectra\n"
	"  -C, --plotContinuum=<BOOL>, Switch to generate plot of flux or degree of polarization spectra\n"
    "  -P, --plotfilename=<FILE_NAME>, Output plot eps file name \n"
    "  -F, --datafilename=<FILE_NAME>, Output data file name \n"
    "  -S, --scriptfilename=<FILE_NAME>, Output gnuplot script file name \n\n"
    "  -p, --plot,  Turn on plotting \n"
    "  -I, --interactive,  Turn on display of plotting \n"
	"  -v, --verbose,  Turn on message sending \n"
	"  -d, --debug,  Turn on debug messages \n"
	"  -t, --trace,  Turn on trace messages \n"
    "  -h, --help,  display help message \n";
}

/*
 * \brief Plot the degree of polarization.
 * \details This function plots and creates the gnuplot script to plot the degree of polarization for every spectral element.
 * \details It can also display that plot after printing it.
 * \param gnuScriptFileName Output gnuplot script file name
 * \param outputPlotEPSFileName EPS plot file name
 * \param dataFileName Name of the data file holding the plot data
 * \param display Boolean value to display the plot on the screen
 * \param minorder minimum order to include in the plot range
 * \param maxorder maximum order to include in the plot range
 * \param StokesParameter A stokes_parameter_t value
 * \return void
 */
void GeneratePolarimetryPlot(string gnuScriptFileName, string outputPlotEPSFileName, string datafilename, bool display, unsigned minorder, unsigned maxorder, stokes_parameter_t StokesParameter) {
    
    ofstream *fgnu = NULL;
    
    if (!gnuScriptFileName.empty()) {
        remove(gnuScriptFileName.c_str()); // delete any existing file with the same name
        fgnu = new ofstream();
        fgnu->open(gnuScriptFileName.c_str());
    } else {
        exit(EXIT_FAILURE);
    }
    
    *fgnu << "reset" << endl;
    //*fgnu << "unset key" << endl;
    
    *fgnu << "set xrange[-200:*]" << endl;
    *fgnu << "set yrange[" << minorder - 1.0 << ":" << maxorder + 1.0 << "]" << endl;
    *fgnu << "\nset xlabel \"distance (pixels)\"" << endl;
    
    if(StokesParameter == StokesQ) {
        *fgnu << "set ylabel \"order + degree of polarization (Stokes Q / Stokes I)\"" << endl;
    } else if (StokesParameter == StokesU) {
        *fgnu << "set ylabel \"order + degree of polarization (Stokes U / Stokes I)\"" << endl;
    } else if (StokesParameter == StokesV) {
        *fgnu << "set ylabel \"order + degree of polarization (Stokes V / Stokes I)\"" << endl;
    }
  
    double scaleFactor = 10;
    
    if(!outputPlotEPSFileName.empty()) {
        *fgnu << "\nset terminal postscript enhanced color solid lw 1.5 \"Helvetica\" 14" << endl;
        *fgnu << "set output \"" << outputPlotEPSFileName << "\"" << endl;
        *fgnu << endl;
        
        *fgnu << "plot \"" << datafilename << "\" u 3:($2+$7*" << scaleFactor << ") t \"degree of polarization*" << scaleFactor << "\" w l lt 3, \"\" u 3:($2+$8*" << scaleFactor << "+0.2) t \"first null polarization*" << scaleFactor << " + 0.2\" w l lt 4, \"\" u 3:($2+$9*" << scaleFactor << "-0.2) t \"second null polarization*" << scaleFactor << " - 0.2\" w l lt 5" << endl;
        
        if (display) {
            *fgnu << "\nset terminal x11" << endl;
            *fgnu << "set output" << endl;
            *fgnu << "replot" << endl;
        } else {
            *fgnu << "\n#set terminal x11" << endl;
            *fgnu << "#set output" << endl;
            *fgnu << "#replot" << endl;
        }
    } else {
        *fgnu << endl;
        
        *fgnu << "plot \"" << datafilename << "\" u 3:($2+$7*" << scaleFactor << ") t \"degree of polarization*" << scaleFactor << "\" w l lt 3, \"\" u 3:($2+$8*" << scaleFactor << "+0.2) t \"first null polarization*" << scaleFactor << " + 0.2\" w l lt 4, \"\" u 3:($2+$9*" << scaleFactor << "-0.2) t \"second null polarization*" << scaleFactor << " - 0.2\" w l lt 5" << endl;
        
        *fgnu << endl;
        
        *fgnu << "\n#set terminal postscript enhanced color solid lw 1.5 \"Helvetica\" 14" << endl;
        *fgnu << "#set output \"outputPlotEPSFileName.eps\"" << endl;
        *fgnu << "#replot" << endl;
        *fgnu << "#set terminal x11" << endl;
        *fgnu << "#set output" << endl;
    }
    
    fgnu->close();
    
    if (display) {
        systemf("gnuplot -persist %s",gnuScriptFileName.c_str());
    } else {
        if(!outputPlotEPSFileName.empty())
            systemf("gnuplot %s",gnuScriptFileName.c_str());
    }    
}

/*
 * \brief Image plot of the degree of polarization or the flux for a given Stokes parameter.
 * \details This function plots and creates a gnuplot script to produce a 3D plot of the degree of polarization or the Stokes flux.
 * \details It can also display that plot after printing it.
 * \param gnuScriptFileName Output gnuplot script file name
 * \param outputPlotEPSFileName EPS plot file name
 * \param dataFileName Name of the data file holding the plot data
 * \param plotContinuum Boolean value to plot flux instead of degree of polarization
 * \param display Boolean value to display the plot on the screen
 * \param StokesParameter A stokes_parameter_t value
 * \return void
 */

void GeneratePolarization3DPlot(string gnuScriptFileName, string outputPlotEPSFileName, string datafilename, bool plotContinuum, bool display, stokes_parameter_t StokesParameter) {
    ofstream *fgnu = NULL;
    
    if (!gnuScriptFileName.empty()) {
        remove(gnuScriptFileName.c_str()); // delete any existing file with the same name
        fgnu = new ofstream();
        fgnu->open(gnuScriptFileName.c_str());
    } else {
        exit(EXIT_FAILURE);
    }
    
    *fgnu << "reset" << endl;
    *fgnu << "unset key" << endl;
    *fgnu << "set view 0,0" << endl;
    
    *fgnu << "set palette gray" << endl;
    *fgnu << "set palette gamma 2.0" << endl;
    *fgnu << "set pm3d map" << endl;
    *fgnu << "unset ztics" << endl;
    
    *fgnu << "set xrange[-200:*]" << endl;
    
    *fgnu << "\nset xlabel \"distance (pixels)\"" << endl;
    *fgnu << "set ylabel \"order number\"" << endl;
    
    unsigned columnForStokesI = 5;
    unsigned columnForStokesQUV = 6;
    unsigned columnForDegreePolQUV = 7;
    unsigned columnForNULL1 = 8;
    unsigned columnForNULL2 = 9;
    
    if(plotContinuum) {
        if(StokesParameter == StokesQ) {
            *fgnu << "set cblabel \"Stokes I and Q\"" << endl;
        } else if (StokesParameter == StokesU) {
            *fgnu << "set cblabel \"Stokes I and U\"" << endl;
        } else if (StokesParameter == StokesV) {
            *fgnu << "set cblabel \"Stokes I and V\"" << endl;
        }
        
        *fgnu << "set log z" << endl;
    } else {
        if(StokesParameter == StokesQ) {
            *fgnu << "set cblabel \"Stokes Q / Stokes I\"" << endl;
        } else if (StokesParameter == StokesU) {
            *fgnu << "set cblabel \"Stokes U / Stokes I\"" << endl;
        } else if (StokesParameter == StokesV) {
            *fgnu << "set cblabel \"Stokes V / Stokes I\"" << endl;
        }        
//        *fgnu << "set zrange[-1:1]" << endl;
    }
    
    if(!outputPlotEPSFileName.empty()) {
        *fgnu << "\nset terminal postscript enhanced color solid lw 1.5 \"Helvetica\" 14" << endl;
        *fgnu << "set output \"" << outputPlotEPSFileName << "\"" << endl;
        *fgnu << endl;
        
        if(plotContinuum) {
            *fgnu << "splot \"" << datafilename << "\""
            << " u 3:($2 + 0.3*$1 - 0.325):" << columnForStokesI <<" w pm3d"
            << ",\"\" u 3:($2 + 0.3*$1 + 0.025):" << columnForStokesQUV << " w pm3d" << endl;
        } else {
            *fgnu << "splot \"" << datafilename << "\""
            << " u 3:($2 + 0.25*$1 - 0.125):" << columnForDegreePolQUV <<" w pm3d"
            << ",\"\" u 3:($2 + 0.2*$1 + 0.125 + 0.05):" << columnForNULL1 << " w pm3d"
            << ",\"\" u 3:($2 - 0.2*$1 - 0.125 - 0.05):" << columnForNULL2 << " w pm3d" << endl;
        }
        
        if (display) {
            *fgnu << "\nset terminal x11" << endl;
            *fgnu << "set output" << endl;
            *fgnu << "replot" << endl;
        } else {
            *fgnu << "\n#set terminal x11" << endl;
            *fgnu << "#set output" << endl;
            *fgnu << "#replot" << endl;
        }
    } else {
        *fgnu << endl;
        
        if(plotContinuum) {
            *fgnu << "splot \"" << datafilename << "\""
            << " u 3:($2 + 0.3*$1 - 0.325):" << columnForStokesI <<" w pm3d"
            << ",\"\" u 3:($2 + 0.3*$1 + 0.025):" << columnForStokesQUV << " w pm3d" << endl;
        } else {
            *fgnu << "splot \"" << datafilename << "\""
            << " u 3:($2 + 0.25*$1 - 0.125):" << columnForDegreePolQUV <<" w pm3d"
            << ",\"\" u 3:($2 + 0.2*$1 + 0.125 + 0.05):" << columnForNULL1 << " w pm3d"
            << ",\"\" u 3:($2 - 0.2*$1 - 0.125 - 0.05):" << columnForNULL2 << " w pm3d" << endl;
        }
        
        *fgnu << "\n#set terminal postscript enhanced color solid lw 1.5 \"Helvetica\" 14" << endl;
        *fgnu << "#set output \"outputPlotEPSFileName.eps\"" << endl;
        *fgnu << "#replot" << endl;
        *fgnu << "#set terminal x11" << endl;
        *fgnu << "#set output" << endl;
    }
    
    fgnu->close();
    
    if (display) {
        systemf("gnuplot -persist %s",gnuScriptFileName.c_str());
    } else {
        if(!outputPlotEPSFileName.empty())
            systemf("gnuplot %s",gnuScriptFileName.c_str());
    }
}
