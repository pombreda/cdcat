/****************************************************************************
                             Hyper's CD Catalog
		A multiplatform qt and xml based catalog program

 Author    : Peter Deak (hyperr@freemail.hu)
 License   : GPL
 Copyright : (C) 2003 Peter Deak
****************************************************************************/

#include <stdio.h>
#include <ctype.h>

#include <expat.h>
#include <zlib.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qregexp.h>

#include <QStringList>
#include <qtextcodec.h>

#include "wdbfile.h"
#include "adddialog.h"
#include "dbase.h"
#include "cdcat.h"

#define XML_ENCODING   "UTF-8"
//#define XML_ENCODING   "ISO-8859-1"
#define BUFFSIZE	8192







#include <iostream>
using namespace std;





char encodeHex[] = "0123456789ABCDEF";
/*************************************************************************/

int getTypeFS ( const char *str ) {
    if ( str == NULL ) return -1;
    if ( !strcmp ( "CD"          ,str ) ) return CD;
    if ( !strcmp ( "DVD"         ,str ) ) return DVD;
    if ( !strcmp ( "HardDisc"    ,str ) ) return HARDDISC;
    if ( !strcmp ( "floppy"      ,str ) ) return FLOPPY;
    if ( !strcmp ( "NetworkPlace",str ) ) return NETPLACE;
    if ( !strcmp ( "flashdrive"  ,str ) ) return FLASHDRV;
    if ( !strcmp ( "other"       ,str ) ) return OTHERD;
    return UNKNOWN;
}

const char *getMType ( int t ) {
    switch ( t ) {
    case UNKNOWN : return "unknown";
    case CD      : return "CD";
    case DVD     : return "DVD";
    case HARDDISC: return "HardDisc";
    case FLOPPY  : return "floppy";
    case NETPLACE: return "NetworkPlace";
    case FLASHDRV: return "flashdrive";
    case OTHERD  : return "other";
    }
    return NULL;
}

float getSizeFS ( const char *str ) {
    float  r=0;
    char   s[10];

    strcpy ( s,"" );
    if ( str == NULL ) return -1;
    if ( sscanf ( str,"%f %s",&r,s ) != 2 )
        return -1;
    return r;
}

int   getSizetFS ( const char *str ) {
//   int scancount = 0;
    float  r;

    if ( str == NULL ) return -1;
//     cerr << "str: \"" << str << "\"" << endl;
    QString unit;
    QStringList l = QString ( str ).simplified().split ( ' ' );
    unit = l.at ( 1 );
    r = l.at ( 0 ).toFloat();
    //cerr << "l: " << qPrintable(l.join(",")) << endl;
    //cerr << "unit: " << qPrintable(unit) << endl;

    if ( unit == "byte" ) return BYTE;
    if ( unit == "Kb" ) return KBYTE;
    if ( unit == "Mb" ) return MBYTE;
    if ( unit == "Gb" ) return GBYTE;
    return -1;
}

const char *getSType ( int t ) {
    switch ( t ) {
    case BYTE : return " byte";
    case KBYTE: return " Kb";
    case MBYTE: return " Mb";
    case GBYTE: return " Gb";
    }
    return NULL;
}

/*****************************************************************************
 WRITING FILE:
*****************************************************************************/
char * FileWriter::spg ( int spn ) {
    if ( spn>=50 )
        return spgtable[49];
    return spgtable[spn];
}

FileWriter::FileWriter ( gzFile ff,bool nicefp ) {
    int i;
    converter = QTextCodec::codecForName ( "utf8" );
    nicef = nicefp;
    level = 0;
    f = ff;

    spgtable= ( char ** ) malloc ( 50*sizeof ( char * ) );
    for ( i=0;i<50;i++ )
        spgtable[i]=strdup ( ( const char * ) ( QString().fill ( ' ',i ) ) );
}

FileWriter::~FileWriter() {
    int i;
    for ( i=0;i<50;i++ )
        free ( spgtable[i] );
    free ( spgtable );
}

char * FileWriter::to_cutf8 ( QString s ) {
    return  strdup ( converter->fromUnicode ( s.replace ( QRegExp ( "<" ),"{" )
                     .replace ( QRegExp ( ">" ),"}" )
                     .replace ( QRegExp ( "&" ),"&amp;" )
                     .replace ( QRegExp ( "\"" ),"\'" )
                                            ) );
 // char *ret;
  //*ret = s.data()->toLatin1();
 // return ret;
}

char * FileWriter::to_dcutf8 ( QDateTime d ) {
char *ret;
  QString o ( "" );
    QDate qdd=d.date();
    QTime qtt=d.time();

    o.sprintf ( "%d-%d-%d %d:%d:%d",
                qdd.year(),qdd.month(),qdd.day(),
                qtt.hour(),qtt.minute(),qtt.second() );

    return strdup ( converter->fromUnicode ( o ) );
	 
  //*ret = o.data()->toAscii();
  //return ret;
}

