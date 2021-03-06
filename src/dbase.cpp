/****************************************************************************
*                            Hyper's CD Catalog
*               A multiplatform qt and xml based catalog program
*
*  Author    : Peter Deak (hyperr@freemail.hu)
*  License   : GPL
*  Copyright : (C) 2003 Peter Deak
****************************************************************************/

#include "dbase.h"

#include "cdcat.h"
#include "mp3tag.h"
#include "wdbfile.h"
#include "adddialog.h"
#include "config.h"
#include "tparser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
    #include <windows.h>
#endif

#include <iostream>
#include <string>
// tar archive scanning

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include <bzlib.h>
#include <zlib.h>

#if !defined(_WIN32)
    #include <pwd.h>
    #include <grp.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QImageReader>
#include <QRegExp>

// lib7zip archive scanning
#ifdef USE_LIB7ZIP
    #include <lib7zip.h>
#endif


#ifdef USE_LIB7ZIP
class TestInStream : public C7ZipInStream {
private:
    FILE *m_pFile;
    std::string m_strFileName;
    wstring m_strFileExt;
    int m_nFileSize;
public:
    TestInStream ( std::string fileName, std::wstring ext ) :
        m_strFileName( fileName ),
        m_strFileExt( ext ) {
        DEBUG_INFO_ENABLED = init_debug_info();

        if (*DEBUG_INFO_ENABLED) {
            printf( "fileName.c_str(): %s\n", fileName.c_str());
        }
        m_pFile = fopen( fileName.c_str(), "rb" );
        if (m_pFile) {
            fseek( m_pFile, 0, SEEK_END );
            m_nFileSize = ftell( m_pFile );
            fseek( m_pFile, 0, SEEK_SET );

            unsigned int pos = m_strFileName.find_last_of( "." );

            if (pos != (unsigned int)(m_strFileName.npos)) {
                QString ext = QString::fromStdString( m_strFileName ).split( '.' ).last();
                m_strFileExt = ext.toStdWString();
            }
            if (*DEBUG_INFO_ENABLED) {
                printf( "Ext:%ls\n", m_strFileExt.c_str());
            }
        } else {
            printf( "fileName.c_str(): %s cant open\n", fileName.c_str());
        }
    }

    ~TestInStream() {
        if (m_pFile) {
            fclose( m_pFile );
        }
    }

public:
    wstring GetExt() const {
        DEBUG_INFO_ENABLED = init_debug_info();

        if (*DEBUG_INFO_ENABLED) {
            printf( "GetExt:%ls\n", m_strFileExt.c_str());
        }
        return m_strFileExt;
    }

    int Read( void *data, unsigned int size, unsigned int *processedSize ) {
        if (!m_pFile) {
            return 1;
        }

        int count = fread( data, 1, size, m_pFile );
        // printf("Read:%d %d\n", size, count);

        if (count >= 0) {
            if (processedSize != NULL) {
                *processedSize = count;
            }

            return 0;
        }

        return 1;
    }

    int Seek( __int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition ) {
        if (!m_pFile) {
            return 1;
        }

        int result = fseek( m_pFile, (long)offset, seekOrigin );

        if (!result) {
            if (newPosition) {
                *newPosition = ftell( m_pFile );
            }

            return 0;
        }

        return result;
    }

    int GetSize( unsigned __int64 *size ) {
        if (size) {
            *size = m_nFileSize;
        }
        return 0;
    }
};
#endif

#ifdef USE_LIBEXIF
    #include "cdcatexif.h"
#endif

// FIXME: currently disabled because lib7zip has problems with std namespace :(
// using namespace std;


QString date_to_str( QDateTime dt ) {
    QString text;

    if (dt.isValid()) {
        text.sprintf( "%d-%02d-%02d %02d:%02d:%02d", dt.date().year(), dt.date().month(), dt.date().day(),
                      dt.time().hour(), dt.time().minute(), dt.time().second());
    } else {
        text = QApplication::translate( "dbase", "Not available" );
    }

    return text;
}

/***************************************************************************/
void caseSensConversion( char *p ) {
    int s, t, ss = strlen( p );
    char *tmp = new char[2048];

    for (s = 0, t = 0; s <= ss; s++, t++) {
        if ((p[s] >= 'a' && p[s] <= 'z') ||
            (p[s] >= 'A' && p[s] <= 'Z')) {
            tmp[t++] = '[';
            tmp[t++] = tolower( p[s] );
            tmp[t++] = '|';
            tmp[t++] = toupper( p[s] );
            tmp[t] = ']';
        } else {
            tmp[t] = p[s];
        }
    }
    strcpy( p, tmp );
    delete [] tmp;
}

/***************************************************************************/
void easyFormConversion( char *p ) {
    int s, t, ss = strlen( p );
    char *tmp = new char[2048];

    for (s = 0, t = 0; s <= ss; s++, t++) {
        if (p[s] == '?') {
            tmp[t] = '.';
        } else if (p[s] == '*') {
            tmp[t++] = '.';
            tmp[t] = '*';
        } else if ((p[s] >= 'a' && p[s] <= 'z') ||
                   (p[s] >= 'A' && p[s] <= 'Z') ||
                   (p[s] >= '0' && p[s] <= '9')) {
            tmp[t] = p[s];
        } else if (p[s] == '\0') {
            tmp[t] = '\0';
        } else {
            tmp[t++] = '\\';
            tmp[t] = p[s];
        }
    }
    sprintf( p, "^%s$", tmp );
    delete [] tmp;
}

/***************************************************************************
*
*  DataBase Class
*
***************************************************************************/

Node::Node ( void ) {
    type = HC_UNINITIALIZED;
    next = NULL;
    child = NULL;
    parent = NULL;
    data = NULL;
}

Node::Node ( int t, Node *p ) {
    type = t;
    next = NULL;
    child = NULL;
    parent = p;
    data = NULL;
}

Node::~Node ( void ) {
    if (child != NULL) {
        delete child;
    }
    if (next != NULL) {
        delete next;
    }
    if (data != NULL) {
        switch (type) {
        case HC_UNINITIALIZED:
            break;
        case HC_CATALOG:
            delete ((DBCatalog *)data);
            break;
        case HC_MEDIA:
            delete ((DBMedia *)data);
            break;
        case HC_DIRECTORY:
            delete ((DBDirectory *)data);
            break;
        case HC_FILE:
            delete ((DBFile *)data);
            break;
        case HC_MP3TAG:
            delete ((DBMp3Tag *)data);
            break;
        case HC_CONTENT:
            delete ((DBContent *)data);
            break;
        case HC_CATLNK:
            delete ((DBCatLnk *)data);
            break;
        case HC_EXIF:
            delete ((DBExifData *)data);
            break;
        case HC_THUMB:
            delete ((DBThumb *)data);
            break;
        }
    }
    child = next = NULL;
    data = NULL;
}

QString Node::getFullPath( void ) {
    Node *up = this;
    QString a = "";

    while (up != NULL) {
        a.prepend( up->getNameOf());
        a.prepend( "/" );
        up = up->parent;
    }
    a.prepend( "/" );
    return a;
}

void Node::touchDB( void ) {
    if (type != HC_CATALOG) {
        parent->touchDB();
    } else {
        ((DBCatalog *)data)->touch();
    }
}


QString Node::getNameOf( void ) {
    switch (type) {
    case HC_UNINITIALIZED:
        return QString( "" );
    case HC_CATALOG:
        return ((DBCatalog *)data)->name;
    case HC_MEDIA:
        return ((DBMedia *)data)->name;
    case HC_DIRECTORY:
        return ((DBDirectory *)data)->name;
    case HC_FILE:
        return ((DBFile *)data)->name;
    case HC_MP3TAG:
        return QString( "" );
    case HC_CONTENT:
        return QString( "" );
    case HC_CATLNK:
        return ((DBCatLnk *)data)->name;
    }
    return QString( "" );
}

DBCatalog::DBCatalog ( QString n, QString o, QString c, QDateTime mod, QString pcategory ) {
    name = n;
    owner = o;
    comment = c;
    category = pcategory;
    writed = 1;
    strcpy( filename, "" );
    fileversion = "";
    modification = mod;
    isEncryptedCatalog = false;
}

DBCatalog::DBCatalog ( void ) {
    name = "";
    owner = "";
    comment = "";
    writed = 1;
    strcpy( filename, "" );
    modification = QDateTime::currentDateTime();
    fileversion = "";
    sortedBy = NAME;
    isEncryptedCatalog = false;
}

DBCatalog::~DBCatalog ( void ) {
}

void DBCatalog::touch( void ) {
    writed = 0;
    modification = QDateTime::currentDateTime();
}

DBMedia::DBMedia ( QString n, int nu, QString o, int t, QString c, QDateTime mod, QString pcategory ) {
    name = n;
    number = nu;
    owner = o;
    type = t;
    modification = mod;
    comment = c;
    category = pcategory;
    borrowing = "";
}

DBMedia::DBMedia ( void ) {
    name = "";
    number = 0;
    owner = "";
    type = 0;
    modification = QDateTime::currentDateTime();
    comment = "";
    borrowing = "";
}

DBMedia::~DBMedia ( void ) {
}


DBDirectory::DBDirectory ( QString n, QDateTime mod, QString c, QString pcategory ) {
    name = n;
    modification = mod;
    comment = c;
    category = pcategory;
}

DBDirectory::DBDirectory ( void ) {
    name = "";
    modification = QDateTime();
    comment = "";
}

DBDirectory::~DBDirectory ( void ) {
}



DBFile::DBFile ( QString n, QDateTime mod, QString c, double s, int st, QString pcategory, QList<ArchiveFile> parchivecontent, QString fileinfo ) {
    name = n;
    modification = mod;
    comment = c;
    category = pcategory;
    archivecontent = parchivecontent;
    this->fileinfo = fileinfo;
    size = s;
    sizeType = st;
    prop = NULL;
}

DBFile::DBFile ( void ) {
    name = "";
    modification = QDateTime();
    category = "";
    archivecontent = QList<ArchiveFile>();
    comment = "";
    fileinfo = "";
    size = 0;
    sizeType = 0;
    prop = NULL;
}

DBFile::~DBFile ( void ) {
    if (prop != NULL) {
        delete prop;
    }
}

DBMp3Tag::DBMp3Tag ( void ) {
    artist = "";
    title = "";
    comment = "";
    album = "";
    year = "";
    tnumber = 0;
}

DBMp3Tag::DBMp3Tag ( QString a, QString t, QString c, QString al, QString y, int tnum ) {
    artist = a;
    title = t;
    comment = c;
    album = al;
    year = y;
    tnumber = tnum;
}

