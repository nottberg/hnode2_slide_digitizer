#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "HNSDAction.h"

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

HNSDAction::HNSDAction()
{
    m_type = HNSD_AR_TYPE_NOTSET;
}

HNSDAction::~HNSDAction()
{

}

void
HNSDAction::setType( HNSD_AR_TYPE_T type )
{
    m_type = type;
}

bool
HNSDAction::decodeStartCapture( std::istream& bodyStream )
{
#if 0
    HNIrrigationZone zone;

    // Clear the update mask
    m_zoneUpdateMask = HNID_ZU_FLDMASK_CLEAR;

    // Parse the json body of the request
    try
    {
        // Attempt to parse the json    
        pjs::Parser parser;
        pdy::Var varRoot = parser.parse( bodyStream );

        // Get a pointer to the root object
        pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

        if( jsRoot->has( "name" ) )
        {
            zone.setName( jsRoot->getValue<std::string>( "name" ) );
            m_zoneUpdateMask |= HNID_ZU_FLDMASK_NAME;
        }

        if( jsRoot->has( "description" ) )
        {
            zone.setDesc( jsRoot->getValue<std::string>( "description" ) );
            m_zoneUpdateMask |= HNID_ZU_FLDMASK_DESC;
        }

        if( jsRoot->has( "secondsPerWeek" ) )
        {
            zone.setWeeklySeconds( jsRoot->getValue<uint>( "secondsPerWeek" ) );
            m_zoneUpdateMask |= HNID_ZU_FLDMASK_SPW;
        }

        if( jsRoot->has( "secondsMaxCycle" ) )
        {
            zone.setMaximumCycleTimeSeconds( jsRoot->getValue<uint>( "secondsMaxCycle" ) );
            m_zoneUpdateMask |= HNID_ZU_FLDMASK_MAXSPC;
        }

        if( jsRoot->has( "secondsMinCycle" ) )
        {
            zone.setMinimumCycleTimeSeconds( jsRoot->getValue<uint>( "secondsMinCycle" ) );
            m_zoneUpdateMask |= HNID_ZU_FLDMASK_MINSPC;
        }

        if( jsRoot->has( "swidList" ) )
        {
            pjs::Array::Ptr jsSWIDList = jsRoot->getArray( "swidList" );

            zone.clearSWIDSet();
            
            std::cout << "Zone Update - start" << std::endl;

            for( uint index = 0; index < jsSWIDList->size(); index++ )
            {
                std::string value = jsSWIDList->getElement<std::string>(index);
                std::cout << "Zone Update - value: " << value << std::endl;
                zone.addSWID( value );
            }
            
            m_zoneUpdateMask |= HNID_ZU_FLDMASK_SWLST;
        }
        
        if( zone.validateSettings() != HNIS_RESULT_SUCCESS )
        {
            std::cout << "updateZone validate failed" << std::endl;
            // zoneid parameter is required
            return true;
        }        
    }
    catch( Poco::Exception ex )
    {
        std::cout << "updateZone exception: " << ex.displayText() << std::endl;
        // Request body was not understood
        return true;
    }

    // Add the zone info to the list
    m_zoneList.push_back( zone );
#endif
    // Done
    return false;

}

HNSD_AR_TYPE_T
HNSDAction::getType()
{
    return m_type;
}

bool 
HNSDAction::hasRspContent( std::string &contentType )
{
    contentType.clear();

#if 0
    switch( m_type )
    {
        case HNSD_AR_TYPE_SWLIST:  
        case HNSD_AR_TYPE_ZONELIST:
        case HNSD_AR_TYPE_ZONEINFO:
        case HNSD_AR_TYPE_SCHINFO: 
        case HNSD_AR_TYPE_PLACELIST:
        case HNSD_AR_TYPE_PLACEINFO:
        case HNSD_AR_TYPE_IRRSTATUS:
        case HNSD_AR_TYPE_MODIFIERSLIST:
        case HNSD_AR_TYPE_MODIFIERINFO:
        case HNSD_AR_TYPE_SEQUENCESLIST:
        case HNSD_AR_TYPE_SEQUENCEINFO:
        case HNSD_AR_TYPE_INHIBITSLIST:
        case HNSD_AR_TYPE_INHIBITINFO:
        case HNSD_AR_TYPE_OPERATIONSLIST:
        case HNSD_AR_TYPE_OPERATIONINFO:
            contentType = "application/json";
            return true;

        default:
        break;
    }
#endif

    return false;
}

