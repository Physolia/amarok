/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
#ifndef PLAYDAR_QUERYOBJECT_H
#define PLAYDAR_QUERYOBJECT_H

#include "Controller.h"
#include "core/meta/Meta.h"
#include "../SpotifyMeta.h"

#include <QWeakPointer>

class KJob;

class QString;

namespace Spotify
{
    /**
     * A Query provides an interface to a single Spotify query.
     * Clients should recieve these from their Spotify::Controller,
     * rather than constructing them. After getting a Query, just
     * wait for newTrackAdded(), querySolved(), or queryDone(),
     * as appropriate. Using QueryPtr will make sure a neglected Query
     * gets deleted, and Controllers won't hold on to the reference,
     * so interested objects should be sure to store their QueryPtrs.
     * NOTE: artist(), album(), and title() are all empty until
     *       the first round of results has come in from Spotify.
     *       Querys are matched with results by qid().
     */
    class Query : public QObject
    {
        Q_OBJECT
        
        public:
            //Constructor and Destructor
            Query( const QString &qid, Spotify::Controller* controller, bool waitForSolution );
            ~Query();
            
            /** @return UID for this query given by Spotify */
            QString qid() const;
            /** @return Name of artist used in query */
            QString artist() const;
            /** @return Name of album used in query */
            QString album() const;
            /** @return Track title used in query */
            QString title() const;
            /**
             * @return @c true A track has received a 1.00 score
             * @return @c false No track has a 1.00 score
             */
            bool isSolved() const;
            
            /**
             * @return 0 if the query is unsolved, and an arbitrary track
             * with a perfect score of 1.00 if the query is solved.
             */
            Meta::SpotifyTrackPtr getSolution() const;
            /**
             * @return The (possibly empty) SpotifyTrackList containing any
             * results found so far.
             */
            Meta::SpotifyTrackList getTrackList() const;
            
        public Q_SLOTS:
            void receiveResults( KJob* );
            
        signals:
            /**
             * Emitted each time a new track is added to the list of results,
             * returning the latest result as a Meta::SpotifyTrack.
             */
            void newTrackAdded( Meta::SpotifyTrackPtr );
            /**
             * Emitted once if a query is solved, and returns the first track
             * with a perfect score of 1.00. Will not be emitted if the query
             * ends otherwise. (Emitted alongside newTrackAdded).
             */
            void querySolved( Meta::SpotifyTrackPtr );
            /**
             * Emitted once all results that may be found by our means have
             * been found, and returns the internal results list in its final state.
             */
            void queryDone( Spotify::Query*, Meta::SpotifyTrackList );
            /**
             * Indicates an error. Don't bother connecting to this if you're
             * already connected to Controller::error, since the controller
             * will pass the signal along.
             */
            void spotifyError( Spotify::Controller::ErrorState );
            
        private:
            QWeakPointer< Spotify::Controller > m_controller;
            bool m_waitForSolution;
            
            QString m_qid;
            QString m_artist;
            QString m_album;
            QString m_title;
            bool m_solved;
            
            bool m_receivedFirstResults;
            
            Meta::SpotifyTrackList m_trackList;
    };
}
#endif
