/****************************************************************************
                             Hyper's CD Catalog
		A multiplatform qt and xml based catalog program

 Author    : Peter Deak (hyperr@freemail.hu)
 License   : GPL
 Copyright : (C) 2003 Peter Deak
****************************************************************************/

#include "find.h"

#include <qvariant.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <q3frame.h>
#include <q3header.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <q3listview.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qmessagebox.h>
#include <qtextcodec.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3VBoxLayout>
#include <QCloseEvent>

// #include <pcre.h>
#include <QRegExp>
#include <string.h>
#include <ctype.h>

#include "dbase.h"
#include "mainwidget.h"
#include "wdbfile.h"
#include "guibase.h"
#include "adddialog.h"
#include "config.h"
#include "cdcat.h"
#include "icons.h"

findDialog::findDialog ( CdCatMainWidget* parent, const char* name, bool modal, Qt::WFlags fl )
        : QDialog ( parent, name, modal, fl )

{
    if ( !name )
        setName ( "findDialog" );
    setIcon ( *get_t_find_icon() );

    mainw = parent;

    setSizeGripEnabled ( TRUE );
    FindDialogBaseLayout = new Q3GridLayout ( this, 1, 1, 11, 6, "FindDialogBaseLayout" );

//    QSpacerItem* spacer = new QSpacerItem( 210, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_2 = new QSpacerItem ( 240, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_3 = new QSpacerItem ( 240, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_4 = new QSpacerItem ( 200, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_5 = new QSpacerItem ( 36, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_6 = new QSpacerItem ( 190, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_7 = new QSpacerItem ( 200, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_8 = new QSpacerItem ( 190, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_9 = new QSpacerItem ( 190, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer_10 = new QSpacerItem ( 200, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    layout40 = new Q3VBoxLayout ( 0, 0, 6, "layout40" );
    layout39 = new Q3GridLayout ( 0, 1, 1, 0, 6, "layout39" );
    layout36 = new Q3GridLayout ( 0, 1, 1, 0, 6, "layout36" );
    layout37 = new Q3GridLayout ( 0, 1, 1, 0, 6, "layout37" );
    layout31 = new Q3VBoxLayout ( 0, 0, 6, "layout31" );
    layout17 = new Q3HBoxLayout ( 0, 0, 6, "layout17" );
    layout15 = new Q3GridLayout ( 0, 1, 1, 0, 6, "layout15" );
    layout30 = new Q3HBoxLayout ( 0, 0, 6, "layout30" );
    layout16 = new Q3GridLayout ( 0, 1, 1, 0, 6, "layout16" );

    leText = new QLineEdit ( this, "leText" );

    cbCasesens = new QCheckBox ( this, "cbCasesens" );
    cbEasy = new QCheckBox ( this, "cbEasy" );

    cbFilename = new QCheckBox ( this, "cbFilename" );
    cbDirname  = new QCheckBox ( this, "cbDirname" );
    cbComment  = new QCheckBox ( this, "cbComment" );
    cbContent  = new QCheckBox ( this, "cbContent" );

    cbArtist = new QCheckBox ( this, "cbArtist" );
    cbTitle = new QCheckBox ( this, "cbTitle" );
    cbAlbum = new QCheckBox ( this, "cbAlbum" );
    cbTcomm = new QCheckBox ( this, "cbTcomm" );

    cbOwner = new QComboBox ( FALSE, this, "cbOwner" );
    cbOwner->setMinimumSize ( QSize ( 0, 0 ) );

    cbSin = new QComboBox ( FALSE, this, "cbSin" );

    buttonOk = new QPushButton ( this, "buttonOk" );
    buttonOk->setAutoDefault ( TRUE );
    buttonOk->setDefault ( TRUE );
    buttonOk->setMinimumWidth ( 80 );

    buttonCancel = new QPushButton ( this, "buttonCancel" );
    buttonCancel->setAutoDefault ( TRUE );
    buttonCancel->setMinimumWidth ( 80 );

    resultsl = new Q3ListView ( this, "resultsl" );
    resultsl->addColumn ( tr ( "Name" ) );
    resultsl->addColumn ( tr ( "Type" ) );
    resultsl->addColumn ( tr ( "Media" ) );
    resultsl->addColumn ( tr ( "Path" ) );
    resultsl->addColumn ( tr ( "Modification" ) );

    buttonClose = new QPushButton ( this, "buttonClose" );

    textLabel3 = new QLabel ( this, "textLabel3" );
    textLabel1 = new QLabel ( this, "textLabel1" );
    textLabel2 = new QLabel ( this, "textLabel2" );
    textLabel5 = new QLabel ( this, "textLabel5" );

    /* saved ops: */

    cbTcomm    -> setChecked ( mainw->cconfig->find_mco );
    cbCasesens -> setChecked ( mainw->cconfig->find_cs );
    cbArtist   -> setChecked ( mainw->cconfig->find_mar );
    cbAlbum    -> setChecked ( mainw->cconfig->find_mal );
    cbComment  -> setChecked ( mainw->cconfig->find_co );
    cbTitle    -> setChecked ( mainw->cconfig->find_mti );
    cbEasy     -> setChecked ( mainw->cconfig->find_em );
    cbFilename -> setChecked ( mainw->cconfig->find_fi );
    cbDirname  -> setChecked ( mainw->cconfig->find_di );
    cbContent  -> setChecked ( mainw->cconfig->find_ct );

    /* layouts:   */
    layout36->addWidget ( cbCasesens, 1, 0 );
    layout36->addWidget ( cbDirname , 2, 0 );
    layout36->addWidget ( cbFilename, 3, 0 );
    layout36->addWidget ( cbComment , 4, 0 );
    layout36->addWidget ( cbContent , 5, 0 );
    layout36->addWidget ( cbEasy  , 1, 1 );
    layout36->addWidget ( cbArtist, 2, 1 );
    layout36->addWidget ( cbTitle , 3, 1 );
    layout36->addWidget ( cbAlbum , 4, 1 );
    layout36->addWidget ( cbTcomm , 5, 1 );
    layout36->addMultiCellWidget ( leText, 0, 0, 0, 1 );

    layout37->addWidget ( cbOwner, 3, 1 );
    layout37->addWidget ( cbSin, 2, 1 );
    layout37->addMultiCellWidget ( textLabel3, 0, 0, 0, 1 );
    layout37->addWidget ( textLabel1, 2, 0 );
    layout37->addWidget ( textLabel2, 3, 0 );
    layout37->addMultiCell ( spacer_3, 1, 1, 0, 1 );

    layout39->addMultiCellLayout ( layout36, 0, 1, 1, 1 );
    layout39->addItem ( spacer_2, 1, 0 );
    layout39->addLayout ( layout37, 0, 0 );

    layout15->addWidget ( buttonOk, 0, 0 );
    layout15->addItem ( spacer_5, 0, 1 );
    layout15->addWidget ( buttonCancel, 0, 2 );

    layout17->addItem ( spacer_6 );
    layout17->addLayout ( layout15 );
    layout17->addItem ( spacer_4 );

    layout30->addItem ( spacer_7 );
    layout30->addWidget ( textLabel5 );
    layout30->addItem ( spacer_8 );

    layout16->addItem ( spacer_9, 0, 2 );
    layout16->addWidget ( buttonClose, 0, 1 );
    layout16->addItem ( spacer_10, 0, 0 );

    layout31->addLayout ( layout17 );
    layout31->addLayout ( layout30 );
    layout31->addWidget ( resultsl );
    layout31->addLayout ( layout16 );

    layout40->addLayout ( layout39 );
    layout40->addLayout ( layout31 );

    FindDialogBaseLayout->addLayout ( layout40, 0, 0 );

    resize ( QSize ( mainw->cconfig->findWidth,mainw->cconfig->findHeight ).expandedTo ( minimumSizeHint() ) );
    move ( mainw->cconfig->findX,mainw->cconfig->findY );

    languageChange();

    fillCBox();
    connect ( buttonCancel,SIGNAL ( clicked() ),this,SLOT ( cancele() ) );
    connect ( buttonOk,SIGNAL ( clicked() ),this,SLOT ( seeke() ) );
    connect ( buttonClose,SIGNAL ( clicked() ),this,SLOT ( closee() ) );
    connect ( resultsl,SIGNAL ( currentChanged ( Q3ListViewItem * ) ),this,SLOT ( select ( Q3ListViewItem * ) ) );
    connect ( resultsl,SIGNAL ( clicked ( Q3ListViewItem * ) ),this,SLOT ( select ( Q3ListViewItem * ) ) );

    setTabOrder ( leText,buttonOk );
    leText->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
findDialog::~findDialog() {
    // no need to delete child widgets, Qt does it all for us
}
/***************************************************************************/
void findDialog::languageChange() {
    setCaption ( tr ( "Search in the database..." ) );
    textLabel1->setText ( tr ( "Seek in:" ) );
    textLabel2->setText ( tr ( "Owner:" ) );
    resultsl->header()->setLabel ( 0, tr ( "Name" ) );
    resultsl->header()->setLabel ( 1, tr ( "Type" ) );
    resultsl->header()->setLabel ( 2, tr ( "Media" ) );
    resultsl->header()->setLabel ( 3, tr ( "Path" ) );
    resultsl->header()->setLabel ( 4, tr ( "Modification" ) );
    textLabel3->setText ( tr ( "Find:" ) );
    cbFilename->setText ( tr ( "File name" ) );
    cbAlbum->setText ( tr ( "mp3-tag Album" ) );
    cbArtist->setText ( tr ( "mp3-tag Artist" ) );
    cbTcomm->setText ( tr ( "mp3-tag Comment" ) );
    cbDirname->setText ( tr ( "Media / Directory name" ) );
    cbTitle->setText ( tr ( "mp3-tag Title" ) );
    cbComment->setText ( tr ( "Comment" ) );
    cbContent->setText ( tr ( "Content" ) );
    buttonOk->setText ( tr ( "&OK" ) );
#ifndef _WIN32
    buttonOk->setAccel ( QKeySequence ( QString::null ) );
#endif
    buttonCancel->setText ( tr ( "&Cancel" ) );
#ifndef _WIN32
    buttonCancel->setAccel ( QKeySequence ( QString::null ) );
#endif
    textLabel5->setText ( tr ( "Results" ) );
    buttonClose->setText ( tr ( "Close / Go to selected" ) );
    cbCasesens->setText ( tr ( "Case sensitive" ) );
    cbEasy->setText ( tr ( "Use easy matching instead regex" ) );
    resultsl->clear();
}
/***************************************************************************/
int findDialog::saveState ( void ) {
    mainw->cconfig->find_em    = cbEasy->isChecked();
    mainw->cconfig->find_cs    = cbCasesens->isChecked();
    mainw->cconfig->find_di    = cbDirname->isChecked();
    mainw->cconfig->find_fi    = cbFilename->isChecked();
    mainw->cconfig->find_co    = cbComment->isChecked();
    mainw->cconfig->find_ct    = cbContent->isChecked();
    mainw->cconfig->find_mar   = cbArtist->isChecked();
    mainw->cconfig->find_mti   = cbTitle->isChecked();
    mainw->cconfig->find_mco   = cbTcomm->isChecked();
    mainw->cconfig->find_mal   = cbAlbum->isChecked();
    mainw->cconfig->findX      = x();
    mainw->cconfig->findY      = y();
    mainw->cconfig->findWidth  = width();
    mainw->cconfig->findHeight = height();
    mainw->cconfig->writeConfig();
    return 0;
}
/***************************************************************************/
int findDialog::fillCBox ( void ) {
    int  i,f,c;
    Node *tmp=mainw->db->getRootNode();

    if ( tmp == NULL ) return 0;

    cbOwner->clear();
    cbOwner->insertItem ( tr ( "All/Everybody" ),0 );
    cbOwner->insertItem ( ( ( DBCatalog * ) ( tmp->data ) )->owner );

    cbSin  ->clear();
    cbSin  ->insertItem ( tr ( "All media" ),0 );

    tmp=tmp->child; //Jump to the first media
    while ( tmp != NULL ) {
        if ( tmp->type != HC_MEDIA ) {
            tmp=tmp->next;
            continue;
        }

        cbSin  ->insertItem ( tmp->getNameOf() );
        c = cbOwner->count();
        for ( i=0,f=1;c>i;i++ )
            if ( ( ( DBMedia * ) ( tmp->data ) )->owner == cbOwner->text ( i ) )
                f=0;
        if ( f )
            cbOwner->insertItem ( ( ( DBMedia * ) ( tmp->data ) )->owner );
        tmp=tmp->next;
    }
    return 0;
}
/***************************************************************************/
// int findDialog::select(Q3ListViewItem *i)
//  {
//   if(i == NULL)
//      return 0;
//
//  // !!! I deleted the code which remove the two // from the beginning of i->text(3)
//  // It should be unnecessary since cdcat unidece support becouse the getNodeFromFullName
//  // is work different way.
//   mainw->guis->updateListFromNode(
//          (mainw->guis->getNodeFromFullName(mainw->db->getRootNode(),i->text(3))));
//
//   for(Q3ListViewItemIterator it=mainw->listView->firstChild();it.current();it++)
//    {
//      if(strcmp((it.current())->text(0),i->text(0)) ==0)
//         mainw->listView->setCurrentItem(it.current());
//    }
//   mainw->listView->curr_vis();
//   mainw->listView->repaint();
//   return 0;
//  }

int findDialog::select ( Q3ListViewItem *i ) {
    if ( i == NULL )
        return 0;

    char loc[1024];

    if ( i->text ( 3 ).isEmpty() ) //Not a real result ("There is no matching" label)
        return 0;

    strcpy ( loc,i->text ( 3 ) );
    mainw->guis->updateListFromNode (
        ( mainw->guis->getNodeFromFullName ( mainw->db->getRootNode(),QString ( loc+2 ) ) ) );
    for ( Q3ListViewItemIterator it=mainw->listView->firstChild();it.current();it++ ) {
        if ( strcmp ( ( it.current() )->text ( 0 ),i->text ( 0 ) ) ==0 )
            mainw->listView->setCurrentItem ( it.current() );
    }
    mainw->listView->curr_vis();
    mainw->listView->repaint();
    return 0;
}
/***************************************************************************/
void findDialog::closeEvent ( QCloseEvent *ce ) {
    saveState();
    QWidget::closeEvent ( ce );
}

/***************************************************************************/
int findDialog::closee ( void ) {
    close ();
    return 0;
}
/***************************************************************************/
int findDialog::cancele ( void ) {

    close();
    return 0;
}
int findDialog::seeke ( void ) {
    seekEngine *se;

    if ( mainw == NULL && mainw->db == NULL )
        return 0;

    if ( ( leText->text() ).isEmpty() )
        return 0;

    se = new seekEngine ( this );
    se->start_seek();
    delete se;
    return 0;
}
/***************************************************************************

  Seek engine Class

***************************************************************************/

seekEngine::seekEngine ( findDialog *fdp ) {
    fd   = fdp;
    patt = new char[2048];
}
/***************************************************************************/
seekEngine::~seekEngine ( void ) {
    delete [] patt;
}
/***************************************************************************/
int seekEngine::start_seek ( void ) {
    DEBUG_INFO_ENABLED = init_debug_info();
    //get the pattern
    strcpy ( patt, ( const char * ) ( ( QTextCodec::codecForLocale() )->fromUnicode ( fd->leText->text() ) ) );

    //recode the pattern /easy/  put ^$ and ? -> .  * -> .*
    if ( fd->cbEasy->isChecked() )
        easyFormConversion ( patt );

    //recode the pattern /Case sens/ lok -> [l|L][o|O][k|K]
    if ( !fd->cbCasesens->isChecked() )
        caseSensConversion ( patt );

    //fprintf(stderr,"The complete pattern is \"%s\"\n",patt);
//     re = pcre_compile ( patt,0,&error,&errptr,NULL );
    re.setPattern(QString( patt));

//     if ( re == NULL ) {
       if(!re.isValid()) {
        sprintf ( patt,"%s: %d",error,errptr );
        QMessageBox::warning ( fd,tr ( "Error in the pattern:" ),patt );
        return 1;
    }

    //// this tries to opimize pattern
//     hints = pcre_study ( re,0,&error );

//     if ( error != NULL ) {
//         QMessageBox::warning ( fd,tr ( "Error in the pattern:" ),error );
//         return 1;
//     }

    pww=new PWw ( fd );
    pww->refreshTime=200;
    progress ( pww );

    fd->resultsl->clear();
    founded = 0;

    dirname  = fd->cbDirname ->isChecked();
    filename = fd->cbFilename->isChecked();
    comment  = fd->cbComment ->isChecked();
    tartist  = fd->cbArtist  ->isChecked();
    ttitle   = fd->cbTitle   ->isChecked();
    tcomment = fd->cbTcomm   ->isChecked();
    talbum   = fd->cbAlbum   ->isChecked();
    content  = fd->cbContent ->isChecked();

    allmedia = false;
    allowner = false;

    media=fd->cbSin->currentText();
    owner=fd->cbOwner->currentText();

    if ( 0 == fd->cbOwner->currentItem() )   allowner = true;
    if ( 0 == fd->cbSin  ->currentItem() )   allmedia = true;

    progress ( pww );
    /*seek...*/
    analyzeNode ( fd->mainw->db->getRootNode() );

    if ( founded == 0 )
        fd->resultsl->insertItem ( new Q3ListViewItem ( fd->resultsl,tr ( "There is no matching." ) ) );

    progress ( pww );
    pww->end();
    delete pww;
    return 0;
}
/***************************************************************************/
int seekEngine::analyzeNode ( Node *n,Node *pa ) {

    if ( n == NULL ) return 0;
    switch ( n->type ) {
    case HC_CATALOG:
        analyzeNode ( n->child );
        return 0;

    case HC_MEDIA:
        progress ( pww );

        //It is necessary to analyze this media node? /Owner/Media/
        if ( ( allmedia || ( media == n->getNameOf() ) ) &&
                ( allowner || ( owner == ( ( DBMedia * ) ( n->data ) )->owner ) ) ) {
            if ( dirname )
                if ( matchIt ( n->getNameOf() ) ) {
                    putNodeToList ( n );
                    analyzeNode ( n->child );
                    analyzeNode ( n->next );
                    return 0;
                }
            if ( comment )
                if ( matchIt ( ( ( DBMedia * ) ( n->data ) )->comment ) )
                    putNodeToList ( n );

            analyzeNode ( n->child );
        }
        analyzeNode ( n->next );
        return 0;

    case HC_CATLNK:
        analyzeNode ( n->next );
        return 0;

    case HC_DIRECTORY:
        progress ( pww );

        if ( dirname )
            if ( matchIt ( n->getNameOf() ) ) {
                putNodeToList ( n );
                analyzeNode ( n->child );
                analyzeNode ( n->next );
                return 0;
            }

        if ( comment )
            if ( matchIt ( ( ( DBDirectory * ) ( n->data ) )->comment ) )
                putNodeToList ( n );

        analyzeNode ( n->child );
        analyzeNode ( n->next );
        return 0;

    case HC_FILE:
        if ( filename )
            if ( matchIt ( n->getNameOf() ) ) {
                putNodeToList ( n );
                analyzeNode ( n->next );
                return 0;
            }
        if ( comment )
            if ( matchIt ( ( ( DBFile * ) ( n->data ) )->comment ) ) {
                putNodeToList ( n );
                analyzeNode ( n->next );
                return 0;

            }
        analyzeNode ( ( ( DBFile * ) ( n->data ) )->prop,n );
        analyzeNode ( n->next );
        return 0;

    case HC_MP3TAG:
        if ( tartist )
            if ( matchIt ( ( ( DBMp3Tag * ) ( n->data ) )->artist ) ) {
                putNodeToList ( pa );
                return 0;
            }
        if ( ttitle )
            if ( matchIt ( ( ( DBMp3Tag * ) ( n->data ) )->title ) ) {
                putNodeToList ( pa );
                return 0;
            }
        if ( talbum )
            if ( matchIt ( ( ( DBMp3Tag * ) ( n->data ) )->album ) ) {
                putNodeToList ( pa );
                return 0;
            }
        if ( tcomment )
            if ( matchIt ( ( ( DBMp3Tag * ) ( n->data ) )->comment ) ) {
                putNodeToList ( pa );
                return 0;
            }

        analyzeNode ( n->next,pa );
        return 0;
    case HC_CONTENT:
        if ( content )
            if ( matchIt ( QString::fromLocal8Bit ( ( const char * ) ( ( DBContent * ) ( n->data ) )->bytes ) ) ) {
                putNodeToList ( pa );
                return 0;
            }
        analyzeNode ( n->next,pa );
        return 0;

    }
    return -1;
}

int seekEngine::matchIt ( QString txt ) {
    const char *encoded;
    int  match;
    if ( txt == "" ) return 0;

    encoded = ( const char * ) ( ( QTextCodec::codecForLocale() )->fromUnicode ( txt ) );
    //match   = pcre_exec ( re,hints,encoded,strlen ( encoded ),0,0,offsets,99 );
    match = re.indexIn(QString( encoded));

    if ( match == 1 )
        return 1;
    return 0;
}

void seekEngine::putNodeToList ( Node *n ) {

    Node *tmp;
    QString   type;
    QString   media;
    QDateTime mod;

    if ( n == NULL ) return;

    switch ( n->type ) {
    case HC_MEDIA:     type = tr ( "media" );
        mod  = ( ( DBMedia * ) ( n->data ) )->modification;
        break;

    case HC_DIRECTORY: type = tr ( "dir" );
        mod  = ( ( DBDirectory * ) ( n->data ) )->modification;
        break;
    case HC_FILE:      type = tr ( "file" ) + QString().sprintf ( "/ %.1f%s",
                                  ( ( DBFile * ) ( n->data ) )->size,
                                  getSType ( ( ( DBFile * ) ( n->data ) )->sizeType ) );
        mod  = ( ( DBFile * ) ( n->data ) )->modification;
        break;
    default:           type = tr ( "error" );
        break;
    }
    tmp=n;
    while ( tmp->type != HC_MEDIA )
        tmp=tmp->parent;

    media = tmp->getNameOf() + "/" + QString().setNum ( ( ( DBMedia * ) ( tmp->data ) )->number );



    fd->resultsl->insertItem ( new Q3ListViewItem ( fd->resultsl,
                               n->getNameOf(),
                               type,
                               media,
                               n->getFullPath(),
                               date_to_str ( mod )
                                                  ) );
    progress ( pww );
    founded++;
}

/***************************************************************************/

