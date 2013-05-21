#include "stdafx.h"
#include "RoomEvent.h"
#include "resource.h"
#include "Npc.h"

extern CRITICAL_SECTION g_region_critical;

CRoomEvent::CRoomEvent()
{
	m_iZoneNumber = 0;
	m_sRoomNumber = 0;
	m_byStatus	= 1;
	m_iInitMinX = 0;
	m_iInitMinZ = 0;
	m_iInitMaxX = 0;
	m_iInitMaxZ = 0;
	m_iEndMinX = 0;
	m_iEndMinZ = 0;
	m_iEndMaxX = 0;
	m_iEndMaxZ = 0;
	m_byCheck = 0;
	m_byRoomType = 0;

	Initialize();
}

CRoomEvent::~CRoomEvent()
{
}

void CRoomEvent::Initialize()
{
	m_tDelayTime = 0;
	m_byLogicNumber = 1;

	for(int i=0; i<MAX_CHECK_EVENT; i++)	{
		m_Logic[i].sNumber = 0;
		m_Logic[i].sOption_1 = 0;
		m_Logic[i].sOption_2 = 0;
		m_Exec[i].sNumber = 0;
		m_Exec[i].sOption_1 = 0;
		m_Exec[i].sOption_2 = 0;
	}
}

void CRoomEvent::MainRoom()
{
	// ���� �˻����� �ؾ� ����..
	BOOL bCheck = FALSE, bRunCheck = FALSE;
	int event_num  = m_Logic[m_byLogicNumber-1].sNumber;

	bCheck = CheckEvent(event_num);

	if( bCheck )	{
		event_num = m_Exec[m_byLogicNumber-1].sNumber; 
		bRunCheck = RunEvent( event_num );
		if( bRunCheck )	{
			//wsprintf(notify, "** �˸� : [%d]���� Ŭ���� �Ǿ���ϴ�. **", m_sRoomNumber);
			//g_pMain->SendSystemMsg(notify, PUBLIC_CHAT);
			m_byStatus = 3;
		}
	}
}

BOOL  CRoomEvent::CheckEvent(int event_num)
{
	int nMinute = 0, nOption_1 = 0, nOption_2 = 0;
	CNpc* pNpc = NULL;
	BOOL bRetValue = FALSE;

	if( m_byLogicNumber == 0 || m_byLogicNumber > MAX_CHECK_EVENT )	{
		TRACE("### Check Event Fail :: array overflow = %d ###\n", m_byLogicNumber);
		return FALSE;
	}

	switch( event_num )	{
	case 1:					// Ư�� ���͸� ���̴� ���
		nOption_1 = m_Logic[ m_byLogicNumber-1 ].sOption_1;
		pNpc = GetNpcPtr( nOption_1 );
		if( pNpc )	{
			if( pNpc->m_byChangeType == 100 )	return TRUE;
		}
		else	{
			TRACE("### CheckEvent Error : monster nid = %d, logic=%d ###\n", nOption_1, m_byLogicNumber);
		}
		//TRACE("---Check Event : monster dead = %d \n", nMonsterNid);
		break;
	case 2:					// ��� ���͸� �׿���
		bRetValue = CheckMonsterCount( 0, 0, 3 );
		if( bRetValue )	{
			TRACE("��� ���͸� �׿��� ����\n");
			return TRUE;
		}
		break;
	case 3:					// ��е��� ���߶�
		nMinute = m_Logic[ m_byLogicNumber-1 ].sOption_1;
		nMinute = nMinute * 60;								// ���� �ʷ� ��ȯ
		if (UNIXTIME >= m_tDelayTime + nMinute )	{		// ���ѽð� ����
			return TRUE;
		}
		break;
	case 4:					// ��ǥ�������� �̵�
		
		break;
	case 5:					// Ư�����͸� �ɼ�2�� ������ ��ŭ �׿���
		nOption_1 = m_Logic[ m_byLogicNumber-1 ].sOption_1;
		nOption_2 = m_Logic[ m_byLogicNumber-1 ].sOption_2;
		bRetValue = CheckMonsterCount( nOption_1, nOption_2, 1 );
		if( bRetValue )	{
			TRACE("Ư������(%d)�� %d���� ����\n", nOption_1, nOption_2);
			return TRUE;
		}
		break;
	default:
		TRACE("### Check Event Fail :: event number = %d ###\n", event_num);
		break;
	}

	return FALSE;
}