DBMp3Tag::~DBMp3Tag ( void ) {
}

DBContent::DBContent ( void ) {
    bytes = NULL;
    storedSize = 0;
}

DBContent::DBContent ( unsigned char *pbytes, unsigned long pstoredSize ) {
    bytes = pbytes;
    storedSize = pstoredSize;
}

DBContent::~DBContent ( void ) {
    if (bytes != NULL) {
        delete [] bytes;
        bytes = NULL;
    }
    storedSize = 0;
}

DBCatLnk::DBCatLnk ( QString pname, char *plocation, QString pcomment, QString pcategory ) {
    name = pname;
    location = mstr( plocation );
    comment = pcomment;
    category = pcategory;
}

DBCatLnk::DBCatLnk ( void ) {
    location = NULL;
}

DBCatLnk::~DBCatLnk ( void ) {
    if (location != NULL) {
        delete[] location;
    }
}

DBExifData::DBExifData( QStringList ExifDataList  ) {
    this->ExifDataList = ExifDataList;
}

DBExifData::~DBExifData( void ) {
}

DBThumb::DBThumb( QImage ThumbImage ) {
    this->ThumbImage = ThumbImage;
}

DBThumb::~DBThumb( void ) {
}

DataBase::DataBase ( void ) {
    nicef = true;
    errormsg = "";
    pww = NULL;
    storeMp3tags = true;
    storeContent = true;
    showProgressedFileInStatus = true;
    displayCurrentScannedFileInTray = false;
    storedFiles = "*.nfo;*.diz;readme.txt";
    storeThumb = true;
    ThumbExtsList.clear();
    ThumbExtsList.append( "png" );
    ThumbExtsList.append( "gif" );
    ThumbExtsList.append( "jpg" );
    ThumbExtsList.append( "jpeg" );
    ThumbExtsList.append( "bmp" );
    storeExifData = true;
    doExcludeFiles = false;
    useWildcardInsteadRegexForExclude = false;
    ignoreReadErrors = false;
    doWork = true;
    ExcludeFileList.clear();
    storeLimit = 32 * 1024;
    root = new Node( HC_CATALOG, NULL );
    root->data = (void *)new DBCatalog();
    ((DBCatalog *)(root->data))->sortedBy = NAME;
    XML_ENCODING = "UTF-8";
    Lib7zipTypes.clear();
    Lib7zipTypes.append( "zip" );
    Lib7zipTypes.append( "7z" );
    DEBUG_INFO_ENABLED = init_debug_info();


#ifdef USE_LIB7ZIP
    C7ZipLibrary lib;
    WStringArray exts;

    if (!lib.Initialize()) {
        qWarning() << "lib7zip initialize failed, lib7zip scanning disabled";
        doScanArchiveLib7zip = false;
    } else {
#ifdef LIB_7ZIP_VERSION
        QString Lib7ZipVersion = LIB_7ZIP_VERSION;
#else
        QString Lib7ZipVersion = tr( "unknown" );
#endif
        qDebug("lib7zip (%s) initialize succeeded, lib7zip scanning enabled", Lib7ZipVersion.toLocal8Bit().constData());
    }
    if (!lib.GetSupportedExts( exts )) {
        qWarning() << "lib7zip get supported exts failed, lib7zip scanning disabled";
        doScanArchiveLib7zip = false;
    } else {
        for (WStringArray::const_iterator extIt = exts.begin(); extIt != exts.end(); extIt++)
            Lib7zipTypes.append( QString().fromWCharArray((*extIt).c_str()));
        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "lib7zip supported extensions:" << qPrintable( Lib7zipTypes.join( " " ));
        }
    }
    lib.Deinitialize();
#else
    if (*DEBUG_INFO_ENABLED) {
        qDebug() << "lib7zip library not supported";
    }
#endif
    if (storeThumb) {
        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "Supported thumbnail image formats:" << QImageReader::supportedImageFormats();
        }
    }
}

DataBase::~DataBase ( void ) {
    delete root;
}

void DataBase::setDBName( QString n ) {
    ((DBCatalog *)(root->data))->name = n;
}

void DataBase::setDBOwner( QString o ) {
    ((DBCatalog *)(root->data))->owner = o;
}

void DataBase::setComment( QString c ) {
    ((DBCatalog *)(root->data))->comment = c;
}

void DataBase::setCategory( QString c ) {
    ((DBCatalog *)(root->data))->category = c;
}

QString& DataBase::getDBName( void ) {
    return ((DBCatalog *)(root->data))->name;
}

QString& DataBase::getDBOwner( void ) {
    return ((DBCatalog *)(root->data))->owner;
}

QString& DataBase::getComment( void ) {
    return ((DBCatalog *)(root->data))->comment;
}

QString& DataBase::getCategory( void ) {
    return ((DBCatalog *)(root->data))->category;
}


/***************************************************************************/
char *pattern;
int DataBase::addMedia( QString what, QString name, int number, int type ) {
    return addMedia( what, name, number, type, "" );
}

int DataBase::addMedia( QString what, QString name, int number, int type, QString owner, QString pcategory ) {
    int returnv = 0;
    Node *tt = root->child;

    this->pcategory = pcategory;
    pww->doCancel = false;
    pww->show();
    progress( pww );
    ((DBCatalog *)(root->data))->touch();
    if (root->child == NULL) {
        root->child = tt = new Node( HC_MEDIA, root );
    } else {
        while (tt->next != NULL)
            tt = tt->next;
        tt->next = new Node( HC_MEDIA, root );
        tt = tt->next;
    }

    progress( pww );
    /* Fill the media Node (tt) */
    tt->data = (void *)
               new DBMedia( name,
                            number,
                            owner.isEmpty() ? ((DBCatalog *)(root->data))->owner : owner,
                            type, pcategory );
    progress( pww );

    /* make the regex pattern for storecontent */

    progress( pww );
    pattern = new char[1024];

    if (doExcludeFiles && !ExcludeFileList.isEmpty()) {
        excludeFileRegExList.clear();
        for (int i = 0; i < ExcludeFileList.size(); ++i) {
            QString patt = ExcludeFileList.at( i );
            QRegExp re;
            re.setPattern( QString( patt ));
            if (useWildcardInsteadRegexForExclude) {
                re.setPatternSyntax( QRegExp::Wildcard );
            } else {
                re.setPatternSyntax( QRegExp::RegExp );
            }
            re.setCaseSensitivity( Qt::CaseInsensitive );
            if (re.isValid()) {
                excludeFileRegExList.append( re );
            } else {
                qWarning() << "exclude pattern is invalid:" << qPrintable( patt );
            }
        }
    }

#ifndef NO_MEDIAINFO
    SupportedFileInfoExtensionsList.clear();
    SupportedFileInfoExtensionsList = me.getSupportedExtensions();
#endif

    returnv = scanFsToNode( what, tt );
    delete [] pattern;
    return returnv;
}
/***************************************************************************/
int DataBase::saveDB( void ) {
    gzFile f = NULL;
    FileWriter *fw = NULL;

    progress( pww );

    if (strcmp(((DBCatalog *)(root->data))->filename, "" ) == 0) {
        return 1;
    }

    f = gzopen(((DBCatalog *)(root->data))->filename, "wb" );
    if (f == NULL) {
        errormsg = tr( "I can't rewrite the file: %1" ).arg(((DBCatalog *)(root->data))->filename );
        return 2;
    }
    progress( pww );

    fw = new FileWriter( f, nicef, this->XML_ENCODING );
    fw->pww = pww;
    progress( pww );
    fw->writeDown( root );
    ((DBCatalog *)(root->data))->writed = 1;
    progress( pww );
    gzclose( f );
    delete fw;
    return 0;
}
/***************************************************************************/
int DataBase::saveAsDB( char *filename ) {
    int i;
    gzFile f = NULL;
    FileWriter *fw = NULL;

    progress( pww );
    /* Check overwriting !!! */
    f = gzopen( filename, "wb" );
    if (f == NULL) {
        errormsg = tr( "I can't create the file: %1" ).arg( filename );
        return 1;
    }

    progress( pww );
    fw = new FileWriter( f, nicef, this->XML_ENCODING );
    fw->pww = pww;
    i = fw->writeDown( root );
    ((DBCatalog *)(root->data))->writed = 1;
    strcpy(((DBCatalog *)(root->data))->filename, filename );

    progress( pww );
    gzclose( f );
    delete fw;
    return 0;
}
/***************************************************************************/
int DataBase::insertDB( char *filename, bool skipDuplicatesOnInsert, bool isGzFile ) {
    int i;
    gzFile gf = NULL;
    FILE *f = NULL;
    FileReader *fw = NULL;

    DEBUG_INFO_ENABLED = init_debug_info();

    if (root == NULL) {
        errormsg = tr( "No database opened!" );
        return 1;
    }
    progress( pww );
    if (isGzFile) {
        gf = gzopen( filename, "rb" );
        if (gf == NULL) {
            errormsg = tr( "I can't open the file: %1" ).arg( filename );
            return 1;
        }
    } else {
        f = fopen( filename, "rb" );
        if (f == NULL) {
            errormsg = tr( "I can't open the file: %1" ).arg( filename );
            return 1;
        }
    }

    // check free memory
    char testbuffer[READ_BLOCKSIZE + 1];
    long long int filesize = 0;
    int readcount = 0;
    if (isGzFile) {
        readcount = gzread( gf, testbuffer, READ_BLOCKSIZE );
    } else {
        readcount = fread( testbuffer, 1, READ_BLOCKSIZE, f );
    }
    while (readcount != 0) {
        filesize += readcount;
        // if(*DEBUG_INFO_ENABLED)
        //  qDebug() << "readcount:" << readcount;
        if (isGzFile) {
            readcount = gzread( gf, testbuffer, READ_BLOCKSIZE );
        } else {
            readcount = fread( testbuffer, 1, READ_BLOCKSIZE, f );
        }
        progress( pww );
    }
    if (isGzFile) {
        gzrewind( gf );
    } else {
        rewind( f );
    }
    if (*DEBUG_INFO_ENABLED) {
        qDebug() << "detected uncompressed size:" << filesize;
    }

    char *allocated_buffer = NULL;
    allocated_buffer = (char *)calloc( filesize, sizeof(QChar));
    if (allocated_buffer == NULL) {
        // fail => no enough memory
        if (isGzFile) {
            gzclose( gf );
        } else {
            fclose( f );
        }
        return 1;
    } else {
    }
    /* end memtest */

    progress( pww );
    if (isGzFile) {
        fw = new FileReader( gf, allocated_buffer, filesize, 1 );
    } else {
        fw = new FileReader( f, allocated_buffer, filesize, 1 );
    }

    fw->pww = pww;
    progress( pww );

    i = fw->readFrom( root, skipDuplicatesOnInsert );

    if (i == 1) {
        progress( pww );
        errormsg = fw->errormsg;
        if (*DEBUG_INFO_ENABLED) {
            qCritical() << "error:" << qPrintable( fw->errormsg );
        }
        delete fw;
        gzclose( gf );
        return 1;
    }

    ((DBCatalog *)(root->data))->touch();
    progress( pww );

    if (isGzFile) {
        gzclose( gf );
    } else {
        fclose( f );
    }
    delete fw;
    return 0;
}

