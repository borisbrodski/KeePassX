/*
 * sync.cpp
 *
 *  Created on: Feb 22, 2009
 *      Author: boris
 */
#include <stdio.h>

#include "qlist.h"
#include "qstring.h"
#include "sync.h"

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
void _printGroupPath(IGroupHandle * group, bool first){
	if (group!=NULL){
		_printGroupPath(group->parent(), false);
		printf("%s%s", group->title().toAscii().constData(), first ? "" : "/");
	}
}

void printGroup(IDatabase * db, IGroupHandle * group){
	printf("%s/", db->file()->fileName().toAscii().constData());
	_printGroupPath(group, true);
}
#endif

template<typename ICustomHandle>
class AbstractListSynchronizer{
public:
	class ISynchronizeCallback{
	public:
		virtual void cloneItem(IDatabase * dbFrom, IDatabase * dbTo,
				IGroupHandle * parentFrom, IGroupHandle * parentTo,
				ICustomHandle* itemFrom)=0;
		virtual void synchronizeItem(IDatabase * db1, IDatabase * db2,
				IGroupHandle * parent1, IGroupHandle * parent2,
				ICustomHandle* item1, ICustomHandle* item2)=0;
		virtual void deleteItem(IDatabase * db, IGroupHandle * parent, ICustomHandle* item)=0;
	};
private:
	QList<ICustomHandle*> list1;
	QList<ICustomHandle*> list2;
	IDatabase * db1;
	IDatabase * db2;
	IGroupHandle * parent1;
	IGroupHandle * parent2;
	int list1Position;
	int list2Position;
	ISynchronizeCallback * callback;
public:
	AbstractListSynchronizer(ISynchronizeCallback * synchronizeCallback,
			IDatabase * db1, IDatabase * db2,
			IGroupHandle * parent1, IGroupHandle * parent2,
			QList<ICustomHandle*> * list1, QList<ICustomHandle*> * list2){
		this->db1 = db1;
		this->db2 = db2;
		this->parent1 = parent1;
		this->parent2 = parent2;
		this->list1 = *list1;
		this->list2 = *list2;
		this->callback = synchronizeCallback;
		sortLists();
	}
	void synchronize(){
		list1Position = 0;
		list2Position = 0;

		while(list1Position<list1.size() || list2Position<list2.size()){
			QString title1;
			bool deleteFlag1=false;
			QString title2;
			bool deleteFlag2=false;

#ifdef DEBUG_OUTPUT
			for (int i=0; i<list1.size(); i++){
				if (i)
					printf("-");
				if (i==list1Position)
					printf("(");
				if (list1[i])
					printf("%s", list1[i]->title().toAscii().constData());
				else
					printf("<NULL>");
				if (i==list1Position)
					printf(")");
			}
			if (list1Position>=list1.size()){
				printf("()");
			}
			printf("\n");

			for (int i=0; i<list2.size(); i++){
				if (i)
					printf("-");
				if (i==list2Position)
					printf("(");
				if (list2[i])
					printf("%s", list2[i]->title().toAscii().constData());
				else
					printf("<NULL>");
				if (i==list2Position)
					printf(")");

			}
			if (list2Position>=list2.size()){
				printf("()");
			}
			printf("\n");
#endif

			if (list1Position<list1.size()){
				title1 = list1[list1Position]->title();
				deleteFlag1=title1.endsWith(getDeleteItPostfix(), Qt::CaseSensitive);
				if (deleteFlag1)
					title1=title1.left(title1.length()-getDeleteItPostfix().length());
			}

			if (list2Position<list2.size()){
				title2 = list2[list2Position]->title();
				deleteFlag2=title2.endsWith(getDeleteItPostfix(), Qt::CaseSensitive);
				if (deleteFlag2)
					title2=title2.left(title2.length()-getDeleteItPostfix().length());
			}

			if (list1Position>=list1.size()){
				if (deleteFlag1){
					callback->deleteItem(db2, parent2, list2[list2Position]);
					list2[list2Position]=NULL;
				}else{
					callback->cloneItem(db2, db1, parent2, parent1, list2[list2Position]);
				}
				list2Position++;
				continue;
			}
			if (list2Position>=list2.size()){
				if (deleteFlag1){
					callback->deleteItem(db1, parent1, list1[list1Position]);
					list1[list1Position]=NULL;
				}else{
					callback->cloneItem(db1, db2, parent1, parent2, list1[list1Position]);
				}
				list1Position++;
				continue;
			}

#ifdef DEBUG_OUTPUT
			printf("Comparing '%s' with '%s'\n", title1.toAscii().constData(), title2.toAscii().constData());
#endif
			if (title1==title2){
				if (deleteFlag1 || deleteFlag2){
					callback->deleteItem(db1, parent1, list1[list1Position]);
					callback->deleteItem(db2, parent2, list2[list2Position]);
					list1[list1Position]=NULL;
					list2[list2Position]=NULL;
				}else{
					callback->synchronizeItem(db1, db2, parent1, parent2, list1[list1Position], list2[list2Position]);
				}
				list1Position++;
				list2Position++;
				continue;
			}
			if (title1<title2){
				if (deleteFlag1){
					callback->deleteItem(db1, parent1, list1[list1Position]);
					list1[list1Position]=NULL;
				}else{
					callback->cloneItem(db1, db2, parent1, parent2, list1[list1Position]);
				}
				list1Position++;
			}else{
				if (deleteFlag2){
					callback->deleteItem(db2, parent2, list2[list2Position]);
					list2[list2Position]=NULL;
				}else{
					callback->cloneItem(db2, db1, parent2, parent1, list2[list2Position]);
				}
				list2Position++;
			}
		}
	}

private:
	static bool CustomHandleLessThan(const ICustomHandle* handle1,const ICustomHandle* handle2){
		QString title1 = const_cast<ICustomHandle*>(handle1)->title();
		QString title2 = const_cast<ICustomHandle*>(handle2)->title();

		if (title1.endsWith(getDeleteItPostfix(), Qt::CaseSensitive))
			title1=title1.left(title1.length()-getDeleteItPostfix().length());
		if (title2.endsWith(getDeleteItPostfix(), Qt::CaseSensitive))
			title2=title2.left(title2.length()-getDeleteItPostfix().length());

		return title1<title2;
	}

