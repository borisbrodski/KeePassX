/***************************************************************************
 *   Copyright (C) 2005-2007 by Tarek Saidi                                *
 *   tarek.saidi@arcor.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *

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

#include <QHeaderView>
#include <QClipboard>
#include <QProcess>
#include "lib/AutoType.h"
#include "lib/EntryView.h"
#include "dialogs/EditEntryDlg.h"

// just for the lessThan funtion
/*QList<EntryViewItem*>* pItems;
KeepassEntryView* pEntryView;*/

KeepassEntryView::KeepassEntryView(QWidget* parent):QTreeWidget(parent){
	ViewMode=Normal;
	header()->setResizeMode(QHeaderView::Interactive);
	header()->setStretchLastSection(false);
	header()->setClickable(true);
	//header()->setCascadingSectionResizes(true);
	retranslateColumns();
	restoreHeaderView();

	connect(this,SIGNAL(itemSelectionChanged()), SLOT(OnItemsChanged()));
	connect(&ClipboardTimer, SIGNAL(timeout()), SLOT(OnClipboardTimeOut()));
	connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)), SLOT(OnEntryActivated(QTreeWidgetItem*,int)));
	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(OnEntryDblClicked(QTreeWidgetItem*,int)));
	Clipboard=QApplication::clipboard();
	ContextMenu=new QMenu(this);
	setAlternatingRowColors(config->alternatingRowColors());

	/*pItems=&Items;
	pEntryView=this;*/
}

KeepassEntryView::~KeepassEntryView(){
	saveHeaderView();
}

void KeepassEntryView::retranslateColumns() {
	setHeaderLabels( QStringList() << tr("Title") << tr("Username") << tr("URL") << tr("Password") << tr("Comments")
		<< tr("Expires") << tr("Creation") << tr("Last Change") << tr("Last Access") << tr("Attachment") << tr("Group") );
}

bool KeepassEntryView::columnVisible(int col) {
	return !header()->isSectionHidden(col);
}

void KeepassEntryView::setColumnVisible(int col, bool visible) {
	header()->setSectionHidden(col, !visible);
}

void KeepassEntryView::saveHeaderView() {
	if (ViewMode == Normal)
		config->setEntryView( header()->saveState() );
	else
		config->setEntryViewSearch( header()->saveState() );
}

void KeepassEntryView::restoreHeaderView() {
	if (ViewMode == Normal) {
		QByteArray state = config->entryView();
		if (state.isEmpty()) {
			for (int i=10; i>=0; --i) {
				if (i <= 3) {
					setColumnVisible(i, true);
					header()->moveSection(header()->visualIndex(i), 0);
				}
				else {
					setColumnVisible(i, false);
				}
			}
			header()->setSortIndicator(0, Qt::AscendingOrder);
			header()->setSortIndicatorShown(true);
			header()->resizeSection(0, (int) (header()->sectionSize(0) * 1.5));
		}
		else {
			header()->restoreState(state);
			setColumnVisible(10, false); // just to be sure
			
			QApplication::processEvents();
			
			//QHash<int, int> sectionSize;
			QList<int> visibleSections;
			for (int i=0; i<=10; ++i) {
				if (columnVisible(i)) {
					qDebug("%d",i);
					visibleSections.append(i);
					header()->hideSection(i);
				}
			}
			
			QApplication::processEvents();
			
			for (int i=0; i<visibleSections.size(); ++i) {
				qDebug("%d",visibleSections[i]);
				header()->showSection(visibleSections[i]);
			}
			
			/*for (int i=0; i<=10; ++i) {
				if (columnVisible(i)) {
					int size = header()->sectionSize(i);
					
					
					
					header()->resizeSection(i, 1);
					header()->resizeSection(i, size);
				}
			}*/
		}
	}
	else {
		QByteArray state = config->entryViewSearch();
		if (state.isEmpty()) {
			for (int i=10; i>=0; --i) {
				if (i <= 3 || i == 10) {
					setColumnVisible(i, true);
					header()->moveSection(header()->visualIndex(i), 0);
				}
				else {
					setColumnVisible(i, false);
				}
			}
			header()->moveSection(header()->visualIndex(10), 0);
			header()->setSortIndicator(10, Qt::AscendingOrder);
			header()->setSortIndicatorShown(true);
		}
		else {
			header()->restoreState(state);
		}
	}
}

void KeepassEntryView::OnGroupChanged(IGroupHandle* group){
	CurrentGroup=group;
	showGroup(group);
}

