/****************************************************************************
                            Hyper's CD Catalog
	A multiplatform qt and xml based catalog program

Author    : Christoph Thielecke <crissi99@gmx.de>
License   : GPL
Copyright : (C) 2003 Christoph Thielecke
****************************************************************************/


#include "importdialog.h"

#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qtooltip.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <QGridLayout>
#include <stdio.h>
#include <stdlib.h>
#include <qvariant.h>


ImportDialog::ImportDialog ( QWidget *parent, const char *name, bool modal, Qt::WFlags fl )
	: QDialog ( parent, fl ) {
	if ( !name )
		setObjectName ( "ImportDialog" );
	setModal ( modal );
	//	ImportDialogLayout = new QGridLayout( this, 1, 1, 11, 7, "ImportDialogLayout" );

	setSizeGripEnabled ( true );

	layout4 = new QGridLayout ( this );

	filename_lab = new QLabel ( this );
	layout4->addWidget ( filename_lab, 2, 0, 1,1 );

	filename_lineedit = new QLineEdit ( this );
	layout4->addWidget ( filename_lineedit, 2, 1, 1, 4 );

	buttonGetFile = new QPushButton ( this );
	layout4->addWidget ( buttonGetFile, 2, 5, 1, 2 );

	info_lab = new QLabel ( this );
	info_lab->setText ( "info" );
	layout4->addWidget ( info_lab, 0, 0, 1, 5 );

	importButtonBox = new QGroupBox ( tr ( "Type" ), this );
	layoutGroupBox = new QVBoxLayout ( this );
	importButtonBox->setLayout ( layoutGroupBox );
	importTypeCsvGtktalog = new QRadioButton ( "&Gtktalog CSV", importButtonBox);
	importTypeCsvKatCeDe = new QRadioButton ( "&Kat-DeCe CSV", importButtonBox );
	importTypeCsvDisclib = new QRadioButton ( "&Disclib CSV", importButtonBox );
	importTypeCsvVisualcd = new QRadioButton ( "&VisualCD CSV", importButtonBox );
	importTypeCsvVvv = new QRadioButton ( "&VVV CSV", importButtonBox );
	importTypeCsvAdvancedFileOrganizer = new QRadioButton ( "&Advanced file organizer CSV", importButtonBox );
	importTypeCsvFileArchivist = new QRadioButton ( "&File Archivist", importButtonBox );
	importTypeCsvAdvancedDiskCatalog = new QRadioButton ( "&Advanced Disk Catalog CSV", importButtonBox );
	importTypeCsvWhereisit = new QRadioButton ( "&Advanced Disk Catalog CSV", importButtonBox );
	importTypeGtktalogXml = new QRadioButton ( "Gtktalog &XML", importButtonBox );
	importTypeWhereisitXml = new QRadioButton ( "&WhereIsIt XML (classic)", importButtonBox );

	layoutGroupBox->addWidget ( importTypeCsvGtktalog );
	layoutGroupBox->addWidget ( importTypeCsvKatCeDe );
	layoutGroupBox->addWidget ( importTypeCsvDisclib );
	layoutGroupBox->addWidget ( importTypeCsvVisualcd );
	layoutGroupBox->addWidget ( importTypeCsvVvv );
	layoutGroupBox->addWidget ( importTypeCsvAdvancedFileOrganizer );
	layoutGroupBox->addWidget ( importTypeCsvFileArchivist );
	layoutGroupBox->addWidget ( importTypeCsvAdvancedDiskCatalog );
	layoutGroupBox->addWidget ( importTypeCsvWhereisit );
	layoutGroupBox->addWidget ( importTypeGtktalogXml );
	layoutGroupBox->addWidget ( importTypeWhereisitXml );

	layout4->addWidget ( importButtonBox, 3, 0, 1, 4 );

	newdatabase = new QCheckBox ( this );
	newdatabase->setText ( tr ( "Create new Database" ) );
	layout4->addWidget ( newdatabase, 4, 0, 1, 2 );




	correctbadstyle = new QCheckBox ( this );
	correctbadstyle->setText ( tr ( "Correct bad style from gtktalog export" ) );
	layout4->addWidget ( correctbadstyle, 4, 2, 1, 2 );


	separator_lab = new QLabel ( this );
	layout4->addWidget ( separator_lab, 5, 0, 1, 2 );
	separator_lineedit = new QLineEdit ( this );
	separator_lineedit->setMinimumSize ( QSize ( 0, 0 ) );
	separator_lineedit->setMaximumSize ( QSize ( 20, 32767 ) );
	separator_lineedit->setMaxLength ( 1 );
	layout4->addWidget ( separator_lineedit, 5, 2, 1, 4 );

	buttonOK = new QPushButton ( this );
	buttonOK->setDefault ( true );
	buttonOK-> setMinimumWidth ( 100 );
	layout4->addWidget ( buttonOK, 7, 1, 1,1 );

	buttonCancel = new QPushButton ( this );
	buttonCancel-> setMinimumWidth ( 100 );
	layout4->addWidget ( buttonCancel, 7, 3, 1, 1 );
	/*
		QSpacerItem* spacer = new QSpacerItem( 181, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
		layout4->addMultiCell( spacer, 5, 5, 0, 2 );

		QSpacerItem* spacer_2 = new QSpacerItem( 291, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
		layout4->addMultiCell( spacer_2, 3, 3, 2, 5 );
	*/
	importTypeWhereisitXml->setChecked ( true );
	correctbadstyle->setEnabled ( false );
	separator_lab->setEnabled ( false );
	separator_lineedit->setEnabled ( false );


	//	ImportDialogLayout->addLayout( layout4, 0, 0 );

	languageChange();
	resize ( QSize ( 500, 350 ).expandedTo ( minimumSizeHint() ) );

	setMinimumSize ( minimumSizeHint() );

	//this->sizeHint();
	//setFixedSize( size() );

	//clearWState( WState_Polished );

	connect ( buttonOK, SIGNAL ( clicked() ), this, SLOT ( bOk() ) );
	connect ( buttonCancel, SIGNAL ( clicked() ), this, SLOT ( bCan() ) );
	connect ( buttonGetFile, SIGNAL ( clicked() ), this, SLOT ( getFileName() ) );
	connect ( importTypeCsvGtktalog, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvKatCeDe, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvDisclib, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvVisualcd, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvVvv, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvAdvancedFileOrganizer, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvFileArchivist, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvAdvancedDiskCatalog, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeCsvWhereisit, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeGtktalogXml, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	connect ( importTypeWhereisitXml, SIGNAL ( clicked() ), this, SLOT ( typeChanged() ) );
	separator_lab->setEnabled ( false );
	separator_lineedit->setEnabled ( false );



	// tmp
	/*
	importTypeCsv->setChecked(true);
	separator_lab->setEnabled(true);
	separator_lineedit->setEnabled(true);
	//filename_lineedit->setText("c:\\devel\\mp3s.csv");
	filename_lineedit->setText("/data3/res3/musikvideo.csv");
	separator_lineedit->setText("*");
	correctbadstyle->setEnabled(true);
	correctbadstyle->setChecked(true);
	*/
	// tmp
	//filename_lineedit->setText("/home/crissi/compile/cvs/CdCat-0.98pre_whereisit_xml_import/sample-export.xml");
	filename_lineedit->setText ( "" );

}

/*
 *  Destroys the object and frees any allocated resources
 */
ImportDialog::~ImportDialog() {
	//no need to delete child widgets, Qt does it for us!
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ImportDialog::languageChange() {
	setWindowTitle ( tr ( "Import CSV file" ) );
	filename_lab->setText ( tr ( "File:" ) );

	separator_lineedit->setText ( ";" );
	separator_lab->setText ( tr ( "Separator:" ) );
	separator_lineedit->setToolTip ( tr ( "This is the separator in dataline\n<path>SEPARATOR<size>SEPARATOR<date><space><time>" ) );
	buttonCancel->setText ( tr ( "Cancel" ) );
	buttonOK->setText ( tr ( "Import" ) );
	buttonGetFile->setText ( tr ( "..." ) );

	importButtonBox->setTitle ( tr ( "Type" ) );
	importTypeCsvGtktalog->setText ( tr ( "&Gtktalog (csv)" ) );
	importTypeCsvKatCeDe->setText ( tr ( "&Kat-CeDe (csv)" ) );
	importTypeCsvDisclib->setText ( tr ( "&Disclib (csv)" ) );
	importTypeCsvVisualcd->setText ( tr ( "&VisualCD (csv)" ) );
	importTypeCsvVvv->setText ( tr ( "&VVV (csv)" ) );
	importTypeCsvAdvancedFileOrganizer->setText ( tr ( "&Advanced File Organizer (csv)" ) );
	importTypeCsvFileArchivist->setText ( tr ( "&File Archivist" ) );
	importTypeCsvAdvancedDiskCatalog->setText ( tr ( "&Advanced Disk Catalog (csv)" ) );
	importTypeCsvWhereisit->setText ( tr ( "W&hereIsIt (csv)" ) );
	importTypeGtktalogXml->setText ( tr ( "Gtktalog &XML" ) );
	importTypeWhereisitXml->setText ( tr ( "&WhereIsIt XML (classic)" ) );

	importButtonBox->setToolTip ( tr ( "Select the type of import here" ) );

	importTypeCsvGtktalog->setToolTip ( tr ( "Select this for importing a text import (csv) generated from Gtktalog" ) );

	importTypeCsvKatCeDe->setToolTip ( tr ( "Select this for importing a text import (csv) generated from Kat-CeDe." ) );

	importTypeCsvDisclib->setToolTip ( tr ( "Select this for importing a text import (csv) generated from Disclib." ) );

	importTypeCsvVisualcd->setToolTip ( tr ( "Select this for importing a text import (csv) generated from VisualCD." ) );

	importTypeCsvVvv->setToolTip ( tr ( "Select this for importing a text import (csv) generated from VVV." ) );

	importTypeCsvAdvancedFileOrganizer->setToolTip ( tr ( "Select this for importing a text import (csv) generated from Advanced File Organizer." ) );

	importTypeCsvFileArchivist->setToolTip ( tr ( "Select this for importing a File Archivist catalog." ) );

	importTypeCsvAdvancedDiskCatalog->setToolTip ( tr ( "Select this for importing a text import (csv) generated from Advanced Disk Catalog." ) );

	importTypeCsvWhereisit->setToolTip ( tr ( "Select this for importing a text import (csv) generated from WhereIsIt." ) );

	importTypeGtktalogXml->setToolTip ( tr ( "Select this for importing a xml report generated from gtktalog" ) );

	importTypeWhereisitXml->setToolTip ( tr ( "Select this for importing a xml report generated from WhereIsIt?" ) );

	buttonGetFile->setToolTip ( tr ( "Open the file dialog for selecting file to import." ) );

	correctbadstyle->setToolTip ( tr ( "Corrects bad output style from gtktalog.\n<media>SEPARATOR/<dir>/SEPARATOR/<dir>\n will be to\n<media>/<dir>/<dir>" ) );
	info_lab->setText ( tr ( "<strong>Please read the README_IMPORT before you import!</strong>" ) );



}

int ImportDialog::bOk ( void ) {
	if ( ( filename_lineedit->text() ).isEmpty() ) {
		QMessageBox::warning ( this, tr ( "Error:" ), tr ( "You must be fill the \"Filename\" field!" ) );
		return 0;
	}

	if ( importTypeCsvGtktalog->isChecked() && separator_lineedit->text().isEmpty() ) {
		QMessageBox::warning ( this, tr ( "Error:" ), tr ( "You must be fill the \"Separator\" field!" ) );
		return 0;
	}

	filename = filename_lineedit->text();
	separator = separator_lineedit->text();
	createdatabase = newdatabase->isChecked();
	if ( importTypeCsvGtktalog->isChecked() )
		type = 0;
	else
		if ( importTypeCsvKatCeDe->isChecked() )
			type = 3;
		else
			if ( importTypeCsvDisclib->isChecked() )
				type = 4;
			else
				if ( importTypeCsvVisualcd->isChecked() )
					type = 5;
				else
					if ( importTypeCsvVvv->isChecked() )
						type = 6;
					else
						if ( importTypeCsvAdvancedFileOrganizer->isChecked() )
							type = 7;
						else
							if ( importTypeCsvFileArchivist->isChecked() )
								type = 8;
							else
								if ( importTypeCsvAdvancedDiskCatalog->isChecked() )
									type = 9;
								else
									if ( importTypeCsvWhereisit->isChecked() )
										type = 10;
									else
										if ( importTypeGtktalogXml->isChecked() )
											type = 1;
										else
											if ( importTypeWhereisitXml->isChecked() )
												type = 2;
											else
												if ( importTypeWhereisitXml->isChecked() )
													type = 2;

	OK = 1;

	close();
	return 0;
}

int ImportDialog::bCan ( void ) {
	OK = 0;
	close();
	return 0;
}

void ImportDialog::getFileName() {
	QString filetypes = "";

	if ( importTypeCsvGtktalog->isChecked() || importTypeCsvKatCeDe->isChecked() || importTypeCsvDisclib->isChecked()
	                || importTypeCsvVisualcd->isChecked() ||  importTypeCsvVvv->isChecked()  ||  importTypeCsvAdvancedFileOrganizer->isChecked()
	                || importTypeCsvAdvancedDiskCatalog->isChecked() || importTypeCsvWhereisit->isChecked() )
		filetypes = QString ( tr ( "csv files(*.csv)" ) );
	else
		if ( importTypeGtktalogXml->isChecked() )
			filetypes = QString ( tr ( "xml files(*.xml)" ) );
		else
			if ( importTypeWhereisitXml->isChecked() )
				filetypes = QString ( tr ( "xml files(*.xml)" ) );
			else
				if ( importTypeCsvFileArchivist->isChecked() )
					filetypes = QString ( tr ( "File Archivist files(*.arch)" ) );
				else
					filetypes = QString ( tr ( "all files(*.*)" ) );

	if ( lastDir.isEmpty() ) {
		QString homedir;
#ifndef _WIN32
		homedir = getenv ( "HOME" );
#else
		homedir = getenv ( "USER_PROFILE" );
#endif
		lastDir = homedir;
	}

	filename_lineedit->setText ( QFileDialog::getOpenFileName ( this, tr ( "Choose a file for import" ), lastDir, filetypes ) );
	filename = filename_lineedit->text();
}

void ImportDialog::typeChanged() {
	if ( importTypeCsvGtktalog->isChecked() || importTypeCsvKatCeDe->isChecked() || importTypeCsvKatCeDe->isChecked() ) {
		if ( importTypeCsvGtktalog->isChecked() || importTypeCsvDisclib->isChecked() ) {
			correctbadstyle->setEnabled ( true );
			separator_lab->setEnabled ( true );
			separator_lineedit->setEnabled ( true );
		} else {
			correctbadstyle->setEnabled ( false );
			separator_lab->setEnabled ( false );
			separator_lineedit->setEnabled ( false );
		}
		if ( importTypeCsvWhereisit->isChecked() ) {
			separator_lab->setEnabled ( true );
			separator_lineedit->setEnabled ( true );
		}
		if ( importTypeCsvGtktalog->isChecked() || importTypeCsvWhereisit->isChecked() ) {
			separator_lineedit->setText ( ";" );
		} else
			if ( importTypeCsvDisclib->isChecked() ) {
				separator_lineedit->setText ( "*" );
			} else
				if ( importTypeCsvAdvancedFileOrganizer->isChecked() ) {
					separator_lineedit->setText ( "," );
				} else
					if ( importTypeCsvFileArchivist->isChecked() ) {
						separator_lineedit->setText ( "|" );
					} else
						if ( importTypeCsvFileArchivist->isChecked() ) {
							separator_lineedit->setText ( ", " );
						}

	} else {
		correctbadstyle->setEnabled ( false );
		separator_lab->setEnabled ( false );
		separator_lineedit->setEnabled ( false );
	}
	if ( importTypeCsvVisualcd->isChecked() ) {
		correctbadstyle->setEnabled ( false );
		correctbadstyle->setChecked ( false );
		separator_lineedit->setText ( ";" );
	}

}

QString ImportDialog::getLastDir() {
	return lastDir;
}

void ImportDialog::setLastDir ( QString lastDir ) {
	this->lastDir = lastDir;
}