	void sortLists(){
		qSort(list1.begin(),list1.end(),CustomHandleLessThan);
		qSort(list2.begin(),list2.end(),CustomHandleLessThan);
	}
	static QString getDeleteItPostfix(){
		return QString(DELETE_POSTFIX);
	}
};

class SynchronizeGroupCallback : public AbstractListSynchronizer<IGroupHandle>::ISynchronizeCallback {
	DatabaseSynchronizer * databaseSynchronizer;
public:
	SynchronizeGroupCallback(DatabaseSynchronizer * databaseSynchronizer){
		this->databaseSynchronizer=databaseSynchronizer;
	}
private:
	void cloneItem(IDatabase * dbFrom, IDatabase * dbTo,
			IGroupHandle * parentFrom, IGroupHandle * parentTo,
			IGroupHandle* itemFrom){
#ifdef DEBUG_OUTPUT
		printf("Cloning group ");
		printGroup(dbFrom, itemFrom);
		printf("\n");
#endif

		CGroup cgroup;
		cgroup.Image = itemFrom->image();
		cgroup.Title = itemFrom->title();
		IGroupHandle * newGroup = dbTo->addGroup(&cgroup, parentTo);

		databaseSynchronizer->statistic().groupsCreated++;

		databaseSynchronizer->synchronizeGroups(dbFrom, dbTo, itemFrom, newGroup);
	}

	void synchronizeItem(IDatabase * db1, IDatabase * db2,
			IGroupHandle * parent1, IGroupHandle * parent2,
			IGroupHandle* item1, IGroupHandle* item2){
#ifdef DEBUG_OUTPUT
		printf("Synchronizing groups ");
		printGroup(db1, item1);
		printf("<->");
		printGroup(db2, item2);
		printf("\n");
#endif

		if (item1->image()!=item2->image()){
			databaseSynchronizer->statistic().groupsSynchronized++;
			item2->setImage(item1->image());
		}

		databaseSynchronizer->synchronizeGroups(db1, db2, item1, item2);
	}
	void deleteItem(IDatabase * db, IGroupHandle * parent, IGroupHandle* item){
#ifdef DEBUG_OUTPUT
		printf("Deleting group ");
		printGroup(db, item);
		printf("\n");
#endif
		db->deleteGroup(item);
		databaseSynchronizer->statistic().groupsDeleted++;
	}
};