//---------------------------------

void FileWriter::commentWriter ( QString& c ) {
    char *c1;
    if ( c.isEmpty() )
        return;

    c1=to_cutf8 ( c );

    level++;
    gzprintf ( f,"%s<comment>",spg ( level ) );
    gzprintf ( f,"%s",c1 );
    gzprintf ( f,"%s</comment>\n",spg ( level ) );

    free ( c1 );
    level--;
}

int  FileWriter::writeDown ( Node *source ) {
    int i=0;
    switch ( source->type ) {
    case HC_UNINITIALIZED: return 1;

    case HC_CATALOG:       writeHeader();
        i=writeCatalog ( source );
        break;
    case HC_MEDIA:         i=writeMedia ( source );
        break;
    case HC_DIRECTORY:     i=writeDirectory ( source );
        break;
    case HC_FILE:          i=writeFile ( source );
        break;
    case HC_MP3TAG:        i=writeMp3Tag ( source );
        break;
    case HC_CONTENT:       i=writeContent ( source );
        break;
    case HC_CATLNK:        i=writeCatLnk ( source );
        break;
    }
    return i;
}

int  FileWriter::writeHeader ( void ) {
    level = 0;
    progress ( pww );
    gzprintf ( f,"<?xml version=\"1.0\" encoding=\"%s\"?>",XML_ENCODING );
    gzprintf ( f,"\n<!DOCTYPE HyperCDCatalogFile>\n\n" );
    gzprintf ( f,"\n<!-- CD Catalog Database file, generated by CdCat " );
    gzprintf ( f,"\n     Homepage: %s  Author: Pe'ter Dea'k  hyperr@freemail.hu)",HOMEPAGE );
    gzprintf ( f,"\n     Program-Version: %s  -->\n\n",VERSION,DVERS );
    return 0;
}

int  FileWriter::writeCatalog ( Node *source ) {
    char *c1,*c2,*c3;

    c1=to_cutf8 ( ( ( DBCatalog * ) ( source->data ) )->name );
    c2=to_cutf8 ( ( ( DBCatalog * ) ( source->data ) )->owner );
    c3=to_dcutf8 ( ( ( DBCatalog * ) ( source->data ) )->modification );
    progress ( pww );
    gzprintf ( f,"<catalog name=\"%s\" owner=\"%s\" time=\"%s\">\n",
               c1,c2,c3 );

    gzprintf ( f,"<datafile version=\"%s\"/>\n",
               DVERS );

    commentWriter ( ( ( DBCatalog * ) ( source->data ) )->comment );
    level++;
    if ( source->child != NULL ) writeDown ( source->child );
    level--;
    gzprintf ( f,"</catalog>\n" );
    if ( source->next  != NULL ) writeDown ( source->next );

    free ( c1 );
    free ( c2 );
    free ( c3 );
    return 0;
}

int  FileWriter::writeMedia ( Node *source ) {
    char *c1,*c2,*c3;

    c1=to_cutf8 ( ( ( DBMedia * ) ( source->data ) )->name );
    c2=to_cutf8 ( ( ( DBMedia * ) ( source->data ) )->owner );
    c3=to_dcutf8 ( ( ( DBMedia * ) ( source->data ) )->modification );
    progress ( pww );
    gzprintf ( f,"%s<media name=\"%s\" number=\"%d\" owner=\"%s\" type=\"%s\" time=\"%s\">\n",
               spg ( level ),c1,
               ( ( DBMedia * ) ( source->data ) )->number,
               c2,getMType ( ( ( DBMedia * ) ( source->data ) )->type ),c3 );

    commentWriter ( ( ( DBMedia * ) ( source->data ) )->comment );

    //write borrowing info if exists
    if ( ! ( ( ( ( DBMedia * ) ( source->data ) )->borrowing ).isEmpty() ) ) {
        char *c4 = to_cutf8 ( ( ( DBMedia * ) ( source->data ) )->borrowing );
        gzprintf ( f,"%s<borrow>%s</borrow>\n",
                   spg ( level ),c4 );
        free ( c4 );
    }
    level++;
    if ( source->child != NULL ) writeDown ( source->child );
    level--;
    gzprintf ( f,"%s</media>\n",spg ( level ) );
    if ( source->next  != NULL ) writeDown ( source->next );

    free ( c1 );
    free ( c2 );
    free ( c3 );

    return 0;
}

int  FileWriter::writeDirectory ( Node *source ) {
    char *c1,*c2;

    c1=to_cutf8 ( ( ( DBDirectory * ) ( source->data ) )->name );
    c2=to_dcutf8 ( ( ( DBDirectory * ) ( source->data ) )->modification );
    progress ( pww );
    gzprintf ( f,"%s<directory name=\"%s\" time=\"%s\">\n",
               spg ( level ),c1,c2 );

    commentWriter ( ( ( DBDirectory * ) ( source->data ) )->comment );

    level++;
    if ( source->child != NULL ) writeDown ( source->child );
    level--;
    gzprintf ( f,"%s</directory>\n",spg ( level ) );
    if ( source->next  != NULL ) writeDown ( source->next );

    free ( c1 );
    free ( c2 );
    return 0;
}

