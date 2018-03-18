/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef ALBUMACTIONSCAPABILITY_H
#define ALBUMACTIONSCAPABILITY_H

#include "amarok_export.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/meta/forward_declarations.h"

namespace Capabilities
{
    /**
     * An ActionsCapability subclass that automatically adds actions that are suitable for
     * albums - the actions are automatically disabled based on what passed album supports.
     */
    class AMAROK_EXPORT AlbumActionsCapability : public ActionsCapability
    {
        public:
            /**
             * @param album album to create common actions for
             * @param actions custom actions to append after common actions
             */
            explicit AlbumActionsCapability( Meta::AlbumPtr album, QList<QAction *> actions = QList<QAction *>() );
            virtual ~AlbumActionsCapability();

        protected:
            Meta::AlbumPtr m_album;
    };
}

#endif // ALBUMACTIONSCAPABILITY_H