/***************************************************************************/
int DataBase::openDB( char *filename ) {
    int parsingSuccess;
    gzFile f = NULL;
    FileReader *fw = NULL;

    DEBUG_INFO_ENABLED = init_debug_info();

    progress( pww );
    f = gzopen( filename, "rb" );
    if (f == NULL) {
        errormsg = tr( "I can't open the file: %1" ).arg( filename );
        return 1;
    }

    // check free memory
    char testbuffer[READ_BLOCKSIZE + 1];
    long long int filesize = 0;
    int readcount = 0;
    readcount = gzread( f, testbuffer, READ_BLOCKSIZE );
    while (readcount != 0) {
        filesize += readcount;
        // if(*DEBUG_INFO_ENABLED)
        //  qDebug() << "readcount:" << readcount;
        readcount = gzread( f, testbuffer, READ_BLOCKSIZE );
        progress( pww );
    }
    gzrewind( f );
    if (*DEBUG_INFO_ENABLED) {
        qDebug() << "detected uncompressed size:" << filesize;
    }

    char *allocated_buffer = NULL;
    allocated_buffer = (char *)calloc( filesize, sizeof(QChar));
    if (allocated_buffer == NULL) {
        // fail => no enough memory
        errormsg = tr( "Not enough memory to open the file: %1" ).arg( filename );
        return 1;
    } else {
    }
    /* end memtest */

    progress( pww );
    fw = new FileReader( f, allocated_buffer, filesize );

    fw->pww = pww;
    progress( pww );

    if (!doWork) {
        gzclose( f );
        delete fw;
        return 1;
    }
    if (root != NULL) {
        delete root;         // Free previous database in memory
    }
    progress( pww );
    root = new Node( HC_CATALOG, NULL );      // Malloc root node.

    root->data = (void *)new DBCatalog();

    progress( pww );
    parsingSuccess = fw->readFrom( root );
    this->XML_ENCODING = fw->XML_ENCODING;

    if (parsingSuccess == 1 || pww->doCancel) {
        progress( pww );
        errormsg = fw->errormsg;
        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "filereader reported error:" << qPrintable( fw->errormsg );
        }

        if (!doWork) {
            gzclose( f );
            delete fw;
            return 1;
        }

        delete root;
        root = NULL;
        delete fw;
        gzclose( f );
        return 1;
    }


    ((DBCatalog *)(root->data))->writed = 1;
    strcpy(((DBCatalog *)(root->data))->filename, filename );
    qDebug() << "isEncryptedCatalog:" << ((DBCatalog *)(root->data))->isEncryptedCatalog;
    progress( pww );

    gzclose( f );
    delete fw;
    return 0;
}
/***************************************************************************/

/*************************************************************************/
int DataBase::scanFsToNode( QString what, Node *to ) {
    if (pww->doCancel) {
        return 2;
    }

    DEBUG_INFO_ENABLED = init_debug_info();
    if (*DEBUG_INFO_ENABLED) {
        qDebug() << "Loading node:" << qPrintable( what );
    }

    int ret;
    QString comm = NULL;
    QList<ArchiveFile> archivecontent = QList<ArchiveFile>();
    ret = 0;
    QDir dir( what );
    if (!dir.isReadable()) {
        if (*DEBUG_INFO_ENABLED) {
            qWarning() << "dir" << qPrintable( what ) << "is not readable";
        }

        int i = 0;
        if (!ignoreReadErrors) {
            if (QFileInfo( what ).isDir()) {
                errormsg = tr( "Cannot read directory: %1" ).arg( what );
            } else { /* socket files and dead symbolic links end here */
                errormsg = tr( "Cannot read file: %1" ).arg( what );
            }
            i = 1 + (QMessageBox::warning( NULL, tr( "Error" ), errormsg, tr( "Ignore" ), tr( "Cancel scanning" )));
        }
        return i;
    }
    QFileInfoList dirlist( dir.entryInfoList( QStringList( QString( "*" )), QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System ));

    for (int fi = 0; fi < dirlist.size(); ++fi) {
        if (pww->doCancel) {
            return 2;
        }
        QFileInfo fileInfo( dirlist.at( fi ));
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..") {
            continue;
        }

        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "processing in dir" << qPrintable( what ) << "node:" << qPrintable( fileInfo.fileName());
        }

        if (showProgressedFileInStatus) {
            emit pathScanned( fileInfo.filePath());
        }

        /* Check against exclude list {{{ */
        if (doExcludeFiles) {
            bool exclude_path_matched = false;
            for (int i = 0; i < excludeFileRegExList.size(); i++) {
                if (*DEBUG_INFO_ENABLED) {
                    qDebug() << "exclude files: checking regex \"" << qPrintable( excludeFileRegExList.at( i ).pattern()) << "\" against file path" << qPrintable( fileInfo.filePath());
                }
                if (excludeFileRegExList.at( i ).exactMatch( fileInfo.filePath())) {
                    if (*DEBUG_INFO_ENABLED) {
                        qDebug() << "exclude files: regex MATCH: \"" << qPrintable( excludeFileRegExList.at( i ).pattern()) << "\" against file path" << qPrintable( fileInfo.filePath()) << ", skipping";
                    }
                    exclude_path_matched = true;
                    break;
                }
            }
            if (exclude_path_matched) {
                continue;
            }
        } /* }}} */

        /* Make a new node {{{ */
        Node *tt = to->child;
        if (to->child == NULL) {
            to->child = tt = new Node( fileInfo.isDir() ? HC_DIRECTORY : HC_FILE, to );
        } else {
            while (tt->next != NULL)
                tt = tt->next;
            tt->next = new Node( fileInfo.isDir() ? HC_DIRECTORY : HC_FILE, to );
            tt = tt->next;
        }
        /* Fill the data field */
        if (fileInfo.isFile()) { /* File {{{ */
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "Scanning file:" << qPrintable( fileInfo.fileName());
            }

            // if ( displayCurrentScannedFileInTray ) {
            //	emit fileScanned ( fileInfo->filePath() );
            // }

            /* File size {{{ */
            double size = fileInfo.size();
            double s = size;
            int st = UNIT_BYTE;

            if (size >= (double)SIZE_ONE_GBYTE * 1024.0) {
                s = size / SIZE_ONE_GBYTE / 1024.0;
                st = UNIT_TBYTE;
            } else {
                if (size >= (double)SIZE_ONE_GBYTE && size < (double)SIZE_ONE_GBYTE * 1024.0) {
                    s = size / SIZE_ONE_GBYTE;
                    st = UNIT_GBYTE;
                } else {
                    if (size >= (double)SIZE_ONE_MBYTE && size < (double)SIZE_ONE_GBYTE) {
                        s = size / SIZE_ONE_MBYTE;
                        st = UNIT_MBYTE;
                    } else {
                        if (size >= (double)SIZE_ONE_KBYTE && size < (double)SIZE_ONE_MBYTE) {
                            s = size / SIZE_ONE_KBYTE;
                            st = UNIT_KBYTE;
                        } else {
                            s = size;
                            st = UNIT_BYTE;
                        }
                    }
                }
            }

            if ( *DEBUG_INFO_ENABLED ) {
                qDebug() << "Saving file size:" << QString().setNum( s ) << qPrintable ( getSType ( st ) );
            }

            progress( pww );

            if (fileInfo.isSymLink()) { /* SYMBOLIC LINK to a FILE */
                    comm = tr( "Symbolic link to file:#" )
                       + dir.relativeFilePath( fileInfo.symLinkTarget());
            } else {
                comm = (char *)NULL;
                QString extension = fileInfo.fileName().toLower().section( '.', -1, -1 );
                if (doScanArchive) {
                    if (doScanArchiveTar) {
                        if (extension == "tar") {
                            if (*DEBUG_INFO_ENABLED) {
                                qDebug() << "tarfile found:" << qPrintable( what + "/" + fileInfo.fileName());
                            }
                            archivecontent = scanArchive( what + "/" + fileInfo.fileName(), Archive_tar );
                        } else if (fileInfo.fileName().toLower().section( '.', -2, -1 ) == "tar.gz") {
                            if (*DEBUG_INFO_ENABLED) {
                                qDebug() << "targz found:" << qPrintable( what + "/" + fileInfo.fileName());
                            }
                            archivecontent = scanArchive( what + "/" + fileInfo.fileName(), Archive_targz );
                        } else if (fileInfo.fileName().toLower().section( '.', -2, -1 ) == "tar.bz2") {
                            if (*DEBUG_INFO_ENABLED) {
                                qDebug() << "tarbz2 found:" << qPrintable( what + "/" + fileInfo.fileName());
                            }
                            archivecontent = scanArchive( what + "/" + fileInfo.fileName(), Archive_tarbz2 );
                        }
                    }
                    if (doScanArchiveLib7zip) {
                        if (Lib7zipTypes.contains( extension )
                            && !(
                                   fileInfo.fileName().toLower().section( '.', -1, -1 ) == "tar"
                                || fileInfo.fileName().toLower().section( '.', -2, -1 ) == "tar.gz"
                                || fileInfo.fileName().toLower().section( '.', -2, -1 ) == "tar.bz2"
                                )
                            ) {

                            if (*DEBUG_INFO_ENABLED) {
                                qDebug() << "lib7zip found:" << qPrintable( what + "/" + fileInfo.fileName());
                            }
                            archivecontent = scanArchive( what + "/" + fileInfo.fileName(), Archive_lib7zip );
                        }
                    }
                }
            }
            tt->data = (void *)new DBFile( fileInfo.fileName(), fileInfo.lastModified(), comm, s, st, this->pcategory, archivecontent );
            archivecontent.clear();
            scanFileProp( &fileInfo, (DBFile *)tt->data );
        } else if (fileInfo.isDir()) { /* Directory {{{ */
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "Adding dir:" << qPrintable( fileInfo.fileName());
            }

            progress( pww );

            if (fileInfo.isSymLink()) {                       /* SYMBOLIC LINK to a DIRECTORY */
                /* These links appear as empty directories in the GUI */
                /* Change to DBFile to show them as files */
                tt->data = (void *)new DBDirectory(
                    fileInfo.fileName(), fileInfo.lastModified(),
                    tr( "Symbolic link to directory:#" )
                    + dir.relativeFilePath( fileInfo.symLinkTarget()), this->pcategory );
                continue;                         /* Do not recurse into symbolically linked directories */
            } else {
                tt->data = (void *)new DBDirectory(
                    fileInfo.fileName(), fileInfo.lastModified(), (char *)NULL, this->pcategory );
            }

            /* Start recursion: */
            QString thr( what );
            thr = thr.append( "/" );
            thr = thr.append( fileInfo.fileName());


            if (pww->doCancel) {
                return 2;
            }
            if ((ret = scanFsToNode( thr, tt )) == 2) {
                return ret;
            }
        } else if (fileInfo.isSymLink()) {                    /* DEAD SYMBOLIC LINK */
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "adding dead symlink:" << qPrintable( fileInfo.fileName());
            }

            progress( pww );

            comm = tr( "DEAD Symbolic link to:#" )
                   + dir.relativeFilePath( fileInfo.symLinkTarget());
            tt->data = (void *)new DBFile( fileInfo.fileName(), QDateTime(),
                                           comm, 0, UNIT_BYTE, this->pcategory );
        } else {                         /* SYSTEM FILE (e.g. FIFO, socket or device file) */
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "adding system file:" << qPrintable( fileInfo.fileName());
            }

            progress( pww );

            comm = tr( "System file (e.g. FIFO, socket or device file)" );
            tt->data = (void *)new DBFile( fileInfo.fileName(), fileInfo.lastModified(),
                                           comm, 0, UNIT_BYTE, this->pcategory );
        }
        if (pww->appl->hasPendingEvents()) {
            pww->appl->processEvents();
        }
        /* }}} */
    }    /* end of for,..next directory entry */
    return ret;
}