int  FileWriter::writeFile ( Node *source ) {
    char *c1,*c2;

    c1=to_cutf8 ( ( ( DBFile * ) ( source->data ) )->name );
    c2=to_dcutf8 ( ( ( DBFile * ) ( source->data ) )->modification );
    gzprintf ( f,"%s<file name=\"%s\" size=\"%.2f%s\" time=\"%s\">\n",
               spg ( level ),c1,
               ( ( DBFile * ) ( source->data ) )->size,
               getSType ( ( ( ( DBFile * ) ( source->data ) )->sizeType ) ),c2 );

    commentWriter ( ( ( DBFile * ) ( source->data ) )->comment );
    level++;
    if ( source->child != NULL )
        writeDown ( source->child );
    if ( ( ( DBFile * ) ( source->data ) )->prop != NULL )
        writeDown ( ( ( DBFile * ) ( source->data ) )->prop );
    level--;
    gzprintf ( f,"%s</file>\n",spg ( level ) );
    if ( source->next  != NULL ) writeDown ( source->next );
    free ( c1 );
    free ( c2 );
    return 0;
}

int  FileWriter::writeMp3Tag ( Node *source ) {
    char *c1,*c2,*c3,*c4,*c5;


    c1=to_cutf8 ( ( ( DBMp3Tag * ) ( source->data ) )->artist );
    c2=to_cutf8 ( ( ( DBMp3Tag * ) ( source->data ) )->title );
    c3=to_cutf8 ( ( ( DBMp3Tag * ) ( source->data ) )->album );
    c4=to_cutf8 ( ( ( DBMp3Tag * ) ( source->data ) )->year );
    gzprintf ( f,"%s<mp3tag artist=\"%s\" title=\"%s\" album=\"%s\"  year=\"%s\">",
               spg ( level ),c1,c2,c3,c4 );

    c5=to_cutf8 ( ( ( DBMp3Tag * ) ( source->data ) )->comment );
    gzprintf ( f,"%s",c5 );
    gzprintf ( f,"%s</mp3tag>\n",spg ( level ) );
    if ( source->next  != NULL ) writeDown ( source->next );

    free ( c1 );
    free ( c2 );
    free ( c3 );
    free ( c4 );
    free ( c5 );
    return 0;
}

int  FileWriter::writeContent ( Node *source ) {
    unsigned long i;
    unsigned char c;
    gzprintf ( f,"%s<content>",spg ( level ) );

    for ( i=0;i< ( ( DBContent * ) ( source->data ) )->storedSize;i++ ) {
        c = ( ( DBContent * ) ( source->data ) )->bytes[i];
        gzputc ( f,encodeHex[ ( c & 0xF0 ) >>4] );
        gzputc ( f,encodeHex[ c & 0x0F     ] );
    }
    gzprintf ( f,"%s</content>\n",spg ( level ) );
    if ( source->next  != NULL ) writeDown ( source->next );
    return 0;
}

int  FileWriter::writeCatLnk ( Node *source ) {
    char *c1;
    QString wr = ( ( DBCatLnk * ) ( source->data ) )->name;
    progress ( pww );
    c1=to_cutf8 ( wr.remove ( 0,1 ) ); //to remove the leading @
    gzprintf ( f,"%s<catlnk name=\"%s\" location=\"%s\" >\n",
               spg ( level ),c1,
               ( ( DBCatLnk * ) ( source->data ) )->location );

    commentWriter ( ( ( DBCatLnk * ) ( source->data ) )->comment );
    gzprintf ( f,"%s</catlnk>\n",spg ( level ) );

    if ( source->next  != NULL ) writeDown ( source->next );
    free ( c1 );
    return 0;
}

/*****************************************************************************
 READING FILE:
*****************************************************************************/
int clearbuffer = 1;
unsigned long buffpos=0;
XML_Parser *pp = NULL;


QString FileReader::get_cutf8 ( char *s ) {
    QString ret;

    if ( s == NULL )
        return QString ( "" );
    //cerr<<"Start-converting |"<<s<<"|"<<endl;
    //ret = converter->toUnicode ( s );
    //cerr<<"End-converting   |" << qPrintable ( ret ) <<"|"<<endl;
	 ret =  QString(s);

    return ret;
}

