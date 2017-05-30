//# FiltrationTVI_GTest:   test of Filtration TVIs
//# Copyright (C) 1995,1999,2000,2001,2016
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This program is free software; you can redistribute it and/or modify it
//# under the terms of the GNU General Public License as published by the Free
//# Software Foundation; either version 2 of the License, or (at your option)
//# any later version.
//#
//# This program is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//# more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with this program; if not, write to the Free Software Foundation, Inc.,
//# 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$
#include <gtest/gtest.h>
#include <memory>
#include <typeinfo>
#include <limits>
#include <cmath>

#include <mstransform/TVI/FiltrationTVI.h>
#include <mstransform/TVI/FiltrationTVI.tcc>

#include <casacore/casa/aips.h>
#include <casacore/casa/OS/EnvVar.h>
#include <casacore/casa/OS/File.h>
#include <casacore/casa/OS/RegularFile.h>
#include <casacore/casa/OS/SymLink.h>
#include <casacore/casa/OS/Directory.h>
#include <casacore/casa/OS/DirectoryIterator.h>
#include <casacore/casa/Exceptions/Error.h>
#include <casacore/casa/iostream.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/iomanip.h>
#include <casacore/casa/Containers/Record.h>
#include <casacore/casa/Utilities/GenSort.h>

#include <casacore/tables/Tables/ArrColData.h>
#include <casacore/tables/DataMan/TiledShapeStMan.h>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <msvis/MSVis/VisibilityIteratorImpl2.h>
#include <msvis/MSVis/LayeredVi2Factory.h>

using namespace std;
using namespace casa;
using namespace casacore;
using namespace casa::vi;

namespace {
string GetCasaDataPath() {
  if (casacore::EnvironmentVariable::isDefined("CASAPATH")) {
    string casapath = casacore::EnvironmentVariable::get("CASAPATH");
    size_t endindex = casapath.find(" ");
    if (endindex != string::npos) {
      string casaroot = casapath.substr(0, endindex);
      cout << "casaroot = " << casaroot << endl;
      return (casaroot + "/data/");
    } else {
      cout << "hit npos" << endl;
      return "/data/";
    }
  } else {
    cout << "CASAPATH is not defined" << endl;
    return "";
  }
}

template<class T>
struct VerboseDeleterForNew {
  void operator()(T *p) {
    cout << "Destructing " << typeid(p).name() << endl;
    delete p;
  }
};

// dummy
class FiltrationTestTVIFactory;
template<class Filter>
class FiltrationTVIWrapper: public FiltrationTVI<Filter> {
  FiltrationTVIWrapper(ViImplementation2 * inputVi, Filter *filter) :
      FiltrationTVI<Filter>(inputVi, filter) {
  }

  virtual ~FiltrationTVIWrapper() {
  }

private:
  friend class FiltrationTestTVIFactory;
};

// Filter class for testing
class FilterTypeLocal {
public:
  enum {
    Porous = -1, Nonporous = -2
  };
};

/**
 * PorousFilter
 *
 * PorousFilter is an implementaiton of the filter that pass through
 * everything. It is equivalent to the case when no filter is
 * inserted to the TVI layer.
 */
class PorousFilter {
public:
  // constructor
  PorousFilter(MeasurementSet const &/*ms*/, Record const &/*configuration*/) {
  }

  // destructor
  ~PorousFilter() {
  }

  // return string representation of the filter type
  String filterType() const {
    return String("Porous");
  }

  // filter query
  // isResidue returns true if given vb doesn't pass through the filter
  bool isResidue(VisBuffer2 const *vb) {
    return !isFiltrate(vb);
  }

  // isFiltrate returns true if given vb does pass through the filter
  // (either fully and partly)
  bool isFiltrate(VisBuffer2 const */*vb*/) {
    return true;
  }

  // row-wise filtration information
  // it fills in is_filtrate vector (resize if necessary)
  // and returns number of rows that pass through the filter
  int isFiltratePerRow(VisBuffer2 const *vb, Vector<bool> &is_filtrate) {
    int nrows = vb->nRows();
    is_filtrate.resize(nrows);
    is_filtrate = true;
    return nrows;
  }

private:
  void initFilter() {
  }
};

/**
 * NonporousFilter
 *
 * NonporousFilter is an implementaiton of the filter that filter out
 * everything. No data will be emerged if it is inserted to the TVI
 * layer.
 */
class NonporousFilter {
public:
  // constructor
  NonporousFilter(MeasurementSet const &/*ms*/,
      Record const &/*configuration*/) {
  }

