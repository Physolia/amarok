// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "app.h"
#include "collectiondb.h"
#include "sqlite/sqlite.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <qcstring.h>

#include <sys/stat.h>


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionDB
//////////////////////////////////////////////////////////////////////////////////////////

CollectionDB::CollectionDB()
        : m_weaver( new ThreadWeaver( this ) )
{
    QCString path = ( KGlobal::dirs() ->saveLocation( "data", kapp->instanceName() + "/" )
                  + "collection.db" ).local8Bit();

    m_db = sqlite_open( path, 0, 0 );
}


CollectionDB::~CollectionDB()
{
    sqlite_close( m_db );
}


QString
CollectionDB::escapeString( QString string )
{
    string.replace( "'", "''" );
    return string;
}


bool
CollectionDB::isEmpty()
{
    QStringList values;
    QStringList names;

    if ( execSql( "SELECT COUNT( url ) FROM tags;", &values, &names ) )
        return ( values[0] == "0" ); 
    else
        return true;
}


QString
CollectionDB::albumSongCount( const QString artist_id, const QString album_id )
{
    QStringList values;
    QStringList names;

    execSql( QString( "SELECT COUNT( url ) FROM tags WHERE album = %1 AND artist = %2;" )
             .arg( album_id )
             .arg( artist_id ), &values, &names );

    return values[0];
}


void
CollectionDB::addImageToPath( const QString path, const QString image, bool temporary )
{
    execSql( QString( "INSERT INTO images%1 ( path, name ) VALUES ( '%1', '%2' );" )
             .arg( temporary ? "_temp" : "" )
             .arg( escapeString( path ) )
             .arg( escapeString( image ) ) );
}


QString
CollectionDB::getImageForAlbum( const QString artist_id, const QString album_id, const QString defaultImage )
{
    QStringList values;
    QStringList names;

    execSql( QString( "SELECT url FROM tags WHERE album = %1 AND artist = %2;" )
             .arg( album_id )
             .arg( artist_id ), &values, &names );

    KURL url;
    url.setPath( values[0] ); 
             
    values.clear();
    names.clear();
    execSql( QString( "SELECT name FROM images WHERE path = '%1';" )
             .arg( escapeString( url.directory() ) ), &values, &names );

    if ( values.isEmpty() )
        return defaultImage;

    QString image( values[0] );
    for ( uint i = 0; i < values.count(); i++ )
    {
        if ( values[i].contains( "front", false ) )
            image = values[i];
    }

    return url.directory() + "/" + image;
}


void
CollectionDB::incSongCounter( const QString url )
{
    QStringList values, names;
    
    if ( execSql( QString( "SELECT playcounter FROM statistics WHERE url = '%1';" )
                  .arg( escapeString( url ) ), &values, &names ) )
    {
        // entry exists, increment playcounter and update accesstime
        execSql( QString( "REPLACE INTO statistics ( url, accessdate, playcounter ) VALUES ( '%1', strftime('%s', 'now'), %2 );" )
                .arg( escapeString( url ) )
                .arg( values[0] + " + 1" ) );
    } else
    {
        // entry didnt exist yet, create a new one
        execSql( QString( "INSERT INTO statistics ( url, createdate, accessdate, playcounter ) VALUES ( '%1', strftime('%s', 'now'), strftime('%s', 'now'), 1 );" )
                .arg( escapeString( url ) ) );
    }
}


void
CollectionDB::updateDirStats( QString path, const long datetime )
{
    if ( path.endsWith( "/" ) ) 
        path = path.left( path.length() - 1 );

    execSql( QString( "REPLACE INTO directories ( dir, changedate ) VALUES ( '%1', %2 );" )
             .arg( escapeString( path ) )
             .arg( datetime ) );
}


void
CollectionDB::removeSongsInDir( QString path )
{
    if ( path.endsWith( "/" ) ) 
        path = path.left( path.length() - 1 );

    execSql( QString( "DELETE FROM tags WHERE dir = '%1';" )
             .arg( escapeString( path ) ) );
}