BOOL  CRoomEvent::RunEvent( int event_num )
{
	CNpc* pNpc = NULL;
	int nOption_1 = 0, nOption_2 = 0;
	BOOL bRetValue = FALSE;
	switch( event_num )	{
	case 1:					// �ٸ� ������ ����
		nOption_1 = m_Exec[ m_byLogicNumber-1 ].sOption_1;
		pNpc = GetNpcPtr( nOption_1 );
		if( pNpc )	{
			pNpc->m_byChangeType = 3;	// ���� �������ּ���...
			pNpc->SetLive();
		}
		else	{
			TRACE("### RunEvent Error : ���� ���� �� �� ���� = %d, logic=%d ###\n", nOption_1, m_byLogicNumber);
		}
		if( m_byCheck == m_byLogicNumber )	{	// ���� Ŭ����
			return TRUE;
		}
		else		m_byLogicNumber++;

		break;
	case 2:					// ���� ����
		nOption_1 = m_Exec[ m_byLogicNumber-1 ].sOption_1;
		pNpc = GetNpcPtr( nOption_1 );
		if( pNpc )	{
			
		}
		else	{
			TRACE("### RunEvent Error : �� ��� ���� ���� �� �� ���� = %d, logic=%d ###\n", nOption_1, m_byLogicNumber);
		}

		//wsprintf(notify, "** �˸� : [%d] ���� �����ϴ� **", m_sRoomNumber);
		//g_pMain->SendSystemMsg(notify, PUBLIC_CHAT);

		if( m_byCheck == m_byLogicNumber )	{	// ���� Ŭ����
			return TRUE;
		}
		else		m_byLogicNumber++;

		break;
	case 3:					// �ٸ� ���ͷ� ��ȯ
		if( m_byCheck == m_byLogicNumber )	{	// ���� Ŭ����
			return TRUE;
		}
		break;
	case 4:					// Ư������ �ɼ�2�� ��������ŭ ����
		nOption_1 = m_Exec[ m_byLogicNumber-1 ].sOption_1;
		nOption_2 = m_Exec[ m_byLogicNumber-1 ].sOption_2;
		bRetValue = CheckMonsterCount( nOption_1, nOption_2, 2 );

		//wsprintf(notify, "** �˸� : [%d, %d] ���� ���� **", nOption_1, nOption_2);
		//g_pMain->SendSystemMsg(notify, PUBLIC_CHAT);

		if( m_byCheck == m_byLogicNumber )	{	// ���� Ŭ����
			return TRUE;
		}
		else		m_byLogicNumber++;
		break;
	case 100:					// Ư������ �ɼ�2�� ��������ŭ ����
		nOption_1 = m_Exec[ m_byLogicNumber-1 ].sOption_1;
		nOption_2 = m_Exec[ m_byLogicNumber-1 ].sOption_2;

		TRACE("RunEvent - room=%d, option1=%d, option2=%d\n", m_sRoomNumber, nOption_1, nOption_2);
		if( nOption_1 != 0 )	{
			EndEventSay( nOption_1, nOption_2 );
		}
		if( m_byCheck == m_byLogicNumber )	{	// ���� Ŭ����
			return TRUE;
		}
		else		m_byLogicNumber++;
		break;
	default:
		TRACE("### RunEvent Fail :: event number = %d ###\n", event_num);
		break;
	}

	return FALSE;
}

CNpc* CRoomEvent::GetNpcPtr( int sid )
{
	int count = 0;
	
	EnterCriticalSection( &g_region_critical );
	int nMonster = m_mapRoomNpcArray.GetSize();
	if( nMonster == 0 )	{
		TRACE("### RoomEvent-GetNpcPtr() : monster empty ###\n");
		LeaveCriticalSection( &g_region_critical );
		return NULL;
	}

	int *pIDList = new int[nMonster];
	foreach_stlmap (itr, m_mapRoomNpcArray)
		pIDList[count++] = *itr->second;

	LeaveCriticalSection( &g_region_critical );

	for(int i=0 ; i<nMonster; i++ ) {
		int nMonsterid = pIDList[i];
		if( nMonsterid < 0 )	continue;
		CNpc *pNpc = g_pMain->m_arNpc.GetData( nMonsterid );
		if( !pNpc )		continue;
		if( pNpc->m_proto->m_sSid == sid )	{
			if(pIDList)	{
				delete [] pIDList;
				pIDList = NULL;
			}
			return pNpc;
		}
	}

	if (pIDList)
		delete [] pIDList;

	return NULL;
}