  // destructor
  ~NonporousFilter() {
  }

  // return string representation of the filter type
  String filterType() const {
    return String("Nonporous");
  }

  // filter query
  // isResidue returns true if given vb doesn't pass through the filter
  bool isResidue(VisBuffer2 const *vb) {
    return !isFiltrate(vb);
  }

  // isFiltrate returns true if given vb does pass through the filter
  // (either fully and partly)
  bool isFiltrate(VisBuffer2 const */*vb*/) {
    return false;
  }

  // row-wise filtration information
  // it fills in is_filtrate vector (resize if necessary)
  // and returns number of rows that pass through the filter
  int isFiltratePerRow(VisBuffer2 const *vb, Vector<bool> &is_filtrate) {
    int nrows = vb->nRows();
    is_filtrate.resize(nrows);
    is_filtrate = false;
    return nrows;
  }

private:
  void initFilter() {
  }
};

class FiltrationTestTVIFactory: public ViFactory {

public:
  // Constructor
  FiltrationTestTVIFactory(Record const &configuration,
      ViImplementation2 *inputVII) :
      inputVII_p(inputVII), configuration_p(configuration) {
  }

  // Destructor
  ~FiltrationTestTVIFactory() {
  }

  virtual ViImplementation2 * createVi() const {
    ViImplementation2 *vii = nullptr;

    Bool is_porous = true;
    if (configuration_p.isDefined("type")
        && configuration_p.asInt("type") == (Int) FilterTypeLocal::Nonporous) {
      is_porous = false;
    }
    cout << "type_enum = " << configuration_p.asInt("type") << endl;
    cout << "is_porous = " << is_porous << endl;

    MeasurementSet const &ms = inputVII_p->ms();

    if (is_porous) {
      PorousFilter *filter = new PorousFilter(ms, configuration_p);
      vii = new FiltrationTVIWrapper<PorousFilter>(inputVII_p, filter);
    } else {
      NonporousFilter *filter = new NonporousFilter(ms, configuration_p);
      vii = new FiltrationTVIWrapper<NonporousFilter>(inputVII_p, filter);
    }

    return vii;
  }

private:
  ViImplementation2 *inputVII_p;
  Record configuration_p;
};

struct ValidatorUtil {
  template<class T>
  static void ValidateArray(Array<T> const &ref, Array<T> const &result) {
    ASSERT_TRUE(ref.conform(result));
    Bool b1, b2;
    auto const p_ref = ref.getStorage(b1);
    auto const p_result = result.getStorage(b2);
    size_t arraySize = ref.size();
    for (size_t i = 0; i < arraySize; ++i) {
      ValidateScalar(p_ref[i], p_result[i]);
    }
  }
private:
  template<class T>
  static void ValidateScalar(T const ref, T const result) {
    EXPECT_EQ(ref, result);
  }

};

template<>
void ValidatorUtil::ValidateScalar<Float>(Float const ref, Float const result) {
  EXPECT_FLOAT_EQ(ref, result);
}

template<>
void ValidatorUtil::ValidateScalar<Complex>(Complex const ref,
    Complex const result) {
  EXPECT_FLOAT_EQ(ref.real(), result.real());
  EXPECT_FLOAT_EQ(ref.imag(), result.imag());
}

struct ValidatorBase {
  template<class T>
  static void ValidateCube(Cube<T> const &data, Cube<T> const &ref, Vector<bool> const &flag) {
    IPosition const refshape = ref.shape();
    IPosition const shape = data.shape();
    ASSERT_EQ((size_t)refshape[2], flag.nelements());
    ASSERT_EQ(shape[2], (ssize_t)ntrue(flag));

    ssize_t n = refshape[2];
    ssize_t j = 0;
    for (ssize_t i = 0; i < n; ++i) {
      if (flag[i]) {
        EXPECT_TRUE(allEQ(data.xyPlane(j), ref.xyPlane(i)));
        ++j;
      }
    }
  }
};

struct PorousValidator : public ValidatorBase {
  static Int GetMode() {
    return FilterTypeLocal::Porous;
  }

  static String GetTypePrefix() {
    return "FiltrationTVI<Porous>(";
  }

  static bool IsResidue(VisBuffer2 const */*vb*/) {
    return false;
  }

