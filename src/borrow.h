/****************************************************************************
*                        Hyper's CD Catalog
*            A multiplatform qt and xml based catalog program
*
*  Author    : Peter Deak (hyperr@freemail.hu)
*  License   : GPL
*  Copyright : (C) 2003 Peter Deak
****************************************************************************/

#ifndef BORROW_H
#define BORROW_H

#include <QVariant>
#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QTableWidgetItem>


class QCheckBox;
class QTableWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class DataBase;
class QPoint;

class borrowDialog : public QDialog {
    Q_OBJECT

public:
    QString m;
    int ok;
    borrowDialog ( QString mname, QWidget *parent = 0, const char *name = 0, bool modal = false, Qt::WindowFlags fl = 0 );
    ~borrowDialog();

    QLabel *textLabel;
    QLineEdit *leWho;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;


protected:
    QVBoxLayout *borrowDialogLayout;
    QHBoxLayout *layout4;

protected slots:
    int sok( void );
    int scancel( void );

    virtual void languageChange();
};

// ===============================================================

class borrowingDialog : public QDialog {
    Q_OBJECT

public:
    int ch;
    borrowingDialog ( DataBase *dbp, QWidget *parent = 0, const char *name = 0, bool modal = false, Qt::WindowFlags fl = 0 );
    ~borrowingDialog();

    DataBase *db;
    QLabel *textLabel;
    QCheckBox *cbOnlyBorrowed;
    QPushButton *buttonClear;
    QTableWidget *table;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
    int last_row_clicked;
    QWidget *parent;

public slots:

    int sok( void );
    int scancel( void );
    int sclear( void );
    int filltable( void );
    int schanged( int row, int col );
    int sonlyb();
    int sstore();
    int click( const QPoint & );
    int click_set( int a );
    int click_clr( void );
    void setCurrentItem( QTableWidgetItem *item );


protected:
    QVBoxLayout *borrowingDialogLayout;
    QHBoxLayout *layout1;
    QHBoxLayout *layout3;
    QHBoxLayout *layout4;

protected slots:
    virtual void languageChange();
};

#endif // BORROW_H
