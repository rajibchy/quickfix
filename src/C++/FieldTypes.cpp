/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "FieldTypes.h"

#ifdef HAVE_FTIME
# include <sys/timeb.h>
#endif //
#include <chrono>

namespace FIX {
DateTime DateTime::nowUtc()
{
#if defined( _POSIX_SOURCE ) || defined(HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday (&tv, 0);
    return fromUtcTimeT( tv.tv_sec, tv.tv_usec, 6 );
#elif defined( HAVE_FTIME )
    timeb tb;
    ftime (&tb);
    return fromUtcTimeT (tb.time, tb.millitm);
#else
    //return fromUtcTimeT( ::time( 0 ), 0 );

    // Get the current time point using high-resolution clock
    auto now = std::chrono::system_clock::now( ); // or std::chrono::high_resolution_clock::now( );
    // Get the duration since epoch
    auto duration = now.time_since_epoch( );
    // Extract time_t value and milliseconds within the current second
    time_t t = std::chrono::duration_cast<std::chrono::seconds>( duration ).count( );
    int millis = static_cast<int>( t % 1000 );
    return fromUtcTimeT( t, millis );
#endif
}
int DateTime::nowMiles( ) {
    // Get the current time point using high-resolution clock
    auto now = std::chrono::system_clock::now( ); // or std::chrono::high_resolution_clock::now( );
    // Get the duration since epoch
    auto duration = now.time_since_epoch( );
    // Extract time_t value and milliseconds within the current second
    time_t t = std::chrono::duration_cast<std::chrono::seconds>( duration ).count( );
    return static_cast<int>( t % 1000 );
}
DateTime DateTime::nowLocal()
{
#if defined( _POSIX_SOURCE ) || defined(HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday (&tv, 0);
    return fromLocalTimeT( tv.tv_sec, tv.tv_usec, 6 );
#elif defined( HAVE_FTIME )
    timeb tb;
    ftime (&tb);
    return fromLocalTimeT( tb.time, tb.millitm );
#else
    //return fromLocalTimeT( ::time( 0 ), 0 );
    
    // Get the current time point using high-resolution clock
    auto now = std::chrono::system_clock::now( ); // or std::chrono::high_resolution_clock::now( );
    // Get the duration since epoch
    auto duration = now.time_since_epoch( );
    // Extract time_t value and milliseconds within the current second
    time_t t = std::chrono::duration_cast<std::chrono::seconds>( duration ).count( );
    int millis = static_cast<int>( t % 1000 );
    return fromLocalTimeT( t, millis );
#endif
}

}