bool
CollectionDB::isDirInCollection( QString path )
{
    QStringList values;
    QStringList names;
    
    if ( path.endsWith( "/" ) ) 
        path = path.left( path.length() - 1 );

    execSql( QString( "SELECT changedate FROM directories WHERE dir = '%1';" )
             .arg( escapeString( path ) ), &values, &names );
             
    return !values.isEmpty();
}


void
CollectionDB::removeDirFromCollection( QString path )
{
    if ( path.endsWith( "/" ) ) 
        path = path.left( path.length() - 1 );

    execSql( QString( "DELETE FROM directories WHERE dir = '%1';" )
             .arg( escapeString( path ) ) );
}


bool
CollectionDB::execSql( const QString& statement, QStringList* const values, QStringList* const names, const bool debug )
{
    if ( debug )
        kdDebug() << "SQL-query: " << statement << endl;

    if ( !m_db ) {
        kdWarning() << k_funcinfo << "SQLite pointer == NULL.\n";
        return false;
    }

    const char* tail;
    sqlite_vm* vm;
    char* errorStr;
    int error;
    //compile SQL program to virtual machine
    error = sqlite_compile( m_db, statement.local8Bit(), &tail, &vm, &errorStr );

    if ( error != SQLITE_OK ) {
        kdWarning() << k_funcinfo << "sqlite_compile error:\n";
        kdWarning() << errorStr << endl;
        kdWarning() << "on query: " << statement << endl;

        sqlite_freemem( errorStr );
        return false;
    }

    int number;
    const char** value;
    const char** colName;
    //execute virtual machine by iterating over rows
    while ( true ) {
        error = sqlite_step( vm, &number, &value, &colName );
        if ( error == SQLITE_DONE || error == SQLITE_ERROR )
            break;
        //iterate over columns
        for ( int i = 0; values && names && i < number; i++ ) {
            *values << QString::fromLocal8Bit( value [i] );
            *names << QString::fromLocal8Bit( colName[i] );
        }
    }
    //deallocate vm ressources
    sqlite_finalize( vm, &errorStr );

    if ( error != SQLITE_DONE ) {
        kdWarning() << k_funcinfo << "sqlite_step error.\n";
        kdWarning() << errorStr << endl;
        return false;
    }

    return true;
}


int
CollectionDB::sqlInsertID()
{
    if ( !m_db ) {
        kdWarning() << k_funcinfo << "SQLite pointer == NULL.\n";
        return -1;
    }

    return sqlite_last_insert_rowid( m_db );
}


