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

#include "SocketInitiator.h"
#include "Session.h"
#include "Settings.h"

namespace FIX {
    SocketInitiator::SocketInitiator( Application& application,
        MessageStoreFactory& factory,
        const SessionSettings& settings )
        EXCEPT( ConfigError )
        : Initiator( application, factory, settings ),
        m_connector( 1 ), m_lastConnect( 0 ),
        m_reconnectInterval( 30 ), m_noDelay( false ), m_sendBufSize( 0 ),
        m_rcvBufSize( 0 ) {
    }

    SocketInitiator::SocketInitiator( Application& application,
        MessageStoreFactory& factory,
        const SessionSettings& settings,
        LogFactory& logFactory )
        EXCEPT( ConfigError )
        : Initiator( application, factory, settings, logFactory ),
        m_connector( 1 ), m_lastConnect( 0 ),
        m_reconnectInterval( 30 ), m_noDelay( false ), m_sendBufSize( 0 ),
        m_rcvBufSize( 0 ) {
    }

    SocketInitiator::~SocketInitiator( ) {
        m_connections.clear( );
        m_pendingConnections.clear( );
        //SocketConnections::iterator i;
        //for ( i = m_connections.begin( );
        //    i != m_connections.end( ); ++i ) {
        //    if ( i->second != nullptr ) {
        //        delete i->second;
        //        i->second = nullptr;
        //    }
        //}

        //for ( i = m_pendingConnections.begin( );
        //    i != m_pendingConnections.end( ); ++i ) {
        //    if ( i->second != nullptr ) {
        //        delete i->second;
        //        i->second = nullptr;
        //    }
        //}
    }

    void SocketInitiator::onConfigure( const SessionSettings& s )
        EXCEPT( ConfigError ) {
        const Dictionary& dict = s.get( );

        if ( dict.has( RECONNECT_INTERVAL ) )
            m_reconnectInterval = dict.getInt( RECONNECT_INTERVAL );
        if ( dict.has( SOCKET_NODELAY ) )
            m_noDelay = dict.getBool( SOCKET_NODELAY );
        if ( dict.has( SOCKET_SEND_BUFFER_SIZE ) )
            m_sendBufSize = dict.getInt( SOCKET_SEND_BUFFER_SIZE );
        if ( dict.has( SOCKET_RECEIVE_BUFFER_SIZE ) )
            m_rcvBufSize = dict.getInt( SOCKET_RECEIVE_BUFFER_SIZE );
    }

    void SocketInitiator::onInitialize( const SessionSettings& s )
        EXCEPT( RuntimeError ) {
    }

    void SocketInitiator::onStart( ) {
        connect( );

        while ( !isStopped( ) ) {
            m_connector.block( *this, false, 1.0 );
            onTimeout( m_connector );
        }

        time_t start = 0;
        time_t now = 0;

        ::time( &start );
        while ( isLoggedOn( ) ) {
            m_connector.block( *this );
            if ( ::time( &now ) - 5 >= start )
                break;
        }
    }

    bool SocketInitiator::onPoll( ) {
        time_t start = 0;
        time_t now = 0;

        if ( isStopped( ) ) {
            if ( start == 0 )
                ::time( &start );
            if ( !isLoggedOn( ) )
                return false;
            if ( ::time( &now ) - 5 >= start )
                return false;
        }

        m_connector.block( *this, true );
        return true;
    }

    void SocketInitiator::onStop( ) {
    }

    void SocketInitiator::doConnect( const SessionID& s, const Dictionary& d ) {
        try {
            std::string address;
            short port = 0;
            std::string sourceAddress;
            short sourcePort = 0;

            Session* session = Session::lookupSession( s );
            if ( !session->isSessionTime( UtcTimeStamp::now( ) ) ) return;

            Log* log = session->getLog( );

            getHost( s, d, address, port, sourceAddress, sourcePort );
            session->setHostInfo( address, port );
            log->onEvent( "Connecting to " + address + " on port " + IntConvertor::convert( (unsigned short)port ) + " (Source " + sourceAddress + ":" + IntConvertor::convert( (unsigned short)sourcePort ) + ")" );
            socket_handle result = m_connector.connect( address, port, m_noDelay, m_sendBufSize, m_rcvBufSize, sourceAddress, sourcePort );
            setPending( s );
            SocketConnection* soc = new SocketConnection( *this, s, result, &m_connector.getMonitor( ) );
            soc->hostName = std::string( address );
            m_pendingConnections[result] = std::move( soc );
        } catch ( std::exception& ) { }
    }

    void SocketInitiator::onConnect( SocketConnector&, socket_handle s ) {
        SocketConnections::iterator i = m_pendingConnections.find( s );
        if ( i == m_pendingConnections.end( ) ) return;
        SocketConnection* pSocketConnection = i->second;
        setHostUseReset( pSocketConnection->hostName );
        m_connections[s] = pSocketConnection;
        m_pendingConnections.erase( i );
        setConnected( pSocketConnection->getSession( )->getSessionID( ) );
        pSocketConnection->onTimeout( );
    }

    void SocketInitiator::onWrite( SocketConnector& connector, socket_handle s ) {
        SocketConnections::iterator i = m_connections.find( s );
        if ( i == m_connections.end( ) ) return;
        SocketConnection* pSocketConnection = i->second;
        if ( pSocketConnection->processQueue( ) )
            pSocketConnection->unsignal( );
    }