QDateTime FileReader::get_dcutf8 ( char *s ) {
    QDateTime r;
    int year,month,day;
    int hour,minute,second;

    if ( sscanf ( s,"%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second ) == 6 ) {
        r.setDate ( QDate ( year,month,day ) );
        r.setTime ( QTime ( hour,minute,second ) );
        return r;
    }

    /* ERROR | OLD VERSION FOUND */
    return r;

// !!! Conversion ntfrom the old versions !!!
    /*
    char *getCurrentTime(void)
     {
      char *m=new char[30];
      QTime t;
      QDate d;
      t = QTime::currentTime();
      d = QDate::currentDate();
        sprintf(m,// "%s.%d %d:%d %d"
        "%s %02d %02d:%02d %d",(const char *)
        (shortMonthName0(d.month())),d.day(),t.hour(),t.minute(),d.year());


      return recodeI(m,&dbb6);
     }
    char *getTime(QDateTime dt)
     {
      char *m=new char[30];
      QTime t;
      QDate d;
      t = dt.time();
      d = dt.date();
      sprintf(m,// "%s.%d %d:%d %d"
               "%s %02d %02d:%02d %d",(const char *)
        (shortMonthName0(d.month())),d.day(),t.hour(),t.minute(),d.year());

      return recodeI(m,&dbb6);
     }
    */
}


unsigned char decodeHexa ( char a,char b ) //try to decode a hexadecimal byte to char very fast
{                                      //possible values a,b: "0123456789ABCDEF"  !!!
    unsigned char r=0;

    if ( a>='A' ) r = ( ( a-'A' ) +10 );
    else       r = ( a-'0' );
    r <<= 4;
    if ( b>='A' ) r += ( ( b-'A' ) +10 );
    else       r += ( b-'0' );

    return r;
}

float FileReader::getFloat ( const char **from,char *what,char *err ) {
    float r;
    int i;

    if ( from == NULL ) {
        errormsg = QString ( "Line %1: %2" ).arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err );
        error = 1;
        return 0;
    }
    for ( i=0;;i+=2 ) {
        if ( from[i] == NULL ) {
            errormsg = QString ( "Line %1: %2,I can't find \"%3\" attribute" )
                       .arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err ).arg ( what );
            error  = 1;
            return   0;
        }
        if ( !strcmp ( from[i],what ) ) {
            if ( from[i+1] == NULL ) {
                errormsg = QString ( "Line %1: %2" ).arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err );
                error  = 1;
                return 0;
            }
            if ( 1 != sscanf ( from[i+1],"%f",& r ) ) {
                errormsg = QString ( "Line %1: %2:I can't understanding \"%3\" attribute." )
                           .arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err ).arg ( what );
                error  = 1;
                return   0;
            }
            return r;
        }
    }
    return 0;
}

char * FileReader::getStr ( const char **from,char *what,char *err ) {
    int i;

    if ( from == NULL ) {
        errormsg = QString ( "Line %1: %2" ).arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err );
        error=1;
        return NULL;
    }
    for ( i=0;;i+=2 ) {
        if ( from[i] == NULL ) {
            errormsg = QString ( "Line %1: %2:I can't find \"%3\" attribute." )
                       .arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err ).arg ( what );
            error  = 1;
            return   NULL;
        }
        if ( !strcmp ( from[i],what ) ) {
            if ( from[i+1] == NULL ) {
                errormsg = QString ( "Line %1: %2" ).arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( err );
                error = 1;
                return NULL;
            }
            return ( ( char * ) ( from[i+1] ) );
        }
    }
    return NULL;
}

int FileReader::isthere ( const char **from,char *what ) {
    int i;

    if ( from == NULL )
        return 0;
    for ( i=0;;i+=2 ) {
        if ( from[i] == NULL )
            return 0;
        if ( !strcmp ( from[i],what ) )
            return 1;
    }
    return 0;
}

#define FREA ((FileReader *)(data))

DBMp3Tag *tmp_tagp=NULL;