/***************************************************************************/

int DataBase::scanFileProp( QFileInfo *fi, DBFile *fc ) {
    DEBUG_INFO_ENABLED = init_debug_info();

    QString file_suffix = fi->suffix();
    QString file_abspath = fi->absoluteFilePath();
    QString file_path = fi->filePath();
    QString file_name = fi->fileName();
    qint64 file_size = fi->size();

    /* MP3 tag scanning {{{ */
    if (storeMp3tags || storeMp3techinfo) {
        if (file_suffix.toLower() == "mp3" || file_suffix.toLower() == "mp2") {
            if (showProgressedFileInStatus) {
                emit pathExtraInfoAppend( tr( "reading mp3 info" ));
            }
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "reading mp3 info for" << qPrintable( file_path );
            }

            ReadMp3Tag *reader = new ReadMp3Tag((const char *)QFile::encodeName( file_abspath ), v1_over_v2 );
            if (pww->appl->hasPendingEvents()) {
                pww->appl->processEvents();
            }
            if (storeMp3tags) {
                if (reader->readed() && reader->exist()) {
                    Node *tt = fc->prop;
                    if (tt == NULL) {
                        fc->prop = tt = new Node( HC_MP3TAG, NULL );
                    } else {
                        while (tt->next != NULL)
                            tt = tt->next;
                        tt->next = new Node( HC_MP3TAG, fc->prop );
                        tt = tt->next;
                    }
                    /* Fill the fields: */
                    tt->data = (void *)new DBMp3Tag(
                        QString::fromLocal8Bit( reader->artist()),
                        QString::fromLocal8Bit( reader->title()),
                        QString::fromLocal8Bit( reader->comment()),
                        QString::fromLocal8Bit( reader->album()),
                        QString::fromLocal8Bit( reader->year()),
                        reader->tnum()
                    );
                }
            }
            // Put some technical info to comment
            if (storeMp3techinfo) {
                const char *info = reader->gettechinfo();
                if (info) {
                    if (!fc->comment.isEmpty()) {
                        fc->comment.append( "#" );
                    }
                    fc->comment.append( info );
                    delete [] info;
                }
            }
            if (reader != NULL) {
                delete reader;
                reader = NULL;
            }
        }
    } /* }}} */

    /* Media info scanning {{{ */
#ifndef NO_MEDIAINFO
    if (storeFileInfo && me.getMediaInfoLibFound()) {
        if (SupportedFileInfoExtensionsList.contains( file_suffix.toLower())) {
            if (showProgressedFileInStatus) {
                emit pathExtraInfoAppend( tr( "reading media info" ));
            }
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "reading media info for" << qPrintable( file_path );
            }

            if (displayCurrentScannedFileInTray) {
                emit fileScanned( tr( "reading media info" ) + ": " + file_path );
            }

            QString info = CdcatMediaInfo( file_abspath ).getInfo();

            if (!info.isEmpty()) {
                fc->fileinfo = info;
            }
            if (pww->appl->hasPendingEvents()) {
                pww->appl->processEvents();
            }
        }
    }
#endif
    /* }}} */

    /* Experimental AVI Header Scanning {{{ */
    if (storeAvitechinfo && (file_suffix).toLower() == "avi") {
        FILE *filePTR;
        filePTR = fopen((const char *)QFile::encodeName( file_abspath ), "r" );
        if (filePTR != NULL) {
            if (showProgressedFileInStatus) {
                emit pathExtraInfoAppend( tr( "reading avi info" ));
            }
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "reading avi info for:" << qPrintable( file_path );
            }

            QString got = parseAviHeader( filePTR ).replace( QRegExp( "\n" ), "#" );
            fclose( filePTR );

            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "avi info for " << qPrintable( file_path ) << ":" << qPrintable( got );
            }

            if (pww->appl->hasPendingEvents()) {
                pww->appl->processEvents();
            }

            // store it as comment
            if (!got.isEmpty()) {
                if (!fc->comment.isEmpty()) {
                    fc->comment.append( "#" );
                }
                fc->comment.append( got );
            }
        } else {
            if (*DEBUG_INFO_ENABLED) {
                qWarning() << "could not reading avi info for:" << qPrintable( file_path );
            }
        }
    } /* }}} */

    /* File content scanning {{{ */
    if (storeContent) {
        //         pcre       *pcc = NULL;
        //         const char *error;
        //         int         erroroffset;
        //         int         ovector[30];
        bool match = false;

        QStringList exts = storedFiles.split( ";" );
        QStringList::Iterator it = exts.begin();

        for (; it != exts.end(); ++it) { // Iterate over the ';' separated patterns.
            strcpy( pattern, ((*it).toLocal8Bit().constData()));
            easyFormConversion( pattern );
            caseSensConversion( pattern );
            QRegExp pcc2;
            pcc2.setPattern( QString( pattern ));
            pcc2.setCaseSensitivity( Qt::CaseInsensitive );
            // pcc   = pcre_compile ( pattern,0,&error,&erroroffset,NULL );
            // if(*DEBUG_INFO_ENABLED)
            //	qDebug() << "pcc2 pattern: " << pattern << ", match:" << pcc2.exactMatch(QString(( const char * ) QFile::encodeName ( file_name)));
            // if(*DEBUG_INFO_ENABLED)
            //  qDebug() << "pcre_exec match:" << pcre_exec ( pcc,NULL, ( const char * ) QFile::encodeName ( file_name )
            //                                   ,strlen ( ( const char * ) QFile::encodeName ( file_name ) )
            //                                   ,0,0,ovector,30 ) ;
            //             if ( 1 == pcre_exec ( pcc,NULL, ( const char * ) QFile::encodeName ( file_name )
            //                                   ,strlen ( ( const char * ) QFile::encodeName ( file_name ) )
            //                                   ,0,0,ovector,30 ) ) {
            if (pcc2.exactMatch( QString((const char *)QFile::encodeName( file_name ))) == 1) {
                match = true;
                break;
            }
        }

        if (match) { // The file needs to be read.
            FILE *f = NULL;
            bool success = true;
            unsigned long rsize = 0, rrsize = 0;
            unsigned char *rdata = 0;
            Node *tt = fc->prop;

            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "Pattern \"" << pattern << "\", matched for file\"" << (const char *)QFile::encodeName( file_name ) << "\", reading content …";
            }

            if (showProgressedFileInStatus) {
                emit pathExtraInfoAppend( tr( "reading file content" ));
            }

            if (storeLimit > MAX_STORED_SIZE) {
                storeLimit = MAX_STORED_SIZE;
            }
            // read the file
            if ((rsize = file_size) > storeLimit) {
                rsize = storeLimit;
            }
            f = fopen((const char *)QFile::encodeName( file_abspath ), "rb" );
            if (f == NULL) {
                errormsg = QString( "I couldn't open the \"%1\" file. (content read 1)\n" )
                           .arg( file_abspath );
                fprintf( stderr, "%s", errormsg.toLocal8Bit().constData());
                success = false;
            } else {
                rdata = new unsigned char[rsize + 1];
                fseek( f, 0, SEEK_SET );
                rrsize = fread( rdata, sizeof(unsigned char), rsize, f );
                if (rsize != rrsize) {
                    errormsg = QString( "I couldn't correctly read the content of file \"%1\" . (content read 2)\n"
                                        "size difference %2 != %3\n" )
                               .arg( file_abspath )
                               .arg( rsize )
                               .arg( rrsize );
                    fprintf( stderr, "%s", errormsg.toLocal8Bit().constData());
                    success = false;
                }
                rdata[rsize] = '\0';
                fclose( f );
            }

            if (pww->appl->hasPendingEvents()) {
                pww->appl->processEvents();
            }

            // make the node in the db
            if (success) {
                if (tt == NULL) {
                    fc->prop = tt = new Node( HC_CONTENT, NULL );
                } else {
                    while (tt->next != NULL)
                        tt = tt->next;
                    tt->next = new Node( HC_CONTENT, fc->prop );
                    tt = tt->next;
                }
                /*Fill the fields:*/
                tt->data = (void *)new DBContent( rdata, rsize );
            }
        } // end of if(match)
    }     // end of if(storeContent)