  static bool IsFiltrate(VisBuffer2 const */*vb*/) {
    return true;
  }

  static int IsFiltratePerRow(VisBuffer2 const *vb, Vector<bool> &is_filtrate) {
    auto const nrow = vb->nRows();
    is_filtrate.resize(nrow);
    is_filtrate = true;
    return nrow;
  }
};

struct NonporousValidator : public ValidatorBase {
  static Int GetMode() {
    return FilterTypeLocal::Nonporous;
  }

  static String GetTypePrefix() {
    return "FiltrationTVI<Nonporous>(";
  }

  static bool IsResidue(VisBuffer2 const */*vb*/) {
    return true;
  }

  static bool IsFiltrate(VisBuffer2 const */*vb*/) {
    return false;
  }

  static int IsFiltratePerRow(VisBuffer2 const *vb, Vector<bool> &is_filtrate) {
    auto const nrow = vb->nRows();
    is_filtrate.resize(nrow);
    is_filtrate = false;
    return nrow;
  }
};

template<class Impl>
class Manufacturer {
public:
  struct Product {
    ViFactory *factory;
    ViImplementation2 *vii;
  };
  static VisibilityIterator2 *ManufactureVI(MeasurementSet *ms,
      Int const &type_enum) {
    cout << "### Manufacturer: " << endl << "###   " << Impl::GetTestPurpose()
        << endl;

    Record type_rec;
    type_rec.define("type", type_enum);

    // build factory object
    Product p = Impl::BuildFactory(ms, type_rec);
    std::unique_ptr<ViFactory> factory(p.factory);

    std::unique_ptr<VisibilityIterator2> vi;
    try {
      vi.reset(new VisibilityIterator2(*factory.get()));
    } catch (...) {
      cout << "Failed to create VI at factory" << endl;
      // vii must be deleted since it is never managed by vi
      if (p.vii) {
        delete p.vii;
      }
      throw;
    }

    cout << "Created VI type \"" << vi->ViiType() << "\"" << endl;

    return vi.release();
  }
};

class TestManufacturer: public Manufacturer<TestManufacturer> {
public:
  static Product BuildFactory(MeasurementSet *ms, Record const &mode) {
    // create read-only VI impl
    Block<MeasurementSet const *> const mss(1, ms);
    SortColumns defaultSortColumns;

    std::unique_ptr<ViImplementation2> inputVii(
        new VisibilityIteratorImpl2(mss, defaultSortColumns, 0.0, VbPlain,
            False));
    std::unique_ptr<ViFactory> factory(
        new FiltrationTestTVIFactory(mode, inputVii.get()));

    Product p;

    // vi will be responsible for releasing inputVii so unique_ptr
    // should release the ownership here
    p.vii = inputVii.release();
    p.factory = factory.release();

    return p;
  }

  static String GetTestPurpose() {
    return "Test FiltrationTestTVIFactory(Record const &, ViImplementation2 *)";
  }
};

class BasicManufacturer : public Manufacturer<BasicManufacturer> {
public:
  static Product BuildFactory(MeasurementSet *ms, Record const &mode) {
    // create read-only VI impl
    Block<MeasurementSet const *> const mss(1, ms);
    SortColumns defaultSortColumns;

    std::unique_ptr<ViImplementation2> inputVii(
        new VisibilityIteratorImpl2(mss, defaultSortColumns, 0.0, VbPlain,
            False));
    std::unique_ptr<ViFactory> factory(
        new FiltrationTVIFactory(mode, inputVii.get()));

    Product p;

    // vi will be responsible for releasing inputVii so unique_ptr
    // should release the ownership here
    p.vii = inputVii.release();
    p.factory = factory.release();

    return p;

  }

  static String GetTestPurpose() {
    return "Test FiltrationTVIFactory(Record const &, ViImplementation2 *)";
  }
};

class LayerManufacturer: public Manufacturer<LayerManufacturer> {
public:
  class LayerFactoryWrapper: public ViFactory {
  public:
    LayerFactoryWrapper(MeasurementSet *ms, Record const &mode) :
        ms_(ms), mode_(mode) {
    }

    ViImplementation2 *createVi() const {
      Vector<ViiLayerFactory *> v(1);
      auto layer0 = VisIterImpl2LayerFactory(ms_, IteratingParameters(0.0),
          false);
      auto layer1 = FiltrationTVILayerFactory(mode_);
      v[0] = &layer0;
      return layer1.createViImpl2(v);
    }
  private:
    MeasurementSet *ms_;
    Record const mode_;
  };