BOOL  CRoomEvent::CheckMonsterCount( int sid, int count, int type )
{
	int nMonsterCount = 0, nTotalMonster = 0;
	BOOL bRetValue = FALSE;
	
	EnterCriticalSection( &g_region_critical );

	int nMonster = m_mapRoomNpcArray.GetSize();
	if( nMonster == 0 )	{
		TRACE("### RoomEvent-GetNpcPtr() : monster empty ###\n");
		LeaveCriticalSection( &g_region_critical );
		return NULL;
	}
	
	int *pIDList = new int[nMonster];
	foreach_stlmap (itr, m_mapRoomNpcArray)
		pIDList[nTotalMonster++] = *itr->second;

	LeaveCriticalSection( &g_region_critical );

	for(int i=0 ; i<nMonster; i++ ) {
		CNpc *pNpc = g_pMain->m_arNpc.GetData(pIDList[i]);
		if( !pNpc )		continue;
		if( type == 4 )	{
			if( pNpc->m_byRegenType == 2 )	pNpc->m_byRegenType = 0;
			pNpc->m_byChangeType = 0;
		}
		else if( type == 3 )	{				// ��� ���͸� �׾������� �Ǵ�
			if( pNpc->m_byDeadType == 100 )	nMonsterCount++;
			if( nMonsterCount == nMonster )	bRetValue = TRUE;
		}
		else	if( pNpc->m_proto->m_sSid == sid )	{
			if( type == 1 )	{					// Ư�� ���Ͱ� ������ ��ŭ �׾������� �Ǵ�
				if( pNpc->m_byChangeType == 100 )	nMonsterCount++;
				if( nMonsterCount == count )	bRetValue = TRUE;
			}
			else if( type == 2 )	{			// Ư�� ���͸� ������ ��ŭ ���� ���Ѷ�,,
				pNpc->m_byChangeType = 3;	nMonsterCount++;
				if( nMonsterCount == count )	bRetValue = TRUE;
			}
		}
	}

	if (pIDList)
		delete [] pIDList;

	return bRetValue;
}

void CRoomEvent::InitializeRoom()
{
	m_byStatus	= 1;			
	m_tDelayTime = 0;
	m_byLogicNumber = 1;

	CheckMonsterCount( 0, 0, 4);	// ������ m_byChangeType=0���� �ʱ�ȭ 
}

void CRoomEvent::EndEventSay( int option1, int option2 )
{
	char buff[512] = {0};

	switch (option1)
	{
		case 1:
		{ 
			switch (option2)
			{
			case 1:
				LoadString(NULL, IDS_KARUS_CATCH_1, buff, sizeof(buff));
				break;
			case 2:
				LoadString(NULL, IDS_KARUS_CATCH_2, buff, sizeof(buff));
				break;
			case 11:
				LoadString(NULL, IDS_ELMORAD_CATCH_1, buff, sizeof(buff));
				break;
			case 12:
				LoadString(NULL, IDS_ELMORAD_CATCH_2, buff, sizeof(buff));
				break;
			}

			g_pMain->SendSystemMsg(buff, WAR_SYSTEM_CHAT);
		} break;

		case 2:
			LoadString(NULL, IDS_KARUS_PATHWAY + (option2-1), buff, sizeof(buff));
			g_pMain->SendSystemMsg(buff, WAR_SYSTEM_CHAT);

			// this is normal, we need to send the following packet as well.

		case 3:
		{
			Packet result(AG_BATTLE_EVENT, uint8(BATTLE_MAP_EVENT_RESULT));
			result << uint8(option2);
			g_pMain->Send(&result);
		} break;
	}
}
