/***************************************************************************
    begin                : Tue Aug 31 21:54:20 EST 2004
    copyright            : (C) 2004 by Michael Pyne <michael.pyne@kdemail.net>
                           (C) 2006 by Ian Monroe <ian@monroe.nu>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DELETEDIALOG_H
#define _DELETEDIALOG_H


#include <QCheckBox>
#include <QLabel>
#include <kdialog.h>
#include <kurl.h>
#include "ui_deletedialogbase.h"

class QStringList;
class K3ListBox;
class KGuiItem;
class QLabel;

class DeleteDialogBase : public QWidget, public Ui::DeleteDialogBase
{
public:
  DeleteDialogBase( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class DeleteWidget : public DeleteDialogBase
{
    Q_OBJECT

public:
    DeleteWidget(QWidget *parent = 0);

    void setFiles(const KUrl::List &files);

protected slots:
    virtual void slotShouldDelete(bool shouldDelete);
};

class DeleteDialog : public KDialog
{
    Q_OBJECT

public:
    explicit DeleteDialog(QWidget *parent, const char *name = "delete_dialog");
    static bool showTrashDialog(QWidget* parent, const KUrl::List &files);

    bool confirmDeleteList(const KUrl::List &condemnedFiles);
    void setFiles(const KUrl::List &files);
    bool shouldDelete() const { return m_widget->ddShouldDelete->isChecked(); }

protected slots:
    virtual void accept();
    void slotShouldDelete(bool shouldDelete);

private:
    DeleteWidget *m_widget;
    KGuiItem m_trashGuiItem;
};

#endif

// vim: set et ts=4 sw=4:
