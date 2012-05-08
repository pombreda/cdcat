/****************************************************************************
                             Hyper's CD Catalog
		A multiplatform qt and xml based catalog program

 Author    : Christoph Thielecke (crissi99@gmx.de)
 License   : GPL
 Copyright : (C) 2011 Christoph Thielecke

 Info: read mediainfo using libmediainfo
****************************************************************************/

#ifndef NO_MEDIAINFO
#include "config.h"
#include "cdcat.h"
#include "cdcatmediainfo.h"
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#ifdef _WIN32
// #include <windows.h>
#else
#include <dlfcn.h>
#endif

using namespace MediaInfoNameSpace;
using namespace std;

static QStringList MediaInfoSupportedFileExtensions;
static bool mediaInfoLibInitDone = false;
static bool mediaInfoLibFound = false;
static MediaInfoNameSpace::MediaInfo *MediaInfoHandler=NULL;

bool cleanupMediainfo() {
	if (MediaInfoHandler != NULL) {
		 delete MediaInfoHandler;
	}
}

/* convienent funcs for MediaInfo */
QString fromMediaInfoStrtoQString(MediaInfoNameSpace::String str) {
   QString str2;
#if defined(_WIN32) || defined(MEDIAINFO_UNICODE)
	str2 = QString::fromStdWString(str);
#else
	str2 = QString::fromStdString(str);
#endif
    return str2;
}



MediaInfoNameSpace::String toMediaInfoString(const QString &str) {
#if defined(_WIN32) || defined(MEDIAINFO_UNICODE)
   return (str.toStdWString());
#else
   return (str.toStdString());
#endif
}




CdcatMediaInfo::CdcatMediaInfo ( void ) {
	filename = "";
	InfoText = "";
	if(!mediaInfoLibInitDone) {
		detectSupportedExtensions();
	}
}

CdcatMediaInfo::CdcatMediaInfo ( QString filename ) {
	DEBUG_INFO_ENABLED = init_debug_info();
	if(*DEBUG_INFO_ENABLED)
		cerr << "CdcatMediaInfo() filename: " << qPrintable(filename)<< endl;
	this->filename = filename;
	if(!mediaInfoLibInitDone) {
		detectSupportedExtensions();
	}
	if(mediaInfoLibFound)
		readCdcatMediaInfo();
}

bool CdcatMediaInfo::readCdcatMediaInfo(){
	
	// read data
	DEBUG_INFO_ENABLED = init_debug_info();
	if (MediaInfoHandler == NULL) {
		if(*DEBUG_INFO_ENABLED)
			cout << "readCdcatMediaInfo() MediaInfoHandler is invalid" << endl;
		return false;
	}
	
	if (filename == "") {
		if(*DEBUG_INFO_ENABLED)
			cout << "readCdcatMediaInfo() no filename set" << endl;
		return false;
	}
	MediaInfoNameSpace::String MediaInfoStr(toMediaInfoString(filename));
	int fileopencount = MediaInfoHandler->Open(MediaInfoStr);
	if(fileopencount > 0 ) {
		//MediaInfoHandler->Option("Language", "German;German" );
		//if(*DEBUG_INFO_ENABLED)
		//	cout << "langs of mediainfo: " << MediaInfoHandler.Option("Language_Get").c_str() << endl;
		
		// 	MediaInfoHandler.Option("Language", QString(QString("Image")+QString(";")+tr("Image")).toLocal8Bit().constData());
		// 	MediaInfoHandler.Option("Language", "Image;Bild");
		String info = MediaInfoHandler->Inform();
		if(*DEBUG_INFO_ENABLED)
			cout << "mediainfo for " << filename.toLocal8Bit().constData() << ": " << fromMediaInfoStrtoQString(info).toLocal8Bit().constData() << endl;
		if(*DEBUG_INFO_ENABLED) {
		String ver = toMediaInfoString(QString("Info_Version"));
		// 	cout << "mediainfo version: " << fromMediaInfoStrtoQString(MediaInfoHandler->Option(ver)).toLocal8Bit().constData() << endl;
		// 	cout << "mediainfo codecs: " << MediaInfoHandler.Option("Info_Codecs").c_str() << endl;
		}
		InfoText = fromMediaInfoStrtoQString(info);
		
		// 	size_t Stream_General_Number;
		// 	size_t GeneralCount =  MediaInfoHandler.Count_Get (Stream_General, Stream_General_Number);
		// 	size_t Stream_Video_Number;
		// 	size_t VideoCount =  MediaInfoHandler.Count_Get (Stream_General, Stream_Video_Number);
		// 	size_t Stream_Audio_Number;
		// 	size_t AudioCount =  MediaInfoHandler.Count_Get (Stream_General, Stream_Audio_Number);
		// 	size_t Stream_Text_Number;
		// 	size_t TextCount =  MediaInfoHandler.Count_Get (Stream_General, Stream_Text_Number);
		// 	size_t Stream_Chapters_Number;
		// 	size_t ChaptersCount =  MediaInfoHandler.Count_Get (Stream_General, Stream_Chapters_Number);
		// 	size_t Stream_Menu_Number;
		// 	size_t MenuCount =  MediaInfoHandler.Count_Get (Stream_General, Stream_Menu_Number);
		// 
		// 	if(*DEBUG_INFO_ENABLED)
		// 		cout << "general: " << GeneralCount << ", video: " << VideoCount << ", audio: " << AudioCount << ", text: " << TextCount << ", chapters: " << ChaptersCount << ", menu: " << MenuCount << endl;
		// 
		// //   Stream_General    StreamKind = General. 
		// //   Stream_Video    StreamKind = Video. 
		// //   Stream_Audio    StreamKind = Audio. 
		// //   Stream_Text    StreamKind = Text. 
		// //   Stream_Chapters    StreamKind = Chapters. 
		// //   Stream_Image    StreamKind = Image. 
		// //   Stream_Menu
		return true;
	}
	else {
		cout << "mediainfo for " << filename.toLocal8Bit().constData() << " has been failed" << endl;

		return false;
	}
}