  static Product BuildFactory(MeasurementSet *ms, Record const &mode) {
    // create read-only VI impl
    std::unique_ptr<ViFactory> factory(new LayerFactoryWrapper(ms, mode));

    Product p;
    p.vii = nullptr;
    p.factory = factory.release();
    return p;
  }

  static String GetTestPurpose() {
    return "Test FiltrationTVILayerFactory";
  }
};

} // anonymous namespace

// explicitly instantiate test TVI
namespace casa {
namespace vi {
template class FiltrationTVI<PorousFilter> ;
template class FiltrationTVI<NonporousFilter> ;
}
}

class FiltrationTVITestBase: public ::testing::Test {
public:
  FiltrationTVITestBase() :
      my_ms_name_("filtration_test.ms"), my_data_name_(), ms_(nullptr) {
  }

  virtual void SetUp() {
//    my_data_name_ = "analytic_spectra.ms";
    my_data_name_ = GetDataName(); //"analytic_type1.bl.ms";
    std::string const data_path = ::GetCasaDataPath() + "/regression/unittest/"
        + GetRelativeDataPath() + "/";
//        + "/regression/unittest/tsdbaseline/";
//    + "/regression/unittest/singledish/";

    ASSERT_TRUE(Directory(data_path).exists());
    cout << "data_path = " << data_path << endl;
    copyDataFromRepository(data_path);
    ASSERT_TRUE(File(my_data_name_).exists());
    deleteTable(my_ms_name_);

    // create MS
    ms_ = new MeasurementSet(my_data_name_, Table::Update);
  }

  virtual void TearDown() {
    // delete MS explicitly to detach from MS on disk
    delete ms_;

    // just to make sure all locks are effectively released
    Table::relinquishAutoLocks();

    cleanup();
  }

protected:
  std::string const my_ms_name_;
  std::string my_data_name_;
  MeasurementSet *ms_;

  virtual std::string GetDataName() {
    return "";
  }

  virtual std::string GetRelativeDataPath() {
    return "";
  }

