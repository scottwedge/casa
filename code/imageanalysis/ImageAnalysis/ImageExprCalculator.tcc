#include "ImageExprCalculator.h"


#include <casacore/casa/BasicSL/String.h>
#include <casacore/images/Images/ImageInterface.h>

#include <imageanalysis/ImageTypedefs.h>

#include <memory>

using namespace std;

namespace casa {

template<class T> ImageExprCalculator<T>::ImageExprCalculator(
	const String& expression, const String& outname, Bool overwrite
) : /*ImageTask<T>(
	SPCIIT(), "", nullptr, "", "", "", "", outname, overwrite
),*/ _expr(expression), _copyMetaDataFromImage(), _outname(outname),
	_overwrite(overwrite), _log() {
	ThrowIf(_expr.empty(), "You must specify an expression");
	if (! outname.empty() && ! overwrite) {
		//NewFile validfile;
		String errmsg;
		ThrowIf(
			! NewFile().valueOK(outname, errmsg), errmsg
		);
	}
}

template<class T> SPIIT ImageExprCalculator<T>::compute() const {
	_log << LogOrigin(getClass(), __func__);

	Record regions;

	// Get LatticeExprNode (tree) from parser.  Substitute possible
	// object-id's by a sequence number, while creating a
	// LatticeExprNode for it.  Convert the GlishRecord containing
	// regions to a PtrBlock<const ImageRegion*>.

	Block<LatticeExprNode> temps;
	String exprName;
	PtrBlock<const ImageRegion*> tempRegs;
	_makeRegionBlock(tempRegs, regions);
	LatticeExprNode node = ImageExprParse::command(_expr, temps, tempRegs);

	// Get the shape of the expression
	const IPosition shape = node.shape();

	// Get the CoordinateSystem of the expression
	const LELAttribute attr = node.getAttribute();
	const LELLattCoordBase* lattCoord = &(attr.coordinates().coordinates());
	ThrowIf(
		! lattCoord->hasCoordinates()
		|| lattCoord->classname() != "LELImageCoord",
		"Images in expression have no coordinates"
	);
	const LELImageCoord* imCoord =
		dynamic_cast<const LELImageCoord*> (lattCoord);
	AlwaysAssert (imCoord != 0, AipsError);
	CoordinateSystem csys = imCoord->coordinates();
	//DataType type = node.dataType();
	SPIIT computedImage = _imagecalc(
		node, shape, csys, imCoord
	);
	// Delete the ImageRegions (by using an empty Record).
	_makeRegionBlock(tempRegs, Record());
	return computedImage;
}

template<class T> SPIIT ImageExprCalculator<T>::_imagecalc(
	const LatticeExprNode& node, const IPosition& shape,
	const CoordinateSystem& csys, const LELImageCoord* const imCoord
) const {
	_log << LogOrigin(getClass(), __func__);

	// Create LatticeExpr create mask if needed
	LatticeExpr<T> latEx(node);
	SPIIT image;
	String exprName;
	// Construct output image - an ImageExpr or a PagedImage
	if (_outname.empty()) {
		image.reset(new ImageExpr<T> (latEx, exprName));
		ThrowIf(! image, "Failed to create ImageExpr");
	}
	else {
		_log << LogIO::NORMAL << "Creating image `" << _outname
			<< "' of shape " << shape << LogIO::POST;
		try {
			image.reset(new PagedImage<T> (shape, csys, _outname));
		}
		catch (const TableError& te) {
			ThrowIf(
				_overwrite,
				"Caught TableError. This often means "
				"the table you are trying to overwrite has been opened by "
				"another method and so cannot be overwritten at this time. "
				"Try closing it and rerunning"
			);
		}
		ThrowIf(! image, "Failed to create PagedImage");

		// Make mask if needed, and copy data and mask
		if (latEx.isMasked()) {
			String maskName("");
			ImageMaskAttacher::makeMask(*image, maskName, False, True, _log, True);
		}
		LatticeUtilities::copyDataAndMask(_log, *image, latEx);
	}

	// Copy miscellaneous stuff over
	SPIIT copyFromImage;
	if (! _copyMetaDataFromImage.empty()) {
		copyFromImage = ImageUtilities::openImage<T>(_copyMetaDataFromImage);
	}
	if (copyFromImage) {
		image->setMiscInfo(copyFromImage->miscInfo());
		image->setImageInfo(copyFromImage->imageInfo());
		image->setCoordinateInfo(copyFromImage->coordinates());
	}
	else {
		image->setMiscInfo(imCoord->miscInfo());
		image->setImageInfo(imCoord->imageInfo());
	}
	if (_expr.contains("spectralindex")) {
		image->setUnits("");
	}
	else if (_expr.contains(Regex("pa\\(*"))) {
		image->setUnits("deg");
		Vector<Int> newstokes(1);
		newstokes = Stokes::Pangle;
		StokesCoordinate scOut(newstokes);
		CoordinateSystem cSys = image->coordinates();
		Int iStokes = cSys.findCoordinate(Coordinate::STOKES, -1);
		cSys.replaceCoordinate(scOut, iStokes);
		image->setCoordinateInfo(cSys);
	}
	else {
		if (copyFromImage) {
			image->setUnits(copyFromImage->units());
		}
		else {
			image->setUnits(imCoord->unit());
		}
	}
	return image;
}

template<class T> void ImageExprCalculator<T>::_makeRegionBlock(
	PtrBlock<const ImageRegion*>& regions,
	const Record& Regions
) {
	for (uInt j = 0; j < regions.nelements(); j++) {
		delete regions[j];
	}
	regions.resize(0, True, True);
	uInt nreg = Regions.nfields();
	if (nreg > 0) {
		regions.resize(nreg);
		regions.set(static_cast<ImageRegion*> (0));
		for (uInt i = 0; i < nreg; i++) {
			regions[i] = ImageRegion::fromRecord(Regions.asRecord(i), "");
		}
	}
}

}