void KeepassEntryView::OnShowSearchResults(){
	CurrentGroup=NULL;
	showSearchResults();
}

void KeepassEntryView::OnItemsChanged(){
	switch(selectedItems().size()){
		case 0:	emit selectionChanged(NONE);
				break;
		case 1:	emit selectionChanged(SINGLE);
				break;
		default:emit selectionChanged(MULTIPLE);
	}
}

void KeepassEntryView::OnSaveAttachment(){
	if (selectedItems().size() == 0) return;
	CEditEntryDlg::saveAttachment(((EntryViewItem*)selectedItems().first())->EntryHandle,this);
}

void KeepassEntryView::OnCloneEntry(){
	QList<QTreeWidgetItem*> entries=selectedItems();
	for(int i=0; i<entries.size();i++){
		Items.append(new EntryViewItem(this));
		Items.back()->EntryHandle=
			db->cloneEntry(((EntryViewItem*)entries[i])->EntryHandle);
		updateEntry(Items.back());
	}
	if (header()->isSortIndicatorShown())
		sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
	emit fileModified();
}

void KeepassEntryView::OnDeleteEntry(){
	QList<QTreeWidgetItem*> entries=selectedItems();
	
	if(config->askBeforeDelete()){
		QString text;
		if(entries.size()==1)
			text=tr("Are you sure you want to delete this entry?");
		else
			text=tr("Are you sure you want to delete these %1 entries?").arg(entries.size());
		if(QMessageBox::question(this,tr("Delete?"),text,QMessageBox::Yes | QMessageBox::No,QMessageBox::No)==QMessageBox::No)
			return;
	}
	
	bool backup = false;
	IGroupHandle* bGroup;
	if (config->backup() && ((EntryViewItem*)entries[0])->EntryHandle->group() != (bGroup=db->backupGroup()))
		backup = true;
	if (backup && !bGroup) {
		emit requestCreateGroup("Backup", 4, NULL);
		bGroup = db->backupGroup();
	}
	for(int i=0; i<entries.size();i++){
		IEntryHandle* entryHandle = ((EntryViewItem*)entries[i])->EntryHandle;
		if (backup && bGroup){
			db->moveEntry(entryHandle, bGroup);
			QDateTime now = QDateTime::currentDateTime();
			entryHandle->setLastAccess(now);
			entryHandle->setLastMod(now);
		}
		else{
			db->deleteEntry(entryHandle);
		}
		Items.removeAt(Items.indexOf((EntryViewItem*)entries[i]));
		delete entries[i];
	}
	emit fileModified();
}



void KeepassEntryView::updateEntry(EntryViewItem* item){
	IEntryHandle* entry = item->EntryHandle;
	int j=0;
	item->setText(j++,entry->title());
	item->setIcon(0,db->icon(entry->image()));
	if(config->hideUsernames())
		item->setText(j++,"******");
	else
		item->setText(j++,entry->username());
	item->setText(j++,entry->url());
	if(config->hidePasswords())
		item->setText(j++,"******");
	else{
		SecString password=entry->password();
		password.unlock();
		item->setText(j++,password.string());
	}
	QString comment = entry->comment();
	int toPos = comment.indexOf(QRegExp("[\\r\\n]"));
	if (toPos == -1)
		item->setText(j++,comment);
	else
		item->setText(j++,comment.left(toPos));
	item->setText(j++,entry->expire().dateToString(Qt::SystemLocaleDate));
	item->setText(j++,entry->creation().dateToString(Qt::SystemLocaleDate));
	item->setText(j++,entry->lastMod().dateToString(Qt::SystemLocaleDate));
	item->setText(j++,entry->lastAccess().dateToString(Qt::SystemLocaleDate));
	item->setText(j++,entry->binaryDesc());
	if (ViewMode == ShowSearchResults) {
		item->setText(j,entry->group()->title());
		item->setIcon(j++,db->icon(entry->group()->image()));
	}
}

