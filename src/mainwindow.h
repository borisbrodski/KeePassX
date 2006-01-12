/***************************************************************************
 *   Copyright (C) 2005 by Tarek Saidi                                     *
 *   tarek.saidi@arcor.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QLabel>
#include <QTime>
#include <qapplication.h>
#include <QShowEvent>
#include <QTranslator>
#include <QDropEvent>
#include <QListWidget>
#include <QTreeWidget>


#include "PwManager.h"
#include "PwmConfig.h"
#include "lib/GroupView.h"

#include "ui_MainWindow.h"



class KeepassMainWindow : public QMainWindow, public Ui_MainWindow{
Q_OBJECT
public:
 KeepassMainWindow (QWidget *parent=0, Qt::WFlags flags=0);
 PwDatabase* db;

private slots:
   void OnFileNew();
   void OnFileOpen();
   void OnFileClose();
   bool OnFileSave();
   bool OnFileSaveAs();
   void OnFileSettings();
   void OnFileChangeKey();
   void OnFileExit();
   void OnImportFromPwm();
   void OnImportFromKWalletXml();
   void OnCurrentGroupChanged(QTreeWidgetItem*,QTreeWidgetItem*);
   void OnEntryItemDoubleClicked(QTreeWidgetItem* item,int column);
   void OnEntrySelectionChanged();
   void OnGroupSelectionChanged();

private:
 enum SelectionState{NONE,SINGLE,MULTIPLE};
 SelectionState GroupSelection, EntrySelection;
 bool FileOpen;
 bool ModFlag;
 inline void setupToolbar();
 inline void setupIcons();
 inline void setupConnections();
 void setStateFileOpen(bool);
 void setStateFileModified(bool);
 void setStateGroupSelected(SelectionState s);
 void setStateEntrySelected(SelectionState s);
 void openDatabase(QString filename);
 bool closeDatabase();
 void editEntry(CEntry* pEntry);

 



};



#endif