  template<class Validator, class Manufacturer = TestManufacturer>
  void TestTVI() {
    cout << "TestTVI" << endl;

    // Create VI
    // VI with filter
    std::unique_ptr<VisibilityIterator2> vi(
        Manufacturer::ManufactureVI(ms_, Validator::GetMode()));
    ASSERT_TRUE(vi->ViiType().startsWith(Validator::GetTypePrefix()));

    // reference VI
    std::unique_ptr<VisibilityIterator2> refvi(new VisibilityIterator2(*ms_));

    // MS property
    auto ms = vi->ms();
    auto refms = refvi->ms();
    uInt const nRowMs = ms.nrow();
    uInt const nRowRefMs = refms.nrow();
    EXPECT_EQ(nRowMs, nRowRefMs);
////    uInt const nRowPolarizationTable = ms.polarization().nrow();
    auto const desc = ms.tableDesc();
    auto const corrected_exists = desc.isColumn("CORRECTED_DATA");
    auto const model_exists = desc.isColumn("MODEL_DATA");
    auto const data_exists = desc.isColumn("DATA");
    auto const float_exists = desc.isColumn("FLOAT_DATA");
    //auto const weightSpExists = desc.isColumn("WEIGHT_SPECTRUM");
    cout << "MS Property" << endl;
    cout << "\tMS Name: \"" << ms.tableName() << "\"" << endl;
    cout << "\tNumber of Rows: " << nRowMs << endl;
    cout << "\tNumber of Spws: " << vi->nSpectralWindows() << endl;
    cout << "\tNumber of Polarizations: " << vi->nPolarizationIds() << endl;
    cout << "\tNumber of DataDescs: " << vi->nDataDescriptionIds() << endl;
    cout << "\tChannelized Weight Exists? "
        << (vi->weightSpectrumExists() ? "True" : "False") << endl;
    //cout << "\tChannelized Sigma Exists? " << (vi->sigmaSpectrumExists() ? "True" : "False") << endl;

//    // VI iteration
//    Vector<uInt> swept(nRowMs, 0);
//    uInt nRowChunkSum = 0;
    VisBuffer2 *vb = vi->getVisBuffer();
    VisBuffer2 *vb_ref = refvi->getVisBuffer();
    vi->originChunks();
    refvi->originChunks();
    // iteration loop is based on refvi
    while (refvi->moreChunks()) {
      // make sure there is a chunk in vi
      EXPECT_TRUE(vi->moreChunks());

      // initialize subchunk iterator
      refvi->origin();
      vi->origin();

      // increment chunk until refvi iteration hits filtrate subchunk
      while (refvi->more() && Validator::IsResidue(vb_ref)) {
        refvi->next();
      }

      // nRowsInChunk returns number of rows in chunk regardless of
      // whether they are filtered out or not
      Int const nrow_chunk = vi->nRowsInChunk();
      Int const nrow_chunk_ref = refvi->nRowsInChunk();
      EXPECT_EQ(nrow_chunk_ref, nrow_chunk);

      // chunk id should be the same
      auto const chunk_id = vi->getSubchunkId().chunk();
      auto const chunk_id_ref = refvi->getSubchunkId().chunk();
      EXPECT_EQ(chunk_id_ref, chunk_id);
//      nRowChunkSum += nRowChunk;
      cout << "*************************" << endl;
      cout << "*** Start loop on chunk " << chunk_id << endl;
      cout << "*** Number of Rows: " << nrow_chunk << endl;
      cout << "*************************" << endl;
//
//      Int nRowSubchunkSum = 0;
//
      // no valid subchunk exists
      if (!refvi->more()) {
        cout << "No valid chunk exists." << endl;
        EXPECT_FALSE(vi->more());

        refvi->nextChunk();
        vi->nextChunk();
        continue;
      }

      // again, iteration loop is based on refvi
      while (refvi->more()) {
        EXPECT_TRUE(vi->more());

        auto const subchunk = vi->getSubchunkId();
        auto const subchunk_ref = refvi->getSubchunkId();
        auto const subchunk_id = subchunk.subchunk();
        auto const subchunk_id_ref = subchunk_ref.subchunk();
        cout << "=== Start loop on subchunk " << subchunk_id_ref << " ==="
            << endl;

        EXPECT_EQ(subchunk_id_ref, subchunk_id);

        Vector<uInt> row_ids;
        Vector<uInt> row_ids_ref;
        vi->getImpl()->getRowIds(row_ids);
        refvi->getImpl()->getRowIds(row_ids_ref);
        Vector<bool> is_filtrate;
        auto const num_filtrates = Validator::IsFiltratePerRow(vb_ref, is_filtrate);
        ASSERT_GE(num_filtrates, 0);
        ASSERT_EQ(refvi->getImpl()->nRows(), num_filtrates);
        ASSERT_EQ((unsigned long)num_filtrates, is_filtrate.nelements());

        // ROW IDs
        cout << "Examining ROW ID";
        cout.flush();
        EXPECT_EQ(num_filtrates, vi->getImpl()->nRows());
        int j = 0;
        for (int i = 0; i < vb_ref->nRows(); ++i) {
          if (is_filtrate[i]) {
            EXPECT_EQ(row_ids_ref[i], row_ids[j]);
            ++j;
          }
        }
        cout << "...OK" << endl;

        // DATA columns
        if (corrected_exists) {
          cout << "Examining CORRECTED_DATA";
          cout.flush();
          Cube<Complex> data = vb->visCubeCorrected();
          Cube<Complex> data_ref = vb_ref->visCubeCorrected();
          Validator::ValidateCube(data, data_ref, is_filtrate);
          cout << "...OK" << endl;
        }

        if (model_exists) {
          cout << "Examining MODEL_DATA";
          cout.flush();
          Cube<Complex> data = vb->visCubeModel();
          Cube<Complex> data_ref = vb_ref->visCubeModel();
          Validator::ValidateCube(data, data_ref, is_filtrate);
          cout << "...OK" << endl;
        }

        if (data_exists) {
          cout << "Examining DATA";
          cout.flush();
          Cube<Complex> data = vb->visCube();
          Cube<Complex> data_ref = vb_ref->visCube();
          Validator::ValidateCube(data, data_ref, is_filtrate);
          cout << "...OK" << endl;
        }

        if (float_exists) {
          cout << "Examining FLOAT_DATA";
          cout.flush();
          Cube<Float> data = vb->visCubeFloat();
          Cube<Float> data_ref = vb_ref->visCubeFloat();
          Validator::ValidateCube(data, data_ref, is_filtrate);
          cout << "...OK" << endl;
        }

        // next round of iteration
        refvi->next();
        vi->next();
      }

      refvi->nextChunk();
      vi->nextChunk();
    }

    // make sure there is no chunk remaining
    EXPECT_FALSE(vi->moreChunks());
  }