#ifdef USE_LIBEXIF
    if (storeExifData) {
        if (getExifSupportedExtensions().contains( file_suffix )) {
            if (showProgressedFileInStatus) {
                emit pathExtraInfoAppend( tr( "reading exif data" ));
            }

            Node *tt = fc->prop;
            CdcatExifData *ed = new CdcatExifData( file_abspath );
            ed->readCdcatExifData();

            if (pww->appl->hasPendingEvents()) {
                pww->appl->processEvents();
            }

            QStringList ExifData = ed->getExifData();
            if (tt == NULL) {
                fc->prop = tt = new Node( HC_EXIF, NULL );
            } else {
                while (tt->next != NULL)
                    tt = tt->next;
                tt->next = new Node( HC_EXIF, fc->prop );
                tt = tt->next;
            }
            /*Fill the fields:*/
            tt->data = (void *)new DBExifData( ExifData );
            delete(ed);
        }
    }
#endif
    if (storeThumb) {
        if (ThumbExtsList.contains( file_suffix )) {
            if (showProgressedFileInStatus) {
                emit pathExtraInfoAppend( tr( "reading thumbnail data" ));
            }

            QImage thumbImage( file_abspath );

            if (pww->appl->hasPendingEvents()) {
                pww->appl->processEvents();
            }

            if (!thumbImage.isNull()) {
                Node *tt = fc->prop;
                thumbImage = thumbImage.scaled( QSize( thumbWidth, thumbHeight ), Qt::KeepAspectRatio );
                if (tt == NULL) {
                    fc->prop = tt = new Node( HC_THUMB, NULL );
                } else {
                    while (tt->next != NULL)
                        tt = tt->next;
                    tt->next = new Node( HC_THUMB, fc->prop );
                    tt = tt->next;
                }
                /*Fill the fields:*/
                tt->data = (void *)new DBThumb( thumbImage );
            }
        }
    }

    /***Other properties: */
    return 0;
}

#if defined(_WIN32)
/* WIN32 fake posix */
int fchmod( int fd, int mode ) {
    // dummy
    return 0;
}

struct passwd {
    char * pw_name;
    char * pw_passwd;
    uid_t  pw_uid;
    gid_t  pw_gid;
    time_t pw_change;
    char * pw_class;
    char * pw_gecos;
    char * pw_dir;
    char * pw_shell;
    time_t pw_expire;
};

struct group {
    char * gr_name;
    char * gr_passwd;
    gid_t  gr_gid;
    char **gr_mem;
};

struct passwd *getpwuid( uid_t ) {
    // dummy
    return 0;
}

struct group *getgrgid( gid_t ) {
    // dummy
    return 0;
}

#endif


int gzopen_frontend( char *pathname, int oflags, int mode ) {
    char gzoflags[3];
    gzFile gzf;
    int fd;

    switch (oflags & O_ACCMODE) {
    case O_WRONLY:
        strncpy( gzoflags, "wb", 2 );
        break;
    case O_RDONLY:
        strncpy( gzoflags, "rb", 2 );
        break;
    default:
    case O_RDWR:
        errno = EINVAL;
        return -1;
    }

    fd = open( pathname, oflags, mode );
    if (fd == -1) {
        return -1;
    }

    if ((oflags & O_CREAT) && fchmod( fd, mode )) {
        return -1;
    }

    gzf = gzdopen( fd, gzoflags );
    if (!gzf) {
        errno = ENOMEM;
        return -1;
    }
    return (int)(long)gzf;
}

int bz2open_frontend( char *pathname, int oflags, int mode ) {
    BZFILE *bz2f;
    char bzoflags[3];
    int fd;

    // int bzerror;
    // int verbose = 0;
    switch (oflags & O_ACCMODE) {
    case O_WRONLY:
        break;
    case O_RDONLY:
        break;
    default:
    case O_RDWR:
        errno = EINVAL;
        return -1;
    }

    fd = open( pathname, oflags, mode );
    if (fd == -1) {
        return -1;
    }

    if ((oflags & O_CREAT) && fchmod( fd, mode )) {
        return -1;
    }

    bz2f = BZ2_bzdopen( fd, bzoflags );
    if (!bz2f) {
        errno = ENOMEM;
        return -1;
    }
//      if (*DEBUG_INFO_ENABLED)
//              qDebug() << "bz2open_frontend ret:" << (int)(long)bz2f;
    return (int)(long)bz2f;
}


tartype_t gztype = { (openfunc_t)gzopen_frontend, (closefunc_t)gzclose, (readfunc_t)gzread, (writefunc_t)gzwrite };
tartype_t bztype = { (openfunc_t)bz2open_frontend, (closefunc_t)BZ2_bzclose, (readfunc_t)BZ2_bzread, (writefunc_t)BZ2_bzwrite };


char *format_time( time_t cal_time ) {
    struct tm *time_struct;
    static char string[30];

    /* Put the calendar time into a structure if type 'tm' */
    time_struct = localtime( &cal_time );

    /* Build a formatted date from the structure.*/
    strftime( string, sizeof string, "%h %e %H:%M\n", time_struct );

    /* Return the date/time */
    return string;
}


const wchar_t *index_names[] = {
    L"kpidPackSize",    // (Packed Size)
    L"kpidAttrib",      // (Attributes)
    L"kpidCTime",       // (Created)
    L"kpidATime",       // (Accessed)
    L"kpidMTime",       // (Modified)
    L"kpidSolid",       // (Solid)
    L"kpidEncrypted",   // (Encrypted)
    L"kpidUser",        // (User)
    L"kpidGroup",       // (Group)
    L"kpidComment",     // (Comment)
    L"kpidPhySize",     // (Physical Size)
    L"kpidHeadersSize", // (Headers Size)
    L"kpidChecksum",    // (Checksum)
    L"kpidCharacts",    // (Characteristics)
    L"kpidCreatorApp",  // (Creator Application)
    L"kpidTotalSize",   // (Total Size)
    L"kpidFreeSpace",   // (Free Space)
    L"kpidClusterSize", // (Cluster Size)
    L"kpidVolumeName",  // (Label)
    L"kpidPath",        // (FullPath)
    L"kpidIsDir",       // (IsDir)
};

#define LIB7ZIP_FILE_ATTRIBUTE_READONLY             1
#define LIB7ZIP_FILE_ATTRIBUTE_HIDDEN               2
#define LIB7ZIP_FILE_ATTRIBUTE_SYSTEM               4
#define LIB7ZIP_FILE_ATTRIBUTE_DIRECTORY           16
#define LIB7ZIP_FILE_ATTRIBUTE_ARCHIVE             32
#define LIB7ZIP_FILE_ATTRIBUTE_DEVICE              64
#define LIB7ZIP_FILE_ATTRIBUTE_NORMAL             128
#define LIB7ZIP_FILE_ATTRIBUTE_TEMPORARY          256
#define LIB7ZIP_FILE_ATTRIBUTE_SPARSE_FILE        512
#define LIB7ZIP_FILE_ATTRIBUTE_REPARSE_POINT     1024
#define LIB7ZIP_FILE_ATTRIBUTE_COMPRESSED        2048
#define LIB7ZIP_FILE_ATTRIBUTE_OFFLINE          0x1000
#define LIB7ZIP_FILE_ATTRIBUTE_ENCRYPTED        0x4000
#define LIB7ZIP_FILE_ATTRIBUTE_UNIX_EXTENSION   0x8000   /* trick for Unix */


/* is strmode from libtar/compat/strmode.c */
// modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
void DataBase::strmode( mode_t mode, char *p ) {
    /* print type */
    switch (mode & S_IFMT) {
    case S_IFDIR:                               /* directory */
        *p++ = 'd';
        break;
    case S_IFCHR:                               /* character special */
        *p++ = 'c';
        break;
#if !defined(_WIN32) // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    case S_IFBLK:    /* block special */
        *p++ = 'b';
        break;
#endif
    case S_IFREG:                               /* regular */
        *p++ = '-';
        break;
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    case S_IFLNK:     /* symbolic link */
        *p++ = 'l';
        break;
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    case S_IFSOCK:    /* socket */
        *p++ = 's';
        break;
#endif
#ifdef S_IFIFO
    case S_IFIFO:                               /* fifo */
        *p++ = 'p';
        break;
#endif
#ifdef S_IFWHT
    case S_IFWHT:                               /* whiteout */
        *p++ = 'w';
        break;
#endif
    default:                                    /* unknown */
        *p++ = ' ';
        break;
    }
    /* usr */
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    if (mode & S_IRUSR) {
        *p++ = 'r';
    } else {
        *p++ = '-';
    }
#else
    *p++ = '-';
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    if (mode & S_IWUSR) {
        *p++ = 'w';
    } else {
        *p++ = '-';
    }
#else
    *p++ = '-';
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    switch (mode & (S_IXUSR | S_ISUID)) {
    case 0:
        *p++ = '-';
        break;
    case S_IXUSR:
        *p++ = 'x';
        break;
    case S_ISUID:
        *p++ = 'S';
        break;
    case S_IXUSR | S_ISUID:
        *p++ = 's';
        break;
    }
#else
    *p++ = '-';
#endif
    /* group */
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    if (mode & S_IRGRP) {
        *p++ = 'r';
    } else {
        *p++ = '-';
    }
#else
    *p++ = '-';
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    if (mode & S_IWGRP) {
        *p++ = 'w';
    } else {
        *p++ = '-';
    }
#else
    *p++ = '-';
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    switch (mode & (S_IXGRP | S_ISGID)) {
    case 0:
        *p++ = '-';
        break;
    case S_IXGRP:
        *p++ = 'x';
        break;
    case S_ISGID:
        *p++ = 'S';
        break;
    case S_IXGRP | S_ISGID:
        *p++ = 's';
        break;
    }
#else
    *p++ = '-';
#endif
    /* other */
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    if (mode & S_IROTH) {
        *p++ = 'r';
    } else {
        *p++ = '-';
    }
#else
    *p++ = '-';
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    if (mode & S_IWOTH) {
        *p++ = 'w';
    } else {
        *p++ = '-';
    }
#else
    *p++ = '-';
#endif
#if !defined(_WIN32)  // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    switch (mode & (S_IXOTH | S_ISVTX)) {
    case 0:
        *p++ = '-';
        break;
    case S_IXOTH:
        *p++ = 'x';
        break;
    case S_ISVTX:
        *p++ = 'T';
        break;
    case S_IXOTH | S_ISVTX:
        *p++ = 't';
        break;
    }
#else
    *p++ = '-';
#endif
    *p++ = ' ';                 /* will be a '+' if ACL's implemented */
    *p = '\0';
}