void KeepassEntryView::editEntry(EntryViewItem* item){
	IEntryHandle* handle = item->EntryHandle;
	CEntry old = handle->data();
	
	CEditEntryDlg dlg(db,handle,this,true);
	int result = dlg.exec();
	switch(result){
		case 0: //canceled or no changes
			break;
		case 1: //modifications but same group
			updateEntry(item);
			emit fileModified();
			break;
		//entry moved to another group
		case 2: //modified
		case 3: //not modified
			Items.removeAll(item);
			delete item;
			emit fileModified();
			break;
	}
	
	IGroupHandle* bGroup;
	if ((result==1 || result==2) && config->backup() && handle->group() != (bGroup=db->backupGroup())){
		old.LastAccess = QDateTime::currentDateTime();
		old.LastMod = old.LastAccess;
		if (bGroup==NULL)
			emit requestCreateGroup("Backup", 4, NULL);
		if ((bGroup = db->backupGroup())!=NULL)
			db->addEntry(&old, bGroup);
	}
	
	if (result == 1)
		OnItemsChanged();
}


void KeepassEntryView::OnNewEntry(){
	IEntryHandle* NewEntry = NULL;
	if (!CurrentGroup){ // We must be viewing search results. Add the new entry to the first group.
		if (db->groups().size() > 0)
			NewEntry=db->newEntry(db->sortedGroups()[0]);
		else{
			QMessageBox::critical(NULL,tr("Error"),tr("At least one group must exist before adding an entry."),tr("OK"));
		}
	}
	else
		NewEntry=db->newEntry(CurrentGroup);
	CEditEntryDlg dlg(db,NewEntry,this,true);
	if(!dlg.exec()){
		db->deleteLastEntry();
	}
	else{
		Items.append(new EntryViewItem(this));
		Items.back()->EntryHandle=NewEntry;
		updateEntry(Items.back());
		emit fileModified();
		if (header()->isSortIndicatorShown())
			sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
	}

}

void KeepassEntryView::OnEntryActivated(QTreeWidgetItem* item, int Column){
	Q_UNUSED(item);
	
	switch (Column){
		case 1:
			OnUsernameToClipboard();
			break;
		case 2:
			OnEditOpenUrl();
			break;
		case 3:
			OnPasswordToClipboard();
			break;
	}
}

void KeepassEntryView::OnEntryDblClicked(QTreeWidgetItem* item, int Column){
	if (Column == 0)
		editEntry((EntryViewItem*)item);
}

void KeepassEntryView::OnEditEntry(){
	if (selectedItems().size() == 0) return;
	editEntry((EntryViewItem*)selectedItems().first());
}

void KeepassEntryView::OnEditOpenUrl(){
	if (selectedItems().size() == 0) return;
	openBrowser( ((EntryViewItem*)selectedItems().first())->EntryHandle );
}

void KeepassEntryView::OnEditCopyUrl(){
	if (selectedItems().size() == 0) return;
	QString url = ((EntryViewItem*)selectedItems().first())->EntryHandle->url();
	if (url.trimmed().isEmpty()) return;
	if (url.startsWith("cmd://") && url.length()>6)
		url = url.right(url.length()-6);
	
	Clipboard->setText(url,  QClipboard::Clipboard);
	if(Clipboard->supportsSelection()){
		Clipboard->setText(url, QClipboard::Selection);
	}
}

void KeepassEntryView::OnUsernameToClipboard(){
	if (selectedItems().size() == 0) return;
	QString username = ((EntryViewItem*)selectedItems().first())->EntryHandle->username();
	if (username.trimmed().isEmpty()) return;
	Clipboard->setText(username,  QClipboard::Clipboard);
	if(Clipboard->supportsSelection()){
		Clipboard->setText(username, QClipboard::Selection);
	}
	
	if (config->clipboardTimeOut()!=0) {
		ClipboardTimer.setSingleShot(true);
		ClipboardTimer.start(config->clipboardTimeOut()*1000);
	}
}

void KeepassEntryView::OnPasswordToClipboard(){
	if (selectedItems().size() == 0) return;
	SecString password;
	password=((EntryViewItem*)selectedItems().first())->EntryHandle->password();
	password.unlock();
	if (password.string().isEmpty()) return;
	Clipboard->setText(password.string(), QClipboard::Clipboard);
	if(Clipboard->supportsSelection()){
		Clipboard->setText(password.string(), QClipboard::Selection);
	}
	
	if (config->clipboardTimeOut()!=0) {
		ClipboardTimer.setSingleShot(true);
		ClipboardTimer.start(config->clipboardTimeOut()*1000);
	}
}

void KeepassEntryView::OnClipboardTimeOut(){
	Clipboard->clear(QClipboard::Clipboard);
	if(Clipboard->supportsSelection()){
		Clipboard->clear(QClipboard::Selection);
	}
#ifdef Q_WS_X11
	QProcess::startDetached("dcop klipper klipper clearClipboardHistory");
	QProcess::startDetached("dbus-send --type=method_call --dest=org.kde.klipper /klipper "
		"org.kde.klipper.klipper.clearClipboardHistory");
#endif
}