bool 
HNSDAction::hasNewObject( std::string &newID )
{
    newID.clear();

#if 0
    switch( m_type )
    {
        case HNSD_AR_TYPE_ZONECREATE:
            newID = getZoneID();
            return true;

        case HNSD_AR_TYPE_PLACECREATE:
            newID = getPlacementID();
            return true;

        case HNSD_AR_TYPE_MODIFIERCREATE:
            newID = getModifierID();
            return true;

        case HNSD_AR_TYPE_SEQUENCECREATE:
            newID = getSequenceID();
            return true;

        case HNSD_AR_TYPE_INHIBITCREATE:
            newID = getInhibitID();
            return true;

        case HNSD_AR_TYPE_OPERATIONCREATE:
            newID = getOperationID();
            return true;

        default:
        break;
    }
#endif

    return false;
}

bool 
HNSDAction::generateRspContent( std::ostream &ostr )
{
    #if 0
    switch( m_type )
    {
        case HNSD_AR_TYPE_SWLIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNSWDSwitchInfo >::iterator sit = refSwitchList().begin(); sit != refSwitchList().end(); sit++ )
            { 
                pjs::Object swObj;

                swObj.set( "swid", sit->getID() );
                swObj.set( "description", sit->getDesc() );
 
                jsRoot.add( swObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_ZONELIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNIrrigationZone >::iterator zit = refZoneList().begin(); zit != refZoneList().end(); zit++ )
            { 
                pjs::Object znObj;
                pjs::Array  jsSwitchList;

                znObj.set( "zoneid", zit->getID() );
                znObj.set( "name", zit->getName() );
                znObj.set( "description", zit->getDesc() );
                znObj.set( "secondsPerWeek", zit->getWeeklySeconds() );
                znObj.set( "secondsMaxCycle", zit->getMaximumCycleTimeSeconds() );
                znObj.set( "secondsMinCycle", zit->getMinimumCycleTimeSeconds() );

                // Compose Switch List
                for( std::set< std::string >::iterator sit = zit->getSWIDSetRef().begin(); sit != zit->getSWIDSetRef().end(); sit++ )
                {
                    jsSwitchList.add( *sit );
                }

                // Add Switch List field
                znObj.set( "swidList", jsSwitchList );

                jsRoot.add( znObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_ZONEINFO:
        {
            // Create a json root object
            pjs::Object  jsRoot;
            pjs::Array   jsSwitchList;
   
            std::vector< HNIrrigationZone >::iterator zone = refZoneList().begin();

            jsRoot.set( "zoneid", zone->getID() );
            jsRoot.set( "name", zone->getName() );
            jsRoot.set( "description", zone->getDesc() );
            jsRoot.set( "secondsPerWeek", zone->getWeeklySeconds() );
            jsRoot.set( "secondsMaxCycle", zone->getMaximumCycleTimeSeconds() );
            jsRoot.set( "secondsMinCycle", zone->getMinimumCycleTimeSeconds() );

            // Compose Switch List
            for( std::set< std::string >::iterator sit = zone->getSWIDSetRef().begin(); sit != zone->getSWIDSetRef().end(); sit++ )
            {
                jsSwitchList.add( *sit );
            }

            // Add Switch List field
            jsRoot.set( "swidList", jsSwitchList );

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_PLACELIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNIrrigationPlacement >::iterator cit = refPlacementsList().begin(); cit != refPlacementsList().end(); cit++ )
            { 
                pjs::Object cObj;
                pjs::Array dayList;
                pjs::Array zoneList;

                cObj.set( "placementid", cit->getID() );
                cObj.set( "name", cit->getName() );
                cObj.set( "description", cit->getDesc() );
                cObj.set( "startTime", cit->getStartTime().getHMSStr() );
                cObj.set( "endTime", cit->getEndTime().getHMSStr() );
                cObj.set( "rank", cit->getRank() );

                // Compose Day List, Empty equals everyday
                uint dayBits = cit->getDayBits();
 
                if( dayBits & HNSC_DBITS_SUNDAY )
                    dayList.add( "Sunday" );
                if( dayBits & HNSC_DBITS_MONDAY )
                    dayList.add( "Monday" );
                if( dayBits & HNSC_DBITS_TUESDAY )
                    dayList.add( "Tuesday" );
                if( dayBits & HNSC_DBITS_WEDNESDAY )
                    dayList.add( "Wednesday" );
                if( dayBits & HNSC_DBITS_THURSDAY )
                    dayList.add( "Thursday" );
                if( dayBits & HNSC_DBITS_FRIDAY )
                    dayList.add( "Friday" );
                if( dayBits & HNSC_DBITS_SATURDAY )
                    dayList.add( "Saturday" );

                // Add Daylist field
                cObj.set( "dayList", dayList );

                // Compose Zone List
                for( std::set< std::string >::iterator zit = cit->getZoneSetRef().begin(); zit != cit->getZoneSetRef().end(); zit++ )
                {
                    zoneList.add( *zit );
                }

                // Add Zonelist field
                cObj.set( "zoneList", zoneList );
                
                // Add new placement object to return list
                jsRoot.add( cObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_PLACEINFO:
        {
            // Create a json root object
            pjs::Object      jsRoot;
            pjs::Array dayList;
            pjs::Array zoneList;

            std::vector< HNIrrigationPlacement >::iterator placement = refPlacementsList().begin();

            jsRoot.set( "placementid", placement->getID() );
            jsRoot.set( "name", placement->getName() );
            jsRoot.set( "description", placement->getDesc() );
            jsRoot.set( "startTime", placement->getStartTime().getHMSStr() );
            jsRoot.set( "endTime", placement->getEndTime().getHMSStr() );
            jsRoot.set( "rank", placement->getRank() );

            // Compose Day List, Empty equals everyday
            uint dayBits = placement->getDayBits();
 
            if( dayBits & HNSC_DBITS_SUNDAY )
                dayList.add( "Sunday" );
            if( dayBits & HNSC_DBITS_MONDAY )
                dayList.add( "Monday" );
            if( dayBits & HNSC_DBITS_TUESDAY )
                dayList.add( "Tuesday" );
            if( dayBits & HNSC_DBITS_WEDNESDAY )
                dayList.add( "Wednesday" );
            if( dayBits & HNSC_DBITS_THURSDAY )
                dayList.add( "Thursday" );
            if( dayBits & HNSC_DBITS_FRIDAY )
                dayList.add( "Friday" );
            if( dayBits & HNSC_DBITS_SATURDAY )
                dayList.add( "Saturday" );

            // Add Daylist field
            jsRoot.set( "dayList", dayList );

            // Compose Zone List
            for( std::set< std::string >::iterator zit = placement->getZoneSetRef().begin(); zit != placement->getZoneSetRef().end(); zit++ )
            {
                zoneList.add( *zit );
            }

            // Add Zonelist field
            jsRoot.set( "zoneList", zoneList );

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_MODIFIERSLIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNIrrigationModifier >::iterator mit = refModifiersList().begin(); mit != refModifiersList().end(); mit++ )
            { 
                pjs::Object mObj;

                mObj.set( "modifierid", mit->getID() );
                mObj.set( "name", mit->getName() );
                mObj.set( "description", mit->getDesc() );
                mObj.set( "type", mit->getTypeAsStr() );
                mObj.set( "value", mit->getValue() );
                mObj.set( "zoneid", mit->getZoneID() );

                // Add new placement object to return list
                jsRoot.add( mObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_MODIFIERINFO:
        {
            // Create a json root object
            pjs::Object      jsRoot;

            std::vector< HNIrrigationModifier >::iterator modifier = refModifiersList().begin();

            jsRoot.set( "modifierid", modifier->getID() );
            jsRoot.set( "name", modifier->getName() );
            jsRoot.set( "description", modifier->getDesc() );
            jsRoot.set( "type", modifier->getTypeAsStr() );
            jsRoot.set( "value", modifier->getValue() );
            jsRoot.set( "zoneid", modifier->getZoneID() );

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_SEQUENCESLIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNIrrigationSequence >::iterator sit = refSequencesList().begin(); sit != refSequencesList().end(); sit++ )
            { 
                pjs::Object sObj;
                pjs::Array objIDList;

                sObj.set( "sequenceid", sit->getID() );
                sObj.set( "name", sit->getName() );
                sObj.set( "description", sit->getDesc() );
                sObj.set( "type", sit->getTypeAsStr() );
                sObj.set( "onDuration", sit->getOnDurationAsStr() );
                sObj.set( "offDuration", sit->getOffDurationAsStr() );
                
                // Compose Object ID List
                for( std::list< std::string >::iterator oit = sit->getObjIDListRef().begin(); oit != sit->getObjIDListRef().end(); oit++ )
                {
                    objIDList.add( *oit );
                }

                // Add Zonelist field
                sObj.set( "objIDList", objIDList );

                // Add new placement object to return list
                jsRoot.add( sObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_SEQUENCEINFO:
        {
            // Create a json root object
            pjs::Object jsRoot;
            pjs::Array  objIDList;

            std::vector< HNIrrigationSequence >::iterator sequence = refSequencesList().begin();

            jsRoot.set( "sequenceid", sequence->getID() );
            jsRoot.set( "name", sequence->getName() );
            jsRoot.set( "description", sequence->getDesc() );

            jsRoot.set( "type", sequence->getTypeAsStr() );
            jsRoot.set( "onDuration", sequence->getOnDurationAsStr() );
            jsRoot.set( "offDuration", sequence->getOffDurationAsStr() );
                
            // Compose Object ID List
            for( std::list< std::string >::iterator oit = sequence->getObjIDListRef().begin(); oit != sequence->getObjIDListRef().end(); oit++ )
            {
                objIDList.add( *oit );
            }

            // Add Zonelist field
            jsRoot.set( "objIDList", objIDList );

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_INHIBITSLIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNIrrigationInhibit >::iterator iit = refInhibitsList().begin(); iit != refInhibitsList().end(); iit++ )
            { 
                pjs::Object iObj;

                iObj.set( "inhibitid", iit->getID() );
                iObj.set( "name", iit->getName() );
                iObj.set( "type", iit->getTypeAsStr() );
                iObj.set( "expirationDateStr", iit->getExpirationDateStr() );
                iObj.set( "zoneid", iit->getZoneID() );

                // Add new placement object to return list
                jsRoot.add( iObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_INHIBITINFO:
        {
            // Create a json root object
            pjs::Object      jsRoot;

            std::vector< HNIrrigationInhibit >::iterator inhibit = refInhibitsList().begin();

            jsRoot.set( "inhibitid", inhibit->getID() );
            jsRoot.set( "name", inhibit->getName() );
            jsRoot.set( "type", inhibit->getTypeAsStr() );
            jsRoot.set( "expirationDateStr", inhibit->getExpirationDateStr() );
            jsRoot.set( "zoneid", inhibit->getZoneID() );

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_OPERATIONSLIST:
        {
            // Create a json root object
            pjs::Array jsRoot;

            for( std::vector< HNIrrigationOperation >::iterator oit = refOperationsList().begin(); oit != refOperationsList().end(); oit++ )
            { 
                pjs::Object opObj;

                opObj.set( "operationid", oit->getID() );
                opObj.set( "type", oit->getTypeAsStr() );

                // Add new placement object to return list
                jsRoot.add( opObj );
            }

            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_OPERATIONINFO:
        {
            // Create a json root object
            pjs::Object      jsRoot;

            std::vector< HNIrrigationOperation >::iterator operation = refOperationsList().begin();

            jsRoot.set( "operationid", operation->getID() );
            jsRoot.set( "type", operation->getTypeAsStr() );


            try { pjs::Stringifier::stringify( jsRoot, ostr, 1 ); } catch( ... ) { return true; }
        }
        break;

        case HNSD_AR_TYPE_SCHINFO:
        case HNSD_AR_TYPE_IRRSTATUS:
            Poco::StreamCopier::copyStream( refRspStream(), ostr );
        break;
    }
#endif

    // Success
    return false;
}