  template<class Manufacturer>
  void TestFactory(Int const &type_enum, String const &expectedClassName) {

    cout << "Type \"" << type_enum << "\" expected class name \""
        << expectedClassName << "\"" << endl;

    if (expectedClassName.size() > 0) {
      std::unique_ptr<VisibilityIterator2> vi(Manufacturer::ManufactureVI(ms_, type_enum));

      // Verify type string
      String viiType = vi->ViiType();
      EXPECT_TRUE(viiType.startsWith(expectedClassName));
    } else {
      cout << "Creation of VI via factory will fail" << endl;
      // exception must be thrown
      EXPECT_THROW( {
            std::unique_ptr<VisibilityIterator2> vi(Manufacturer::ManufactureVI(ms_, type_enum)); //new VisibilityIterator2(factory));
          },
          AipsError)<< "The process must throw AipsError";
    }
  }

private:
  void copyRegular(String const &src, String const &dst) {
    RegularFile r(src);
    r.copy(dst);
  }
  void copySymLink(String const &src, String const &dst) {
    Path p = SymLink(src).followSymLink();
    String actual_src = p.absoluteName();
    File f(actual_src);
    if (f.isRegular()) {
      copyRegular(actual_src, dst);
    } else if (f.isDirectory()) {
      copyDirectory(actual_src, dst);
    }
  }
  void copyDirectory(String const &src, String const &dst) {
    Directory dsrc(src);
    Directory ddst(dst);
    ddst.create();
    DirectoryIterator iter(dsrc);
    while (!iter.pastEnd()) {
      String name = iter.name();
      if (name.contains(".svn")) {
        iter++;
        continue;
      }
      File f = iter.file();
      Path psrc(src);
      Path pdst(dst);
      psrc.append(name);
      String sub_src = psrc.absoluteName();
      pdst.append(name);
      String sub_dst = pdst.absoluteName();
      if (f.isSymLink()) {
        copySymLink(sub_src, sub_dst);
      } else if (f.isRegular()) {
        copyRegular(sub_src, sub_dst);
      } else if (f.isDirectory()) {
        copyDirectory(sub_src, sub_dst);
      }
      iter++;
    }
  }
  void copyDataFromRepository(std::string const &data_dir) {
    if (my_data_name_.size() > 0) {
      std::string full_path = data_dir + my_data_name_;
      std::string work_path = my_data_name_;
      File f(full_path);
      ASSERT_TRUE(f.exists());
      if (f.isSymLink()) {
        copySymLink(full_path, work_path);
      } else if (f.isRegular()) {
        copyRegular(full_path, work_path);
      } else if (f.isDirectory()) {
        copyDirectory(full_path, work_path);
      }
    }
  }
  void cleanup() {
    if (my_data_name_.size() > 0) {
      deleteTable(my_data_name_);
    }
    deleteTable(my_ms_name_);
  }
  void deleteTable(std::string const &name) {
    File file(name);
    if (file.exists()) {
      std::cout << "Removing " << name << std::endl;
      Table::deleteTable(name, true);
    }
  }
};

class FiltrationTVITest: public FiltrationTVITestBase {
protected:
  virtual std::string GetDataName() {
    return "analytic_type1.bl.ms";
  }

  virtual std::string GetRelativeDataPath() {
    return "tsdbaseline";
  }
};

TEST_F(FiltrationTVITest, PorousTest) {
  TestTVI<PorousValidator, TestManufacturer>();
}

TEST_F(FiltrationTVITest, NonporousTest) {
  TestTVI<NonporousValidator, TestManufacturer>();
}

TEST_F(FiltrationTVITest, FactoryTest) {
  Int const type_enum = (Int)FilteringType::SDDoubleCircleFilter;
  String const expected_name("FiltrationTVI<SDDoubleCircle>");
  TestFactory<BasicManufacturer>(type_enum, expected_name);
  TestFactory<LayerManufacturer>(type_enum, expected_name);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  std::cout << "FiltrationTVI test " << std::endl;
  return RUN_ALL_TESTS();
}