QList<ArchiveFile> DataBase::scanArchive( QString path, ArchiveType type ) {
    QList<ArchiveFile> filelist;
    filelist.clear();

    if (type == Archive_tar || type == Archive_targz || type == Archive_tarbz2) {
        int use_zlib = 0;
        // int use_bzip2 = 0;
        int verbose = 0;
        int use_gnu = 0;
        if (type == Archive_targz) {
            use_zlib = 1;
        }
        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "scanning archive " << qPrintable( path ) << ",  type:" << type;
        }

        if (showProgressedFileInStatus) {
            emit pathExtraInfoAppend( tr( "scanning archive" ));
        }

        if (displayCurrentScannedFileInTray) {
            emit fileScanned( tr( "scanning archive" ) + ": " + path );
        }

        int i = 0;
        TAR *t = NULL;
        int tar_open_ret = -1;
        if (type == Archive_tar) {
            tar_open_ret = tar_open( &t, path.toLocal8Bit().data(), (NULL), O_RDONLY, 0, (verbose ? TAR_VERBOSE : 0) | (use_gnu ? TAR_GNU : 0));
        }
        if (type == Archive_targz) {
            tar_open_ret = tar_open( &t, path.toLocal8Bit().data(), (&gztype), O_RDONLY, 0, (verbose ? TAR_VERBOSE : 0) | (use_gnu ? TAR_GNU : 0));
        }
        if (type == Archive_tarbz2) {
            tar_open_ret = tar_open( &t, path.toLocal8Bit().data(), (&bztype), O_RDONLY, 0, (verbose ? TAR_VERBOSE : 0) | (use_gnu ? TAR_GNU : 0));
        }
        if (*DEBUG_INFO_ENABLED) {
            fprintf( stderr, "tar_open() tar_open_ret: %d\n", tar_open_ret );
        }

        if (tar_open_ret == -1) {
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "scanning archive" << qPrintable( path ) << "failed";
            }
            return filelist;
        } else {
            while ((i = th_read( t )) == 0) {
                //  -rw-r--r-- crissi   crissi       29656 Mar 17  8:33 2009 home/crissi/Desktop/file.gif
                ArchiveFile af;
                if (*DEBUG_INFO_ENABLED) {
                    if (type == Archive_targz) {
                        qDebug() << "file inside tar.gz:" << t->th_buf.name;
                    } else if (type == Archive_tarbz2) {
                        qDebug() << "file inside tar.bz2:" << t->th_buf.name;
                    } else if (type == Archive_tar) {
                        qDebug() << "file inside tar:" << t->th_buf.name;
                    }
                }
                if (showProgressedArchiveFileInStatus) {
                    emit pathExtraInfoAppend( tr( "scanning archive, file:" ) + " " + QString( t->th_buf.name ));
                }

                char modestring[20];
                struct passwd *pw;
                struct group *gr;
                uid_t uid;
                gid_t gid;
                QString username;
                QString groupname;
                time_t mtime;
                struct tm *mtm;

#ifdef HAVE_STRFTIME
                char timebuf[18];
#else
                // const char *months[] = {
                //	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                //	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                // };
#endif

                uid = th_get_uid( t );
                pw = getpwuid( uid );
                if (pw == NULL) {
                    username.setNum( uid );
                } else {
                    username = QString( pw->pw_name );
                }

                gid = th_get_gid( t );
                gr = getgrgid( gid );
                if (gr == NULL) {
                    groupname.setNum( gid );
                } else {
                    groupname = QString( gr->gr_name );
                }
                mode_t m1 = th_get_mode( t );
                strmode( m1, modestring );
//                              af.fileattr.sprintf("%.10s %-8.8s %-8.8s ", modestring, username.toLocal8Bit().constData(), groupname.toLocal8Bit().constData());
                af.fileattr = QString( modestring );
                if (af.fileattr.size() == 9) {
                    af.fileattr = " " + af.fileattr;
                }
                if (TH_ISCHR( t ) || TH_ISBLK( t )) {
                    af.filetype = tr( "device " );
                    af.filetype += QString().sprintf( "%3d, %3d ", th_get_devmajor( t ), th_get_devminor( t ));
                }
                af.size = (long)th_get_size( t );

                mtime = th_get_mtime( t );
                mtm = localtime( &mtime );

                af.date.setTime_t( mtime );
                af.path = QString().sprintf( "%s", th_get_pathname( t ));

#if !defined(_WIN32)
                if (TH_ISSYM( t ) || TH_ISLNK( t )) {
                    if (TH_ISSYM( t )) {
                        af.filetype = QString().sprintf( " -> " );
                    } else {
                        af.filetype = tr( " link to " );
                    }
                    if ((t->options & TAR_GNU) && t->th_buf.gnu_longlink != NULL) {
                        af.filetype += QString().sprintf( "%s", t->th_buf.gnu_longlink );
                    } else {
                        af.filetype += QString().sprintf( "%.100s", t->th_buf.linkname );
                    }
                }
#endif


                filelist.append( af );
                progress( pww );

                if (TH_ISREG( t ) && tar_skip_regfile( t ) == 0) {
                    // fprintf(stderr, "tar_skip_regfile(): %s\n", strerror(errno));
                }
                if (pww->appl->hasPendingEvents()) {
                    pww->appl->processEvents();
                }
                if (pww->doCancel) {
                    return filelist;
                }
            }
            if (*DEBUG_INFO_ENABLED) {
                qDebug() << "reading" << qPrintable( path ) << "done.";
            }
        }
        tar_close( t );
    }
#ifdef USE_LIB7ZIP
    if (type == Archive_lib7zip) {
        C7ZipLibrary lib;
        C7ZipArchive *pArchive = NULL;
        WStringArray exts;
        char file_attr[100];
        file_attr[0] = '\0';

        if (!lib.Initialize()) {
            // fprintf(stderr, "lib7zip initialize failed, lib7zip scanning disabled\n");
            doScanArchiveLib7zip = false;
        }

        if (!lib.GetSupportedExts( exts )) {
            // fprintf(stderr, "lib7zip get supported exts failed, lib7zip scanning disabled\n");
            doScanArchiveLib7zip = false;
        } else {
            for (WStringArray::const_iterator extIt = exts.begin(); extIt != exts.end(); extIt++)
                Lib7zipTypes.append( QString().fromWCharArray((*extIt).c_str()));
            // qDebug() << "lib7zip supported extensions:" << qPrintable(Lib7zipTypes.join(" "));
        }

        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "scanning archive " << qPrintable( path ) << ",  type:" << type;
        }

        if (showProgressedFileInStatus) {
            emit pathExtraInfoAppend( tr( "scanning archive" ));
        }

        if (displayCurrentScannedFileInTray) {
            emit fileScanned( tr( "scanning archive" ) + ": " + path );
        }

        TestInStream stream( path.toLocal8Bit().data(), path.toLower().section( '.', -1 ).toStdWString());
        if (lib.OpenArchive( &stream, &pArchive )) {
            unsigned int numItems = 0;
            pArchive->GetItemCount( &numItems );
            printf( "" );              // important!!!
            // printf("items found: %d\n", numItems);

            for (unsigned int i = 0; i < numItems; i++) {
                C7ZipArchiveItem *pArchiveItem = NULL;
                if (pArchive->GetItemInfo( i, &pArchiveItem )) {
                    //                          printf("%d,%ls,%d\n", pArchiveItem->GetArchiveIndex(),
                    //                                          pArchiveItem->GetFullPath().c_str(),
                    //                                          pArchiveItem->IsDir());

                    ArchiveFile af;
                    QString filetype;
                    bool result = false;
                    unsigned __int64 attr = 0;
                    result = pArchiveItem->GetUInt64Property( lib7zip::kpidAttrib, attr );
                    QString attr2 = "";
                    af.fileattr = "";

                    if (pArchiveItem->IsDir()) {
                        af.fileattr += "d";
                    } else {
                        af.fileattr += "-";
                    }
                    if ((result & LIB7ZIP_FILE_ATTRIBUTE_READONLY) != 0) {
                        // read
                        if (pArchiveItem->IsDir()) {
                            attr2 += "r-x";
                        } else {
                            attr2 += "r--";
                        }
                    } else {
                        // read + write
                        if (pArchiveItem->IsDir()) {
                            attr2 += "rwx";
                        } else {
                            attr2 += "rw-";
                        }
                    }
                    if ((result & LIB7ZIP_FILE_ATTRIBUTE_HIDDEN) != 0) {
                        // hidden
                    }
                    if ((result & LIB7ZIP_FILE_ATTRIBUTE_SYSTEM) != 0) {
                        // system
                    }
                    if ((result & LIB7ZIP_FILE_ATTRIBUTE_ARCHIVE) != 0) {
                        // archive
                    }

                    // fake group
                    af.fileattr += attr2;
                    af.fileattr += " ";

                    // fake other
                    af.fileattr += attr2;
                    af.fileattr += " ";

                    // qDebug() << "file: " << qPrintable(QString::fromWCharArray ( pArchiveItem->GetFullPath().c_str() )) << ", attr:" << qPrintable(af.fileattr);
                    wstring user = L"";
                    result = pArchiveItem->GetStringProperty( lib7zip::kpidUser, user );
                    QString user2 = QString::fromWCharArray( user.c_str());
                    if (user2.isEmpty()) {
                        user2 = "unknown";
                    }

                    wstring group = L"";
                    result = pArchiveItem->GetStringProperty( lib7zip::kpidGroup, user );
                    QString group2 = QString::fromWCharArray( group.c_str());
                    if (group2.isEmpty()) {
                        group2 = "unknown";
                    }

                    unsigned __int64 size = 0;
                    result = pArchiveItem->GetUInt64Property( lib7zip::kpidSize, size );
                    size = pArchiveItem->GetSize();

                    unsigned __int64 compressed_size = 0;
                    result = pArchiveItem->GetUInt64Property( lib7zip::kpidPackSize, compressed_size );

                    unsigned __int64 mtime = 0;
                    result = pArchiveItem->GetFileTimeProperty( lib7zip::kpidMTime, mtime );
                    // printf("%ld\n", mtime);
                    QDateTime mtime2;
                    mtime2.setTime_t( 0 );
                    unsigned long int offset = -116444736;
                    mtime2 = mtime2.addSecs((mtime / 10000000) + (offset * 100));                            // microsoft time stamp: diff from 1. Jan 1601, 100 ns (steps)
                    //                          mtime2 = mtime2.addSecs(-11644473600);

                    QString path = QString::fromWCharArray( pArchiveItem->GetFullPath().c_str());
                    if (*DEBUG_INFO_ENABLED) {
                        qDebug() << "file inside archive:" << qPrintable( path );
                    }

                    if (showProgressedArchiveFileInStatus) {
                        emit pathExtraInfoAppend( tr( "scanning archive, file:" ) + " " + QString( path ));
                    }


                    //  -rw-r--r-- crissi   crissi       29656 Mar 17  8:33 2009 home/crissi/Desktop/file.gif
                    // QString line = QString().sprintf("%s %s %s    %ld %s %s", file_attr, user2.toLocal8Bit().data(), group2.toLocal8Bit().data(), size, mtime2.toString("MMM d h:s yyyy").toLocal8Bit().data(), path.toLocal8Bit().data() );

                    if (af.fileattr.size() == 9) {
                        af.fileattr = " " + af.fileattr;
                    }

                    af.user = user2;
                    af.group = group2;
                    af.size = size;
                    af.date = mtime2;
                    af.path = path;
                    af.filetype = filetype;
                    filelist.append( af );
                    progress( pww );
                }
                if (pww->appl->hasPendingEvents()) {
                    pww->appl->processEvents();
                }
                if (pww->doCancel) {
                    return filelist;
                }
            }
        }
        lib.Deinitialize();
        if (*DEBUG_INFO_ENABLED) {
            qDebug() << "reading" << qPrintable( path ) << "done.";
        }
    }
