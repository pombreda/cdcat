/****************************************************************************
                             Hyper's CD Catalog
		A multiplatform qt and xml based catalog program

 Author    : Peter Deak (hyperr@freemail.hu)
 License   : GPL
 Copyright : (C) 2003 Peter Deak
****************************************************************************/

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <qvariant.h>
#include <qdialog.h>
#include <qobject.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QLabel>
#include <Q3Frame>
#include <QCloseEvent>
// #include <pcre.h>
#include <QRegExp>
#include <QtGui/QDateTimeEdit>

class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QCheckBox;
class QComboBox;
class Q3Frame;
class QLabel;
class QLineEdit;
class Q3ListView;
class Q3ListViewItem;
class Q3HBoxLayout;
class Q3VBoxLayout;
class QPushButton;
class DateTimeEdit;
class CdCatMainWidget;
class findDialog;
class Node;
class PWw;

class seekEngine : public QObject {
    Q_OBJECT
private:
    int founded;
    PWw *pww;
//     pcre *re;
    QRegExp re;

    char *patt;
    int  offsets[99];
//     pcre_extra *hints;
    const char *error;
    int errptr;

    findDialog *fd;

    int  analyzeNode ( Node *n,Node *pa=NULL );
    void putNodeToList ( Node *n );
    int  matchIt ( QString txt );

public:
    seekEngine ( findDialog *fdp );
    ~seekEngine ( void );

    int start_seek ( void );

private:
    bool dirname;
    bool filename;
    bool comment;
    bool tartist;
    bool ttitle;
    bool talbum;
    bool tcomment;
    bool content;

    bool allmedia;
    bool allowner;

    bool dateStartChecked;
    bool dateEndChecked;

    QString  media;
    QString  owner;

    QDateTime dateStart;
    QDateTime dateEnd;
};

/***********************************************************************/

class findDialog : public QDialog {
    Q_OBJECT

public:
    findDialog ( CdCatMainWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    ~findDialog();


    Q3Frame *input;
    Q3HBoxLayout *h1,*h2,*h3,*h4;
    Q3VBoxLayout *v1;
    CdCatMainWidget *mainw;
    QPushButton* buttonClose;
    Q3Frame* line1;
    QComboBox* cbOwner;
    QComboBox* cbSin;
    QPushButton* buttonCancel;
    QPushButton* buttonOk;
    QLabel* textLabel3;
    QLabel* textLabel1;
    QLabel* textLabel2;
    QLabel* textLabel5;
    QLabel* textLabel6;
    QLabel* textLabel7;
    Q3Frame* line2;
    Q3ListView* resultsl;
    QLineEdit* leText;
    QDateTimeEdit* deDateStart;
    QDateTimeEdit* deDateEnd;
    QCheckBox* cbComment;
    QCheckBox* cbFilename;
    QCheckBox* cbDirname;
    QCheckBox* cbTcomm;
    QCheckBox* cbAlbum;
    QCheckBox* cbTitle;
    QCheckBox* cbArtist;
    QCheckBox* cbCasesens;
    QCheckBox* cbEasy;
    QCheckBox* cbContent;
    QCheckBox* cbDateStart;
    QCheckBox* cbDateEnd;

protected:
    int fillCBox ( void );
    void closeEvent ( QCloseEvent *ce );

    Q3GridLayout* FindDialogBaseLayout;
    Q3VBoxLayout* layout40;
    Q3GridLayout* layout39;
    Q3GridLayout* layout36;
    Q3GridLayout* layout37;
    Q3VBoxLayout* layout31;
    Q3HBoxLayout* layout17;
    Q3GridLayout* layout15;
    Q3HBoxLayout* layout30;
    Q3GridLayout* layout16;

protected slots:
    virtual void languageChange();
    int select ( Q3ListViewItem *i );
    int closee ( void );
    int cancele ( void );
    int seeke ( void );
    int saveState ( void );
    void dateStartChanged(int);
    void dateEndChanged(int);
    void dateStartDoubleClicked();
    void dateEndDoubleClicked();
};

#endif // FINDDIALOG_H
