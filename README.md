# CdCat

The cdcat is graphical (QT based) multiplatform (Linux/Windows/MacOS)
catalog program which scans the directories/drives you want and memorize the
filesystem including the tags of mp3�s�and other data and store it
in a small file.
The database is stored in a gzipped XML format, so you can hack it, or use
it if necessary :-)
And the program can store the content of some specified files up
to a limit size if you want (for example: .nfo files).

## Prerequisites:

* Qt application framework (ver >= 5.x): https://www.qt.io/
* zlib data compression library (ver >= 1.1.4): http://www.zlib.net/
* mediainfo / http://mediainfo.sourceforge.net
* libtar

You can find this as packages in all distributions,
but if you want to compile cdcat you will have to install the
dev packages too.

## Compiling on Linux:

Unpack the source, and check the prerequisites!
If it is done, check or set the QTDIR environment variable
(It must point the root directory of Qt.)

$export QTDIR=/usr/local/qt/

go to the "src" directory.

    $cd src

Rebuild the makefile with

    $qmake cdcat.pro

then compile the program

    $make

/the qmake utility is part of the QT library/
If you got error messages try to check first the dependencies and the rights!
(before you send me bug report :-) )

That case you didn't get errors type: make install to copy the files
to the necessary place.

    #make install

## Compiling on MacOS:

Rebuid the cdcat Makefile with:  "qmake -macx cdcat.pro"
Then build the cdcat with:       "make"


## Author:
see ./AUTHORS

The main program was written by Peter Deak (hungary) but the active Maintainer is Christoph Thielecke.
E-mail: crissi99 at gmx dot de

## Bug reports and questions:
You can send your bug reports or any other questions or others to
the cdcat mailing list:

 cdcat-list@lists.sourceforge.net

## Homepage:
 http://cdcat.sourceforge.net