static void start ( void *data, const char *el, const char **attr ) {
    QString ts1,ts2,ts3,ts4,ts5,ts6;
    QDateTime td1;
    float tf1;
    int   ti1;

    if ( FREA->error ) return;
    /*That case I found an error in file -> needs exit the pasing as fast as I can.*/
//     cerr <<"Start_start:"<<el<<endl;
    clearbuffer = 1;
    if ( !strcmp ( el,"catalog" ) ) {
        if ( FREA->insert ) return;

        ( ( DBCatalog * ) ( ( FREA->sp )->data ) ) ->name =
            FREA->get_cutf8 ( FREA->getStr ( attr,"name","Error while parsing \"catalog\" node" ) );
        if ( FREA->error ) return;

        ( ( DBCatalog * ) ( ( FREA->sp )->data ) ) ->owner=
            FREA->get_cutf8 ( FREA->getStr ( attr,"owner","Error while parsing \"catalog\" node" ) );
        if ( FREA->error ) return;

        ( ( DBCatalog * ) ( ( FREA->sp )->data ) ) ->modification=
            FREA->get_dcutf8 ( FREA->getStr ( attr,"time","Error while parsing \"catalog\" node" ) );
        if ( FREA->error ) return;
    }

    if ( !strcmp ( el,"datafile" ) ) {
        Node *tmp=FREA->sp;
        if ( FREA->insert ) return;

        while ( tmp->type != HC_CATALOG ) tmp=tmp->parent;

        ( ( DBCatalog * ) ( tmp->data ) ) ->fileversion =
            FREA->get_cutf8 ( FREA->getStr ( attr,"version","Error while parsing \"datafile\" node" ) );
        if ( FREA->error ) return;
    }

    if ( !strcmp ( el,"media" ) ) {
        Node *tt = FREA->sp->child;

        if ( tt == NULL ) FREA->sp->child = tt = new Node ( HC_MEDIA,FREA->sp );
        else {
            while ( tt->next != NULL ) tt = tt->next;
            tt->next =  new Node ( HC_MEDIA,FREA->sp );
            tt=tt->next;
        }

        if ( FREA->insert ) {
            int i,newnum,ser=0;
            Node *ch = tt->parent->child;
            QString newname;


            newname = FREA->get_cutf8 ( FREA->getStr ( attr,"name"  ,"Error while parsing \"media\" node" ) );
            if ( FREA->error ) return;

            if ( newname.startsWith ( "@" ) ) {
                FREA->errormsg = QString (
                                     "Line %1: The media name begin with \"@\".\n\
It is disallowed since cdcat version 0.98 !\n\
Please change it with an older version or rewrite it in the xml file!" )
                                 .arg ( XML_GetCurrentLineNumber ( *pp ) );

                FREA->error = 1;
                return;
            }

            newnum = ( int ) FREA->getFloat ( attr,"number","Error while parsing \"media\" node" );
            if ( FREA->error ) return;

            /*make the number unique*/
            for ( i=1;i==1; ) {
                i=0;
                for ( ch = tt->parent->child;ch != NULL; ch = ch->next ) {
                    if ( ch->data != NULL )
                        if ( ( ( DBMedia * ) ( ch->data ) )->number ==  newnum )
                            i=1;
                }
                if ( i ) newnum++;
            }

            /*make the name unique*/
            for ( i=1;i==1; ) {
                i=0;
                for ( ch = tt->parent->child;ch != NULL; ch = ch->next ) {
                    if ( ch->data != NULL )
                        if ( ( ( DBMedia * ) ( ch->data ) )->name == newname )
                            i=1;
                }
                if ( i ) {
                    ser++;
                    newname.append ( "#" + QString().setNum ( ser ) );
                }
            }


            /*Fill data part:*/
            ts1 = FREA->get_cutf8 ( FREA->getStr ( attr,"owner" ,"Error while parsing \"media\" node" ) );
            if ( FREA->error ) return;
            td1 = FREA->get_dcutf8 ( FREA->getStr ( attr,"time"  ,"Error while parsing \"media\" node" ) );
            if ( FREA->error ) return;

            ti1 = getTypeFS ( FREA->getStr ( attr,"type"  ,"Error while parsing \"media\" node" ) );
            if ( FREA->error ) return;
            if ( ti1 == UNKNOWN ) {
                FREA->errormsg = QString ( "Line %1: Unknown media type in the file. (\"%2\")" )
                                 .arg ( XML_GetCurrentLineNumber ( *pp ) )
                                 .arg ( ( const char * ) FREA->getStr ( attr,"type"  ,"Error while parsing \"media\" node" ) );
                FREA->error = 1;
                return;
            }

            tt->data = ( void * ) new DBMedia ( newname,newnum,ts1,ti1,"",td1 );

        } else {
            /*Fill data part:*/
            ts1 = FREA->get_cutf8 ( FREA->getStr ( attr,"name"  ,"Error while parsing \"media\" node" ) );
            ts2 = FREA->get_cutf8 ( FREA->getStr ( attr,"owner" ,"Error while parsing \"media\" node" ) );
            tf1 = FREA->getFloat ( attr,"number","Error while parsing \"media\" node" );
            td1 = FREA->get_dcutf8 ( FREA->getStr ( attr,"time"  ,"Error while parsing \"media\" node" ) );

            if ( FREA->error ) return;

            if ( ts2.startsWith ( "@" ) ) {
                FREA->errormsg = QString (
                                     "Line %1: The media name begin with \"@\".\n\
It is disallowed since cdcat version 0.98 !\n\
Please change it with an older version or rewrite it in the xml file!" )
                                 .arg ( XML_GetCurrentLineNumber ( *pp ) );
                FREA->error = 1;
                return;
            }

            ti1 = getTypeFS ( FREA->getStr ( attr,"type"  ,"Error while parsing \"media\" node" ) );
            if ( FREA->error ) return;
            if ( ti1 == UNKNOWN ) {
                FREA->errormsg = QString ( "Line %1: Unknown media type in the file. (\"%2\")" )
                                 .arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( FREA->getStr ( attr,"type"  ,"Error while parsing \"media\" node" ) );
                FREA->error = 1;
                return;
            }

            tt->data = ( void * ) new DBMedia ( ts1, ( int ) tf1,ts2,ti1,"",td1 );
        }
        /*Make this node to the actual node:*/
        FREA->sp = tt;
    }

    if ( !strcmp ( el,"directory" ) ) {
        Node *tt = FREA->sp->child;

        if ( tt == NULL ) FREA->sp->child = tt = new Node ( HC_DIRECTORY,FREA->sp );
        else {
            while ( tt->next != NULL ) tt = tt->next;
            tt->next =  new Node ( HC_DIRECTORY,FREA->sp );
            tt=tt->next;
        }
        /*Fill data part:*/
        ts1 = FREA->get_cutf8 ( FREA->getStr ( attr,"name"  ,"Error while parsing \"directory\" node" ) );
        td1 = FREA->get_dcutf8 ( FREA->getStr ( attr,"time" ,"Error while parsing \"directory\" node" ) );
        if ( FREA->error ) return;
        tt->data = ( void * ) new DBDirectory ( ts1,td1,"" );

        /*Make this node to the actual node:*/
        FREA->sp = tt;
    }

    if ( !strcmp ( el,"file" ) ) {

        Node *tt = FREA->sp->child;

        if ( tt == NULL ) FREA->sp->child = tt = new Node ( HC_FILE,FREA->sp );
        else {
            while ( tt->next != NULL ) tt = tt->next;
            tt->next =  new Node ( HC_FILE,FREA->sp );
            tt=tt->next;
        }
        /*Fill data part:*/
//         cerr <<"1"<<endl;
        ts1 = FREA->get_cutf8 ( FREA->getStr ( attr,"name"  ,"Error while parsing \"file\" node" ) );
//         cerr <<"2"<<endl;

        if ( FREA->error ) return;

        td1 = FREA->get_dcutf8 ( FREA->getStr ( attr,"time"  ,"Error while parsing \"file\" node" ) );
//         cerr <<"3"<<endl;

        if ( FREA->error ) return;
//         cerr <<"4"<<endl;

        tf1 = getSizeFS ( FREA->getStr ( attr,"size","Error while parsing \"file\" node" ) );
//         cerr <<"5"<<endl;

        if ( FREA->error ) return;
        if ( tf1 == -1 ) {
            FREA->errormsg = QString ( "Line %1: Unknown size data in file node. (\"%2\")" )
                             .arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( FREA->getStr ( attr,"size","Error while parsing \"file\" node" ) );
            FREA->error = 1;
            return;
        }

        ti1 = getSizetFS ( FREA->getStr ( attr,"size","Error while parsing \"file\" node" ) );
        if ( FREA->error ) return;
        if ( ti1 == -1 ) {
            FREA->errormsg = QString ( "Line %1: Unknown size type in file node. (\"%2\")" )
                             .arg ( XML_GetCurrentLineNumber ( *pp ) ).arg ( FREA->getStr ( attr,"size","Error while parsing \"file\" node" ) );
            FREA->error = 1;
            return;
        }

        tt->data = ( void * ) new DBFile ( ts1,td1,"",tf1,ti1 );

        /*Make this node to the actual node:*/
        FREA->sp = tt;

    }
    if ( !strcmp ( el,"mp3tag" ) ) {
        Node *tt = ( ( DBFile * ) ( FREA->sp->data ) )->prop;
        if ( tt == NULL ) ( ( DBFile * ) ( FREA->sp->data ) )->prop = tt = new Node ( HC_MP3TAG,FREA->sp );
        else {
            while ( tt->next != NULL ) tt = tt->next;
            tt->next =  new Node ( HC_MP3TAG,FREA->sp );
            tt=tt->next;
        }
        /*Fill data part:*/
        ts1 = FREA->get_cutf8 ( FREA->getStr ( attr,"artist"  ,"Error while parsing \"mp3tag\" node" ) );
        ts2 = FREA->get_cutf8 ( FREA->getStr ( attr,"title"   ,"Error while parsing \"mp3tag\" node" ) );
        ts3 = FREA->get_cutf8 ( FREA->getStr ( attr,"album"  ,"Error while parsing \"mp3tag\" node" ) );
        ts4 = FREA->get_cutf8 ( FREA->getStr ( attr,"year"   ,"Error while parsing \"mp3tag\" node" ) );
        if ( FREA->error ) return;

        tmp_tagp = new DBMp3Tag ( ts1,ts2,"no comment",ts3,ts4 );

        tt->data = ( void * ) tmp_tagp;
        /*I don't make this node to the actual node because this won't be parent.*/
    }

    if ( !strcmp ( el,"catlnk" ) ) {
        char *readed_loc;
        Node *tt = FREA->sp->child;

        if ( tt == NULL ) FREA->sp->child = tt = new Node ( HC_CATLNK,FREA->sp );
        else {
            while ( tt->next != NULL ) tt = tt->next;
            tt->next =  new Node ( HC_CATLNK,FREA->sp );
            tt=tt->next;
        }
        /*Fill data part:*/
        ts1 = FREA->get_cutf8 ( FREA->getStr ( attr,"name"  ,"Error while parsing \"catlnk\" node" ) );
        readed_loc = mstr ( FREA->getStr ( attr,"location" ,"Error while parsing \"catlnk\" node" ) );
        if ( FREA->error ) return;

        tt->data = ( void * ) new DBCatLnk ( ts1.prepend ( "@" ),readed_loc,NULL );

        /*Make this node to the actual node:*/
        FREA->sp = tt;
    }

    if ( !strcmp ( el,"content" ) ) {
        /*nothing*/
    }

    if ( !strcmp ( el,"comment" ) ) {
        /*nothing*/
    }
    if ( !strcmp ( el,"borrow" ) ) {
        /*nothing*/
    }

//     cerr <<"end_start:"<<el<<endl;

}
/*********************************************************************/
static void end ( void *data, const char *el ) {
//     cerr <<"Start_end:"<<el<<endl;


    if ( FREA->error ) return;
    /*That case I found an error in file -> needs exit the pasing as fast as I can.*/

    if ( !strcmp ( el,"catalog" ) ) {
        //End parsing !
    }

    if ( !strcmp ( el,"datafile" ) ) {
        //End parsing !
    }

    if ( !strcmp ( el,"media" ) ) {
        /*Restore the parent node tho the actual node:*/
        FREA->sp = FREA->sp->parent;
    }
    if ( !strcmp ( el,"directory" ) ) {
        /*Restore the parent node tho the actual node:*/
        FREA->sp = FREA->sp->parent;
    }
    if ( !strcmp ( el,"file" ) ) {
        /*Restore the parent node tho the actual node:*/
        FREA->sp = FREA->sp->parent;
    }
    if ( !strcmp ( el,"mp3tag" ) ) {
        //strcpy(tmp_tagp->comment,FREA->dataBuffer);
//         cerr <<"1"<<endl;
//         cerr <<"2"<<endl;
        tmp_tagp->comment = FREA->get_cutf8 ( FREA->dataBuffer );
//         cerr <<"3"<<endl;
        tmp_tagp = NULL;
//         cerr <<"4"<<endl;
    }
    if ( !strcmp ( el,"catlnk" ) ) {
        /*Restore the parent node tho the actual node:*/
        FREA->sp = FREA->sp->parent;
    }

    if ( !strcmp ( el,"content" ) ) {
        unsigned char *bytes;
        unsigned long rsize,i;

        Node *tt = ( ( DBFile * ) ( FREA->sp->data ) )->prop;
        if ( tt == NULL )
            ( ( DBFile * ) ( FREA->sp->data ) )->prop = tt = new Node ( HC_CONTENT,FREA->sp );
        else {
            while ( tt->next != NULL ) tt = tt->next;
            tt->next =  new Node ( HC_CONTENT,FREA->sp );
            tt=tt->next;
        }
        /*Fill data part:*/
        bytes = new unsigned char[ ( rsize= ( strlen ( FREA->dataBuffer ) /2 ) ) + 1];
        for ( i=0;i<rsize;i++ )
            bytes[i] = decodeHexa ( FREA->dataBuffer[i*2],FREA->dataBuffer[i*2 + 1] );
        bytes[rsize] = '\0';
        tt->data = ( void * ) new DBContent ( bytes,rsize );
    }

    if ( !strcmp ( el,"comment" ) ) {
        switch ( FREA->sp->type ) {
        case HC_CATALOG  :

            ( ( DBCatalog    * ) ( FREA->sp->data ) ) -> comment = FREA->get_cutf8 ( FREA->dataBuffer );
            break;
        case HC_MEDIA    :
            ( ( DBMedia      * ) ( FREA->sp->data ) ) -> comment = FREA->get_cutf8 ( FREA->dataBuffer );
            break;
        case HC_DIRECTORY:
            ( ( DBDirectory * ) ( FREA->sp->data ) ) -> comment = FREA->get_cutf8 ( FREA->dataBuffer );
            break;
        case HC_FILE     :
            ( ( DBFile      * ) ( FREA->sp->data ) ) -> comment = FREA->get_cutf8 ( FREA->dataBuffer );
            break;
        case HC_CATLNK   :
            ( ( DBCatLnk    * ) ( FREA->sp->data ) ) -> comment = FREA->get_cutf8 ( FREA->dataBuffer );
            break;
        case HC_MP3TAG:
            FREA->errormsg = QString ( "Line %1: It isnt allowed comment node in mp3tag node." )
                             .arg ( XML_GetCurrentLineNumber ( *pp ) );
            FREA->error = 1;
            break;

        }
    }

    if ( !strcmp ( el,"borrow" ) ) {
        switch ( FREA->sp->type ) {
        case HC_CATALOG  :
        case HC_DIRECTORY:
        case HC_FILE     :
        case HC_MP3TAG   :
        case HC_CATLNK   :
            FREA->errormsg = QString ( "Line %1: The borrow node only allowed in media node." )
                             .arg ( XML_GetCurrentLineNumber ( *pp ) );
            FREA->error = 1;
            break;
        case HC_MEDIA    :
            ( ( DBMedia      * ) ( FREA->sp->data ) ) -> borrowing = FREA->get_cutf8 ( FREA->dataBuffer );
            break;

        }
    }

    clearbuffer = 1;
//     cerr <<"end_end:"<<el<<endl;

}

