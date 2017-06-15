#include <iostream>
#include <casaqt/QwtPlotter/QPHeaderTableWidget.qo.h>
#include <QMessageBox>

//#include <QDebug>

//using namespace casacore;
namespace casa {

// Constructor & Destructor
QPHeaderTable::QPHeaderTable(QWidget* parent)
         : QTableView(parent) {

    // Identity properties
	setObjectName("HeaderTable");

    // Display properties
    // -------- Table frame
    setFrameStyle(QFrame::NoFrame);
    // -------- Table headers
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->hide();
    horizontalHeader()->hide();
    // -------- Table grid
    setShowGrid(false);

    // Events handling properties
    //setFocusPolicy(Qt::NoFocus);
    //setFocusPolicy(Qt::FocusPolicy);
    setSelectionMode(QAbstractItemView::NoSelection);

    // Connections
    // Debug
    connect(horizontalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(debugInfo(int)));
}

void QPHeaderTable::resizeEvent(QResizeEvent *e){
	// Widget already has its new geometry
	const int spacerColumnWidth = 50;
	if ( width() > spacerColumnWidth && horizontalHeader()->count() == 3 ) {
		int logicalColumnsWidth = (width() - spacerColumnWidth)/2;
		setColumnWidth(1,spacerColumnWidth);
		setColumnWidth(0,logicalColumnsWidth);
		setColumnWidth(2,logicalColumnsWidth);
	}
	QTableView::resizeEvent(e);
	/*
	QTableView::resizeEvent(e);
	setColumnWidth(1,50);
	*/
}


QPHeaderTable::~QPHeaderTable() { }

} // namespace casa