class SynchronizeEntryCallback : public AbstractListSynchronizer<IEntryHandle>::ISynchronizeCallback {
	DatabaseSynchronizer * databaseSynchronizer;
public:
	SynchronizeEntryCallback(DatabaseSynchronizer * databaseSynchronizer){
		this->databaseSynchronizer=databaseSynchronizer;
	}
private:
	void cloneItem(IDatabase * dbFrom, IDatabase * dbTo,
			IGroupHandle * parentFrom, IGroupHandle * parentTo,
			IEntryHandle* itemFrom){
#ifdef DEBUG_OUTPUT
		printf("Cloning entry %s\n", itemFrom->title().toAscii().constData());
#endif

		CEntry centry;
		centry.Title = itemFrom->title();
		centry.Image = itemFrom->image();
		centry.Url = itemFrom->url();
		centry.Username = itemFrom->username();
		centry.Password = itemFrom->password();
		centry.Comment = itemFrom->comment();
		centry.BinaryDesc = itemFrom->binaryDesc();
		centry.Binary = itemFrom->binary();
		centry.Creation = itemFrom->creation();
		centry.LastMod = itemFrom->lastMod();
		centry.LastAccess = itemFrom->lastAccess();
		centry.Expire = itemFrom->expire();

		if (!parentTo){
			printf("Can't create entry!\n\nparentTo==NULL");
			exit(1);
		}
		dbTo->addEntry(&centry, parentTo);

		databaseSynchronizer->statistic().entriesCreated++;
	}
	void synchronizeItem(IDatabase * db1, IDatabase * db2,
			IGroupHandle * parent1, IGroupHandle * parent2,
			IEntryHandle* item1, IEntryHandle* item2){
#ifdef DEBUG_OUTPUT
		printf("Synchronizing entry %s, %s\n", item1->title().toAscii().constData(), item2->title().toAscii().constData());
#endif

		if (item1->lastMod().toTime_t() == item2->lastMod().toTime_t())
			return;

		if (item1->lastMod()<item2->lastMod())
			synchronizeEntry(item2, item1);
		else
			synchronizeEntry(item1, item2);

		databaseSynchronizer->statistic().entriesSynchronized++;
	}
	void synchronizeEntry(IEntryHandle* fromEntry, IEntryHandle* toEntry) {
		toEntry->setTitle(fromEntry->title());
		toEntry->setImage(fromEntry->image());
		toEntry->setUrl(fromEntry->url());
		toEntry->setUsername(fromEntry->username());
		toEntry->setPassword(fromEntry->password());
		toEntry->setComment(fromEntry->comment());
		toEntry->setBinaryDesc(fromEntry->binaryDesc());
		toEntry->setBinary(fromEntry->binary());
		toEntry->setCreation(fromEntry->creation());
		toEntry->setLastMod(fromEntry->lastMod());
		toEntry->setLastAccess(fromEntry->lastAccess());
		toEntry->setExpire(fromEntry->expire());
	}
	void deleteItem(IDatabase * db, IGroupHandle * parent, IEntryHandle* item){
#ifdef DEBUG_OUTPUT
		printf("Deleting entry %s\n", item->title().toAscii().constData());
#endif
		db->deleteEntry(item);
		databaseSynchronizer->statistic().entriesDeleted++;
	}
};

void DatabaseSynchronizer::synchronizeGroups(IDatabase * db1, IDatabase * db2, IGroupHandle* parent1, IGroupHandle* parent2) {
#ifdef DEBUG_OUTPUT
	printf("Synchronizing %s\n", parent1 != NULL ? parent1->title().toAscii().constData(): "");
#endif

	QList<IGroupHandle*> groups1 = db1->groups();
	QList<IGroupHandle*> groups2 = db2->groups();

	filterGroupsForSynchronization(groups1, parent1);
	filterGroupsForSynchronization(groups2, parent2);

	statistic().groupsProcessed+=groups1.size()+groups2.size();

	SynchronizeGroupCallback synchronizeGroupCallback(this);
	AbstractListSynchronizer<IGroupHandle> groupListSynchronizer(
			(AbstractListSynchronizer<IGroupHandle>::ISynchronizeCallback *)&synchronizeGroupCallback,
			db1, db2, parent1, parent2, &groups1, &groups2);
	groupListSynchronizer.synchronize();

	if (parent1 && parent2){
		QList<IEntryHandle*> entries1 = db1->entries(parent1);
		QList<IEntryHandle*> entries2 = db2->entries(parent2);

		statistic().entriesProcessed+=entries1.size()+entries2.size();

		SynchronizeEntryCallback synchronizeEntryCallback(this);
		AbstractListSynchronizer<IEntryHandle> entryListSynchronizer(
				(AbstractListSynchronizer<IEntryHandle>::ISynchronizeCallback *)&synchronizeEntryCallback,
				db1, db2, parent1, parent2, &entries1, &entries2);
		entryListSynchronizer.synchronize();
	}
}

void DatabaseSynchronizer::filterGroupsForSynchronization(QList<IGroupHandle*> & groups, IGroupHandle * parent){
	for (int i=0; i<groups.size(); ){
		if (groups[i]->parent()!=parent
#ifdef BACKUP_GROUP_TITLE
				|| (parent==NULL && groups[i]->title() == BACKUP_GROUP_TITLE)
#endif
				){
			groups.removeAt(i);
		}else{
			i++;
		}
	}
}

void DatabaseSynchronizer::synchronize(IDatabase * db1, IDatabase * db2){
	synchronizeGroups(db1, db2, NULL, NULL);
}