void getCharDataFromXML ( void *data,const char *s,int len ) {
    int copylen=len;

    if ( FREA->error ) return;
    /*That case I found an error in file -> needs exit the pasing as fast as I can.*/

    if ( clearbuffer ) {
        buffpos = 0;
        clearbuffer = 0;
    }

    if ( buffpos + len >= ( MAX_STORED_SIZE*2 ) )
        copylen = ( MAX_STORED_SIZE*2 ) - buffpos;

    memcpy ( ( FREA->dataBuffer ) +buffpos,s,sizeof ( char ) * copylen );
    buffpos += copylen;
    FREA->dataBuffer[buffpos] = '\0';
}


int FileReader::readFrom ( Node *source ) {
    char *Buff = new char[BUFFSIZE];
    XML_Parser p = XML_ParserCreate ( NULL );
    pp = &p;

    error = 0;
    sp    = source;


    dataBuffer = new char[ ( MAX_STORED_SIZE*2 ) +64];
    //I allocated MAX_STORED_SIZE*2 becouse the hexadecimal data is twice
    //  as long than the normal data
    //WARNING: big data segment 256 kByte !!!

    if ( p == NULL ) {
        errormsg = QString ( "Couldn't allocate memory for parser" );
        delete [] Buff;
        delete [] dataBuffer;
        return 1;
    }

//     cerr <<"Start Cycle"<<endl;

    XML_SetUserData ( p,this );
    XML_SetElementHandler ( p, start, end );
    XML_SetCharacterDataHandler ( p,getCharDataFromXML );
    XML_SetEncoding ( p,"UTF-8" );

    for ( ;; ) {
        int done;
        int len;

        progress ( pww );

        len = gzread ( f,Buff,BUFFSIZE );
        if ( len == -1 ) {
            errormsg = QString ( "Read error" );
            delete [] Buff;
            delete [] dataBuffer;
            return 1;
        }

       // Olivier.Dormond@gmail.com
       // This is apparently buggy most probably because it can be set when the eof
       // of the gzipped file is seen but the decompressed content is bigger than
       // the read buffer. In such a case, the very last chunk of data is never read
       // and the XML_Parse call will fail because it will miss the closing tag.
       // done = gzeof(f);
       // So simply check we didn't get any more data to read as calling XML_Parse
       // with an empty buffer is safe.
       done = len == 0;

        if ( ! XML_Parse ( p, Buff, len, done ) ) {
            errormsg = QString ( "Parse error at line %1:\n%2\n" )
                       .arg ( XML_GetCurrentLineNumber ( p ) )
                       .arg ( XML_ErrorString ( XML_GetErrorCode ( p ) ) );

            delete [] Buff;
            delete [] dataBuffer;
            return 1;
        }

        if ( done || error ) {
            delete [] Buff;
            delete [] dataBuffer;
            return error;
        }
    }
//     cerr <<"End Cycle"<<endl;

}