void KeepassEntryView::contextMenuEvent(QContextMenuEvent* e){
	if(itemAt(e->pos())){
		EntryViewItem* item=(EntryViewItem*)itemAt(e->pos());
		if(!selectedItems().size()){
			setItemSelected(item,true);
		}
		else{
				if(!isItemSelected(item)){
					while(selectedItems().size()){
						setItemSelected(selectedItems().first(),false);
					}
					setItemSelected(item,true);
				}
			}
	}
	else{
		while (selectedItems().size())
			setItemSelected(selectedItems().first(),false);
	}
	e->accept();
	ContextMenu->popup(e->globalPos());
}

void KeepassEntryView::resizeEvent(QResizeEvent* e){
	// TODO resizeColumns();
	e->accept();
}


void KeepassEntryView::showSearchResults(){
	if(ViewMode == Normal){
		saveHeaderView();
		ViewMode = ShowSearchResults;
		restoreHeaderView();
		emit viewModeChanged(true);
	}
	clear();
	Items.clear();
	createItems(SearchResults);
}


void KeepassEntryView::showGroup(IGroupHandle* group){
	if(ViewMode == ShowSearchResults){
		saveHeaderView();
		ViewMode = Normal;
		restoreHeaderView();
		emit viewModeChanged(false);
	}
	clear();
	Items.clear();
	if(group==NULL)return;
	QList<IEntryHandle*>entries=db->entries(group);
	createItems(entries);
}

void KeepassEntryView::createItems(QList<IEntryHandle*>& entries){
	for(int i=0;i<entries.size();i++){
		if(!entries[i]->isValid())continue;
		EntryViewItem* item=new EntryViewItem(this);
		Items.push_back(item);
		Items.back()->EntryHandle=entries[i];
		int j=0;
		item->setText(j++,entries[i]->title());
		item->setIcon(0,db->icon(entries[i]->image()));
		if(config->hideUsernames())
			item->setText(j++,"******");
		else
			item->setText(j++,entries[i]->username());
		item->setText(j++,entries[i]->url());
		if(config->hidePasswords())
			item->setText(j++,"******");
		else{
			SecString password=entries[i]->password();
			password.unlock();
			item->setText(j++,password.string());
		}
		QString comment = entries[i]->comment();
		int toPos = comment.indexOf(QRegExp("[\\r\\n]"));
		if (toPos == -1)
			item->setText(j++,comment);
		else
			item->setText(j++,comment.left(toPos));
		item->setText(j++,entries[i]->expire().dateToString(Qt::SystemLocaleDate));
		item->setText(j++,entries[i]->creation().dateToString(Qt::SystemLocaleDate));
		item->setText(j++,entries[i]->lastMod().dateToString(Qt::SystemLocaleDate));
		item->setText(j++,entries[i]->lastAccess().dateToString(Qt::SystemLocaleDate));
		item->setText(j++,entries[i]->binaryDesc());
		if (ViewMode == ShowSearchResults) {
			item->setText(j,entries[i]->group()->title());
			item->setIcon(j++,db->icon(entries[i]->group()->image()));
		}
	}
}

void KeepassEntryView::updateIcons(){
	for(int i=0;i<Items.size();i++){
		Items[i]->setIcon(0,db->icon(Items[i]->EntryHandle->image()));
	}
}

void KeepassEntryView::refreshItems(){
	for (int i=0;i<Items.size();i++)
		updateEntry(Items.at(i));
}

void KeepassEntryView::mousePressEvent(QMouseEvent *event){
	//save event position - maybe this is the start of a drag
	if (event->button() == Qt::LeftButton)
		DragStartPos = event->pos();
	QTreeWidget::mousePressEvent(event);
}

