/*
 * sync.h
 *
 *  Created on: Feb 22, 2009
 *      Author: boris
 */

#ifndef SYNC_H_
#define SYNC_H_

#include <qalgorithms.h>

#include "Database.h"
#include "Kdb3Database.h"


#define DELETE_POSTFIX ".deleteit" // TODO put it into settings
#define BACKUP_GROUP_TITLE "Backup"
//#undef BACKUP_GROUP_TITLE

class SynchronizationStatistik{
public:
	int groupsProcessed;
	int groupsSynchronized;
	int groupsCreated;
	int groupsDeleted;
	int entriesProcessed;
	int entriesSynchronized;
	int entriesCreated;
	int entriesDeleted;
	SynchronizationStatistik(){
		groupsProcessed=0;
		groupsSynchronized=0;
		groupsCreated=0;
		groupsDeleted=0;
		entriesProcessed=0;
		entriesSynchronized=0;
		entriesCreated=0;
		entriesDeleted=0;
	}
};

//class SynchronizeGroupCallback;
//class SynchronizeEntryCallback;

class DatabaseSynchronizer{
	friend class SynchronizeGroupCallback;
private:
	SynchronizationStatistik synchronizationStatistik;
public:
	void synchronize(IDatabase * db1, IDatabase * db2);
	SynchronizationStatistik& statistic(){
		return synchronizationStatistik;
	}
private:
	void synchronizeGroups(IDatabase * db1, IDatabase * db2, IGroupHandle* parent1, IGroupHandle* parent2);
	static void filterGroupsForSynchronization(QList<IGroupHandle*> & groups, IGroupHandle * parent);
};


#endif /* SYNC_H_ */
