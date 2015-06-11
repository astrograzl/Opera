/*******************************************************************
 ****               		OPERA PIPELINE v1.0                 ****
 *******************************************************************
 Library name: operaExtendedSpectrumCreation
 Version: 1.0
 Author(s): CFHT OPERA team
 Affiliation: Canada France Hawaii Telescope 
 Location: Hawaii USA
 Date: May/2015
 
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

#include "libraries/operaSpectralOrderVector.h"
#include "libraries/operaArgumentHandler.h"
#include "libraries/operaCommonModuleElements.h"

using namespace std;

/*! 
 * operaExtendedSpectrumCreation
 * \author Eder Martioli / Christopher Usher
 * \brief Encapulates the functionality of modules which generate calibrated spectra.
 * \file operaSpectrumCalibration.cpp
 * \ingroup libraries
 */

void GenerateExtractionPlot(const char *gnuScriptFileName, const char *outputPlotEPSFileName,const char *dataFileName, unsigned nbeams, bool display);

int ExtendedSpectrumCreation(int argc, char *argv[], const string moduleName, const bool StarPlusSky, const bool PolarimetryCorrection)
{
	operaArgumentHandler args;
	
	string input; 
	string outputSpectraFile;
	string object;
	unsigned spectralOrderTypeVal = CalibratedExtendedBeamSpectrum;
	string wavelengthCalibration;
	string inputFlatFluxCalibration;
    string inputWavelengthMaskForUncalContinuum;
    unsigned numberOfPointsInUniformSample = 150;
    unsigned normalizationBinsize = 100;
    bool starplusskyInvertSkyFiber = false; // Only a parameter in star+sky mode

    double SkyOverStarFiberAreaRatio = 1.0; // Sky over Star fiber area ratio to compensate for different apertures.
    // For ESPaDOnS S+S -> SkyOverStarFiberAreaRatio = (2.2*2.2)/(1.6*1.6)
    
	string radialvelocitycorrection;
	string telluriccorrection;
	
    // Parameters for flux calibration   
    string fluxCalibration;
    string flatResponse;
	double exposureTime = 0.0;
    bool AbsoluteCalibration = false;

	int ordernumber = NOTPROVIDED;
    int minorder = NOTPROVIDED;
    int maxorder = NOTPROVIDED;
    
    string plotfilename;
	string spectrumDataFilename;
	string scriptfilename;
	bool interactive = false;
	
	if(PolarimetryCorrection) args.AddRequiredArgument("polar", input, "Input file name (.p)");
	else args.AddRequiredArgument("inputUncalibratedSpectrum", input, "Input file name (.e)");
	args.AddRequiredArgument("outputCalibratedSpectrum", outputSpectraFile, "Output file name (.spc)");
	args.AddRequiredArgument("object", object, "Output object name");
	args.AddRequiredArgument("spectrumtype", spectralOrderTypeVal, "Spectrum type");
	args.AddRequiredArgument("wavelengthCalibration", wavelengthCalibration, "Wavelength calibration file (.wcal)");
	args.AddRequiredArgument("inputFlatFluxCalibration", inputFlatFluxCalibration, "Flat field spectrum (.fcal)");
	args.AddRequiredArgument("inputWavelengthMaskForUncalContinuum", inputWavelengthMaskForUncalContinuum, "");
	args.AddRequiredArgument("numberOfPointsInUniformSample", numberOfPointsInUniformSample, "");
	args.AddRequiredArgument("normalizationBinsize", normalizationBinsize, "Binsize for normalization");
	if(StarPlusSky) args.AddOptionalArgument("starplusskyInvertSkyFiber", starplusskyInvertSkyFiber, false, "Invert sky fiber (default is beam[0]=star, beam[1]=sky)");
    args.AddOptionalArgument("SkyOverStarFiberAreaRatio", SkyOverStarFiberAreaRatio, 1.0, "Sky over Star fiber area ratio, to compensate for different apertures.");
    args.AddOptionalArgument("radialvelocitycorrection", radialvelocitycorrection, "", "Heliocentric wavelength correction file (.rvel)");
	args.AddOptionalArgument("telluriccorrection", telluriccorrection, "", "Telluric wavelength correction file (.tell)");
	args.AddOptionalArgument("flatResponse", flatResponse, "", "Flat response calibration file (LE .s file)");
	args.AddOptionalArgument("fluxCalibration", fluxCalibration, "", "Flux calibration file (.fcal), overrides flatResponse");
	args.AddOptionalArgument("etime", exposureTime, 0.0, "Exposure time, used with flux calibration");
	args.AddOptionalArgument("AbsoluteCalibration", AbsoluteCalibration, false, "Perform absolute flux calibration instead of relative");
	args.AddOrderLimitArguments(ordernumber, minorder, maxorder, NOTPROVIDED);
	args.AddPlotFileArguments(plotfilename, spectrumDataFilename, scriptfilename, interactive);
	
	try {
		args.Parse(argc, argv);
		
		operaSpectralOrder_t spectralOrderType = (operaSpectralOrder_t)spectralOrderTypeVal;
		// We need input and output files
		if (input.empty()) throw operaException(moduleName + ": ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);	
		if (outputSpectraFile.empty()) throw operaException(moduleName + ": ", operaErrorNoOutput, __FILE__, __FUNCTION__, __LINE__);
		if (wavelengthCalibration.empty()) throw operaException(moduleName + ": wcal: ", operaErrorNoInput, __FILE__, __FUNCTION__, __LINE__);	
		
		if (args.verbose) {
			if(PolarimetryCorrection) cout << moduleName << ": polar = " << input << endl; 
			else cout << moduleName << ": input spectrum = " << input << endl; 
			cout << moduleName << ": object = " << object << endl; 
			cout << moduleName << ": output spectrum file = " << outputSpectraFile << endl;
			cout << moduleName << ": spectrum type = " << spectralOrderType << endl;							
			cout << moduleName << ": wavelength calibration file = " << wavelengthCalibration << endl;
            cout << moduleName << ": radialvelocitycorrection = " << radialvelocitycorrection << endl;
            cout << moduleName << ": telluriccorrection = " << telluriccorrection << endl;
            cout << moduleName << ": inputFlatFluxCalibration = " << inputFlatFluxCalibration << endl;
            cout << moduleName << ": inputWavelengthMaskForUncalContinuum = " << inputWavelengthMaskForUncalContinuum << endl;
            cout << moduleName << ": numberOfPointsInUniformSample = " << numberOfPointsInUniformSample << endl;            
            cout << moduleName << ": binsize for normalization = " << normalizationBinsize << endl;  
            cout << moduleName << ": input flux calibration file = " << fluxCalibration << endl; 
            cout << moduleName << ": input flat response calibration file = " << flatResponse << endl;
            cout << moduleName << ": exposure time = " << exposureTime << endl;
            cout << moduleName << ": SkyOverStarFiberAreaRatio = " << SkyOverStarFiberAreaRatio << endl;
			cout << moduleName << ": absolute calibration = " << AbsoluteCalibration << endl;
            if (ordernumber != NOTPROVIDED) cout << moduleName << ": ordernumber = " << ordernumber << endl;            
            if (args.plot) {
                cout << moduleName << ": plotfilename = " << plotfilename << endl;
                cout << moduleName << ": spectrumDataFilename = " << spectrumDataFilename << endl;
                cout << moduleName << ": scriptfilename = " << scriptfilename << endl; 
                cout << moduleName << ": interactive = " << (interactive ? "YES" : "NO") << endl; 
            }
		}
        
		/*
		 * Down to business, read in all the source and calibration data.
		 */
        operaSpectralOrderVector spectralOrders(input);
        spectralOrders.ReadSpectralOrders(wavelengthCalibration);
        
		UpdateOrderLimits(ordernumber, minorder, maxorder, spectralOrders);
        if (args.verbose) cout << moduleName << ": minorder ="<< minorder << " maxorder=" << maxorder << endl;
        
        int minPossibleOrder = 0;
        int maxPossibleOrder = 0;
        
        for (int order=minorder; order<=maxorder; order++) {
			operaSpectralOrder *spectralOrder = spectralOrders.GetSpectralOrder(order);
            
			if (spectralOrder->gethasSpectralElements() && spectralOrder->gethasWavelength()) {
				spectralOrder->getSpectralElements()->CreateExtendedvectors(spectralOrder->getSpectralElements()->getnSpectralElements());
                
                // Save the raw flux for later
                spectralOrder->getSpectralElements()->copyTOrawFlux();
                spectralOrder->getSpectralElements()->copyTOnormalizedFlux();
                spectralOrder->getSpectralElements()->copyTOfcalFlux();
                
                operaWavelength *Wavelength = spectralOrder->getWavelength();
                spectralOrder->getSpectralElements()->setwavelengthsFromCalibration(Wavelength);
                spectralOrder->getSpectralElements()->copyTOtell();
                
                if(order < minPossibleOrder || minPossibleOrder==0) {
                    minPossibleOrder = order;
                }
                if(order > maxPossibleOrder) {
                    maxPossibleOrder = order;
                }
            }
		}
        
        if(minPossibleOrder > minorder) {
            minorder = minPossibleOrder;
            if (args.verbose) cout << moduleName << ": minorder reset to " << minorder << endl;
        }
        if(maxPossibleOrder < maxorder) {
            maxorder = maxPossibleOrder;
            if (args.verbose) cout << moduleName << ": maxorder reset to " << maxorder << endl;
        }
        
        unsigned NumberofBeams = spectralOrders.getNumberofBeams(minorder, maxorder);

        // Load telluric correction for wavelength calibration
		if (!telluriccorrection.empty()) {
            spectralOrders.readTelluricRVINTOExtendendSpectra(telluriccorrection, minorder, maxorder);
		}
        
        // Load Heliocentric RV wavelength correction and also wavelength calibration
        if (!radialvelocitycorrection.empty()) {
            spectralOrders.readRVCorrectionINTOExtendendSpectra(radialvelocitycorrection, wavelengthCalibration, minorder, maxorder);
        }
        
        // Correct flat-field
        if (!inputFlatFluxCalibration.empty()) {
            spectralOrders.correctFlatField(inputFlatFluxCalibration, minorder, maxorder, StarPlusSky, starplusskyInvertSkyFiber);
            spectralOrders.saveExtendedRawFlux(minorder, maxorder);
        }
        
        // Flux Normalization and Flux Calibration
		if (!inputWavelengthMaskForUncalContinuum.empty()) {
			const double delta_wl = 1.0; // Wavelength range (in nm) for stiching non-overlapping orders
			if(PolarimetryCorrection) exposureTime *= 4.0;
            if (!fluxCalibration.empty()) {
                spectralOrders.normalizeAndCalibrateFluxINTOExtendendSpectra(inputWavelengthMaskForUncalContinuum,fluxCalibration, exposureTime, AbsoluteCalibration,numberOfPointsInUniformSample,normalizationBinsize, delta_wl, minorder, maxorder, false, SkyOverStarFiberAreaRatio, StarPlusSky);
            } else if (!flatResponse.empty()) {
                spectralOrders.normalizeAndApplyFlatResponseINTOExtendendSpectra(inputWavelengthMaskForUncalContinuum,flatResponse,numberOfPointsInUniformSample,normalizationBinsize, delta_wl, minorder, maxorder, false, StarPlusSky);
            } else {
                spectralOrders.normalizeFluxINTOExtendendSpectra(inputWavelengthMaskForUncalContinuum,numberOfPointsInUniformSample,normalizationBinsize, delta_wl, minorder, maxorder, false);
            }
        } else {
            spectralOrders.normalizeOrderbyOrderAndSaveFluxINTOExtendendSpectra(normalizationBinsize, minorder, maxorder, false);
        }
        
        // Output wavelength calibrated spectrum
		spectralOrders.setObject(object);
		spectralOrders.WriteSpectralOrders(outputSpectraFile, spectralOrderType);

		if (!spectrumDataFilename.empty() && !plotfilename.empty() && !scriptfilename.empty()) {
			GenerateExtractionPlot(scriptfilename.c_str(),plotfilename.c_str(),spectrumDataFilename.c_str(), NumberofBeams, interactive);
		}
	}
	catch (operaException e) {
		cerr << moduleName << ": " << e.getFormattedMessage() << endl;
		return EXIT_FAILURE;
	}
	catch (...) {
		cerr << moduleName << ": " << operaStrError(errno) << endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void GenerateExtractionPlot(const char *gnuScriptFileName, const char *outputPlotEPSFileName,const char *dataFileName, unsigned nbeams, bool display)
{
    FILE *fgnu;
    remove(gnuScriptFileName); // delete any existing file with the same name
	
    fgnu = fopen(gnuScriptFileName,"w");
    
    fprintf(fgnu,"unset key\n");
    fprintf(fgnu,"set view 0,0\n");
    fprintf(fgnu,"set iso 100\n");
    fprintf(fgnu,"set samples 100\n");
    fprintf(fgnu,"set pm3d at s\n");
    fprintf(fgnu,"set ticslevel 0\n");   
    
    fprintf(fgnu,"set terminal postscript enhanced color solid lw 1.5 \"Helvetica\" 14\n");
    fprintf(fgnu,"set output \"%s\"\n",outputPlotEPSFileName);
	
    unsigned fluxcol = 6 + 4*nbeams;
    fprintf(fgnu,"splot \"%s\" u 5:1:%u with pm3d\n",dataFileName,fluxcol);
	
    if (display) {
		fprintf(fgnu,"set output\n");
		fprintf(fgnu,"set terminal x11\n");
		fprintf(fgnu,"replot\n");       
		fclose(fgnu);   
		systemf("gnuplot -persist %s",gnuScriptFileName);
    } else {
		fclose(fgnu);  
	}
}