void KeepassEntryView::mouseMoveEvent(QMouseEvent *event){
	if (!(event->buttons() & Qt::LeftButton))
		return;
	if ((event->pos() - DragStartPos).manhattanLength() < QApplication::startDragDistance())
		return;

	DragItems.clear();
	EntryViewItem* DragStartItem=(EntryViewItem*)itemAt(DragStartPos);
	if(!DragStartItem){
		while(selectedItems().size()){
			setItemSelected(selectedItems().first(),false);
		}
		return;
	}
	if(selectedItems().isEmpty()){
			setItemSelected(DragStartItem,true);
	}
	else{
		bool AlreadySelected=false;
		for(int i=0;i<selectedItems().size();i++){
			if(selectedItems()[i]==DragStartItem){
				AlreadySelected=true;
				break;
			}
		}
		if(!AlreadySelected){
			while(selectedItems().size()){
				setItemSelected(selectedItems().first(),false);
			}
			setItemSelected(DragStartItem,true);
		}
	}

	DragItems=selectedItems();
	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
	void* pDragItems=&DragItems;
	mimeData->setData("text/plain;charset=UTF-8",DragItems.first()->text(0).toUtf8());
	mimeData->setData("application/x-keepassx-entry",QByteArray((char*)&pDragItems,sizeof(void*)));
	drag->setMimeData(mimeData);
	EventOccurredBlock = true;
	drag->exec(Qt::MoveAction);
	EventOccurredBlock = false;
}

void KeepassEntryView::removeDragItems(){
	for(int i=0;i<DragItems.size();i++){
		for(int j=0;j<Items.size();j++){
			if(Items[j]==DragItems[i]){
				Items.removeAt(j);
				j--;
				delete DragItems[i];
			}
		}
	}
}

#ifdef AUTOTYPE
void KeepassEntryView::OnAutoType(){
	if (selectedItems().size() == 0) return;
	autoType->perform(((EntryViewItem*)selectedItems().first())->EntryHandle);
}
#endif

void KeepassEntryView::paintEvent(QPaintEvent * event){
QTreeWidget::paintEvent(event);
}


EntryViewItem::EntryViewItem(QTreeWidget *parent):QTreeWidgetItem(parent){

}

EntryViewItem::EntryViewItem(QTreeWidget *parent, QTreeWidgetItem *preceding):QTreeWidgetItem(parent,preceding){

}

EntryViewItem::EntryViewItem(QTreeWidgetItem *parent):QTreeWidgetItem(parent){

}

EntryViewItem::EntryViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *preceding):QTreeWidgetItem(parent,preceding){

}


bool EntryViewItem::operator<(const QTreeWidgetItem& other) const{
	int SortCol = treeWidget()->header()->sortIndicatorSection();
	int ListIndex = ((KeepassEntryView*)treeWidget())->header()->logicalIndex(SortCol);
	
	int comp = compare(other, SortCol, ListIndex);
	if (comp!=0)
		return (comp<0);
	else {
		int visibleCols = treeWidget()->header()->count() - treeWidget()->header()->hiddenSectionCount();
		int ListIndexOrg = ListIndex;
		for (int i=0; i<visibleCols; i++){
			SortCol = treeWidget()->header()->logicalIndex(i);
			ListIndex = ((KeepassEntryView*)treeWidget())->header()->logicalIndex(SortCol);
			if (ListIndex==ListIndexOrg || ListIndex==3) // sort or password column
				continue;
			
			comp = compare(other, SortCol, ListIndex);
			if (comp!=0)
				return (comp<0);
		}
		return true; // entries are equal
	}
}

int EntryViewItem::compare(const QTreeWidgetItem& other, int col, int index) const{
	if (index < 5 || index > 8){ //columns with string values (Title, Username, Password, URL, Comment, Group)
		return QString::localeAwareCompare(text(col),other.text(col));
	}
	
	KpxDateTime DateThis;
	KpxDateTime DateOther;

	switch (index){
		case 5:
			DateThis=EntryHandle->expire();
			DateOther=((EntryViewItem&)other).EntryHandle->expire();
			break;
		case 6:
			DateThis=EntryHandle->creation();
			DateOther=((EntryViewItem&)other).EntryHandle->creation();
			break;
		case 7:
			DateThis=EntryHandle->lastMod();
			DateOther=((EntryViewItem&)other).EntryHandle->lastMod();
			break;
		case 8:
			DateThis=EntryHandle->lastAccess();
			DateOther=((EntryViewItem&)other).EntryHandle->lastAccess();
			break;
		default:
			Q_ASSERT(false);
	}
	
	if (DateThis==DateOther)
		return 0;
	else if (DateThis < DateOther)
		return -1;
	else
		return 1;
}

void KeepassEntryView::setCurrentEntry(IEntryHandle* entry){
	bool found=false;
	int i;
	for(i=0;i<Items.size();i++)
		if(Items.at(i)->EntryHandle==entry){found=true; break;}
	if(!found)return;
	setCurrentItem(Items.at(i));
}
