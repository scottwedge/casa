//# Copyright (C) 2008
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
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//#
//# $Id$

#include <plotms/PlotMS/PlotMS.h>
#include <plotms/Plots/PlotMSPlotParameterGroups.h>
#include <plotms/Plots/PlotMSPlot.h>
#include <plotms/test/tUtil.h>


#include <iostream>
#include <msvis/MSVis/UtilJ.h>
#include <casa/namespace.h>
#include <QApplication>

/**
 * Tests whether an iteration plot can be exported as one plot per page.
 * Supposedly this is the scenario the pipeline will be running.
 */

int main(int /*argc*/, char** /*argv[]*/) {

	String dataPath = tUtil::getFullPath( "pm_ngc5921.ms" );
    cout << "tExportRangePipeline using data from "<<dataPath.c_str()<<endl;
    String exportPath = tUtil::getExportPath();
    cout << "Writing plotfiles to " << exportPath << endl;

    // Set up plotms object.
    PlotMSApp app(false, false);

    // Set up parameters for plot.
    PlotMSPlotParameters plotParams = PlotMSPlot::makeParameters(&app);

    PMS_PP_MSData* ppdata = plotParams.typedGroup<PMS_PP_MSData>();
    if (ppdata == NULL) {
        plotParams.setGroup<PMS_PP_MSData>();
        ppdata = plotParams.typedGroup<PMS_PP_MSData>();
    }
    ppdata->setFilename( dataPath );

    //We are going to iterate over scan, using a shared axis and global
    //scale for the iteration.
    PMS_PP_Iteration* iterationParams = plotParams.typedGroup<PMS_PP_Iteration>();
    if ( iterationParams == NULL ){
    	plotParams.setGroup<PMS_PP_Iteration>();
    	iterationParams = plotParams.typedGroup<PMS_PP_Iteration>();
    }
    iterationParams->setIterationAxis( PMS::SCAN );


    //Set a left y-axis and a top x-axis
    PMS_PP_Axes* axisParams = plotParams.typedGroup<PMS_PP_Axes>();
    if (axisParams == NULL) {
    	plotParams.setGroup<PMS_PP_Axes>();
        axisParams = plotParams.typedGroup<PMS_PP_Axes>();
    }
    axisParams->setAxes(X_BOTTOM, Y_LEFT, 0 );

    //We want to print all pages in the output.
    PlotMSExportParam& exportParams = app.getExportParameters();
    exportParams.setExportRange( PMS::PAGE_ALL );

    //Make the plot.
    app.addOverPlot( &plotParams );

    //Note:  There will be seven plots generated, but we will just
    //check the first.
    String outFile = exportPath + "plotMSExportRangePipeline";
    String outFile1( outFile + "_Scan1.jpg");
    String outFile2( outFile + "_Scan2_2.jpg");
    String outFile3( outFile + "_Scan3_3.jpg");
    String outFile4( outFile + "_Scan4_4.jpg");
    String outFile5( outFile + "_Scan5_5.jpg");
    String outFile6( outFile + "_Scan6_6.jpg");
    String outFile7( outFile + "_Scan7_7.jpg");

    PlotExportFormat::Type type = PlotExportFormat::JPG;
	PlotExportFormat format(type, outFile + ".jpg" );
	format.resolution = PlotExportFormat::SCREEN;

	bool ok = app.save(format);
	cout << "tExportRangePipeline:: Result of save=" << ok << endl;
	bool okOutput = tUtil::checkFile( outFile1, 120000, 140000, -1 );
	cout << "tExportRangePipeline:: Result of first save file check=" << okOutput << endl;
    bool test = ok && okOutput;

    // clean up
    tUtil::clearFile(outFile1);
    tUtil::clearFile(outFile2);
    tUtil::clearFile(outFile3);
    tUtil::clearFile(outFile4);
    tUtil::clearFile(outFile5);
    tUtil::clearFile(outFile6);
    tUtil::clearFile(outFile7);
    tUtil::clearFile(exportPath);

    bool checkGui = tUtil::exitMain( false );
    return !(test && checkGui);
}