#endif
    return filelist;
}

void DataBase::addLnk( const char *loc ) {
    QString catname;
    gzFile f = NULL;
    FileReader *fw = NULL;

    DEBUG_INFO_ENABLED = init_debug_info();

    Node *tmp = root;
    Node root2( HC_CATALOG, NULL );      // Malloc root node
    root2.data = (void *)new DBCatalog();
    Node *n = new Node( HC_CATLNK, root );

    /* Reading database name from the pointed file */
    f = gzopen( loc, "rb" );
    if (f == NULL) {
        QMessageBox::warning( NULL, tr( "Error" ), tr( "I can't open the file: %1" ).arg( loc ));
        return;
    }

    // check free memory
    char testbuffer[1024];
    long long int filesize = 0;
    int readcount = 0;
    readcount = gzread( f, testbuffer, 1024 );
    while (readcount != 0) {
        filesize += readcount;
        // if(*DEBUG_INFO_ENABLED)
        //  qDebug() << "readcount:" << readcount;
        readcount = gzread( f, testbuffer, 1024 );
        progress( pww );
    }
    gzrewind( f );
    if (*DEBUG_INFO_ENABLED) {
        qDebug() << "detected uncompressed size:" << filesize;
    }

    char *allocated_buffer = NULL;
    allocated_buffer = (char *)malloc( filesize );
    if (allocated_buffer == NULL) {
        // fail => no enough memory
        QMessageBox::warning( NULL, tr( "Error" ), tr( "Not enough memory to open the file: %1" ).arg( QString( loc )));
        return;
    } else {
    }
    /* end memtest */

    fw = new FileReader( f, allocated_buffer, filesize );
    fw->pww = pww;
    fw->sp = &root2;
    catname = fw->getCatName();
    if (catname.isEmpty()) {
        QMessageBox::warning( NULL, tr( "Error" ), tr( "Error while parsing file: %1" ).arg( loc ));
        delete fw;
        gzclose( f );
        return;
    }
    gzclose( f );
    delete fw;

    /* end reading from the file */

    catname.prepend( "@" );

    n->data = new DBCatLnk( catname, mstr( loc ), "" );

    if (root->child == NULL) {
        root->child = n;
    } else {
        tmp = root;
        while (tmp->next != NULL)
            tmp = tmp->next;
        tmp->next = n;
    }

    root->touchDB();
}
/***************************************************************************/
void DataBase::deleteNode( Node *d ) {
    Node *p;

    if (d == NULL) {
        return;
    }
    p = d->parent;
    if (p->child == d) {
        p->child = p->child->next;
        d->next = NULL;
        delete d;
    } else {
        p = p->child;
        while (p->next != d)
            p = p->next;
        p->next = p->next->next;
        d->next = NULL;
        delete d;
    }
    ((DBCatalog *)((getRootNode())->data))->touch();
}
/***************************************************************************/
double DataBase::getSize( Node *s, int level ) {
    double v = 0.0;

    if (s->type == HC_FILE) {
        switch (((DBFile *)(s->data))->sizeType) {
        case UNIT_BYTE:
            v += (((DBFile *)(s->data))->size);
            break;                     // byte
        case UNIT_KBYTE:
            v += (((DBFile *)(s->data))->size) * 1024.0;
            break;                     // Kb
        case UNIT_MBYTE:
            v += (((DBFile *)(s->data))->size) * 1024.0 * 1024.0;
            break;                     // Mb
        case UNIT_GBYTE:
            v += (((DBFile *)(s->data))->size) * 1024.0 * 1024.0 * 1024.0;
            break;                     // Gb
        case UNIT_TBYTE:
            v += (((DBFile *)(s->data))->size) * 1024.0 * 1024.0 * 1024.0 * 1024.0;
            break;                     // Tb
        }
    }
    if (s->child != NULL) {
        v += getSize( s->child, level + 1 );
    }
    if (level != 0) {
        if (s->next != NULL) {
            v += getSize( s->next, level + 1 );
        }
    }

    return v;
}

unsigned long DataBase::getCountDirs( Node *s, int level ) {
    unsigned long v = 0;

    if (s->type == HC_DIRECTORY) {
        v = 1;
    }
    if (s->child != NULL) {
        v += getCountDirs( s->child, level + 1 );
    }
    if (level != 0) {
        if (s->next != NULL) {
            v += getCountDirs( s->next, level + 1 );
        }
    }
    return v;
}

unsigned long DataBase::getCountFiles( Node *s, int level ) {
    unsigned long v = 0;

    if (s->type == HC_FILE) {
        v = 1;
    }
    if (s->child != NULL) {
        v += getCountFiles( s->child, level + 1 );
    }
    if (level != 0) {
        if (s->next != NULL) {
            v += getCountFiles( s->next, level + 1 );
        }
    }
    return v;
}

void DataBase::setNice( bool nic ) {
    nicef = nic;
}
/*****************************************************************/
/* ???
 * int compare(char *a,char *b)
 * {
 * int i;
 * for(i=0;;i++)
 * {
 *   if(a[i]>b[i]) return 0;
 *   if(a[i]<b[i]) return 1;
 *   if(a[i]=='\0') return 1;
 *   if(b[i]=='\0') return 0;
 * }
 * }
 */
Node *DataBase::getMOnPos( int p ) {
    int i;
    Node *step;

    step = root->child;
    for (i = 0; i < p; i++)
        step = step->next;
    return step;
}

void DataBase::sortM( int mode, bool ascending ) {
    Node *step;
    void *data;
    int length;
    int type;
    int i, j;

    if (root == NULL) {
        return;
    }
    length = 0;
    step = root->child;
    while (step != NULL) {
        length++;
        step = step->next;
    }

    for (i = 0; i < length; i++) {
        for (j = i; j < length; j++) {
            if (getMOnPos( i )->type == HC_CATLNK) {
                continue;
            } else {
                if (getMOnPos( j )->type == HC_CATLNK) {
                    ;                     /* nothing */
                } else {
                    switch (mode) {
                    case NUMBER:
                        if (ascending) {
                            if (((DBMedia *)(getMOnPos( i )->data))->number < ((DBMedia *)(getMOnPos( j )->data))->number) {
                                continue;
                            }
                        } else {
                            if (((DBMedia *)(getMOnPos( i )->data))->number > ((DBMedia *)(getMOnPos( j )->data))->number) {
                                continue;
                            }
                        }
                        break;
                    case NAME:
                        if (0 < QString::localeAwareCompare(
                                ((DBMedia *)(getMOnPos( i )->data))->name,
                                ((DBMedia *)(getMOnPos( j )->data))->name )) {
                            continue;
                        }
                        break;
                    case TYPE:
                        if (ascending) {
                            if (((DBMedia *)(getMOnPos( i )->data))->type <= ((DBMedia *)(getMOnPos( j )->data))->type) {
                                continue;
                            }
                        } else {
                            if (((DBMedia *)(getMOnPos( i )->data))->type >= ((DBMedia *)(getMOnPos( j )->data))->type) {
                                continue;
                            }
                        }
                        break;
                    case TIME:
                        if (ascending) {
                            if (((DBMedia *)(getMOnPos( i )->data))->modification <= ((DBMedia *)(getMOnPos( j )->data))->modification) {
                                continue;
                            }
                        } else {
                            if (((DBMedia *)(getMOnPos( i )->data))->modification >= ((DBMedia *)(getMOnPos( j )->data))->modification) {
                                continue;
                            }
                        }
                        break;
                    }
                }
            }
            // swap
            step = getMOnPos( i )->child;
            data = getMOnPos( i )->data;
            type = getMOnPos( i )->type;

            getMOnPos( i )->child = getMOnPos( j )->child;
            getMOnPos( i )->data = getMOnPos( j )->data;
            getMOnPos( i )->type = getMOnPos( j )->type;

            getMOnPos( j )->child = step;
            getMOnPos( j )->data = data;
            getMOnPos( j )->type = type;
        }
    }
    ((DBCatalog *)root->data)->sortedBy = mode;
    root->touchDB();
}

void DataBase::setShowProgressedFileInStatus( bool showProgressedFileInStatus ) {
    this->showProgressedFileInStatus = showProgressedFileInStatus;
}


void DataBase::setShowProgressedArchiveFileInStatus( bool showProgressedArchiveFileInStatus ) {
    this->showProgressedArchiveFileInStatus = showProgressedArchiveFileInStatus;
}


/*************************************************************************/
const char *shortMonthName0( int i ) {
    switch (i) {
    case 1:
        return "Jan";
    case 2:
        return "Feb";
    case 3:
        return "Mar";
    case 4:
        return "Apr";
    case 5:
        return "May";
    case 6:
        return "Jun";
    case 7:
        return "Jul";
    case 8:
        return "Aug";
    case 9:
        return "Sep";
    case 10:
        return "Oct";
    case 11:
        return "Nov";
    case 12:
        return "Dec";
    }
    return "Err";
}

/* ???
 * char *getCurrentTime(void)
 * {
 * char *m=new char[30];
 * QTime t;
 * QDate d;
 * t = QTime::currentTime();
 * d = QDate::currentDate();
 *  sprintf(m,// "%s.%d %d:%d %d"
 *  "%s %02d %02d:%02d %d",(const char *)
 *  (shortMonthName0(d.month())),d.day(),t.hour(),t.minute(),d.year());
 *
 *
 * return recodeI(m,&dbb6);
 * }
 */
/* ???
 * char *getTime(QDateTime dt)
 * {
 * char *m=new char[30];
 * QTime t;
 * QDate d;
 * t = dt.time();
 * d = dt.date();
 * sprintf(m,// "%s.%d %d:%d %d"
 *         "%s %02d %02d:%02d %d",(const char *)
 *  (shortMonthName0(d.month())),d.day(),t.hour(),t.minute(),d.year());
 *
 * return recodeI(m,&dbb6);
 * }
 */