FileReader::FileReader ( gzFile ff,int ins ) {
    f = ff;
    insert=ins;
    converter = QTextCodec::codecForName ( "utf8" );
}


/* ********************************************************************* */
QString catname;

static void gn_start ( void *data, const char *el, const char **attr ) {
    if ( !strcmp ( el,"catalog" ) ) {
        catname = FREA->get_cutf8 ( FREA->getStr ( attr,"name","Error while parsing \"catalog\" node" ) );
    }
}

static void gn_end ( void *data, const char *el ) {
}

QString FileReader::getCatName ( void ) {
    catname = "";
    char *Buff = new char[BUFFSIZE];
    XML_Parser p = XML_ParserCreate ( NULL );

    if ( p == NULL ) {
        delete [] Buff;
        return QString ( "" );
    }

    XML_SetUserData ( p,this );
    XML_SetElementHandler ( p, gn_start, gn_end );
    for ( ;; ) {
        int done;
        int len;

        len = gzread ( f,Buff,BUFFSIZE );
        if ( len == -1 ) {
            delete [] Buff;
            return QString ( "" );
        }

        done = gzeof ( f );
        if ( ! XML_Parse ( p, Buff, len, done ) ) {
            delete [] Buff;
            return QString ( "" );
        }

        if ( done || catname != "" ) {
            delete [] Buff;
            return catname;
        }
    }
}