CdcatMediaInfo::~CdcatMediaInfo ( void ) {
	// FIXME close & delete should be done at close cdcat
// 	MediaInfoHandler->Close();
//	delete MediaInfoHandler;
	
}

bool CdcatMediaInfo::initMediaInfoLib() {
	DEBUG_INFO_ENABLED = init_debug_info();
#ifdef MEDIAINFO_STATIC
	MediaInfoHandler = new MediaInfo();
    if (MediaInfoHandler != NULL) {
	mediaInfoLibFound = true;
	if(*DEBUG_INFO_ENABLED) {
		cout << "initMediaInfoLib(): mediainfo lib version: " << fromMediaInfoStrtoQString(MediaInfoHandler->Option(toMediaInfoString(QString("Info_Version")))).split(" - ").at(1).toStdString()  << endl;
	  }
   }
	else {
		if(*DEBUG_INFO_ENABLED)
			cout << "initMediaInfoLib(): init mediainfo lib failed" << endl;
	}
#else
	int success = MediaInfoDLL_Load();
	if (MediaInfoDLL_IsLoaded()) {
		mediaInfoLibFound = true;
		if(*DEBUG_INFO_ENABLED)
			cout << "initMediaInfoLib(): loading mediainfo lib " << MEDIAINFODLL_NAME << " succeeded" << endl;
		MediaInfoHandler = new MediaInfo();
		if(*DEBUG_INFO_ENABLED) {
			cout << "initMediaInfoLib(): mediainfo lib version: " << fromMediaInfoStrtoQString(MediaInfoHandler->Option(toMediaInfoString(QString("Info_Version")))).split(" - ").at(1).toStdString()  << endl;
		}
	}
	else {
		if(*DEBUG_INFO_ENABLED)
			cout << "initMediaInfoLib(): loading mediainfo lib " << MEDIAINFODLL_NAME << " failed" << endl;
	}
#endif // MEDIAINFO_STATIC
	mediaInfoLibInitDone = true;
	return mediaInfoLibFound;
}

QString CdcatMediaInfo::getMediaInfoVersion() {
	if (mediaInfoLibFound)
		return fromMediaInfoStrtoQString(MediaInfoHandler->Option(toMediaInfoString(QString("Info_Version")))).split(" - ").at(1);
	else
		return QString("-");
}

