/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AMAROK_DATABASEUPDATER_H
#define AMAROK_DATABASEUPDATER_H

#include <QHash>
#include <QString>

class SqlCollection;

class DatabaseUpdater {
public:
    DatabaseUpdater( SqlCollection *collection );
    ~DatabaseUpdater();

    bool needsUpdate() const;
    void update();

    void createTemporaryTables();
    void prepareTemporaryTables(); //copies data into temporary tables
    void prepareTemporaryTablesForFullScan(); //copies data to temporary urls table
    void removeTemporaryTables();
    void copyToPermanentTables();
    void cleanPermanentTables();
    void upgradeVersion1to2();

    void deleteAllRedundant( const QString &type ); //type is artist,album,genre,composer or year

    void removeFilesInDir( int deviceid, const QString &rdir );
    void removeFilesInDirFromTemporaryTables( int deviceid, const QString &rdir );
    
    void cleanupDatabase();

private:
    /** creates all the necessary tables, indexes etc. for the database */
    void createTables() const;

    int adminValue( const QString &key ) const;

    SqlCollection *m_collection;

};

#endif