    bool SocketInitiator::onData( SocketConnector& connector, socket_handle s ) {
        SocketConnections::iterator i = m_connections.find( s );
        if ( i == m_connections.end( ) ) return false;
        SocketConnection* pSocketConnection = i->second;
        return pSocketConnection->read( connector );
    }

    void SocketInitiator::onDisconnect( SocketConnector&, socket_handle s ) {
        SocketConnections::iterator i = m_connections.find( s );
        SocketConnections::iterator j = m_pendingConnections.find( s );

        SocketConnection* pSocketConnection = 0;
        if ( i != m_connections.end( ) )
            pSocketConnection = i->second;
        if ( j != m_pendingConnections.end( ) )
            pSocketConnection = j->second;
        if ( !pSocketConnection )
            return;

        setDisconnected( pSocketConnection->getSession( )->getSessionID( ) );

        Session* pSession = pSocketConnection->getSession( );
        if ( pSession ) {
            pSession->disconnect( );
            setDisconnected( pSession->getSessionID( ) );
        }

        delete pSocketConnection;
        m_connections.erase( s );
        m_pendingConnections.erase( s );
    }

    void SocketInitiator::onError( SocketConnector& connector ) {
        onTimeout( connector );
    }

    void SocketInitiator::onTimeout( SocketConnector& ) {
        time_t now;
        ::time( &now );

        if ( ( now - m_lastConnect ) >= m_reconnectInterval ) {
            connect( );
            m_lastConnect = now;
        }

        SocketConnections::iterator i;
        for ( i = m_connections.begin( ); i != m_connections.end( ); ++i )
            i->second->onTimeout( );
    }
    void SocketInitiator::setHostUseReset( const std::string& host ) {
        if ( !host.empty( ) ) {
            auto it = _conInf.find( host );
            if ( it == _conInf.end( ) ) {
                _conInf.insert( std::make_pair( host, 0 ) );
            } else {
                _conInf[host] = 0;
            }
#ifdef TEST_LIB
            ::fprintf( stdout, "reset->: %s: %d\n", host.c_str(), _conInf[host] );
#endif // TEST_LIB
        }
        return;
    }
    int32_t SocketInitiator::hostUseCount( const std::string& host ) const {
        if ( host.empty( ) ) return 0;
        auto it = _conInf.find( host );
        if ( it == _conInf.end( ) ) {
            return 0;
        }
#ifdef TEST_LIB
        ::fprintf( stdout, "get-> %s: %d\n", host.c_str(), it->second );
#endif // TEST_LIB
        return it->second;
    }
    void SocketInitiator::setHostUse( const std::string& host ) {
        if ( !host.empty( ) ) {
            auto it = _conInf.find( host );
            if ( it == _conInf.end( ) ) {
                _conInf.insert( std::make_pair( host, 1 ) );
            } else {
                _conInf[host] = it->second + 1;
            }
#ifdef TEST_LIB
            ::fprintf( stdout, "set->: %s: %d\n", host.c_str( ), _conInf[host] );
#endif // TEST_LIB
        }
        return;
    }
    constexpr int32_t MAX_RETRY_COUNT = 10;
    void SocketInitiator::getHost( const SessionID& s, const Dictionary& d,
        std::string& address, short& port,
        std::string& sourceAddress, short& sourcePort ) {
        int num = 0;
        SessionToHostNum::iterator i = m_sessionToHostNum.find( s );
        if ( i != m_sessionToHostNum.end( ) ) num = i->second;

        std::stringstream hostStream;
        hostStream << SOCKET_CONNECT_HOST << num;
        std::string hostString = hostStream.str( );

        std::stringstream portStream;
        portStream << SOCKET_CONNECT_PORT << num;
        std::string portString = portStream.str( );

        sourcePort = 0;
        sourceAddress.clear( );

        if ( d.has( hostString ) && d.has( portString ) ) {
            address = d.getString( hostString );
            port = (short)d.getInt( portString );
            std::stringstream sourceHostStream;
            sourceHostStream << SOCKET_CONNECT_SOURCE_HOST << num;
            hostString = sourceHostStream.str( );
            if ( d.has( hostString ) ) {
                sourceAddress = d.getString( hostString );
            }
            std::stringstream sourcePortStream;
            sourcePortStream << SOCKET_CONNECT_SOURCE_PORT << num;
            portString = sourcePortStream.str( );
            if ( d.has( portString ) ) {
                sourcePort = (short)d.getInt( portString );
            }
        } else {
            num = 0;
            address = d.getString( SOCKET_CONNECT_HOST );
            port = (short)d.getInt( SOCKET_CONNECT_PORT );

            if ( d.has( SOCKET_CONNECT_SOURCE_HOST ) )
                sourceAddress = d.getString( SOCKET_CONNECT_SOURCE_HOST );
            if ( d.has( SOCKET_CONNECT_SOURCE_PORT ) )
                sourcePort = (short)d.getInt( SOCKET_CONNECT_SOURCE_PORT );
        }
        if ( hostUseCount( address ) < MAX_RETRY_COUNT ) {
            m_sessionToHostNum[s] = num;
        } else {
            _conInf.clear( );
            m_sessionToHostNum[s] = ++num;
        }
        setHostUse( address );
    }
}