void
CollectionDB::createTables( bool temporary )
{
    kdDebug() << k_funcinfo << endl;
    
    //create tag table
    execSql( QString( "CREATE %1 TABLE tags%2 ("
                        "url VARCHAR(100),"
                        "dir VARCHAR(100),"
                        "createdate INTEGER,"
                        "album INTEGER,"
                        "artist INTEGER,"
                        "genre INTEGER,"
                        "title VARCHAR(100),"
                        "year INTEGER,"
                        "comment VARCHAR(100),"
                        "track NUMBER(4) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );
    
    //create album table
    execSql( QString( "CREATE %1 TABLE album%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );
    
    //create artist table
    execSql( QString( "CREATE %1 TABLE artist%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );
    
    //create genre table
    execSql( QString( "CREATE %1 TABLE genre%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );
    
    //create year table
    execSql( QString( "CREATE %1 TABLE year%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );
    
    //create images table
    execSql( QString( "CREATE %1 TABLE images%2 ("
                        "path VARCHAR(100),"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create indexes
    execSql( QString( "CREATE INDEX album_idx%1 ON album%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "CREATE INDEX artist_idx%1 ON artist%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "CREATE INDEX genre_idx%1 ON genre%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "CREATE INDEX year_idx%1 ON year%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    
    if ( !temporary )
    {
        execSql( "CREATE INDEX album_tag ON tags( album );" );
        execSql( "CREATE INDEX artist_tag ON tags( artist );" );
        execSql( "CREATE INDEX genre_tag ON tags( genre );" );
        execSql( "CREATE INDEX year_tag ON tags( year );" );
        
        // create directory statistics database
        execSql( QString( "CREATE TABLE directories ("
                            "dir VARCHAR(100) UNIQUE,"
                            "changedate INTEGER );" ) );
    }
}


void
CollectionDB::dropTables( bool temporary )
{
    kdDebug() << k_funcinfo << endl;
    
    execSql( QString( "DROP TABLE tags%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE album%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE artist%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE genre%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE year%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE images%1;" ).arg( temporary ? "_temp" : "" ) );
}


void
CollectionDB::moveTempTables()
{
    execSql( "INSERT INTO tags SELECT * FROM tags_temp;" );
    execSql( "INSERT INTO album SELECT * FROM album_temp;" );
    execSql( "INSERT INTO artist SELECT * FROM artist_temp;" );
    execSql( "INSERT INTO genre SELECT * FROM genre_temp;" );
    execSql( "INSERT INTO year SELECT * FROM year_temp;" );
    execSql( "INSERT INTO images SELECT * FROM images_temp;" );
}


void
CollectionDB::createStatsTable()
{
    kdDebug() << k_funcinfo << endl;
    
    // create music statistics database
    execSql( QString( "CREATE TABLE statistics ("
                      "url VARCHAR(100) UNIQUE,"
                      "createdate INTEGER,"
                      "accessdate INTEGER,"
                      "playcounter INTEGER );" ) );
}


void
CollectionDB::dropStatsTable()
{
    kdDebug() << k_funcinfo << endl;
    
    execSql( "DROP TABLE statistics;" );
}


void
CollectionDB::purgeDirCache()
{
    execSql( "DELETE FROM directories;" );
}


void
CollectionDB::scan( const QStringList& folders, bool recursively )
{
    kdDebug() << k_funcinfo << endl;
    
    if ( !folders.isEmpty() )
        m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), folders, recursively, false ) );
}


void
CollectionDB::scanModifiedDirs( bool recursively )
{
    QStringList values;
    QStringList names;
    QStringList folders;
    struct stat statBuf;
    bool removedFiles = false;

    QString command = QString( "SELECT dir, changedate FROM directories;" );
    execSql( command, &values, &names );
    
    for ( uint i = 0; i < values.count() / 2; i++ )
    {
        if ( stat( values[i*2].local8Bit(), &statBuf ) == 0 )
        {
            if ( QString::number( (long)statBuf.st_mtime ) != values[i*2 + 1] )
            {
                folders << values[i*2];
                kdDebug() << "Collection dir changed: " << values[i*2] << endl;    
            }
        }
        else
        {
            // this folder has been removed
            removedFiles = true;
            removeSongsInDir( values[i*2] );
            removeDirFromCollection( values[i*2] );
            kdDebug() << "Collection dir removed: " << values[i*2] << endl;    
        }
    }
    
    if ( !folders.isEmpty() )
        m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), folders, recursively, true ) );
    else
        emit scanDone( removedFiles );
}


uint
CollectionDB::getValueID( QString name, QString value, bool autocreate, bool useTempTables )
{
    QStringList values;
    QStringList names;

    if ( useTempTables )
        name.append( "_temp" );

    QString command = QString( "SELECT id FROM %1 WHERE name LIKE '%2';" )
                      .arg( name )
                      .arg( escapeString( value ) );
    execSql( command, &values, &names );

    //check if item exists. if not, should we autocreate it?
    if ( values.isEmpty() && autocreate )
    {
        command = QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                  .arg( name )
                  .arg( escapeString( value ) );

        execSql( command );
        int id = sqlInsertID();
        return id;
    }

    return values[0].toUInt();
}


void
CollectionDB::dirDirty( const QString& path )
{
    kdDebug() << k_funcinfo << "Dirty: " << path << endl;

    m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), path, false, true ) );
}


void
CollectionDB::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) ThreadWeaver::Job::CollectionReader )
    {
        kdDebug() << k_funcinfo << endl;
        emit scanDone( true );
    }
}


void
CollectionDB::retrieveFirstLevel( QString category, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND ( " + category.lower() + ".name LIKE '%" + filter + "%' OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command = "SELECT DISTINCT " + category.lower() + ".name FROM tags, " + category.lower()
                    + " WHERE tags." + category.lower() + "=" + category.lower() + ".id " + filterToken
                    + " ORDER BY " + category.lower() + ".name DESC;";
    execSql( command, values, names );
}


void
CollectionDB::retrieveSecondLevel( QString itemText, QString category1, QString category2, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
        filter = escapeString( filter );

    QString command;
    if ( category2 == i18n( "None" ) )
    {
        if ( filter != "" )
            filterToken = "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' OR tags.title LIKE '%" + filter + "%' )";

        QString id = QString::number( getValueID( category1.lower(), itemText, false ) );
        command = "SELECT DISTINCT tags.title, tags.url FROM tags, " + category1.lower() + " WHERE tags."
                + category1.lower() + "=" + id + " " + filterToken + " ORDER BY tags.track DESC, tags.url DESC;";
    }
    else
    {
        if ( filter != "" )
            filterToken = "AND ( " + category1.lower() + ".id = tags." + category1.lower() + " AND " + category1.lower() + ".name LIKE '%" + filter + "%' OR tags.title LIKE '%" + filter + "%' )";

        QString id = QString::number( getValueID( category1.lower(), itemText, false ) );
        command = "SELECT DISTINCT " + category2.lower() + ".name, '0' FROM tags, " + category2.lower()
                + ", " + category1.lower() + " WHERE tags." + category2.lower() + "=" + category2.lower()
                + ".id AND tags." + category1.lower() + "=" + id + " " + filterToken + " ORDER BY "
                + category2.lower() + ".name DESC;";
    }

    execSql( command, values, names );
}


void
CollectionDB::retrieveThirdLevel( QString itemText1, QString itemText2, QString category1, QString category2, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' OR tags.title LIKE '%" + filter + "%' )";
    }

    QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
    QString id_sub = QString::number( getValueID( category2.lower(), itemText2, false ) );

    QString command = "SELECT tags.title, tags.url FROM tags, " + category1.lower() + " WHERE tags."
                    + category1.lower() + " = " + id + " AND tags." + category2.lower() + " = " + id_sub
                    + " AND tags." + category1.lower() + " = " + category1.lower() + ".id " + filterToken
                    + " ORDER BY tags.track DESC, tags.url DESC;";

    execSql( command, values, names );
}


void
CollectionDB::retrieveFirstLevelURLs( QString itemText, QString category, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND ( " + category.lower() + ".name LIKE '%" + filter + "%' OR tags.title LIKE '%" + filter + "%' )";
    }

    //query database for all tracks in our sub-category
    QString id = QString::number( getValueID( category.lower(), itemText, false ) );
    QString command = "SELECT DISTINCT tags.url FROM tags, " + category.lower() + " WHERE tags."
                    + category.lower() + "=" + category.lower() + ".id AND tags." + category.lower() 
                    + "=" + id + " " + filterToken + " ORDER BY tags.dir, tags.track, tags.url;";

    execSql( command, values, names );
}


void
CollectionDB::retrieveSecondLevelURLs( QString itemText1, QString itemText2, QString category1, QString category2, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND ( " + category1.lower() + ".id = tags." + category1.lower() + " AND " + category1.lower() + ".name LIKE '%" + filter + "%' OR tags.title LIKE '%" + filter + "%' )";
    }

    QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
    QString id_sub = QString::number( getValueID( category2.lower(), itemText2, false ) );

    QString command = "SELECT DISTINCT tags.url FROM tags, " + category1.lower() + " WHERE tags."
                    + category2.lower() + "=" + id_sub + " AND tags." + category1.lower() + "="
                    + id + " " + filterToken + " ORDER BY tags.track, tags.url;";

    execSql( command, values, names );
}


#include "collectiondb.moc"
