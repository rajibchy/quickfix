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

#ifdef HAVE_POSTGRESQL

#include "PostgreSQLLog.h"
#include "SessionID.h"
#include "SessionSettings.h"
#include "Utility.h"
#include "strptime.h"
#include <fstream>

namespace FIX
{

const std::string PostgreSQLLogFactory::DEFAULT_DATABASE = "quickfix";
const std::string PostgreSQLLogFactory::DEFAULT_USER = "postgres";
const std::string PostgreSQLLogFactory::DEFAULT_PASSWORD = "";
const std::string PostgreSQLLogFactory::DEFAULT_HOST = "localhost";
const short PostgreSQLLogFactory::DEFAULT_PORT = 0;

PostgreSQLLog::PostgreSQLLog
( const SessionID& s, const DatabaseConnectionID& d, PostgreSQLConnectionPool* p )
: m_pConnectionPool( p )
{
  init();
  m_pSessionID = new SessionID( s );
  m_pConnection = m_pConnectionPool->create( d );
}

PostgreSQLLog::PostgreSQLLog
( const DatabaseConnectionID& d, PostgreSQLConnectionPool* p )
: m_pConnectionPool( p ), m_pSessionID( 0 )
{
  init();
  m_pConnection = m_pConnectionPool->create( d );
}

PostgreSQLLog::PostgreSQLLog
( const SessionID& s, const std::string& database, const std::string& user,
  const std::string& password, const std::string& host, short port )
  : m_pConnectionPool( 0 )
{
  init();
  m_pSessionID = new SessionID( s );
  m_pConnection = new PostgreSQLConnection( database, user, password, host, port );
}

PostgreSQLLog::PostgreSQLLog
( const std::string& database, const std::string& user,
  const std::string& password, const std::string& host, short port )
  : m_pConnectionPool( 0 ), m_pSessionID( 0 )
{
  init();
  m_pConnection = new PostgreSQLConnection( database, user, password, host, port );
}

void PostgreSQLLog::init()
{
  setIncomingTable( "messages_log" );
  setOutgoingTable( "messages_log" );
  setEventTable( "event_log" );
}

PostgreSQLLog::~PostgreSQLLog()
{
  if( m_pConnectionPool )
    m_pConnectionPool->destroy( m_pConnection );
  else
    delete m_pConnection;
  delete m_pSessionID;
}

Log* PostgreSQLLogFactory::create()
{
  std::string database;
  std::string user;
  std::string password;
  std::string host;
  short port;

  init( m_settings.get(), database, user, password, host, port );
  DatabaseConnectionID id( database, user, password, host, port );
  PostgreSQLLog* result = new PostgreSQLLog( id, m_connectionPoolPtr.get() );
  initLog( m_settings.get(), *result );
  return result;
}

Log* PostgreSQLLogFactory::create( const SessionID& s )
{
  std::string database;
  std::string user;
  std::string password;
  std::string host;
  short port;

  Dictionary settings;
  if( m_settings.has(s) ) 
    settings = m_settings.get( s );

  init( settings, database, user, password, host, port );
  DatabaseConnectionID id( database, user, password, host, port );
  PostgreSQLLog* result = new PostgreSQLLog( s, id, m_connectionPoolPtr.get() );
  initLog( settings, *result );
  return result;
}

void PostgreSQLLogFactory::init( const Dictionary& settings, 
           std::string& database, 
           std::string& user,
           std::string& password,
           std::string& host,
           short &port )
{
  database = DEFAULT_DATABASE;
  user = DEFAULT_USER;
  password = DEFAULT_PASSWORD;
  host = DEFAULT_HOST;
  port = DEFAULT_PORT;

  if( m_useSettings )
  {
    try { database = settings.getString( POSTGRESQL_LOG_DATABASE ); }
    catch( ConfigError& ) {}

    try { user = settings.getString( POSTGRESQL_LOG_USER ); }
    catch( ConfigError& ) {}

    try { password = settings.getString( POSTGRESQL_LOG_PASSWORD ); }
    catch( ConfigError& ) {}

    try { host = settings.getString( POSTGRESQL_LOG_HOST ); }
    catch( ConfigError& ) {}

    try { port = ( short ) settings.getInt( POSTGRESQL_LOG_PORT ); }
    catch( ConfigError& ) {}
  }
  else
  {
    database = m_database;
    user = m_user;
    password = m_password;
    host = m_host;
    port = m_port;
  }
}

void PostgreSQLLogFactory::initLog( const Dictionary& settings, PostgreSQLLog& log )
{
  try { log.setIncomingTable( settings.getString( POSTGRESQL_LOG_INCOMING_TABLE ) ); }
  catch( ConfigError& ) {}

  try { log.setOutgoingTable( settings.getString( POSTGRESQL_LOG_OUTGOING_TABLE ) ); }
  catch( ConfigError& ) {}

  try { log.setEventTable( settings.getString( POSTGRESQL_LOG_EVENT_TABLE ) ); }
  catch( ConfigError& ) {}
}

void PostgreSQLLogFactory::destroy( Log* pLog )
{
  delete pLog;
}

void PostgreSQLLog::clear()
{
  std::stringstream whereClause;

  whereClause << "WHERE ";

  if( m_pSessionID )
  {
    whereClause
    << "BeginString = '" << m_pSessionID->getBeginString().getValue() << "' "
    << "AND SenderCompID = '" << m_pSessionID->getSenderCompID().getValue() << "' "
    << "AND TargetCompID = '" << m_pSessionID->getTargetCompID().getValue() << "' ";

    if( m_pSessionID->getSessionQualifier().size() )
      whereClause << "AND SessionQualifier = '" << m_pSessionID->getSessionQualifier() << "'";
  }
  else
  {
    whereClause << "BeginString = NULL AND SenderCompID = NULL && TargetCompID = NULL";
  }

  std::stringstream incomingQuery;
  std::stringstream outgoingQuery;
  std::stringstream eventQuery;

  incomingQuery 
    << "DELETE FROM " << m_incomingTable << " " << whereClause.str();
  outgoingQuery 
    << "DELETE FROM " << m_outgoingTable << " " << whereClause.str();
  eventQuery 
    << "DELETE FROM " << m_eventTable << " " << whereClause.str();

  PostgreSQLQuery incoming( incomingQuery.str() );
  PostgreSQLQuery outgoing( outgoingQuery.str() );
  PostgreSQLQuery event( eventQuery.str() );
  m_pConnection->execute( incoming );
  m_pConnection->execute( outgoing );
  m_pConnection->execute( event );
}

void PostgreSQLLog::backup()
{
   std::stringstream msgLogQuery;
   std::stringstream eventLogQuery;
   // copy data to backup table
   msgLogQuery
       << "INSERT INTO " << m_incomingTable << "_backup(msgtype, time, beginstring, sendercompid, targetcompid, session_qualifier, text, action_date)"
       << "SELECT msgtype, time, beginstring, sendercompid, targetcompid, session_qualifier, text, action_date FROM " << m_incomingTable << " ORDER BY id";

   eventLogQuery
       << "INSERT INTO " << m_eventTable << "_backup(msgtype, time, beginstring, sendercompid, targetcompid, session_qualifier, text, action_date)"
       << "SELECT msgtype, time, beginstring, sendercompid, targetcompid, session_qualifier, text, action_date FROM " << m_eventTable << " ORDER BY id";

   PostgreSQLQuery msgLog( msgLogQuery.str( ) );
   PostgreSQLQuery eventLog( eventLogQuery.str( ) );
   if ( !m_pConnection->execute( msgLog ) )
       msgLog.throwException( );
   if(!m_pConnection->execute( eventLog ) )
       eventLog.throwException( );
   // truncate table
   std::stringstream( ).swap( msgLogQuery );
   std::stringstream( ).swap( eventLogQuery );
   msgLogQuery
       << "TRUNCATE TABLE " << m_incomingTable << " RESTART IDENTITY";
   eventLogQuery
       << "TRUNCATE TABLE " << m_eventTable << " RESTART IDENTITY";
   msgLog.reset( msgLogQuery.str( ) );
   eventLog.reset( eventLogQuery.str( ) );
   if ( !m_pConnection->execute( msgLog ) )
       msgLog.throwException( );
   if ( !m_pConnection->execute( eventLog ) )
       eventLog.throwException( );
}
static void findFieldValue( const std::string& fixMessage, const std::string& key, std::string& result ) {
    size_t startIndex = fixMessage.find( key );
    if ( startIndex == std::string::npos )return;
    startIndex += key.size( ); // skip find key size
    // Check if the field value ends with the delimiter
    size_t endIndex = fixMessage.find( '\001', startIndex );
    if ( endIndex == std::string::npos ) {
        // The field value extends to the end of the message
        result = fixMessage.substr( startIndex );
    } else {
        // Normal case: extract the substring between startIndex and endIndex
        result = fixMessage.substr( startIndex, endIndex - startIndex );
    }
}
void PostgreSQLLog::insert( const std::string& table, const std::string& value )
{
  UtcTimeStamp time = UtcTimeStamp::now( );
  int year, month, day, hour, minute, second, millis;
  time.getYMD( year, month, day );
  time.getHMS( hour, minute, second, millis );

  char sqlTime[ 100 ];
  STRING_SPRINTF( sqlTime, "%d-%02d-%02d %02d:%02d:%02d.%03d",
           year, month, day, hour, minute, second, millis );
  
  char* valueCopy = new char[ (value.size() * 2) + 1 ];
  PQescapeString( valueCopy, value.c_str(), value.size() );

  std::stringstream queryString;
  queryString << "INSERT INTO " << table << " "
      << "(msgtype, time, beginstring, sendercompid, targetcompid, session_qualifier, text) "
      << "VALUES (";
  if ( table != m_eventTable ) {
      // message log table
      std::string msg_type;
      findFieldValue( value, "35=", msg_type );
      if ( msg_type.empty( ) ) {
          queryString << "NULL" << ",";
      } else {
          queryString << "'" << msg_type << "',";
      }
  } else {
      queryString << "NULL" << ",";
  }
  queryString << "'" << sqlTime << "',";

  if( m_pSessionID )
  {
    queryString
    << "'" << m_pSessionID->getBeginString().getValue() << "',"
    << "'" << m_pSessionID->getSenderCompID().getValue() << "',"
    << "'" << m_pSessionID->getTargetCompID().getValue() << "',";
    if( m_pSessionID->getSessionQualifier() == "" )
      queryString << "NULL" << ",";
    else
      queryString << "'" << m_pSessionID->getSessionQualifier() << "',";
  }
  else
  {
    queryString << "NULL, NULL, NULL, NULL, ";
  }

  queryString << "'" << valueCopy << "')";
  delete [] valueCopy;

  PostgreSQLQuery query( queryString.str() );
  m_pConnection->execute( query );
}

} // namespace FIX

#endif //HAVE_POSTGRESQL
