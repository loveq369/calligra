/* This file is part of the KDE project
   Copyright (C) 2001, The Karbon Developers
   Copyright (C) 2002, The Karbon Developers

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <koApplication.h>

#include "karbon_factory.h"


static const KCmdLineOptions options[] =
	{
		{ "+[file]", I18N_NOOP( "File to open" ), 0 } ,
		KCmdLineLastOption
	};

int main( int argc, char* argv[] )
{
	KCmdLineArgs::init( argc, argv, KarbonFactory::aboutData() );
	KCmdLineArgs::addCmdLineOptions( options );
	KoApplication app;

	app.dcopClient()->attach();
	app.dcopClient()->registerAs( "karbon" );

	if( !app.start() )  // parses command line args, create initial docs and shells
		return 1;

	return app.exec();
}