/*********************************************************************************
*  Import functions:
*********************************************************************************/
Node *DataBase::getMediaNode( QString name ) {
    Node *t = NULL;

    t = root->child;     // first media
    while (t != NULL) {
        if (t->type != HC_MEDIA) {
            continue;
        }
        if (t->getNameOf() == name) {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

Node *DataBase::putMediaNode( QString name, int number, QString owner, int type, QString comment, QDateTime modification, QString category ) {
    Node *t = NULL, *n = NULL;

    n = new Node( HC_MEDIA, root );
    n->data = (void *)new DBMedia( name, number, owner, type, comment, modification, category );

    if (root->child == NULL) {
        root->child = n;
    } else {
        t = root->child;         // first media
        while (t->next != NULL)
            t = t->next;
        t->next = n;
    }
    return n;
}

Node *DataBase::getMediaNode( int id ) {
    Node *t = NULL;

    t = root->child;     // first media
    while (t != NULL) {
        if (((DBMedia *)(t->data))->number == id) {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

Node *DataBase::getDirectoryNode( Node *meddir, QString name ) {
    Node *t = NULL;

    t = meddir->child;
    while (t != NULL) {
        if (t->type != HC_DIRECTORY) {
            t = t->next;
            continue;
        }
        if (t->getNameOf() == name) {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

Node *DataBase::putDirectoryNode( Node *meddir, QString name, QDateTime modification, QString comment, QString category ) {
    Node *t = NULL, *n = NULL;

    n = new Node( HC_DIRECTORY, meddir );
    n->data = (void *)new DBDirectory( name, modification, comment, category );

    if (meddir->child == NULL) {
        meddir->child = n;
    } else {
        t = meddir->child;
        while (t->next != NULL)
            t = t->next;
        t->next = n;
    }
    return n;
}

Node *DataBase::getFileNode( Node *directory, QString name ) {
    Node *t = NULL;

    t = directory->child;
    while (t != NULL) {
        if (t->type != HC_FILE) {
            t = t->next;
            continue;
        }
        if (t->getNameOf() == name) {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

Node *DataBase::putFileNode( Node *directory, QString name, QDateTime modification, QString comment, int sizeType, double size, QString category, QList<ArchiveFile> archivecontent, QString fileinfo ) {
    Node *t = NULL, *n = NULL;

    n = new Node( HC_FILE, directory );
    n->data = (void *)
              new DBFile( name, modification, comment, size, sizeType, category, archivecontent, fileinfo );

    if (directory->child == NULL) {
        directory->child = n;
    } else {
        t = directory->child;
        while (t->next != NULL)
            t = t->next;
        t->next = n;
    }
    return n;
}


Node *DataBase::putTagInfo( Node *file, QString artist, QString title, QString comment, QString album, QString year, int tnumber ) {
    Node *t = NULL, *n = NULL;

    n = new Node( HC_MP3TAG, NULL );
    n->data = (void *)
              new DBMp3Tag( artist, title, comment, album, year, tnumber );

    if (((DBFile *)(file->data))->prop == NULL) {
        ((DBFile *)(file->data))->prop = n;
    } else {
        t = ((DBFile *)(file->data))->prop;
        while (t->next != NULL)
            t = t->next;
        t->next = n;
    }
    return n;
}

/* Archive file class (file inside archive */
ArchiveFile::ArchiveFile ( QString fileattr, QString user, QString group, long long int size, QDateTime date, QString path, QString filetype ) {
    this->fileattr = fileattr;
    this->user = user;
    this->group = group;
    this->size = size;
    this->date = date;
    this->path = path;
    this->filetype = filetype;
    this->div = '?';
}

ArchiveFile::ArchiveFile ( const ArchiveFile& af ) : QObject() {
    this->fileattr = af.fileattr;
    this->user = af.user;
    this->group = af.group;
    this->size = af.size;
    this->date = af.date;
    this->path = af.path;
    this->filetype = af.filetype;
    this->div = af.div;
}

ArchiveFile& ArchiveFile::operator =( const ArchiveFile& af ) {
    this->fileattr = af.fileattr;
    this->user = af.user;
    this->group = af.group;
    this->size = af.size;
    this->date = af.date;
    this->path = af.path;
    this->filetype = af.filetype;
    this->div = af.div;
    return *this;
}

void ArchiveFile::setDbString( QString DbString ) {
    // from db
    if (DbString.at( 0 ) == this->div) {
        DbString = DbString.right( DbString.size() - 2 );
        qDebug() << "first char was sep, skipping, new DbString:" << DbString.toLocal8Bit().constData();
    }
    QStringList fileentry = DbString.split( this->div );
//      qDebug() << "DbString: " << qPrintable(DbString) << "count of " << qPrintable(QString(this->div)) << ":" << DbString.count(this->div);
//      qDebug() << "FileEntry (size: " << fileentry.size() << "):" << qPrintable(fileentry.join(" "));
//      for ( int i = 0; i < fileentry.size(); ++i )
//              qDebug() << "fileentry[" << i << "]:" << fileentry.at ( i ).toLocal8Bit().constData();
//

    if (fileentry.size() == 8) {
        this->fileattr = fileentry.at( 0 );
        this->user = fileentry.at( 1 );
        this->group = fileentry.at( 2 );
        this->size = fileentry.at( 3 ).toLongLong();
        this->date = QDateTime().fromString( fileentry.at( 4 ), "MMM d h:s yyyy" );
        this->path = fileentry.at( 5 );
        this->filetype = fileentry.at( 6 );
//              qDebug() << "FileEntry(8):" << qPrintable(toPrettyString());
    }
}

QString ArchiveFile::toPrettyString( bool showAttr, bool showUser, bool showGroup, bool showSize, bool showDate, bool showFileType, bool doHtmlTableLine, int fontsize ) {
    QString ret;

    if (doHtmlTableLine) {
        ret += "<tr class=\"tableline\">";
        if (showAttr) {
            ret += "<td>";
            ret += QString( fileattr + "\t" );
            ret += "</td>";
        }
        if (showUser) {
            ret += "<td style=\"font-size:" + QString().setNum( fontsize ) + "pt;\">";
            ret += QString( user + "\t" );
            ret += "</td>";
        }
        if (showGroup) {
            ret += "<td style=\"font-size:" + QString().setNum( fontsize ) + "pt;\">";
            ret += QString( group + "\t" );
            ret += "</td>";
        }
        if (showSize) {
            double s = size;
            int st = UNIT_BYTE;
            if (size >= (double)SIZE_ONE_GBYTE * 1024.0) {
                s = size / SIZE_ONE_GBYTE / 1024.0;
                st = UNIT_TBYTE;
            } else {
                if (size >= (double)SIZE_ONE_GBYTE && size < (double)SIZE_ONE_GBYTE * 1024.0) {
                    s = size / SIZE_ONE_GBYTE;
                    st = UNIT_GBYTE;
                } else {
                    if (size >= (double)SIZE_ONE_MBYTE && size < (double)SIZE_ONE_GBYTE) {
                        s = size / SIZE_ONE_MBYTE;
                        st = UNIT_MBYTE;
                    } else {
                        if (size >= (double)SIZE_ONE_KBYTE && size < (double)SIZE_ONE_MBYTE) {
                            s = size / SIZE_ONE_KBYTE;
                            st = UNIT_KBYTE;
                        } else {
                            s = size;
                            st = UNIT_BYTE;
                        }
                    }
                }
            }
            QString ret_size_str;
            ret_size_str.sprintf( " %.2f ", s );
            ret_size_str += getSType( st, true );
            ret += "<td align=\"right\" style=\"font-size:" + QString().setNum( fontsize ) + "pt;\">";
            ret += ret_size_str;
            ret += "</td>";
        }
        if (showDate) {
            ret += "<td align=\"right\" style=\"font-size:" + QString().setNum( fontsize ) + "pt;\">";
            ret += QString( date.toString( "MMM d h:s yyyy" ));
            ret += "</td>";
        }

        ret += "<td style=\"font-size:" + QString().setNum( fontsize ) + "pt;\">";
        ret += QString( path );
        ret += "</td>";

        if (showFileType) {
            ret += "<td style=\"font-size:" + QString().setNum( fontsize ) + "pt;\">";
            ret += QString( filetype );
            ret += "</td>";
        }
        ret += "</tr>";
    } else {
        if (showAttr) {
            ret += QString( fileattr + "\t" );
        }
        if (showUser) {
            ret += QString( user + "\t" );
        }
        if (showGroup) {
            ret += QString( group + "\t" );
        }
        if (showSize) {
            double s = size;
            int st = UNIT_BYTE;
            if (size >= (double)SIZE_ONE_GBYTE * 1024.0) {
                s = size / SIZE_ONE_GBYTE / 1024.0;
                st = UNIT_TBYTE;
            } else {
                if (size >= (double)SIZE_ONE_GBYTE && size < (double)SIZE_ONE_GBYTE * 1024.0) {
                    s = size / SIZE_ONE_GBYTE;
                    st = UNIT_GBYTE;
                } else {
                    if (size >= (double)SIZE_ONE_MBYTE && size < (double)SIZE_ONE_GBYTE) {
                        s = size / SIZE_ONE_MBYTE;
                        st = UNIT_MBYTE;
                    } else {
                        if (size >= (double)SIZE_ONE_KBYTE && size < (double)SIZE_ONE_MBYTE) {
                            s = size / SIZE_ONE_KBYTE;
                            st = UNIT_KBYTE;
                        } else {
                            s = size;
                            st = UNIT_BYTE;
                        }
                    }
                }
            }
            QString ret_size_str;
            ret_size_str.sprintf( " %.2f ", s );
            ret_size_str += getSType( st, true );
            ret += ret_size_str;
            ret += "\t";
        }
        if (showDate) {
            ret += QString( date.toString( "MMM d h:s yyyy" ) + "\t" );
        }
        ret += QString( path + "\t" );
        if (showFileType) {
            ret += QString( filetype );
        }
    }
    return ret;
}

QString ArchiveFile::toDbString() {
    QString ret = "";

    ret += QString( fileattr + this->div );
    ret += QString( user + this->div );
    ret += QString( group + this->div );
    ret += QString( QString().setNum( size ) + this->div );
    ret += QString( date.toString( "MMM d h:s yyyy" ) + this->div );
    ret += QString( path + this->div );
    ret += QString( filetype + this->div );
    return ret;
}


