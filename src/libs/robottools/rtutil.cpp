/**************************************************************************

    file        : rtutil.cpp
    copyright   : (C) 2007 by Mart Kelder                 
    web         : http://speed-dreams.sourceforge.net   

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
    		Robot tools utilities 
    @ingroup	robottools
*/

#include <algorithm>

#include <portability.h>
#include <tgf.h>

#include "robottools.h"

#define BUFFERSIZE 256

void RtGetCarindexString( int index, const char *bot_dname, char extended, char *result, int resultLength )
{
	char buffer[ BUFFERSIZE ];
	void *carnames_xml;

	if( !extended )
	{
		snprintf( result, resultLength, "%d", index );
	}
	else
	{
		snprintf( buffer, BUFFERSIZE, "%sdrivers/curcarnames.xml", GfLocalDir() );
		buffer[ BUFFERSIZE - 1 ] = '\0';
		carnames_xml = GfParmReadFile( buffer, GFPARM_RMODE_STD );
		if( carnames_xml )
		{
			snprintf( buffer, resultLength, "drivers/%s/%d", bot_dname, index );
			result = strncpy( result, GfParmGetStr( carnames_xml, buffer, "car name", "" ), resultLength );
			GfParmReleaseHandle( carnames_xml );
		}
		else
		{
			result[ 0 ] = '\0';
		}
	}

	result[ resultLength - 1 ] = '\0';
}

tdble getSpeedDepAccel(tdble speed, tdble maxAccel, tdble startAccel, tdble incUntilSpeed, tdble maxUntilSpeed, tdble decUntilSpeed)
{
	tdble accel = 0.0;
    if(speed < incUntilSpeed)
    {
        accel = startAccel + (maxAccel - startAccel) / incUntilSpeed * speed; // accel increases from startAccel to maxAccel from speed 0 to speed incUntilSpeed
    }
    else if (speed < maxUntilSpeed) 
    {
        accel = maxAccel; // accel is maxAccel until speed is lower than maxUntil speed
    }
    else if (speed < decUntilSpeed)
    {
        accel = (decUntilSpeed - speed) / (decUntilSpeed - maxUntilSpeed) * maxAccel; // accel decreases from maxAccel to 0.0 from speed maxUntilSpeed to speed decUntilSpeed
    }

    return accel;
}

int getSpeedDepGear(tdble speed, int currentGear)
{
                     // 0   60  100 150 200 250 km/h
    tdble gearUP[6] = {-1, 17, 27, 41, 55, 70}; //Game uses values in m/s: xyz m/s = (3.6 * xyz) km/h
    tdble gearDN[6] = {0,  0,  15, 23, 35, 48};

    int gear = currentGear;

    if (speed > gearUP[gear])
    {
        gear = std::min(5, currentGear + 1);
    }
    if (speed < gearDN[gear])
    {
        gear = std::max(1, currentGear - 1);
    }
    return gear;
}