bool CdcatMediaInfo::detectSupportedExtensions() {
   bool success = false;
	if (!mediaInfoLibInitDone) {
      MediaInfoSupportedFileExtensions.clear();
		// init lib
		success = mediaInfoLibFound = initMediaInfoLib();
	}
	
	if ( success && mediaInfoLibFound) {
		MediaInfoSupportedFileExtensions.append("mkv");
		MediaInfoSupportedFileExtensions.append("mka");
		MediaInfoSupportedFileExtensions.append("mks");
		MediaInfoSupportedFileExtensions.append("ogg");
		MediaInfoSupportedFileExtensions.append("ogm");
		MediaInfoSupportedFileExtensions.append("avi");
		MediaInfoSupportedFileExtensions.append("wav");
		MediaInfoSupportedFileExtensions.append("mpeg");
		MediaInfoSupportedFileExtensions.append("mpg");
		MediaInfoSupportedFileExtensions.append("vob");
		MediaInfoSupportedFileExtensions.append("mp4");
		MediaInfoSupportedFileExtensions.append("mpgv");
		MediaInfoSupportedFileExtensions.append("mpv");
		MediaInfoSupportedFileExtensions.append("m1v");
		MediaInfoSupportedFileExtensions.append("m2v");
		MediaInfoSupportedFileExtensions.append("mp2");
		MediaInfoSupportedFileExtensions.append("mp3");
		MediaInfoSupportedFileExtensions.append("asf");
		MediaInfoSupportedFileExtensions.append("wma");
		MediaInfoSupportedFileExtensions.append("wmv");
		MediaInfoSupportedFileExtensions.append("qt");
		MediaInfoSupportedFileExtensions.append("mov");
		MediaInfoSupportedFileExtensions.append("rm");
		MediaInfoSupportedFileExtensions.append("rmvb");
		MediaInfoSupportedFileExtensions.append("ra");
		MediaInfoSupportedFileExtensions.append("ifo");
		MediaInfoSupportedFileExtensions.append("ac3");
		MediaInfoSupportedFileExtensions.append("dts");
		MediaInfoSupportedFileExtensions.append("aac");
		MediaInfoSupportedFileExtensions.append("ape");
		MediaInfoSupportedFileExtensions.append("mac");
		MediaInfoSupportedFileExtensions.append("flac");
		MediaInfoSupportedFileExtensions.append("dat");
		MediaInfoSupportedFileExtensions.append("aiff");
		MediaInfoSupportedFileExtensions.append("aifc");
		MediaInfoSupportedFileExtensions.append("au");
		MediaInfoSupportedFileExtensions.append("iff");
		MediaInfoSupportedFileExtensions.append("paf");
		MediaInfoSupportedFileExtensions.append("sd2");
		MediaInfoSupportedFileExtensions.append("irca");
		MediaInfoSupportedFileExtensions.append("w64");
		MediaInfoSupportedFileExtensions.append("mat");
		MediaInfoSupportedFileExtensions.append("pvf");
		MediaInfoSupportedFileExtensions.append("xi");
		MediaInfoSupportedFileExtensions.append("sds");
		MediaInfoSupportedFileExtensions.append("avr");
		MediaInfoSupportedFileExtensions.append("mkv");
		MediaInfoSupportedFileExtensions.append("ogg");
		MediaInfoSupportedFileExtensions.append("ogm");
		MediaInfoSupportedFileExtensions.append("riff");
		MediaInfoSupportedFileExtensions.append("mpeg");
		MediaInfoSupportedFileExtensions.append("m4a ");
		MediaInfoSupportedFileExtensions.append("mp2");
		MediaInfoSupportedFileExtensions.append("mp3");
		MediaInfoSupportedFileExtensions.append("wm");
		MediaInfoSupportedFileExtensions.append("qt");
		MediaInfoSupportedFileExtensions.append("real");
		MediaInfoSupportedFileExtensions.append("ifo");
		MediaInfoSupportedFileExtensions.append("ac3");
		MediaInfoSupportedFileExtensions.append("dts");
		MediaInfoSupportedFileExtensions.append("aac");
		MediaInfoSupportedFileExtensions.append("mac");
		MediaInfoSupportedFileExtensions.append("m4v");
	return true;
	}

	return false;
}

QStringList CdcatMediaInfo::getSupportedExtensions() {
	return MediaInfoSupportedFileExtensions;
}

bool CdcatMediaInfo::getMediaInfoLibFound() {
	return mediaInfoLibFound;
}


QString CdcatMediaInfo::getInfo() {
	return InfoText;
}

#endif

/*end of file*/
