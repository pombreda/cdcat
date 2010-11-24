/****************************************************************************
                         Hyper's CD Catalog
         A multiplatform qt and xml based catalog program

      Author    : Peter Deak (hyperr@freemail.hu)
      License   : GPL
      Copyright : (C) 2003 Peter Deak
****************************************************************************/

#ifndef SELREADABLE_H
#define SELREADABLE_H

#include <qvariant.h>
#include <qdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3Frame>
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <QLabel>

class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QCheckBox;
class Q3Frame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class CdCatConfig;
class Q3ButtonGroup;
class QRadioButton;

class SelReadable : public QDialog {
    Q_OBJECT

public:
    SelReadable ( CdCatConfig *confp,QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    ~SelReadable();

    CdCatConfig *conf;
    QCheckBox* cbTag;
    QCheckBox* cbInfo;
    Q3Frame* line0;
    QCheckBox* cbaInfo;
    Q3Frame* line1;
    QCheckBox* cbCont;
    QLineEdit* lineFiles;
    QLabel* textLabel1;
    QSpinBox* maxSpinBox;
    QLabel* textLabel2;
    Q3Frame* line2;
    QPushButton* buttonOK;
    QPushButton* buttonCancel;
    Q3ButtonGroup* tagselector;
    QRadioButton *rad_v1,*rad_v2;


public slots:
    int schanged ( int state );
    int sok ( void );
    int scan ( void );

protected:
    Q3VBoxLayout* SelReadableLayout;
    Q3HBoxLayout* layout12;
    Q3VBoxLayout* layout11;
    Q3HBoxLayout* layout9;
    Q3HBoxLayout* layout10;
    Q3HBoxLayout* layout3;
    Q3HBoxLayout* layout1;

protected slots:
    virtual void languageChange();
};

#endif // SELREADABLE_H
