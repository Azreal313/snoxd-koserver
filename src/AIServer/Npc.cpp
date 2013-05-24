#include "stdafx.h"
#include "Region.h"
#include "extern.h"
#include "Npc.h"
#include "User.h"
#include "NpcThread.h"

// 1m
//float surround_fx[8] = {0.0f, -0.7071f, -1.0f, -0.7083f,  0.0f,  0.7059f,  1.0000f, 0.7083f};
//float surround_fz[8] = {1.0f,  0.7071f,  0.0f, -0.7059f, -1.0f, -0.7083f, -0.0017f, 0.7059f};
// 2m
static float surround_fx[8] = {0.0f, -1.4142f, -2.0f, -1.4167f,  0.0f,  1.4117f,  2.0000f, 1.4167f};
static float surround_fz[8] = {2.0f,  1.4142f,  0.0f, -1.4167f, -2.0f, -1.4167f, -0.0035f, 1.4117f};

#define ATROCITY_ATTACK_TYPE 1				// ����
#define TENDER_ATTACK_TYPE	 0				// �İ�	

// �ൿ���� ���� 
#define NO_ACTION				0
#define ATTACK_TO_TRACE			1				// ���ݿ��� �߰�
#define MONSTER_CHANGED			1 
#define LONG_ATTACK_RANGE		30				// ��Ÿ� ���� ��ȿ�Ÿ�
#define SHORT_ATTACK_RANGE		3				// �������� ��ȿ�Ÿ�

#define ARROW_MIN				391010000
#define ARROW_MAX				392010000

#define ATTACK_LIMIT_LEVEL		10
#define FAINTING_TIME			2 // in seconds

bool CNpc::SetUid(float x, float z, int id)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr) 
	{
		TRACE("#### Npc-SetUid Zone Fail : [name=%s], zone=%d #####\n", m_proto->m_strName, m_bCurZone);
		return false;
	}

	int x1 = (int)x / TILE_SIZE;
	int z1 = (int)z / TILE_SIZE;
	int nRX = (int)x / VIEW_DIST;
	int nRY = (int)z / VIEW_DIST;

	if(x1 < 0 || z1 < 0 || x1 >= pMap->GetMapSize() || z1 >= pMap->GetMapSize())
	{
		TRACE("#### SetUid failed : [nid=%d, sid=%d, zone=%d], coordinates out of range of map x=%d, z=%d, map size=%d #####\n", 
			m_sNid+NPC_BAND, m_proto->m_sSid, m_bCurZone, x1, z1, pMap->GetMapSize());
		return false;
	}

	// map �̵��� �Ұ����̸� npc��� ����.. 
	// �۾� : �� �κ��� ���߿� ���� ó��....
	// if(pMap->m_pMap[x1][z1].m_sEvent == 0) return false;
	if(nRX > pMap->GetXRegionMax() || nRY > pMap->GetZRegionMax() || nRX < 0 || nRY < 0)
	{
		TRACE("#### SetUid Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, nRX, nRY);
		return false;
	}

	if(m_iRegion_X != nRX || m_iRegion_Z != nRY)
	{
		int nOld_RX = m_iRegion_X;
		int nOld_RZ = m_iRegion_Z;
		m_iRegion_X = nRX;		m_iRegion_Z = nRY;

		//TRACE("++ Npc-SetUid RegionAdd : [nid=%d, name=%s], x=%.2f, z=%.2f, nRX=%d, nRZ=%d \n", m_sNid+NPC_BAND, m_proto->m_strName,x,z, nRX, nRY);
		// ���ο� region���� npc�̵� - npc�� ���� �߰�..
		CNpc* pNpc = g_pMain->m_arNpc.GetData( id-NPC_BAND );
		if(pNpc == nullptr)
			return false;
		pMap->RegionNpcAdd(m_iRegion_X, m_iRegion_Z, id);

		// ������ region�������� npc�� ���� ����..
		pMap->RegionNpcRemove(nOld_RX, nOld_RZ, id);
		//TRACE("-- Npc-SetUid RegionRemove : [nid=%d, name=%s], nRX=%d, nRZ=%d \n", m_sNid+NPC_BAND, m_proto->m_strName, nOld_RX, nOld_RZ);
	}

	return true;
}

CNpc::CNpc() : m_NpcState(NPC_LIVE), m_byGateOpen(0), m_byObjectType(NORMAL_OBJECT), m_byPathCount(0),
	m_byAttackPos(0), m_ItemUserLevel(0), m_Delay(0), 
	m_proto(nullptr), m_pZone(nullptr), m_pPath(nullptr)
{
	InitTarget();

	m_fDelayTime = getMSTime();

	m_tNpcAttType = ATROCITY_ATTACK_TYPE;		// ���� ����
	m_tNpcOldAttType = ATROCITY_ATTACK_TYPE;		// ���� ����
	m_tNpcLongType = 0;		// ���Ÿ�(1), �ٰŸ�(0)
	m_tNpcGroupType = 0;	// ������ �ִ³�(1), ���ִ³�?(0)
	m_byNpcEndAttType = 1;
	m_byWhatAttackType = 0;
	m_byMoveType = 1;
	m_byInitMoveType  = 1;
	m_byRegenType = 0;
	m_byDungeonFamily = 0;
	m_bySpecialType = 0;
	m_byTrapNumber = 0;
	m_byChangeType = 0;
	m_byDeadType = 0;
	m_sChangeSid = 0;
	m_sControlSid = 0;
	m_sPathCount = 0;
	m_sMaxPathCount = 0;
	m_byMoneyType = 0;

	m_bFirstLive = true;

	m_fHPChangeTime = getMSTime();
	m_tFaintingTime = 0;

	m_iRegion_X = 0;
	m_iRegion_Z = 0;
	m_nLimitMinX = m_nLimitMinZ = 0;
	m_nLimitMaxX = m_nLimitMaxZ = 0;
	m_lEventNpc = 0;
	m_fSecForRealMoveMetor = 0.0f;
	InitUserList();
	InitMagicValuable();

	m_bTracing = false;
	m_fTracingStartX = m_fTracingStartZ = 0.0f;

	for(int i=0; i<NPC_MAX_PATH_LIST; i++)	{
		m_PathList.pPattenPos[i].x = -1;
		m_PathList.pPattenPos[i].z = -1;
	}
	m_pPattenPos.x = m_pPattenPos.z = 0;

}

CNpc::~CNpc()
{
	ClearPathFindData();
	InitUserList();
}

///////////////////////////////////////////////////////////////////////
//	��ã�� �����͸� �����.
//
void CNpc::ClearPathFindData()
{
	m_bPathFlag = false;
	m_sStepCount = 0;
	m_iAniFrameCount = 0;
	m_iAniFrameIndex = 0;
	m_fAdd_x = 0.0f;	m_fAdd_z = 0.0f;

	for(int i=0; i<MAX_PATH_LINE; i++)
	{
		m_pPoint[i].byType = 0;
		m_pPoint[i].bySpeed = 0;
		m_pPoint[i].fXPos = -1.0f;
		m_pPoint[i].fZPos = -1.0f;
	}
}

/////////////////////////////////////////////////////////////////////////////
//	��������Ʈ�� �ʱ�ȭ�Ѵ�.
//
void CNpc::InitUserList()
{
	m_sMaxDamageUserid = -1;
	m_TotalDamage = 0;
	for(int i = 0; i < NPC_HAVE_USER_LIST; i++)
	{
		m_DamagedUserList[i].bIs = false;
		m_DamagedUserList[i].iUid = -1;
		m_DamagedUserList[i].nDamage = 0;
		::ZeroMemory(m_DamagedUserList[i].strUserID, sizeof(m_DamagedUserList[i].strUserID));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	���ݴ��(Target)�� �ʱ�ȭ �Ѵ�.
//
void CNpc::InitTarget()
{
	if(m_byAttackPos != 0)
	{
		if(m_Target.id >= 0 && m_Target.id < NPC_BAND)
		{
			CUser* pUser = g_pMain->GetUserPtr(m_Target.id);
			if(pUser != nullptr)
			{
				if(m_byAttackPos > 0 && m_byAttackPos < 9)
					pUser->m_sSurroundNpcNumber[m_byAttackPos-1] = -1;
			}
		}
	}
	m_byAttackPos = 0;
	m_Target.id = -1;
	m_Target.x = 0.0;
	m_Target.y = 0.0;
	m_Target.z = 0.0;
	m_Target.failCount = 0;
	m_bTracing = false;
}


//	NPC �⺻���� �ʱ�ȭ
void CNpc::Init()
{
	m_pZone = g_pMain->GetZoneByID(m_bCurZone);
	m_Delay = 0;
	m_fDelayTime = getMSTime();

	if (GetMap() == nullptr) 
	{
		TRACE("#### Npc-Init Zone Fail : [name=%s], zone=%d #####\n", m_proto->m_strName, m_bCurZone);
		return;
	}
}

//	NPC �⺻��ġ ���� �ʱ�ȭ(�н� ���� �����̴� NPC�� ������ ���߾� �ش�..
void CNpc::InitPos()
{
	float fDD = 1.5f;
	if(m_byBattlePos == 0)	{
		m_fBattlePos_x = 0.0f;
		m_fBattlePos_z = 0.0f;
	}
	else if(m_byBattlePos == 1)	{
		float fx[5] = {0.0f, -(fDD*2),  -(fDD*2), -(fDD*4),  -(fDD*4)};
		float fz[5] = {0.0f,  (fDD*1),  -(fDD*1),  (fDD*1),  -(fDD*1)};
		m_fBattlePos_x = fx[m_byPathCount-1];
		m_fBattlePos_z = fz[m_byPathCount-1];
	}
	else if(m_byBattlePos == 2)	{
		float fx[5] = {0.0f,  0.0f, -(fDD*2), -(fDD*2), -(fDD*2)};
		float fz[5] = {0.0f, -(fDD*2), (fDD*1), (fDD*1), (fDD*3)};
		m_fBattlePos_x = fx[m_byPathCount-1];
		m_fBattlePos_z = fz[m_byPathCount-1];
	}
	else if(m_byBattlePos == 3)	{
		float fx[5] = {0.0f, -(fDD*2),  -(fDD*2), -(fDD*2), -(fDD*4)};
		float fz[5] = {0.0f,  (fDD*2),   0.0f,    -(fDD*2),  0.0f};
		m_fBattlePos_x = fx[m_byPathCount-1];
		m_fBattlePos_z = fz[m_byPathCount-1];
	}
}

void CNpc::InitMagicValuable()
{
	for(int i=0; i<MAX_MAGIC_TYPE4; i++)	{
		m_MagicType4[i].byAmount = 100;
		m_MagicType4[i].sDurationTime = 0;
		m_MagicType4[i].tStartTime = 0;
	}

	for(int i=0; i<MAX_MAGIC_TYPE3; i++)	{
		m_MagicType3[i].sHPAttackUserID = -1;
		m_MagicType3[i].sHPAmount = 0;
		m_MagicType3[i].byHPDuration = 0;
		m_MagicType3[i].byHPInterval = 2;
		m_MagicType3[i].tStartTime = 0;
	}
}

void CNpc::Load(uint16 sNpcID, CNpcTable * proto)
{
	m_sNid = sNpcID;
	m_proto = proto;

	m_sSize				= proto->m_sSize;		// ĳ������ ����(100 �ۼ�Ʈ ����)
	m_iWeapon_1			= proto->m_iWeapon_1;	// ���빫��
	m_iWeapon_2			= proto->m_iWeapon_2;	// ���빫��
	m_byGroup			= proto->m_byGroup;		// �Ҽ�����
	m_byActType			= proto->m_byActType;	// �ൿ����
	m_byRank			= proto->m_byRank;		// ����
	m_byTitle			= proto->m_byTitle;		// ����
	m_iSellingGroup		= proto->m_iSellingGroup;
	m_iHP				= proto->m_iMaxHP;		// �ִ� HP
	m_iMaxHP			= proto->m_iMaxHP;		// ���� HP
	m_sMP				= proto->m_sMaxMP;		// �ִ� MP
	m_sMaxMP			= proto->m_sMaxMP;		// ���� MP
	m_sAttack			= proto->m_sAttack;		// ���ݰ�
	m_sDefense			= proto->m_sDefense;	// ��
	m_sHitRate			= proto->m_sHitRate;	// Ÿ�ݼ�����
	m_sEvadeRate		= proto->m_sEvadeRate;	// ȸ�Ǽ�����
	m_sDamage			= proto->m_sDamage;		// �⺻ ������
	m_sAttackDelay		= proto->m_sAttackDelay;// ���ݵ�����
	m_sSpeed			= proto->m_sSpeed;		// �̵��ӵ�

	// Object NPCs should have an effective speed of 1x (not that it should matter, mind)
	if (m_byObjectType == SPECIAL_OBJECT)
		m_sSpeed = 1000;

	m_fSpeed_1			= (float)(proto->m_bySpeed_1 * (m_sSpeed / 1000));	// �⺻ �̵� Ÿ��
	m_fSpeed_2			= (float)(proto->m_bySpeed_2 * (m_sSpeed / 1000));	// �ٴ� �̵� Ÿ��..
	m_fOldSpeed_1		= (float)(proto->m_bySpeed_1 * (m_sSpeed / 1000));	// �⺻ �̵� Ÿ��
	m_fOldSpeed_2		= (float)(proto->m_bySpeed_2 * (m_sSpeed / 1000));	// �ٴ� �̵� Ÿ��..

	m_fSecForMetor		= 4.0f;						// �ʴ� �� �� �ִ� �Ÿ�..
	m_sStandTime		= proto->m_sStandTime;	// ���ִ� �ð�
	m_byFireR			= proto->m_byFireR;		// ȭ�� ���׷�
	m_byColdR			= proto->m_byColdR;		// �ñ� ���׷�
	m_byLightningR		= proto->m_byLightningR;	// ���� ���׷�
	m_byMagicR			= proto->m_byMagicR;	// ���� ���׷�
	m_byDiseaseR		= proto->m_byDiseaseR;	// ���� ���׷�
	m_byPoisonR			= proto->m_byPoisonR;	// �� ���׷�
	m_bySearchRange		= proto->m_bySearchRange;	// �� Ž�� ����
	m_byAttackRange		= proto->m_byAttackRange;	// �����Ÿ�
	m_byTracingRange	= proto->m_byTracingRange;	// �߰ݰŸ�
	m_iMoney			= proto->m_iMoney;			// �������� ��
	m_iItem				= proto->m_iItem;			// �������� ������
	m_tNpcLongType		= proto->m_byDirectAttack;
	m_byWhatAttackType	= proto->m_byDirectAttack;


	m_sRegenTime		= 10000 * SECOND;	// ��(DB)����-> �и��������
	m_sMaxPathCount		= 0;
	m_tItemPer			= proto->m_tItemPer;	// NPC Type
	m_tDnPer			= proto->m_tDnPer;	// NPC Type

	m_pZone = g_pMain->GetZoneByID(m_bCurZone);
	m_bFirstLive = 1;
}

// NPC ���º��� ��ȭ�Ѵ�.
time_t CNpc::NpcLive()
{
	// Dungeon Work : ���ϴ� ������ ��� ���ϰ� ó��..
	if( m_byRegenType == 2 || (m_byRegenType==1 && m_byChangeType==100) )	{	// ������ ���� ���ϵ���,,, 
		m_NpcState = NPC_LIVE;
		return m_sRegenTime;
	}
	if( m_byChangeType == 1 )	{			// ������ ������ �ٲپ� �ش�..
		m_byChangeType = 2;
		ChangeMonsterInfomation( 1 );
	}

	m_NpcState = SetLive() ? NPC_STANDING : NPC_LIVE;
	return m_sStandTime;
}

/////////////////////////////////////////////////////////////////////////////
//	NPC�� �����ϴ� ���.
//
time_t CNpc::NpcFighting()
{
	if (m_iHP <= 0)	{
		Dead();
		return -1;
	}
	
	return Attack();
}

/////////////////////////////////////////////////////////////////////////////
//	NPC�� ������ �����ϴ� ���.
//
time_t CNpc::NpcTracing()
{
	float fMoveSpeed=0.0f;

	if(m_sStepCount != 0)	{
		if(m_fPrevX < 0 || m_fPrevZ < 0)	{
			TRACE("### Npc-NpcTracing  Fail : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fPrevX, m_fPrevZ);
		}
		else	{
			m_fCurX = m_fPrevX;		m_fCurZ = m_fPrevZ; 
		}
	}

	//  ���� ����� ������ ���� �ʵ����Ѵ�.
	if (m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT)
	{
		InitTarget();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}	

	/* �۾��Ұ�
	   ���� ������ ��� ���������� ����� ���ϵ��� üũ�ϴ� ��ƾ 	
	*/
	int nFlag = IsCloseTarget(m_byAttackRange, 1);
	if(nFlag == 1)						// �������� ���ϸ�ŭ ����� �Ÿ��ΰ�?
	{
		//TRACE("Npc-NpcTracing : trace->attack���� �ٲ� : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
		NpcMoveEnd();	// �̵� ��..
		m_NpcState = NPC_FIGHTING;
		return 0;
	}
	else if(nFlag == -1)		// Ÿ���� ������...
	{
		InitTarget();
		NpcMoveEnd();	// �̵� ��..
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}
	//else if(nFlag == 2 && m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{
	else if(nFlag == 2 && m_tNpcLongType == 2)	{
		NpcMoveEnd();	// �̵� ��..
		m_NpcState = NPC_FIGHTING;
		return 0;
	}

	if(m_byActionFlag == ATTACK_TO_TRACE)
	{
		m_byActionFlag = NO_ACTION;
		m_byResetFlag = 1;

		// If we're not already following a user, define our start coords.
		if (!m_bTracing)
		{
			m_fTracingStartX = m_fCurX;
			m_fTracingStartZ = m_fCurZ;
			m_bTracing = true;
		}
	}

	if(m_byResetFlag == 1)
	{
		if (!ResetPath())// && !m_tNpcTraceType)
		{
			TRACE("##### NpcTracing Fail : �н����ε� ���� , NPC_STANDING���� ######\n");
			InitTarget();
			NpcMoveEnd();	// �̵� ��..
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}
	}

	if (  (!m_bPathFlag && !StepMove())
		|| (m_bPathFlag && !StepNoPathMove()))
	{
		m_NpcState = NPC_STANDING;
		TRACE("### NpcTracing Fail : StepMove ����, %s, %d ### \n", m_proto->m_strName, m_sNid+NPC_BAND);
		return m_sStandTime;
	}

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result << uint16(m_sNid + NPC_BAND);
	if (IsMovingEnd())
		result	<< m_fCurX << m_fCurZ << m_fCurY
				<< float(0.0f);
	else
		result	<< m_fPrevX << m_fPrevZ << m_fPrevY 
				<< (float)(m_fSecForRealMoveMetor / ((double)m_sSpeed / 1000));
	g_pMain->Send(&result);

	if(nFlag == 2 && m_tNpcLongType == 0 && m_proto->m_tNpcType != NPC_HEALER)
	{
		// Trace Attack
		int nRet = TracingAttack();
		//TRACE("--> Npc-NpcTracing : TracingAttack : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
		if(nRet == 0)	{
			InitTarget();
			NpcMoveEnd();	// �̵� ��..
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}
	}	

	return m_sSpeed;	
}

time_t CNpc::NpcAttacking()
{
	if (m_iHP <= 0)	{	
		Dead();		 // �ٷ� ���͸� ���δ�.. (����ġ ��� ����)
		return -1;
	}

//	TRACE("Npc Attack - [nid=%d, sid=%d]\n", m_sNid, m_proto->m_sSid);
	
	int ret = IsCloseTarget(m_byAttackRange);
	if(ret == 1)	{	// ������ �� �ִ¸�ŭ ����� �Ÿ��ΰ�?
		m_NpcState = NPC_FIGHTING;
		return 0;
	}

	//if(m_proto->m_tNpcType == NPCTYPE_DOOR || m_proto->m_tNpcType == NPCTYPE_ARTIFACT || m_proto->m_tNpcType == NPCTYPE_PHOENIX_GATE || m_proto->m_tNpcType == NPCTYPE_GATE_LEVER)		return;		// ���� NPC�� ����ó�� ���ϰ�

	// �۾� : �� �κп���.. ��񺴵� ������ �����ϰ�...
	if(m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT)
	{
		m_NpcState = NPC_STANDING;
		return m_sStandTime/2;
	}

	int nValue = GetTargetPath();
	if(nValue == -1)	{	// Ÿ���� �������ų�,, �־���������...
		if (!RandomMove())
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}

		InitTarget();
		m_NpcState = NPC_MOVING;
		return m_sSpeed;
	}
	else if(nValue == 0)	{
		m_fSecForMetor = m_fSpeed_2;			// �����϶��� �ٴ� �ӵ���... 
		IsNoPathFind(m_fSecForMetor);			// Ÿ�� �������� �ٷ� ����..
	}

	m_NpcState = NPC_TRACING;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//	NPC�� �̵��ϴ� ���.
//
time_t CNpc::NpcMoving()
{
	float fMoveSpeed = 0.0f;

	if(m_iHP <= 0) {
		Dead();
		return -1;
	}

	if(m_sStepCount != 0)	{
		if(m_fPrevX < 0 || m_fPrevZ < 0)	{
			TRACE("### Npc-Moving Fail : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fPrevX, m_fPrevZ);
		}
		else	{
			m_fCurX = m_fPrevX;		m_fCurZ = m_fPrevZ; 
		}
	}

	if (FindEnemy())
	{
	/*	if(m_proto->m_tNpcType == NPCTYPE_GUARD) 
		{ 
			NpcMoveEnd();	// �̵� ��..
			m_NpcState = NPC_FIGHTING; 
			return 0; 
		}
		else */
		{ 
			NpcMoveEnd();	// �̵� ��..
			m_NpcState = NPC_ATTACKING;
			return m_sSpeed;
		}
	}	

	if(IsMovingEnd())	{				// �̵��� ��������
		m_fCurX = m_fPrevX;		m_fCurZ = m_fPrevZ; 
		if(m_fCurX < 0 || m_fCurZ < 0)	{
			TRACE("Npc-NpcMoving-2 : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
		}

		int rx = (int)(m_fCurX / VIEW_DIST);
		int rz = (int)(m_fCurZ / VIEW_DIST);
		//TRACE("** NpcMoving --> IsMovingEnd() �̵��� ����,, rx=%d, rz=%d, stand��\n", rx, rz);
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	if (  (!m_bPathFlag && !StepMove())
		|| (m_bPathFlag && !StepNoPathMove()))
	{
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result << uint16(m_sNid + NPC_BAND);
	if (IsMovingEnd())
		result	<< m_fPrevX << m_fPrevZ << m_fPrevY 
				<< float(0.0f);
	else
		result	<< m_fPrevX << m_fPrevZ << m_fPrevY 
				<< (float)(m_fSecForRealMoveMetor / ((double)m_sSpeed / 1000));
	g_pMain->Send(&result);

	return m_sSpeed;	
}

/////////////////////////////////////////////////////////////////////////////
//	NPC�� ���ִ°��.
//
time_t CNpc::NpcStanding()
{
/*	if(g_pMain->m_byNight == 2)	{	// ���̸�
		m_NpcState = NPC_SLEEPING;
		return 0;
	}	*/

	MAP* pMap = GetMap();
	if (pMap == nullptr)	
	{
		TRACE("### NpcStanding Zone Index Error : nid=%d, name=%s, zone=%d ###\n", m_sNid+NPC_BAND, m_proto->m_strName, m_bCurZone);
		return -1;
	}

/*	bool bCheckRange = false;
	bCheckRange = IsInRange( (int)m_fCurX, (int)m_fCurZ);
	if( bCheckRange )	{	// Ȱ�������ȿ� �ִٸ�
		if( m_tNpcAttType != m_tNpcOldAttType )	{
			m_tNpcAttType = ATROCITY_ATTACK_TYPE;	// ���ݼ�������
			//TRACE("���ݼ����� �������� ����\n");
		}
	}
	else	{
		if( m_tNpcAttType == ATROCITY_ATTACK_TYPE )	{
			m_tNpcAttType = TENDER_ATTACK_TYPE;
			//TRACE("���ݼ����� �İ����� ����\n");
		}
	}	*/

	// dungeon work
	// ���� �������� ���� �Ǵ�
	CRoomEvent* pRoom = nullptr;
	pRoom = pMap->m_arRoomEventArray.GetData( m_byDungeonFamily );
	if( pRoom )	{
		if( pRoom->m_byStatus == 1 )	{	// ���� ���°� ������� �ʾҴٸ�,, ���ʹ� standing
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}
	}

	if (RandomMove())
	{
		m_iAniFrameCount = 0;
		m_NpcState = NPC_MOVING;
		return m_sStandTime;
	}	

	m_NpcState = NPC_STANDING;
	
	if( m_proto->m_tNpcType == NPC_SPECIAL_GATE && g_pMain->m_byBattleEvent == BATTLEZONE_OPEN )	{
		m_byGateOpen = !m_byGateOpen;
		Packet result(AG_NPC_GATE_OPEN);
		result << uint16(m_sNid+NPC_BAND) << m_byGateOpen;
		g_pMain->Send(&result);
	}

	return m_sStandTime;
}

/////////////////////////////////////////////////////////////////////////////
//	Ÿ�ٰ��� �Ÿ��� �����Ÿ� ������ �����Ѵ�.(������)
//
time_t CNpc::NpcBack()
{
	if(m_Target.id >= 0 && m_Target.id < NPC_BAND)	{
		if(g_pMain->GetUserPtr((m_Target.id - USER_BAND)) == nullptr)	{	// Target User �� �����ϴ��� �˻�
			m_NpcState = NPC_STANDING;
			return m_sSpeed;//STEP_DELAY;
		}
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)	{
		if(g_pMain->m_arNpc.GetData(m_Target.id-NPC_BAND) == nullptr)	{
			m_NpcState = NPC_STANDING;
			return m_sSpeed;
		}
	}

	if(m_iHP <= 0)	{
		Dead();
		return -1;
	}

	if(m_sStepCount != 0)	{
		if(m_fPrevX < 0 || m_fPrevZ < 0)	{
			TRACE("### Npc-NpcBack Fail-1 : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fPrevX, m_fPrevZ);
		}
		else	{
			m_fCurX = m_fPrevX;		m_fCurZ = m_fPrevZ; 
		}
	}
	
	if(IsMovingEnd())	{				// �̵��� ��������
		m_fCurX = m_fPrevX;		m_fCurZ = m_fPrevZ; 
		if(m_fCurX < 0 || m_fCurZ < 0)
			TRACE("Npc-NpcBack-2 : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);



		Packet result(MOVE_RESULT, uint8(SUCCESS));
		result << uint16(m_sNid + NPC_BAND) << m_fCurX << m_fCurZ << m_fCurY << float(0.0f);
		g_pMain->Send(&result);

//		TRACE("** NpcBack �̵��� ����,, stand��\n");
		m_NpcState = NPC_STANDING;

		//���� �ۿ� ������ ���ִ� �ð��� ª��...
		return m_sStandTime;
	}

	if (  (!m_bPathFlag && !StepMove())
		|| (m_bPathFlag && !StepNoPathMove()))
	{
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result	<< uint16(m_sNid + NPC_BAND) << m_fPrevX << m_fPrevZ << m_fPrevY 
			<< (float)(m_fSecForRealMoveMetor / ((double)m_sSpeed / 1000));
	g_pMain->Send(&result);

	return m_sSpeed;
}

///////////////////////////////////////////////////////////////////////
// NPC �� ó�� ����ų� �׾��ٰ� ��Ƴ� ���� ó��
//
bool CNpc::SetLive()
{
	//TRACE("**** Npc SetLive ***********\n");
	// NPC�� HP, PP �ʱ�ȭ ----------------------//	
	int i = 0, j = 0;
	m_iHP = m_iMaxHP;
	m_sMP = m_sMaxMP;
	m_sPathCount = 0;
	m_iPattenFrame = 0;
	m_byResetFlag = 0;
	m_byActionFlag = NO_ACTION;
	m_byMaxDamagedNation = 0;

	m_iRegion_X = -1;	m_iRegion_Z = -1;
	m_fAdd_x = 0.0f;	m_fAdd_z = 0.0f;
	m_fStartPoint_X = 0.0f;		m_fStartPoint_Y = 0.0f; 
	m_fEndPoint_X = 0.0f;		m_fEndPoint_Y = 0.0f;
	m_min_x = m_min_y = m_max_x = m_max_y = 0;

	InitTarget();
	ClearPathFindData();
	InitUserList();					// Ÿ�������� ����Ʈ�� �ʱ�ȭ.
	//InitPos();

	CNpc* pNpc = nullptr;

	/* Event Monster�� �ٽ� ��Ƴ� ��쿡�� Event Monster�� ���δ� �̺�Ʈ �����忡���� �����͸� nullptr */
	if (m_lEventNpc == 1 && !m_bFirstLive)
	{
		NpcSet::iterator itr = g_pMain->m_arEventNpcThread[0]->m_pNpcs.find(this);
		if (itr != g_pMain->m_arEventNpcThread[0]->m_pNpcs.end())
		{
			m_lEventNpc = 0;
			g_pMain->m_arEventNpcThread[0]->m_pNpcs.erase(itr);
			TRACE("��ȯ ���� ������ ��ȯ ,, thread index=%d, nid=%d\n", i, m_sNid+NPC_BAND);
		}
		return true;
	}

	MAP* pMap = GetMap();
	if (pMap == nullptr)	
		return false;

	if(m_bFirstLive)	{	// NPC �� ó�� ��Ƴ��� ���	
		m_nInitX = m_fPrevX = m_fCurX;
		m_nInitY = m_fPrevY = m_fCurY;
		m_nInitZ = m_fPrevZ = m_fCurZ;
	}

	if(m_fCurX < 0 || m_fCurZ < 0)	{
		TRACE("Npc-SetLive-1 : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
	}

	int dest_x = (int)m_nInitX / TILE_SIZE;
	int dest_z = (int)m_nInitZ / TILE_SIZE;

	bool bMove = pMap->IsMovable(dest_x, dest_z);

	if(m_proto->m_tNpcType != NPCTYPE_MONSTER || m_lEventNpc == 1)	{
		m_fCurX = m_fPrevX = m_nInitX;
		m_fCurY = m_fPrevY = m_nInitY;
		m_fCurZ = m_fPrevZ = m_nInitZ;
	}
	else	{
		int nX = 0;
		int nZ = 0;
		int nTileX = 0;
		int nTileZ = 0;
		int nRandom = 0;

		while(1)	{
			i++;
			nRandom = abs(m_nInitMinX - m_nInitMaxX);
			if(nRandom <= 1)	nX = m_nInitMinX;
			else	{
				if(m_nInitMinX < m_nInitMaxX)
					nX = myrand(m_nInitMinX, m_nInitMaxX);
				else
					nX = myrand(m_nInitMaxX, m_nInitMinX);
			}
			nRandom = abs(m_nInitMinY - m_nInitMaxY);
			if(nRandom <= 1)	nZ = m_nInitMinY;
			else	{
				if(m_nInitMinY < m_nInitMaxY)
					nZ = myrand(m_nInitMinY, m_nInitMaxY);
				else
					nZ = myrand(m_nInitMaxY, m_nInitMinY);
			}

			nTileX = nX / TILE_SIZE;
			nTileZ = nZ / TILE_SIZE;

			if(nTileX >= pMap->GetMapSize())
				nTileX = pMap->GetMapSize();
			if(nTileZ >= pMap->GetMapSize())
				nTileZ = pMap->GetMapSize();

			if(nTileX < 0 || nTileZ < 0)	{
				TRACE("#### Npc-SetLive() Fail : nTileX=%d, nTileZ=%d #####\n", nTileX, nTileZ);
				return false;
			}

			m_nInitX = m_fPrevX = m_fCurX = (float)nX;
			m_nInitZ = m_fPrevZ = m_fCurZ = (float)nZ;

			if(m_fCurX < 0 || m_fCurZ < 0)	{
				TRACE("Npc-SetLive-2 : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
			}

			break;
		}	
	}

	//SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND);

	// �����̻� ���� �ʱ�ȭ
	m_fHPChangeTime = getMSTime();
	m_tFaintingTime = 0;
	InitMagicValuable();

	if(m_bFirstLive)	{	// NPC �� ó�� ��Ƴ��� ���
		NpcTypeParser();
		m_bFirstLive = false;

		InterlockedIncrement(&g_pMain->m_CurrentNPC);

		if(g_pMain->m_TotalNPC == g_pMain->m_CurrentNPC)	// ���� �� ���� �ʱ�ȭ�� ������ ���� ���ٸ�
		{
			printf("Monster All Init Success - %d\n", g_pMain->m_CurrentNPC);

			TRACE("Npc - SerLive : GameServerAcceptThread, cur = %d\n", g_pMain->m_CurrentNPC);
			g_pMain->GameServerAcceptThread();				// ���Ӽ��� Accept
		}
		//TRACE("Npc - SerLive : CurrentNpc = %d\n", g_pMain->m_CurrentNPC);
	}
	
	// NPC�� �ʱ� ���� �ִ� ����,,
	// �ؾ� �� �� : Npc�� �ʱ� ����,, �����ϱ�..
	if(m_byMoveType == 3 && m_sMaxPathCount == 2)	// Npc�� ��� �ʱ� ������ �߿������ν�..
	{
		__Vector3 vS, vE, vDir;
		float fDir;
		vS.Set((float)m_PathList.pPattenPos[0].x, 0, (float)m_PathList.pPattenPos[0].z);
		vE.Set((float)m_PathList.pPattenPos[1].x, 0, (float)m_PathList.pPattenPos[1].z);
		vDir = vE - vS;
		vDir.Normalize();
		Yaw2D(vDir.x, vDir.z, fDir);
		m_byDirection = (uint8)fDir;
	}

	if( m_bySpecialType == 5 && m_byChangeType == 0)	{			// ó���� �׾��ִٰ� ��Ƴ��� ����
		return false;
	}
	else if( m_bySpecialType == 5 && m_byChangeType == 3)	{		// ������ ����,,,
	//else if( m_byChangeType == 3)	{		// ������ ����,,,
		//char notify[50];
		//sprintf_s( notify, sizeof(notify), "** �˸� : %s ���Ͱ� �����Ͽ����ϴ� **", m_proto->m_strName);
		//g_pMain->SendSystemMsg(notify, PUBLIC_CHAT);
	}

	SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND);
	m_byDeadType = 0;
	//CTime t = CTime::GetCurrentTime();
	//TRACE("NPC Init(nid=%d, sid=%d, th_num=%d, name=%s) - %.2f %.2f, gate = %d, m_byDeadType=%d, time=%d:%d-%d\n", 
	//	m_sNid+NPC_BAND, m_proto->m_sSid, m_sThreadNumber, m_proto->m_strName, m_fCurX, m_fCurZ, m_byGateOpen, m_byDeadType, t.GetHour(), t.GetMinute(), t.GetSecond());						


	Packet result(AG_NPC_INFO);
	result.SByte();
	FillNpcInfo(result);
	g_pMain->Send(&result);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
//	�ֺ��� ���� ���ų� �������� ��� ������ ������ ��ã�⸦ �� �� �����δ�.
//
bool CNpc::RandomMove()
{
	// �����̵��϶��� �ȴ� �ӵ��� ���߾��ش�...
	m_fSecForMetor = m_fSpeed_1;

	if (GetMap() == nullptr
		|| m_bySearchRange == 0
		|| m_byMoveType == 0
		|| !GetUserInView()
		// 4 means non-moving.
		|| m_byMoveType == 4)
		return false;

	float fDestX = -1.0f, fDestZ = -1.0f;
	int max_xx = GetMap()->GetMapSize();
	int max_zz = GetMap()->GetMapSize();
	int x = 0, y = 0;

	__Vector3 vStart, vEnd, vNewPos;
	float fDis = 0.0f;
 
	int nPathCount = 0;

	int random_x = 0, random_z = 0;
	bool bPeedBack = false;

	if(m_byMoveType == 1)	{// �����ϰ� ���ݾ� �����̴� NPC
		bPeedBack = IsInRange( (int)m_fCurX, (int)m_fCurZ);
		if( bPeedBack == false )	{	
			//TRACE("�ʱ���ġ�� �����,,,  patten=%d \n", m_iPattenFrame);
		}

		if(m_iPattenFrame == 0)		{		// ó����ġ�� ���ư�����...
			m_pPattenPos.x = (short)m_nInitX;
			m_pPattenPos.z = (short)m_nInitZ;
		}

		random_x = myrand(3, 7);
		random_z = myrand(3, 7);

		fDestX = m_fCurX + (float)random_x;
		fDestZ = m_fCurZ + (float)random_z;

		if(m_iPattenFrame == 2)	{
			fDestX = m_pPattenPos.x;
			fDestZ = m_pPattenPos.z;
			m_iPattenFrame = 0;
		}
		else	{
			m_iPattenFrame++;
		}

		vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
		vEnd.Set(fDestX, 0, fDestZ);
		fDis = GetDistance(vStart, vEnd);
		if(fDis > 50)	{	// �ʱ���ȿ�Ÿ� 50m�� ��� ���
			GetVectorPosition(vStart, vEnd, 40, &vNewPos);
			fDestX = vNewPos.x;
			fDestZ = vNewPos.z;
			m_iPattenFrame = 2;
			bPeedBack = true;
			//TRACE("&&& RandomMove �ʱ���ġ ��Ż.. %d,%s ==> x=%.2f, z=%.2f,, init_x=%.2f, init_z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, fDestX, fDestZ, m_nInitX, m_nInitZ); 
		}

//		TRACE("&&& RandomMove ==> x=%.2f, z=%.2f,, dis=%.2f, patten = %d\n", fDestX, fDestZ, fDis, m_iPattenFrame); 
	}
	else if(m_byMoveType == 2)  {  // PathLine�� ���� �����̴� NPC	
		if(m_sPathCount == m_sMaxPathCount)		m_sPathCount = 0;

		// ���� ��ġ��,, �н� ����Ʈ���� �־����ٸ�,, ������ m_sPathCount�� ������ m_sPathCount�� ��ġ�� 
		// �Ǵ��ؼ� ����� ��ġ�� ��ã�⸦ �Ѵ�,,
		if(m_sPathCount != 0 && IsInPathRange() == false)	{
			m_sPathCount--;
			nPathCount = GetNearPathPoint();

			// �̵��� �� ���� �ʹ� �հŸ��� npc�� �̵��Ǿ��� ���,, npc�� ���̰�, �ٽ� �츮����..
			// npc�� �ʱ���ġ�� ���� ��Ű����.. �Ѵ�..
			if(nPathCount  == -1)	{
				TRACE("##### RandomMove Fail : [nid = %d, sid=%d], path = %d/%d, �̵��� �� �ִ� �Ÿ����� �ʹ� �־�����,, ������ #####\n", 
					m_sNid+NPC_BAND, m_proto->m_sSid, m_sPathCount, m_sMaxPathCount);
				// ������ 0�� ��ġ �������� 40m �̵��ϰ� ó������.. 
				vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
				fDestX = (float)m_PathList.pPattenPos[0].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[0].z + m_fBattlePos_z;
				vEnd.Set(fDestX, 0, fDestZ);
				GetVectorPosition(vStart, vEnd, 40, &vNewPos);
				fDestX = vNewPos.x;
				fDestZ = vNewPos.z;
				//m_sPathCount++;
				//return false;	// ������ standing���·�..
			}
			else	{
				//m_byPathCount; ��ȣ�� �����ֱ�
				if(nPathCount < 0)	return false;
				fDestX = (float)m_PathList.pPattenPos[nPathCount].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[nPathCount].z + m_fBattlePos_z;
				m_sPathCount = nPathCount;
			}
		}
		else	{
			if(m_sPathCount < 0)	return false;
			fDestX = (float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x;
			fDestZ = (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_z;
		}

		//TRACE("RandomMove ����� �̵� : [nid=%d, sid=%d, name=%s], path=%d/%d, nPathCount=%d, curx=%.2f, z=%.2f -> dest_X=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_sSid, m_proto->m_strName, m_sPathCount, m_sMaxPathCount, nPathCount, m_fCurX, m_fCurZ, fDestX, fDestZ);
		m_sPathCount++;
	}
	else if(m_byMoveType == 3)	{	// PathLine�� ���� �����̴� NPC
		if(m_sPathCount == m_sMaxPathCount)	{
			m_byMoveType = 0;
			m_sPathCount = 0;
			return false;
		}

		// ���� ��ġ��,, �н� ����Ʈ���� �־����ٸ�,, ������ m_sPathCount�� ������ m_sPathCount�� ��ġ�� 
		// �Ǵ��ؼ� ����� ��ġ�� ��ã�⸦ �Ѵ�,,
		if(m_sPathCount != 0 && IsInPathRange() == false)	{
			m_sPathCount--;
			nPathCount = GetNearPathPoint();

			// �̵��� �� ���� �ʹ� �հŸ��� npc�� �̵��Ǿ��� ���,, npc�� ���̰�, �ٽ� �츮����..
			// npc�� �ʱ���ġ�� ���� ��Ű����.. �Ѵ�..
			if(nPathCount  == -1)	{
				// ������ 0�� ��ġ �������� 40m �̵��ϰ� ó������.. 
				TRACE("##### RandomMove Fail : [nid = %d, sid=%d], path = %d/%d, �̵��� �� �ִ� �Ÿ����� �ʹ� �־�����,, ������ #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, m_sPathCount, m_sMaxPathCount);
				vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
				fDestX = (float)m_PathList.pPattenPos[0].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[0].z + m_fBattlePos_z;
				vEnd.Set(fDestX, 0, fDestZ);
				GetVectorPosition(vStart, vEnd, 40, &vNewPos);
				fDestX = vNewPos.x;
				fDestZ = vNewPos.z;
				//return false;	// ������ standing���·�..
			}
			else	{
				if(nPathCount < 0)	return false;
				fDestX = (float)m_PathList.pPattenPos[nPathCount].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[nPathCount].z + m_fBattlePos_x;
				m_sPathCount = nPathCount;
			}
		}
		else	{
			if(m_sPathCount < 0)	return false;
			fDestX = (float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x;
			fDestZ = (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_x;
		}

		m_sPathCount++;
	}

	vStart.Set(m_fCurX, 0, m_fCurZ);
	vEnd.Set(fDestX, 0, fDestZ);

	if(m_fCurX < 0 || m_fCurZ < 0 || fDestX < 0 || fDestZ < 0)	{
		TRACE("##### RandomMove Fail : value is negative.. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f#####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ, fDestX, fDestZ);
		return false;
	}

	int mapWidth = (int)(max_xx * GetMap()->GetUnitDistance());

	if(m_fCurX >= mapWidth || m_fCurZ >= mapWidth || fDestX >= mapWidth || fDestZ >= mapWidth)	{
		TRACE("##### RandomMove Fail : value is overflow .. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f#####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ, fDestX, fDestZ);
		return false;
	}

	// �۾��Ұ� :	 ���� ������ ��� ���������� ����� ���ϵ��� üũ�ϴ� ��ƾ 	
    if ( m_proto->m_tNpcType == NPC_DUNGEON_MONSTER )	{
		if( IsInRange( (int)fDestX, (int)fDestZ ) == false)
			return false;	
    }

	fDis = GetDistance(vStart, vEnd);
	if(fDis > NPC_MAX_MOVE_RANGE)	{						// 100���� ���� ������ ���ĵ����·�..
		if(m_byMoveType == 2 || m_byMoveType == 3)	{
			m_sPathCount--;
			if(m_sPathCount <= 0) m_sPathCount=0;
		}
		TRACE("##### RandomMove Fail : NPC_MAX_MOVE_RANGE overflow  .. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f, fDis=%.2f#####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ, fDestX, fDestZ, fDis);
		return false;
	}

	if(fDis <= m_fSecForMetor)		{		// �̵��Ÿ� �ȿ� ��ǥ���� �ִٸ� �ٷ� �̵��ϰ� ó��...
		ClearPathFindData();
		m_fStartPoint_X = m_fCurX;		m_fStartPoint_Y = m_fCurZ;
		m_fEndPoint_X = fDestX;			m_fEndPoint_Y = fDestZ;
		m_bPathFlag = true;
		m_iAniFrameIndex = 1;
		m_pPoint[0].fXPos = m_fEndPoint_X;
		m_pPoint[0].fZPos = m_fEndPoint_Y;
		//TRACE("** Npc Random Direct Move  : [nid = %d], fDis <= %d, %.2f **\n", m_sNid, m_fSecForMetor, fDis);
		return true;
	}

	float fTempRange = (float)fDis+2;				// �Ͻ������� �����Ѵ�.
	int min_x = (int)(m_fCurX - fTempRange)/TILE_SIZE;	if(min_x < 0) min_x = 0;
	int min_z = (int)(m_fCurZ - fTempRange)/TILE_SIZE;	if(min_z < 0) min_z = 0;
	int max_x = (int)(m_fCurX + fTempRange)/TILE_SIZE;	if(max_x >= max_xx) max_x = max_xx - 1;
	int max_z = (int)(m_fCurZ + fTempRange)/TILE_SIZE;	if(min_z >= max_zz) min_z = max_zz - 1;

	CPoint start, end;
	start.x = (int)(m_fCurX/TILE_SIZE) - min_x;
	start.y = (int)(m_fCurZ/TILE_SIZE) - min_z;
	end.x = (int)(fDestX/TILE_SIZE) - min_x;
	end.y = (int)(fDestZ/TILE_SIZE) - min_z;

	if(start.x < 0 || start.y < 0 || end.x < 0 || end.y < 0)	return false;

	m_fStartPoint_X = m_fCurX;		m_fStartPoint_Y = m_fCurZ;
	m_fEndPoint_X = fDestX;	m_fEndPoint_Y = fDestZ;

	m_min_x = min_x;
	m_min_y = min_z;
	m_max_x = max_x;
	m_max_y = max_z;

	// �н��� ���� ���� ���ͳ� NPC�� �н����ε��� �����ʰ� ����ǥ�� �ٷ� �̵�..
	if(m_byMoveType == 2 || m_byMoveType == 3 || bPeedBack == true) 	{
		IsNoPathFind(m_fSecForMetor);
		return true;
	}

	int nValue = PathFind(start, end, m_fSecForMetor);
	if(nValue == 1)		return true;

	return false;
}

// Target User�� �ݴ� �������� �����ϰ� �����δ�.
bool CNpc::RandomBackMove()
{
	m_fSecForMetor = m_fSpeed_2;		// ����������.. �ӵ��� �ٴ� �ӵ��� ���߾��ش�..

	if (m_bySearchRange == 0) return false;
	
	float fDestX = -1.0f, fDestZ = -1.0f;
	if (GetMap() == nullptr) 
	{
		TRACE("#### Npc-RandomBackMove Zone Fail : [name=%s], zone=%d #####\n", m_proto->m_strName, m_bCurZone);
		return false;
	}

	int max_xx = GetMap()->GetMapSize();
	int max_zz = GetMap()->GetMapSize();
	int x = 0, y = 0;
	float fTempRange = (float)m_bySearchRange*2;				// �Ͻ������� �����Ѵ�.
	int min_x = (int)(m_fCurX - fTempRange)/TILE_SIZE;	if(min_x < 0) min_x = 0;
	int min_z = (int)(m_fCurZ - fTempRange)/TILE_SIZE;	if(min_z < 0) min_z = 0;
	int max_x = (int)(m_fCurX + fTempRange)/TILE_SIZE;	if(max_x > max_xx) max_x = max_xx;
	int max_z = (int)(m_fCurZ + fTempRange)/TILE_SIZE;	if(max_z > max_zz) max_z = max_zz;

	__Vector3 vStart, vEnd, vEnd22;
	float fDis = 0.0f;
	vStart.Set(m_fCurX, m_fCurY, m_fCurZ);

	int nID = m_Target.id;					// Target �� ���Ѵ�.
	CUser* pUser = nullptr;

	int iDir = 0;

	int iRandomX = 0, iRandomZ = 0, iRandomValue=0;
	iRandomValue = rand() % 2;
	
	if(nID >= USER_BAND && nID < NPC_BAND)	// Target �� User �� ���
	{
		pUser = g_pMain->GetUserPtr(nID - USER_BAND);
		if(pUser == nullptr)	
			return false;
		// ������ ������ ����,,  ���� x������
		if((int)pUser->m_curx != (int)m_fCurX)  
		{
			iRandomX = myrand((int)m_bySearchRange, (int)(m_bySearchRange*1.5));
			iRandomZ = myrand(0, (int)m_bySearchRange);

			if((int)pUser->m_curx > (int)m_fCurX)  
				iDir = 1;
			else
				iDir = 2;
		}
		else	// z������
		{
			iRandomZ = myrand((int)m_bySearchRange, (int)(m_bySearchRange*1.5));
			iRandomX = myrand(0, (int)m_bySearchRange);
			if((int)pUser->m_curz > (int)m_fCurZ)  
				iDir = 3;
			else
				iDir = 4;
		}

		switch(iDir)
		{
		case 1:
			fDestX = m_fCurX - iRandomX;
			if(iRandomValue == 0)
				fDestZ = m_fCurZ - iRandomX;
			else
				fDestZ = m_fCurZ + iRandomX;
			break;
		case 2:
			fDestX = m_fCurX + iRandomX;
			if(iRandomValue == 0)
				fDestZ = m_fCurZ - iRandomX;
			else
				fDestZ = m_fCurZ + iRandomX;
			break;
		case 3:
			fDestZ = m_fCurZ - iRandomX;
			if(iRandomValue == 0)
				fDestX = m_fCurX - iRandomX;
			else
				fDestX = m_fCurX + iRandomX;
			break;
		case 4:
			fDestZ = m_fCurZ - iRandomX;
			if(iRandomValue == 0)
				fDestX = m_fCurX - iRandomX;
			else
				fDestX = m_fCurX + iRandomX;
			break;
		}

		vEnd.Set(fDestX, 0, fDestZ);
		fDis = GetDistance(vStart, vEnd);
		if(fDis > 20)	// 20���� �̻��̸� 20���ͷ� �����,,
		{
			GetVectorPosition(vStart, vEnd, 20, &vEnd22);
			fDestX = vEnd22.x;
			fDestZ = vEnd22.z;
		}
	}
	else if(nID >= NPC_BAND && m_Target.id < INVALID_BAND)	// Target �� Npc �� ���
	{
	}

	CPoint start, end;
	start.x = (int)(m_fCurX/TILE_SIZE) - min_x;
	start.y = (int)(m_fCurZ/TILE_SIZE) - min_z;
	end.x = (int)(fDestX/TILE_SIZE) - min_x;
	end.y = (int)(fDestZ/TILE_SIZE) - min_z;

	if(start.x < 0 || start.y < 0 || end.x < 0 || end.y < 0)
	{
		return false;
	}

	m_fStartPoint_X = m_fCurX;		m_fStartPoint_Y = m_fCurZ;
	m_fEndPoint_X = fDestX;	m_fEndPoint_Y = fDestZ;

	m_min_x = min_x;
	m_min_y = min_z;
	m_max_x = max_x;
	m_max_y = max_z;

	int nValue = PathFind(start, end, m_fSecForMetor);
	if(nValue == 1)
		return true;

	return false;
}

bool CNpc::IsInPathRange()
{
	float fPathRange = 40.0f;	// ���� �н� ���� 
	__Vector3 vStart, vEnd;
	float fDistance = 0.0f;
	vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
	if(m_sPathCount < 0)	return false;
	vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_z);
	fDistance = GetDistance(vStart, vEnd);

	if((int)fDistance <= (int)fPathRange+1)
		return true;

	return false;
}

int CNpc::GetNearPathPoint()
{
	__Vector3 vStart, vEnd;
	float fMaxPathRange = (float)NPC_MAX_MOVE_RANGE;
	float fDis1 = 0.0f, fDis2 = 0.0f;
	int nRet = -1;
	vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
	vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_z);
	fDis1 = GetDistance(vStart, vEnd);

	if(m_sPathCount+1 >= m_sMaxPathCount)	{
		if(m_sPathCount-1 > 0)	{
			vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount-1].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount-1].z + m_fBattlePos_z);
			fDis2 = GetDistance(vStart, vEnd);
		}
		else {
			vEnd.Set((float)m_PathList.pPattenPos[0].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[0].z + m_fBattlePos_z);
			fDis2 = GetDistance(vStart, vEnd);
		}
	}
	else
	{
		vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount+1].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount+1].z + m_fBattlePos_z);
		fDis2 = GetDistance(vStart, vEnd);
	}

	if(fDis1 <= fDis2)
	{
		if(fDis1 <= fMaxPathRange)
			nRet =  m_sPathCount;
	}
	else
	{
		if(fDis2 <= fMaxPathRange)
			nRet =  m_sPathCount+1;
	}

	return nRet;
}

/////////////////////////////////////////////////////////////////////////////////////
//	NPC �� �ʱ� ������ġ �ȿ� �ִ��� �˻�
//
bool CNpc::IsInRange(int nX, int nZ)
{
	// NPC �� �ʱ� ��ġ�� ������� �Ǵ��Ѵ�.
	bool bFlag_1 = false, bFlag_2 = false;
	if (m_nLimitMinX < m_nLimitMaxX)
		if  (COMPARE(nX, m_nLimitMinX, m_nLimitMaxX))		bFlag_1 = true;
	else if (COMPARE(nX, m_nLimitMaxX, m_nLimitMinX))		bFlag_1 = true;

	if (m_nLimitMinZ < m_nLimitMaxZ)
		if  (COMPARE(nZ, m_nLimitMinZ, m_nLimitMaxZ))		bFlag_2 = true;
	else if (COMPARE(nZ, m_nLimitMaxZ, m_nLimitMinZ))		bFlag_2 = true;

	return (bFlag_1 && bFlag_2);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	PathFind �� �����Ѵ�.
//
int CNpc::PathFind(CPoint start, CPoint end, float fDistance)
{
	ClearPathFindData();

	if(start.x < 0 || start.y < 0 || end.x < 0 || end.y < 0)
	{
		return -1;
	}

	if(start.x == end.x && start.y == end.y)	// ���� Ÿ�� �ȿ���,, ���� �������� �־��ٸ�,,
	{
		m_bPathFlag = true;
		m_iAniFrameIndex = 1;
		m_pPoint[0].fXPos = m_fEndPoint_X;
		m_pPoint[0].fZPos = m_fEndPoint_Y;
		return 1;
	}
	
	// ���⿡�� �н����ε带 �����Ұ���.. �ٷ� ��ǥ������ ���������� �Ǵ�..
	if(IsPathFindCheck(fDistance) == true)
	{
		m_bPathFlag = true;
		return 1;
	}


	int i;
	int min_x, max_x;
	int min_y, max_y;

	min_x = m_min_x;
	min_y = m_min_y;
	max_x = m_max_x;
	max_y = m_max_y;

	m_vMapSize.cx = max_x - min_x + 1;		
	m_vMapSize.cy = max_y - min_y + 1;
	
	m_pPath = nullptr;

	m_vPathFind.SetMap(m_vMapSize.cx, m_vMapSize.cy, GetMap()->GetEventIDs(), GetMap()->GetMapSize(), min_x, min_y);
	m_pPath = m_vPathFind.FindPath(end.x, end.y, start.x, start.y);

	int count = 0;

	while(m_pPath)
	{
		m_pPath = m_pPath->Parent;
		if(m_pPath == nullptr)			break;
		m_pPoint[count].pPoint.x = m_pPath->x+min_x;		m_pPoint[count].pPoint.y = m_pPath->y+min_y;
		//m_pPath = m_pPath->Parent;
		count++;
	}	
	
	if(count <= 0 || count >= MAX_PATH_LINE)	{	
		//TRACE("#### PathFind Fail : nid=%d,%s, count=%d ####\n", m_sNid+NPC_BAND, m_proto->m_strName, count);
		return 0;
	}

	m_iAniFrameIndex = count-1;
	
	float x1 = 0.0f;
	float z1 = 0.0f;

	int nAdd = GetDir(m_fStartPoint_X, m_fStartPoint_Y, m_fEndPoint_X, m_fEndPoint_Y);

	for(i=0; i<count; i++)
	{
		if(i==(count-1))
		{
			m_pPoint[i].fXPos = m_fEndPoint_X;
			m_pPoint[i].fZPos = m_fEndPoint_Y;
		}
		else
		{
			x1 = (float)(m_pPoint[i].pPoint.x * TILE_SIZE + m_fAdd_x);
			z1 = (float)(m_pPoint[i].pPoint.y * TILE_SIZE + m_fAdd_z);
			m_pPoint[i].fXPos = x1;
			m_pPoint[i].fZPos = z1;
		}
	}

	return 1;
}


//	NPC ���ó��
void CNpc::Dead(int iDeadType)
{
	MAP* pMap = GetMap();
	if(pMap == nullptr)	return;

	m_iHP = 0;
	m_NpcState = NPC_DEAD;
	m_Delay = m_sRegenTime;
	m_bFirstLive = false;
	m_byDeadType = 100;		// �����̺�Ʈ�߿��� �״� ���

	if(m_iRegion_X > pMap->GetXRegionMax() || m_iRegion_Z > pMap->GetZRegionMax())	{
		TRACE("#### Npc-Dead() Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, m_iRegion_X, m_iRegion_Z);
		return;
	}
	// map�� region���� ���� ���� ����..
	pMap->RegionNpcRemove(m_iRegion_X, m_iRegion_Z, m_sNid+NPC_BAND);

	//TRACE("-- Npc-Dead RegionRemove : [nid=%d, name=%s], nRX=%d, nRZ=%d \n", m_sNid+NPC_BAND, m_proto->m_strName, m_iRegion_X, m_iRegion_Z);

	if(iDeadType == 1)	{	// User�� ���� �������� �ƴϱ� ������... Ŭ���̾�Ʈ�� Dead��Ŷ����...
		Packet result(AG_DEAD);
		result << uint16(m_sNid + NPC_BAND);
		g_pMain->Send(&result);
	}

	// Dungeon Work : ���ϴ� ������ ��� ���ϰ� ó��..
	if( m_bySpecialType == 1 || m_bySpecialType == 4)	{
		if( m_byChangeType == 0 )	{
			m_byChangeType = 1;
			//ChangeMonsterInfomation( 1 );
		}
		else if( m_byChangeType == 2 )	{		// ���� ������ ���� (���� Ÿ���� ������������� �˻��ؾ� ��)
			if( m_byDungeonFamily < 0 || m_byDungeonFamily >= MAX_DUNGEON_BOSS_MONSTER )	{
				TRACE("#### Npc-Dead() m_byDungeonFamily Fail : [nid=%d, name=%s], m_byDungeonFamily=%d #####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_byDungeonFamily);
				return;
			}
//			pMap->m_arDungeonBossMonster[m_byDungeonFamily] = 0;
		}
	}
	else	{
		m_byChangeType = 100;
	}

	
/*
	if( m_byDungeonFamily < 0 || m_byDungeonFamily >= MAX_DUNGEON_BOSS_MONSTER )	{
		TRACE("#### Npc-Dead() m_byDungeonFamily Fail : [nid=%d, name=%s], m_byDungeonFamily=%d #####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_byDungeonFamily);
		return;
	}
	if( pMap->m_arDungeonBossMonster[m_byDungeonFamily] == 0 )	{
		m_byRegenType = 2;				// ������ �ȵǵ���.. 
	}	*/
}

//	NPC �ֺ��� ���� ã�´�.
bool CNpc::FindEnemy()
{
	if(m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT )		return false;

	// Healer Npc
	int iMonsterNid = 0;
	if( m_proto->m_tNpcType == NPC_HEALER )	{		// Heal
		iMonsterNid = FindFriend( 2 );
		if( iMonsterNid != 0 )	return true;
	}

	MAP* pMap = GetMap();
	if (pMap == nullptr)	return false;
	CUser *pUser = nullptr;
	CNpc *pNpc = nullptr;

	int target_uid = 0;
	__Vector3 vUser, vNpc;
	float fDis = 0.0f;
	float fCompareDis = 0.0f;
	vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);

	float fSearchRange = (float)m_bySearchRange;

	int iExpand = FindEnemyRegion();

	// �ڽ��� region�� �ִ� UserArray�� ���� �˻��Ͽ�,, ����� �Ÿ��� ������ �ִ����� �Ǵ�..
	if(m_iRegion_X > pMap->GetXRegionMax() || m_iRegion_Z > pMap->GetZRegionMax() || m_iRegion_X < 0 || m_iRegion_Z < 0)	{
	//	TRACE("#### Npc-FindEnemy() Fail : [nid=%d, sid=%d, name=%s, th_num=%d, cur_x=%.2f, cur_z=%.2f], nRX=%d, nRZ=%d #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, m_proto->m_strName, m_sThreadNumber, m_fCurX, m_fCurZ, m_iRegion_X, m_iRegion_Z);
		return false;
	}

	fCompareDis = FindEnemyExpand(m_iRegion_X, m_iRegion_Z, fCompareDis, 1);

	int x=0, y=0;

	// �̿��� �ִ� Region�� �˻��ؼ�,,  ���� ��ġ�� ���� ����� User�� ����.. �̵�..
	for(int l=0; l<4; l++)	{
		if(m_iFind_X[l] == 0 && m_iFind_Y[l] == 0)		continue;

		x = m_iRegion_X + (m_iFind_X[l]);
		y = m_iRegion_Z + (m_iFind_Y[l]);

		// �̺κ� �������,,
		if(x < 0 || y < 0 || x > pMap->GetXRegionMax() || y > pMap->GetZRegionMax())		continue;

		fCompareDis = FindEnemyExpand(x, y, fCompareDis, 1);
	}

	if(m_Target.id >= 0 && (fCompareDis <= fSearchRange))		return true;

	fCompareDis = 0.0f;

	// Ÿ���� ����� ��쿡�� ���� ������ ���Ͱ� �ƴѰ�쿡�� ���͸� �����ϵ��� �Ѵ�..
	if(m_proto->m_tNpcType == NPC_GUARD || m_proto->m_tNpcType == NPC_PATROL_GUARD || m_proto->m_tNpcType == NPC_STORE_GUARD) // || m_proto->m_tNpcType == NPCTYPE_MONSTER)	
	{
		fCompareDis = FindEnemyExpand(m_iRegion_X, m_iRegion_Z, fCompareDis, 2);

		int x=0, y=0;

		// �̿��� �ִ� Region�� �˻��ؼ�,,  ���� ��ġ�� ���� ����� User�� ����.. �̵�..
		for(int l=0; l<4; l++)	{
			if(m_iFind_X[l] == 0 && m_iFind_Y[l] == 0)			continue;

			x = m_iRegion_X + (m_iFind_X[l]);
			y = m_iRegion_Z + (m_iFind_Y[l]);

			// �̺κ� �������,,
			if(x < 0 || y < 0 || x > pMap->GetXRegionMax() || y > pMap->GetZRegionMax())	continue;

			fCompareDis = FindEnemyExpand(x, y, fCompareDis, 2);
		}
	}

	if(m_Target.id >= 0 && (fCompareDis <= fSearchRange))	return true;

	// �ƹ��� �����Ƿ� ����Ʈ�� �����ϴ� ������ �ʱ�ȭ�Ѵ�.
	InitUserList();		
	InitTarget();
	return false;
}

// Npc�� ������ �˻��Ҷ� ��� Region���� �˻��ؾ� �ϴ����� �Ǵ�..
int CNpc::FindEnemyRegion()
{
	/*
        1	2	3
		4	0	5
		6	7	8
	*/
	int iRetValue = 0;
	int  iSX = m_iRegion_X * VIEW_DIST;
	int  iSZ = m_iRegion_Z * VIEW_DIST;
	int  iEX = (m_iRegion_X+1) * VIEW_DIST;
	int  iEZ = (m_iRegion_Z+1) * VIEW_DIST;
	int  iSearchRange = m_bySearchRange;
	int iCurSX = (int)m_fCurX - m_bySearchRange;
	int iCurSY = (int)m_fCurX - m_bySearchRange;
	int iCurEX = (int)m_fCurX + m_bySearchRange;
	int iCurEY = (int)m_fCurX + m_bySearchRange;
	
	int iMyPos = GetMyField();

	switch(iMyPos)
	{
	case 1:
		if((iCurSX < iSX) && (iCurSY < iSZ))
			iRetValue = 1;
		else if((iCurSX > iSX) && (iCurSY < iSZ))
			iRetValue = 2;
		else if((iCurSX < iSX) && (iCurSY > iSZ))
			iRetValue = 4;
		else if((iCurSX >= iSX) && (iCurSY >= iSZ))
			iRetValue = 0;
		break;
	case 2:
		if((iCurEX < iEX) && (iCurSY < iSZ))
			iRetValue = 2;
		else if((iCurEX > iEX) && (iCurSY < iSZ))
			iRetValue = 3;
		else if((iCurEX <= iEX) && (iCurSY >= iSZ))
			iRetValue = 0;
		else if((iCurEX > iEX) && (iCurSY > iSZ))
			iRetValue = 5;
		break;
	case 3:
		if((iCurSX < iSX) && (iCurEY < iEZ))
			iRetValue = 4;
		else if((iCurSX >= iSX) && (iCurEY <= iEZ))
			iRetValue = 0;
		else if((iCurSX < iSX) && (iCurEY > iEZ))
			iRetValue = 6;
		else if((iCurSX > iSX) && (iCurEY > iEZ))
			iRetValue = 7;
		break;
	case 4:
		if((iCurEX <= iEX) && (iCurEY <= iEZ))
			iRetValue = 0;
		else if((iCurEX > iEX) && (iCurEY < iEZ))
			iRetValue = 5;
		else if((iCurEX < iEX) && (iCurEY > iEZ))
			iRetValue = 7;
		else if((iCurEX > iEX) && (iCurEY > iEZ))
			iRetValue = 8;
		break;
	}

	if(iRetValue <= 0) // �ӽ÷� ����(������),, �ϱ� ���Ѱ�..
		iRetValue = 0;

	switch(iRetValue)
	{
	case 0:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 1:
		m_iFind_X[0] = -1;  m_iFind_Y[0] = -1;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = -1;
		m_iFind_X[2] = -1;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 2:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = -1;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 3:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 1;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 1;  m_iFind_Y[3] = 1;
		break;
	case 4:
		m_iFind_X[0] = -1;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 5:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 1;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 6:
		m_iFind_X[0] = -1;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = -1;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 1;
		break;
	case 7:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 8:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 1;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 1;  m_iFind_Y[3] = 1;
		break;
	}

	return iRetValue;
}

float CNpc::FindEnemyExpand(int nRX, int nRZ, float fCompDis, int nType)
{
	MAP* pMap = GetMap();
	float fDis = 0.0f;
	if(pMap == nullptr)	return fDis;
	float fComp = fCompDis;
	float fSearchRange = (float)m_bySearchRange;
	int target_uid = -1;
	__Vector3 vUser, vNpc, vMon;
	vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
	int* pIDList = nullptr;
	int iLevelComprison = 0;
	
	if(nType == 1)	{		// user�� Ÿ������ ��� ���
		FastGuard lock(pMap->m_lock);
		CRegion *pRegion = &pMap->m_ppRegion[nRX][nRZ];
		int nUser = pRegion->m_RegionUserArray.GetSize(), count = 0;

		//TRACE("FindEnemyExpand type1,, region_x=%d, region_z=%d, user=%d, mon=%d\n", nRX, nRZ, nUser, nMonster);
		if( nUser == 0 )
			return 0.0f;

		pIDList = new int[nUser];
		foreach_stlmap (itr, pRegion->m_RegionUserArray)
			pIDList[count++] = *itr->second;

		for(int i=0 ; i<nUser; i++ ) {
			CUser *pUser = g_pMain->GetUserPtr(pIDList[i]);
			if( pUser != nullptr && pUser->m_bLive == USER_LIVE)	{
				// ���� ������ ������ ������ ���� �ʵ��� �Ѵ�...
				if (m_byGroup == pUser->m_bNation
					|| pUser->m_bInvisibilityType
					|| pUser->m_byIsOP == MANAGER_USER)
					continue;

				vUser.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz); 
				fDis = GetDistance(vUser, vNpc);

				// �۾� : ���⿡�� ���� ���ݰŸ��� �ִ� ���������� �Ǵ�
				if(fDis <= fSearchRange)	{
					if(fDis >= fComp)	{	// 
						target_uid = pUser->m_iUserId;
						fComp = fDis;

						//�İ���...
						if(!m_tNpcAttType)	{	// �� ������ ���� ã�´�.
							if(IsDamagedUserList(pUser) || (m_tNpcGroupType && m_Target.id == target_uid))	{
								m_Target.id	= target_uid;
								m_Target.failCount = 0;
								m_Target.x	= pUser->m_curx;
								m_Target.y	= pUser->m_cury;
								m_Target.z	= pUser->m_curz;
							}
						}
						else	{	// ������...
							iLevelComprison = pUser->m_bLevel - m_proto->m_sLevel;
							// �۾��� �� : Ÿ�Կ� ���� ���ݼ�������..
							//if(iLevelComprison > ATTACK_LIMIT_LEVEL)	continue;

							m_Target.id	= target_uid;
							m_Target.failCount = 0;
							m_Target.x	= pUser->m_curx;
							m_Target.y	= pUser->m_cury;
							m_Target.z	= pUser->m_curz;
							//TRACE("Npc-FindEnemyExpand - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);
						}
					}
				}	
			}
		}
	}
	else if(nType == 2)		{		// ����� ���͸� Ÿ������ ��� ���
		FastGuard lock(pMap->m_lock);
		CRegion *pRegion = &pMap->m_ppRegion[nRX][nRZ];
		int nMonster = pRegion->m_RegionNpcArray.GetSize(), count = 0;
	
		//TRACE("FindEnemyExpand type1,, region_x=%d, region_z=%d, user=%d, mon=%d\n", nRX, nRZ, nUser, nMonster);
		if( nMonster == 0 )
			return 0.0f;

		pIDList = new int[nMonster];
		foreach_stlmap (itr, pRegion->m_RegionNpcArray)
			pIDList[count++] = *itr->second;

		//TRACE("FindEnemyExpand type2,, region_x=%d, region_z=%d, user=%d, mon=%d\n", nRX, nRZ, nUser, nMonster);

		for(int i=0 ; i<nMonster; i++ ) {
			int nNpcid = pIDList[i];
			if( nNpcid < NPC_BAND )	continue;
			CNpc *pNpc = (CNpc*)g_pMain->m_arNpc.GetData(nNpcid - NPC_BAND);

			if(m_sNid == pNpc->m_sNid)	continue;

			if( pNpc != nullptr && pNpc->m_NpcState != NPC_DEAD && pNpc->m_sNid != m_sNid)	{
				// ���� ������ ���ʹ� ������ ���� �ʵ��� �Ѵ�...
				if(m_byGroup == pNpc->m_byGroup)	continue;

				vMon.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 
				fDis = GetDistance(vMon, vNpc);

				// �۾� : ���⿡�� ���� ���ݰŸ��� �ִ� ���������� �Ǵ�
				if(fDis <= fSearchRange)	{
					if(fDis >= fComp)	{	// 
						target_uid = nNpcid;
						fComp = fDis;
						m_Target.id	= target_uid;
						m_Target.failCount = 0;
						m_Target.x	= pNpc->m_fCurX;
						m_Target.y	= pNpc->m_fCurY;
						m_Target.z	= pNpc->m_fCurZ;
					//	TRACE("Npc-IsCloseTarget - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);
					}
				}	
				else continue;
			}
		}
	}

	if(pIDList)	{
		delete [] pIDList;
		pIDList = nullptr;
	}

	return fComp;
}

// region�� 4����ؼ� ������ ���� ��ġ�� region�� ��� �κп� �������� �Ǵ�
int CNpc::GetMyField()
{
	int iRet = 0;
	int iX = m_iRegion_X * VIEW_DIST;
	int iZ = m_iRegion_Z * VIEW_DIST;
	int iAdd = VIEW_DIST / 2;
	int iCurX = (int)m_fCurX;	// monster current position_x
	int iCurZ = (int)m_fCurZ;
	if(COMPARE(iCurX, iX, iX+iAdd) && COMPARE(iCurZ, iZ, iZ+iAdd))
		iRet = 1;
	else if(COMPARE(iCurX, iX+iAdd, iX+VIEW_DIST) && COMPARE(iCurZ, iZ, iZ+iAdd))
		iRet = 2;
	else if(COMPARE(iCurX, iX, iX+iAdd) && COMPARE(iCurZ, iZ+iAdd, iZ+VIEW_DIST))
		iRet = 3;
	else if(COMPARE(iCurX, iX+iAdd, iX+VIEW_DIST) && COMPARE(iCurZ, iZ+iAdd, iZ+VIEW_DIST))
		iRet = 4;

	return iRet;
}

//	�ֺ��� ���� ������ ������ �ִ��� �˾ƺ���
bool CNpc::IsDamagedUserList(CUser *pUser)
{
	if(pUser == nullptr) return false;

	for(int i = 0; i < NPC_HAVE_USER_LIST; i++)
	{
		if(strcmp(m_DamagedUserList[i].strUserID, pUser->m_strUserID) == 0) return true;
	}

	return false;
}

//	Ÿ���� �ѷ� �׿� ������ ���� Ÿ���� ã�´�.
int CNpc::IsSurround(CUser* pUser)
{
	if(m_tNpcLongType) return 0;		//���Ÿ��� ���

	if(pUser == nullptr)	return -2;		// User�� �����Ƿ� Ÿ������ ����..
	int nDir = pUser->IsSurroundCheck(m_fCurX, 0.0f, m_fCurZ, m_sNid+NPC_BAND);
	if(nDir != 0)
	{
		m_byAttackPos = nDir;
		return nDir;
	}
	return -1;					// Ÿ���� �ѷ� �׿� ����...
}

//	x, y �� ������ �� �ִ� ��ǥ���� �Ǵ�
bool CNpc::IsMovable(float x, float z)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr
		|| x < 0 || z < 0
		|| x >= pMap->GetMapSize() || z >= pMap->GetMapSize()
		|| pMap->GetEventID((int)x, (int)z) == 0)
		return false;

	return true;
}

//	Path Find �� ã������ �� �̵� �ߴ��� �Ǵ�
bool CNpc::IsMovingEnd()
{
	//if(m_fCurX == m_fEndPoint_X && m_fCurZ == m_fEndPoint_Y) 
	if(m_fPrevX == m_fEndPoint_X && m_fPrevZ == m_fEndPoint_Y) 
	{
		//m_sStepCount = 0;
		m_iAniFrameCount = 0;
		return true;
	}

	return false;
}

//	Step �� ��ŭ Ÿ���� ���� �̵��Ѵ�.
bool CNpc::StepMove()
{
	if(m_NpcState != NPC_MOVING && m_NpcState != NPC_TRACING && m_NpcState != NPC_BACK) return false;
	
	POINT ptPre={-1,-1};

	__Vector3 vStart, vEnd, vDis;
	float fDis;

	float fOldCurX = 0.0f, fOldCurZ = 0.0f;

	if(m_sStepCount == 0)	{
		fOldCurX = m_fCurX;  fOldCurZ = m_fCurZ;
	}
	else	{
		fOldCurX = m_fPrevX; fOldCurZ = m_fPrevZ;
	}

	vStart.Set(fOldCurX, 0, fOldCurZ);
	vEnd.Set(m_pPoint[m_iAniFrameCount].fXPos, 0, m_pPoint[m_iAniFrameCount].fZPos);

	// ���� �ڵ�..
	if(m_pPoint[m_iAniFrameCount].fXPos < 0 || m_pPoint[m_iAniFrameCount].fZPos < 0)
	{
		m_fPrevX = m_fEndPoint_X;
		m_fPrevZ = m_fEndPoint_Y;
		TRACE("##### Step Move Fail : [nid = %d,%s] m_iAniFrameCount=%d/%d ######\n", m_sNid+NPC_BAND, m_proto->m_strName, m_iAniFrameCount, m_iAniFrameIndex);
		SetUid(m_fPrevX, m_fPrevZ, m_sNid + NPC_BAND);
		return false;	
	}

	fDis = GetDistance(vStart, vEnd);

	// For as long as speeds are broken, this check's going to cause problems
	// It's disabled for now, but note that the removal of this check is the reason 
	// why mobs are going to have weird looking bursts of speed.
	// Without, they'll just get stuck going back and forth in position. Compromises.
//#if 0
	if(fDis >= m_fSecForMetor)
	{
		GetVectorPosition(vStart, vEnd, m_fSecForMetor, &vDis);
		m_fPrevX = vDis.x;
		m_fPrevZ = vDis.z;
	}
	else
//#endif
	{
		m_iAniFrameCount++;
		if(m_iAniFrameCount == m_iAniFrameIndex)
		{
			vEnd.Set(m_pPoint[m_iAniFrameCount].fXPos, 0, m_pPoint[m_iAniFrameCount].fZPos);
			fDis = GetDistance(vStart, vEnd);
			// ������ ��ǥ�� m_fSecForMetor ~ m_fSecForMetor+1 ���̵� �����ϰ� �̵�
			if(fDis > m_fSecForMetor)
			{
				GetVectorPosition(vStart, vEnd, m_fSecForMetor, &vDis);
				m_fPrevX = vDis.x;
				m_fPrevZ = vDis.z;
				m_iAniFrameCount--;
			}
			else
			{
				m_fPrevX = m_fEndPoint_X;
				m_fPrevZ = m_fEndPoint_Y;
			}
		}
		else
		{
			vEnd.Set(m_pPoint[m_iAniFrameCount].fXPos, 0, m_pPoint[m_iAniFrameCount].fZPos);
			fDis = GetDistance(vStart, vEnd);
			if(fDis >= m_fSecForMetor)
			{
				GetVectorPosition(vStart, vEnd, m_fSecForMetor, &vDis);
				m_fPrevX = vDis.x;
				m_fPrevZ = vDis.z;
			}
			else
			{
				m_fPrevX = m_fEndPoint_X;
				m_fPrevZ = m_fEndPoint_Y;
			}
		}
	}

	vStart.Set(fOldCurX, 0, fOldCurZ);
	vEnd.Set(m_fPrevX, 0, m_fPrevZ);

	m_fSecForRealMoveMetor = GetDistance(vStart, vEnd);

	if(m_fSecForRealMoveMetor > m_fSecForMetor+1)
	{
		TRACE("#### move fail : [nid = %d], m_fSecForMetor = %.2f\n", m_sNid+NPC_BAND, m_fSecForRealMoveMetor);
	}

	if (m_sStepCount++ > 0)
	{
		m_fCurX = fOldCurX;		 m_fCurZ = fOldCurZ;
		if(m_fCurX < 0 || m_fCurZ < 0)
			TRACE("Npc-StepMove : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
		
		return SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND);
	}

	return true;
}

bool CNpc::StepNoPathMove()
{
	if(m_NpcState != NPC_MOVING && m_NpcState != NPC_TRACING && m_NpcState != NPC_BACK) return false;
	
	__Vector3 vStart, vEnd;
	float fOldCurX = 0.0f, fOldCurZ = 0.0f;

	if(m_sStepCount == 0)	{
		fOldCurX = m_fCurX; fOldCurZ = m_fCurZ;
	}
	else	{
		fOldCurX = m_fPrevX; fOldCurZ = m_fPrevZ;
	}

	
	if(m_sStepCount < 0 || m_sStepCount >= m_iAniFrameIndex)	{	
		TRACE("#### IsNoPtahfind Fail : nid=%d,%s, count=%d/%d ####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_sStepCount, m_iAniFrameIndex);
		return false;
	}

	vStart.Set(fOldCurX, 0, fOldCurZ);
	m_fPrevX = m_pPoint[m_sStepCount].fXPos;
	m_fPrevZ = m_pPoint[m_sStepCount].fZPos;
	vEnd.Set(m_fPrevX, 0, m_fPrevZ);

	if(m_fPrevX == -1 || m_fPrevZ == -1)	{
		TRACE("##### StepNoPath Fail : nid=%d,%s, x=%.2f, z=%.2f #####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fPrevX, m_fPrevZ);
		return false;
	}

	m_fSecForRealMoveMetor = GetDistance(vStart, vEnd);
	
	if (m_sStepCount++ > 0)
	{
		if(fOldCurX < 0 || fOldCurZ < 0)	{
			TRACE("#### Npc-StepNoPathMove Fail : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, fOldCurX, fOldCurZ);
			return false;
		}
		else	
		{
			m_fCurX = fOldCurX;	 m_fCurZ = fOldCurZ;
		}
		
		return (SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND));
	}

	return true;
}

//	NPC�� Target ���� �Ÿ��� ���� �������� ������ �Ǵ�
int CNpc::IsCloseTarget(int nRange, int Flag)
{
	__Vector3 vUser, vWillUser, vNpc, vDistance;
	CUser* pUser = nullptr;
	CNpc* pNpc = nullptr;
	float fDis = 0.0f, fWillDis = 0.0f, fX = 0.0f, fZ = 0.0f;
	bool  bUserType = false;	// Ÿ���� �����̸� true
	vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);

	if(m_Target.id >= USER_BAND && m_Target.id < NPC_BAND)	{	// Target �� User �� ���
		pUser = g_pMain->GetUserPtr(m_Target.id - USER_BAND);
		if(pUser == nullptr)	{
			InitTarget();
			return -1;
		}
		vUser.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz); 
		vWillUser.Set(pUser->m_fWill_x, pUser->m_fWill_y, pUser->m_fWill_z); 
		fX = pUser->m_curx;		
		fZ = pUser->m_curz;

		vDistance = vWillUser - vNpc;
		fWillDis = vDistance.Magnitude();	
		fWillDis = fWillDis - m_proto->m_fBulk;
		bUserType = true;
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)	// Target �� mon �� ���
	{
		pNpc = g_pMain->m_arNpc.GetData(m_Target.id - NPC_BAND);
		if(pNpc == nullptr) 
		{
			InitTarget();
			return -1;
		}
		vUser.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 
		fX = pNpc->m_fCurX;
		fZ = pNpc->m_fCurZ;
	}
	else	{
		return -1;
	}
	
	vDistance = vUser - vNpc;
	fDis = vDistance.Magnitude();	

	fDis = fDis - m_proto->m_fBulk;

	// �۾��Ұ� :	 ���� ������ ��� ���������� ����� ���ϵ��� üũ�ϴ� ��ƾ 	
    if ( m_proto->m_tNpcType == NPC_DUNGEON_MONSTER )	{
		if( IsInRange( (int)vUser.x, (int)vUser.z ) == false)
			return -1;	
    }		

	if(Flag == 1)	{
		m_byResetFlag = 1;
		if(pUser)	{
			if(m_Target.x == pUser->m_curx && m_Target.z == pUser->m_curz)
				m_byResetFlag = 0;
		}
		//TRACE("NpcTracing-IsCloseTarget - target_x = %.2f, z=%.2f, dis=%.2f, Flag=%d\n", m_Target.x, m_Target.z, fDis, m_byResetFlag);
	}
	
	if((int)fDis > nRange)	{
		if(Flag == 2)			{
			//TRACE("NpcFighting-IsCloseTarget - target_x = %.2f, z=%.2f, dis=%.2f\n", m_Target.x, m_Target.z, fDis);
			m_byResetFlag = 1;
			m_Target.x = fX;
			m_Target.z = fZ;
		}
		return 0; 
	}

	/* Ÿ���� ��ǥ�� �ֽ� ������ �����ϰ�, ������ ������ ��ǥ�� �����Ѵ�,, */
	m_fEndPoint_X = m_fCurX;
	m_fEndPoint_Y = m_fCurZ;
	m_Target.x = fX;
	m_Target.z = fZ;

	//if( m_tNpcLongType && m_proto->m_tNpcType != NPC_BOSS_MONSTER)	{		// ��Ÿ� ������ �����Ѱ��� ���ݰŸ��� �Ǵ�..
	if( m_tNpcLongType == 1 )	{		// ��Ÿ� ������ �����Ѱ��� ���ݰŸ��� �Ǵ�..
		if(fDis < LONG_ATTACK_RANGE)	return 1;
		else if(fDis > LONG_ATTACK_RANGE && fDis <= nRange) return 2;
	}
	else	{					// �ܰŸ�(��������)
		if( Flag == 1 )		{	// ������ �̵��ϸ鼭�� �Ÿ�üũ��
			if(fDis < (SHORT_ATTACK_RANGE+m_proto->m_fBulk) )	return 1;
			if(fDis > (SHORT_ATTACK_RANGE+m_proto->m_fBulk) && fDis <= nRange) return 2;				// ������ ������ǥ�� ��������
			if( bUserType == true )	{	// �����϶���,, Will��ǥ�� �������� �Ѵ�
				if(fWillDis > (SHORT_ATTACK_RANGE+m_proto->m_fBulk) && fWillDis <= nRange) return 2;		// ������ Will��ǥ�� ��������
			}
		}
		else {
			if(fDis < (SHORT_ATTACK_RANGE+m_proto->m_fBulk) )	return 1;
			else if(fDis > (SHORT_ATTACK_RANGE+m_proto->m_fBulk) && fDis <= nRange) return 2;
		}
	}


	//TRACE("Npc-IsCloseTarget - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);
	return 0;
}

//	Target �� NPC �� Path Finding�� �����Ѵ�.
int CNpc::GetTargetPath(int option)
{
	int nInitType = m_byInitMoveType;
	if(m_byInitMoveType >= 100)	{
		nInitType = m_byInitMoveType - 100;
	}
	// �ൿ Ÿ�� ����
	if(m_proto->m_tNpcType != 0)	{
		//if(m_byMoveType != m_byInitMoveType)
		//	m_byMoveType = m_byInitMoveType;	// �ڱ� �ڸ��� ���ư� �� �ֵ���..
		if(m_byMoveType != nInitType)	m_byMoveType = nInitType;	// �ڱ� �ڸ��� ���ư� �� �ֵ���..
	}

	// �߰��Ҷ��� �ٴ� �ӵ��� ���߾��ش�...
	m_fSecForMetor = m_fSpeed_2;
	CUser* pUser = nullptr;
	CNpc* pNpc = nullptr;
	float iTempRange = 0.0f;
	__Vector3 vUser, vNpc, vDistance, vEnd22;
	float fDis = 0.0f;
	float fDegree = 0.0f, fTargetDistance = 0.0f;
	float fSurX = 0.0f, fSurZ = 0.0f;

	if(m_Target.id >= USER_BAND && m_Target.id < NPC_BAND)	{	// Target �� User �� ���
		pUser = g_pMain->GetUserPtr(m_Target.id - USER_BAND);
		if(pUser == nullptr)	{
			InitTarget();
			return -1;
		}
		if(pUser->m_sHP <= 0 /*|| pUser->m_state != GAME_STATE_INGAME*/ || pUser->m_bLive == false)	{
			InitTarget();
			return -1;
		}
		if(pUser->m_curZone != m_bCurZone)	{
			InitTarget();
			return -1;
		}

		if(option == 1)	{	// magic�̳� Ȱ������ ���� ���ߴٸ�...
			vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
			vUser.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz); 
			fDis = GetDistance(vNpc, vUser);
			if(fDis >= NPC_MAX_MOVE_RANGE)		return -1;	// �ʹ� �Ÿ��� �־,, ������ �ȵǰ�..
			iTempRange = fDis + 10;
		}
		else	{
			iTempRange = (float)m_bySearchRange;				// �Ͻ������� �����Ѵ�.
			if(IsDamagedUserList(pUser)) iTempRange = (float)m_byTracingRange;	// ���ݹ��� ���¸� ã�� ���� ����.
			else iTempRange += 2;
		}

		if (m_bTracing)
		{
			float dx = m_fCurX - m_fTracingStartX;
			float dy = m_fCurZ - m_fTracingStartZ;
			if (pow(dx, 2.0f) + pow(dy, 2.0f) > pow(iTempRange, 2.0f))
			{
				InitTarget();
				return -1;
			}
		}
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)	{	// Target �� mon �� ���
		pNpc = g_pMain->m_arNpc.GetData(m_Target.id - NPC_BAND);
		if(pNpc == nullptr) {
			InitTarget();
			return false;
		}
		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			InitTarget();
			return -1;
		}

		iTempRange = (float)m_byTracingRange;				// �Ͻ������� �����Ѵ�.
	}

	MAP* pMap = GetMap();
	if (pMap == nullptr) 
		return -1;

	int max_xx = pMap->GetMapSize();
	int max_zz = pMap->GetMapSize();

	int min_x = (int)(m_fCurX - iTempRange)/TILE_SIZE;	if(min_x < 0) min_x = 0;
	int min_z = (int)(m_fCurZ - iTempRange)/TILE_SIZE;	if(min_z < 0) min_z = 0;
	int max_x = (int)(m_fCurX + iTempRange)/TILE_SIZE;	if(max_x > max_xx) max_x = max_xx;
	int max_z = (int)(m_fCurZ + iTempRange)/TILE_SIZE;	if(min_z > max_zz) min_z = max_zz;

	if(m_Target.id >= USER_BAND && m_Target.id < NPC_BAND)	{	// Target �� User �� ���
		// ��ǥ���� Search Range�� ����� �ʴ��� �˻�
		CRect r = CRect(min_x, min_z, max_x+1, max_z+1);
		if(r.PtInRect((int)pUser->m_curx/TILE_SIZE, (int)pUser->m_curz/TILE_SIZE) == false)	{
			TRACE("### Npc-GetTargetPath() User Fail return -1: [nid=%d] t_Name=%s, AttackPos=%d ###\n", m_sNid+NPC_BAND, pUser->m_strUserID, m_byAttackPos);
			return -1;
		}

		m_fStartPoint_X = m_fCurX;		m_fStartPoint_Y = m_fCurZ;
	
		vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
		vUser.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz); 
		
		// ���⿡�� ������ ��� �������� �����Ұ������� �Ǵ�...(����)
		// �� �κп��� Npc�� �������� �˾ƿͼ� �����ϵ��� �Ѵ�,,
		IsSurround(pUser);	//�ѷ� �׿� ������ �����Ѵ�.(���Ÿ�, �ٰŸ� ����)

		//vEnd22 = CalcAdaptivePosition(vNpc, vUser, 2.0+m_proto->m_fBulk);

		//TRACE("Npc-GetTargetPath() : [nid=%d] t_Name=%s, AttackPos=%d\n", m_sNid, pUser->m_strUserID, m_byAttackPos);

		if(m_byAttackPos > 0 && m_byAttackPos < 9)	{
			fDegree = (float)((m_byAttackPos-1)*45);
			fTargetDistance = 2.0f+m_proto->m_fBulk;
			ComputeDestPos(vUser, 0.0f, fDegree, fTargetDistance, &vEnd22);
			fSurX = vEnd22.x - vUser.x;			fSurZ = vEnd22.z - vUser.z;
			m_fEndPoint_X = vUser.x + fSurX;	m_fEndPoint_Y = vUser.z + fSurZ;
		}
		else
		{
			CalcAdaptivePosition(vNpc, vUser, 2.0f+m_proto->m_fBulk, &vEnd22);
			m_fEndPoint_X = vEnd22.x;	m_fEndPoint_Y = vEnd22.z;
		}
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)	{	// Target �� mon �� ���
		// ��ǥ���� Search Range�� ����� �ʴ��� �˻�
		CRect r = CRect(min_x, min_z, max_x+1, max_z+1);
		if(r.PtInRect((int)pNpc->m_fCurX/TILE_SIZE, (int)pNpc->m_fCurZ/TILE_SIZE) == false)	{
			TRACE("### Npc-GetTargetPath() Npc Fail return -1: [nid=%d] t_Name=%s, AttackPos=%d ###\n", m_sNid+NPC_BAND, pNpc->m_proto->m_strName, m_byAttackPos);
			return -1;
		}

		m_fStartPoint_X = m_fCurX;		m_fStartPoint_Y = m_fCurZ;
	
		vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
		vUser.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 
		
		CalcAdaptivePosition(vNpc, vUser, 2.0f+m_proto->m_fBulk, &vEnd22);
		m_fEndPoint_X = vEnd22.x;	m_fEndPoint_Y = vEnd22.z;
	}

	vDistance = vEnd22 - vNpc;
	fDis = vDistance.Magnitude();

	if(fDis <= m_fSecForMetor)	{
		ClearPathFindData();
		m_bPathFlag = true;
		m_iAniFrameIndex = 1;
		m_pPoint[0].fXPos = m_fEndPoint_X;
		m_pPoint[0].fZPos = m_fEndPoint_Y;
		//TRACE("** Npc Direct Trace Move  : [nid = %d], fDis <= %d, %.2f **\n", m_sNid, m_fSecForMetor, fDis);
		return true;
	}
	
	if((int)fDis > iTempRange)	{
		TRACE("Npc-GetTargetPath() searchrange over Fail return -1: [nid=%d,%s]\n", m_sNid+NPC_BAND, m_proto->m_strName);
		return -1; 
	}

	
	if( m_proto->m_tNpcType != NPC_DUNGEON_MONSTER )	{		// ���� ���ʹ� ������ �н����ε��� �ϵ���..
		// ���ݴ���� ������ �н����ε��� ���� �ʰ� �ٷ� Ÿ������ ���� �Ѵ�.
		if(m_Target.id != -1)	return 0;	
	}

	CPoint start, end;
	start.x = (int)(m_fCurX/TILE_SIZE) - min_x;
	start.y = (int)(m_fCurZ/TILE_SIZE) - min_z;
	end.x = (int)(vEnd22.x/TILE_SIZE) - min_x;
	end.y = (int)(vEnd22.z/TILE_SIZE) - min_z;

	// �۾��Ұ� :	 ���� ������ ��� ���������� ����� ���ϵ��� üũ�ϴ� ��ƾ 	
    if ( m_proto->m_tNpcType == NPC_DUNGEON_MONSTER )	{
		if( IsInRange( (int)vEnd22.x, (int)vEnd22.z ) == false)
			return -1;	
    }

	m_min_x = min_x;
	m_min_y = min_z;
	m_max_x = max_x;
	m_max_y = max_z;

	// Run Path Find ---------------------------------------------//
	return PathFind(start, end, m_fSecForMetor);
}

int CNpc::Attack()
{
	// �ڷ���Ʈ �����ϰ�,, (��������,, )
	int nRandom = 0, nPercent=1000;
	bool bTeleport = false;

/*	nRandom = myrand(1, 10000);
	if( COMPARE( nRandom, 8000, 10000) )	{
		bTeleport = Teleport();
		if( bTeleport )		return m_Delay;
	}	*/

	//if( m_tNpcLongType==1 && m_proto->m_tNpcType != NPC_BOSS_MONSTER )	{
	if( m_tNpcLongType == 1 )	{		// ��Ÿ� ������ �����Ѱ��� ���ݰŸ��� �Ǵ�..
		return LongAndMagicAttack();
	}
		
	int ret = 0;
	int nStandingTime = m_sStandTime;

	ret = IsCloseTarget(m_byAttackRange, 2);

	if(ret == 0)   {
		if(m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT ) // ���� ����� ������ ���� �ʵ���..
		{
			m_NpcState = NPC_STANDING;
			InitTarget();
			return 0;
		}	
		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;			// �����ϰ� �������� ������ ���� �������(������ ���� ������) 
		return 0;							// IsCloseTarget()�� ���� x, y���� �����ϰ� Delay = 0���� ��
	}	
	else if( ret == 2 )	{
		//if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{		// ���� �����̸�.....
		if(m_tNpcLongType == 2)	{		// ����, ����(��)������ ������ ���� �̹Ƿ� ��Ÿ� ������ �� �� �ִ�.
			return LongAndMagicAttack();
		}
		else	{
			if(m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT ) // ���� ����� ������ ���� �ʵ���..
			{
				m_NpcState = NPC_STANDING;
				InitTarget();
				return 0;
			}	
			m_sStepCount = 0;
			m_byActionFlag = ATTACK_TO_TRACE;
			m_NpcState = NPC_TRACING;			// �����ϰ� �������� ������ ���� �������(������ ���� ������) 
			return 0;							// IsCloseTarget()�� ���� x, y���� �����ϰ� Delay = 0���� ��
		}
	}
	else if( ret == -1 )	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}

	CNpc*	pNpc		= nullptr;	
	CUser*	pUser		= nullptr;
	int		nDamage		= 0;
	int nID = m_Target.id;					// Target �� ���Ѵ�.

	// ȸ�ǰ�/��������/������ ��� -----------------------------------------//
	if(nID >= USER_BAND && nID < NPC_BAND)	{	// Target �� User �� ���
		pUser = g_pMain->GetUserPtr(nID - USER_BAND);
		
		if(pUser == nullptr)	{				// User �� Invalid �� ���
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_bLive == USER_DEAD)	{		// User �� �̹� �������
			//SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, pUser->m_iHP);
			SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if (pUser->m_bInvisibilityType
			/*|| pUser->m_state == STATE_DISCONNECTED*/)
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_byIsOP == MANAGER_USER)	{	// ��ڴ� ������ ���ϰ�..
			InitTarget();
			m_NpcState = NPC_MOVING;
			return nStandingTime;
		}
		// Npc�� �������� HP�� ���Ͽ�.. ������ �� �������� �Ǵ�...
	/*	if(m_byNpcEndAttType)	{
			if(IsCompStatus(pUser) == true)	{
				m_NpcState = NPC_BACK;
				return 0;
			}	
		}	*/

		//if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{		// ���� �����̸�.....
		if(m_byWhatAttackType == 4 || m_byWhatAttackType == 5)	{		// ���� ���� ��� �����̸�.....
			nRandom = myrand(1, 10000);
			if(nRandom < nPercent)	{				// ������������...
				CNpcMagicProcess::MagicPacket(MAGIC_EFFECTING, m_proto->m_iMagic2, m_sNid + NPC_BAND, -1, int16(m_fCurX), int16(m_fCurY), int16(m_fCurZ));
				//TRACE("++++ AreaMagicAttack --- sid=%d, magicid=%d\n", m_sNid+NPC_BAND, m_proto->m_iMagic2);
				return m_sAttackDelay + 1000;	// ���������� ���� �ð��� �ɸ�����.....
			}
		}
		else	{
			if(m_byWhatAttackType == 2)	{	// �� �����ϴ� ���Ͷ��... (10%�� ��������)
				nRandom = myrand(1, 10000);

				if(nRandom < nPercent)	{				// ������...
					Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
					result	<< m_proto->m_iMagic1 << uint16(m_sNid + NPC_BAND) << pUser->m_iUserId
							<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
					g_pMain->Send(&result);

					//TRACE("LongAndMagicAttack --- sid=%d, tid=%d\n", m_sNid+NPC_BAND, pUser->m_iUserId);
					return m_sAttackDelay;
				}
			}
		}

		// �����̸� //Damage ó�� ----------------------------------------------------------------//
		nDamage = GetFinalDamage(pUser);	// ���� �����
		//TRACE("Npc-Attack() : [mon: x=%.2f, z=%.2f], [user : x=%.2f, z=%.2f]\n", m_fCurX, m_fCurZ, pUser->m_curx, pUser->m_curz);
		
		if(nDamage > 0) {
			pUser->SetDamage(nDamage, m_sNid+NPC_BAND);
			if(pUser->m_bLive != USER_DEAD)	{
			//	TRACE("Npc-Attack Success : [npc=%d, %s]->[user=%d, %s]\n", m_sNid+NPC_BAND, m_proto->m_strName, pUser->m_iUserId, pUser->m_strUserID);
				SendAttackSuccess(ATTACK_SUCCESS, pUser->m_iUserId, nDamage, pUser->m_sHP);
			}
		}
		else
			SendAttackSuccess(ATTACK_FAIL, pUser->m_iUserId, nDamage, pUser->m_sHP);

		// ����� ������ ����
	}
	else if(nID >= NPC_BAND && m_Target.id < INVALID_BAND)	{
		pNpc = g_pMain->m_arNpc.GetData(nID - NPC_BAND);
	
		if(pNpc == nullptr)	{				// User �� Invalid �� ���
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if( m_proto->m_tNpcType == NPC_HEALER && pNpc->m_byGroup == m_byGroup )	{	// healer�̸鼭 ���������� NPC�ΰ�쿡�� ��
			m_NpcState = NPC_HEALING;
			return 0;
		}

		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->m_sNid+NPC_BAND, 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		// Npc�� �������� HP�� ���Ͽ�.. ������ �� �������� �Ǵ�...
	/*	if(IsCompStatus(pUser) == true)	{
			m_NpcState = NPC_BACK;
			return 0;
		}	*/

		// MoveAttack
		//MoveAttack();

		// �����̸� //Damage ó�� ----------------------------------------------------------------//
		nDamage = GetNFinalDamage(pNpc);	// ���� �����

		if(pUser)
		{
			//TRACE("Npc-Attack() : [mon: x=%.2f, z=%.2f], [user : x=%.2f, z=%.2f]\n", m_fCurX, m_fCurZ, pUser->m_curx, pUser->m_curz);
		}
		
		if(nDamage > 0)	{
			pNpc->SetDamage(0, nDamage, m_proto->m_strName, m_sNid+NPC_BAND);
			//if(pNpc->m_iHP > 0)
			SendAttackSuccess(ATTACK_SUCCESS, pNpc->m_sNid+NPC_BAND, nDamage, pNpc->m_iHP);
		}
		else
			SendAttackSuccess(ATTACK_FAIL, pNpc->m_sNid+NPC_BAND, nDamage, pNpc->m_iHP);
	}

	return m_sAttackDelay;
}

int CNpc::LongAndMagicAttack()
{
	int ret = 0;
	int nStandingTime = m_sStandTime;

	ret = IsCloseTarget(m_byAttackRange, 2);

	if(ret == 0)	{
		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;			// �����ϰ� �������� ������ ���� �������(������ ���� ������) 
		return 0;							// IsCloseTarget()�� ���� x, y���� �����ϰ� Delay = 0���� ��
	}	
	else if( ret == 2 )	{
		//if(m_proto->m_tNpcType != NPC_BOSS_MONSTER)	{		// ���� �����̸�.....
		if(m_tNpcLongType == 1)	{		// ��Ÿ� �����̸�.....
			m_sStepCount = 0;
			m_byActionFlag = ATTACK_TO_TRACE;
			m_NpcState = NPC_TRACING;			// �����ϰ� �������� ������ ���� �������(������ ���� ������) 
			return 0;							// IsCloseTarget()�� ���� x, y���� �����ϰ� Delay = 0���� ��
		}
	}
	if( ret == -1 )	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}

	CNpc*	pNpc		= nullptr;	
	CUser*	pUser		= nullptr;
	int		nDamage		= 0;
	int nID = m_Target.id;					// Target �� ���Ѵ�.

	// ȸ�ǰ�/��������/������ ��� -----------------------------------------//
	if(nID >= USER_BAND && nID < NPC_BAND)	{	// Target �� User �� ���
		pUser = g_pMain->GetUserPtr(nID - USER_BAND);
		
		if(pUser == nullptr)	{				// User �� Invalid �� ���
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_bLive == USER_DEAD)	{		// User �� �̹� �������
			SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if (pUser->m_bInvisibilityType
			/*|| pUser->m_state == STATE_DISCONNECTED*/)
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_byIsOP == MANAGER_USER)	{		// ��ڴ� ������ ���ϰ�..
			InitTarget();
			m_NpcState = NPC_MOVING;
			return nStandingTime;
		}
		// Npc�� �������� HP�� ���Ͽ�.. ������ �� �������� �Ǵ�...
	/*	if(m_byNpcEndAttType)
		{
			if(IsCompStatus(pUser) == true)
			{
				m_NpcState = NPC_BACK;
				return 0;
			}	
		}	*/

		CNpcMagicProcess::MagicPacket(MAGIC_CASTING, m_proto->m_iMagic1, m_sNid + NPC_BAND, pUser->m_iUserId);
		//TRACE("**** LongAndMagicAttack --- sid=%d, tid=%d\n", m_sNid+NPC_BAND, pUser->m_iUserId);
	}
	else if(nID >= NPC_BAND && m_Target.id < INVALID_BAND)	{
		pNpc = g_pMain->m_arNpc.GetData(nID - NPC_BAND);
		//pNpc = g_pMain->m_arNpc[nID - NPC_BAND];
		
		if(pNpc == nullptr)	{				// User �� Invalid �� ���
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->m_sNid+NPC_BAND, 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		// Npc�� �������� HP�� ���Ͽ�.. ������ �� �������� �Ǵ�...
	/*	if(IsCompStatus(pUser) == true)
		{
			m_NpcState = NPC_BACK;
			return 0;
		}	*/
	}

	return m_sAttackDelay;
}

int CNpc::TracingAttack()		// 0:attack fail, 1:attack success
{
	CNpc*	pNpc		= nullptr;	
	CUser*	pUser		= nullptr;

	int		nDamage		= 0;

	int nID = m_Target.id;					// Target �� ���Ѵ�.

	// ȸ�ǰ�/��������/������ ��� -----------------------------------------//
	if(nID >= USER_BAND && nID < NPC_BAND)	{	// Target �� User �� ���
		pUser = g_pMain->GetUserPtr(nID - USER_BAND);
		if(pUser == nullptr)	return 0;				// User �� Invalid �� ���
		if(pUser->m_bLive == USER_DEAD)		{		// User �� �̹� �������
			SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, 0);
			return 0;
		}

		if (pUser->m_bInvisibilityType
			/*|| pUser->m_state == STATE_DISCONNECTED*/
			|| pUser->m_byIsOP == MANAGER_USER)
			return 0;

		// �����̸� //Damage ó�� ----------------------------------------------------------------//
		nDamage = GetFinalDamage(pUser);	// ���� �����
		
		if(nDamage > 0)		{
			pUser->SetDamage(nDamage, m_sNid+NPC_BAND);
			if(pUser->m_bLive != USER_DEAD)	{
			//	TRACE("Npc-Attack Success : [npc=%d, %s]->[user=%d, %s]\n", m_sNid+NPC_BAND, m_proto->m_strName, pUser->m_iUserId, pUser->m_strUserID);
				SendAttackSuccess(ATTACK_SUCCESS, pUser->m_iUserId, nDamage, pUser->m_sHP);
			}
		}
		else
			SendAttackSuccess(ATTACK_FAIL, pUser->m_iUserId, nDamage, pUser->m_sHP);

		// ����� ������ ����
	}
	else if(nID >= NPC_BAND && m_Target.id < INVALID_BAND)	{
		pNpc = g_pMain->m_arNpc.GetData(nID - NPC_BAND);
		
		if(pNpc == nullptr)	return 0;				// User �� Invalid �� ���

		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->m_sNid+NPC_BAND, 0, 0);
			return 0;
		}

		// �����̸� //Damage ó�� ----------------------------------------------------------------//
		nDamage = GetNFinalDamage(pNpc);	// ���� �����
		//TRACE("Npc-Attack() : [mon: x=%.2f, z=%.2f], [user : x=%.2f, z=%.2f]\n", m_fCurX, m_fCurZ, pUser->m_curx, pUser->m_curz);
		
		if(nDamage > 0)		{
			if(pNpc->SetDamage(0, nDamage, m_proto->m_strName, m_sNid+NPC_BAND) == true) {
				SendAttackSuccess(ATTACK_SUCCESS, pNpc->m_sNid+NPC_BAND, nDamage, pNpc->m_iHP);
			}
			else{
				SendAttackSuccess(ATTACK_SUCCESS, pNpc->m_sNid+NPC_BAND, nDamage, pNpc->m_iHP);
				SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->m_sNid+NPC_BAND, nDamage, pNpc->m_iHP);
				return 0;
			}
		}
		else{
			SendAttackSuccess(ATTACK_FAIL, pNpc->m_sNid+NPC_BAND, nDamage, pNpc->m_iHP);
		}
	}

	return 1;
}

void CNpc::MoveAttack()
{
	__Vector3 vUser;
	__Vector3 vNpc;
	__Vector3 vDistance;
	__Vector3 vEnd22;
	CUser* pUser = nullptr;
	CNpc* pNpc = nullptr;
	float fDis = 0.0f;
	float fX = 0.0f, fZ = 0.0f;
	vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);

	if(m_Target.id >= USER_BAND && m_Target.id < NPC_BAND)	// Target �� User �� ���
	{
		pUser = g_pMain->GetUserPtr(m_Target.id - USER_BAND);
		if(pUser == nullptr) 
		{
			InitTarget();
			return;
		}
		vUser.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz); 

		CalcAdaptivePosition(vNpc, vUser, 2, &vEnd22);

		if(m_byAttackPos > 0 && m_byAttackPos < 9)
		{
			fX = vUser.x + surround_fx[m_byAttackPos-1];	fZ = vUser.z + surround_fz[m_byAttackPos-1];
			vEnd22.Set(fX, 0, fZ);
			//TRACE("MoveAttack 11 - nid(%s, %d), fx=%.2f, fz=%.2f, attackpos=%d\n", m_proto->m_strName, m_sNid+NPC_BAND, fX, fZ, m_byAttackPos);
		}
		else
		{
			fX = vEnd22.x;	fZ = vEnd22.z;
			//TRACE("MoveAttack 22 - nid(%s, %d), fx=%.2f, fz=%.2f, attackpos=%d\n", m_proto->m_strName, m_sNid+NPC_BAND, fX, fZ, m_byAttackPos);
		}
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)	// Target �� mon �� ���
	{
		pNpc = g_pMain->m_arNpc.GetData(m_Target.id - NPC_BAND);		
		//pNpc = g_pMain->m_arNpc[m_Target.id - NPC_BAND];
		if(pNpc == nullptr) 
		{
			InitTarget();
			return;
		}
		vUser.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 

		CalcAdaptivePosition(vNpc, vUser, 2, &vEnd22);
		fX = vEnd22.x;	fZ = vEnd22.z;
	}
	
	vDistance = vUser - vNpc;
	fDis = vDistance.Magnitude();	
	
	if((int)fDis < 3) return;	// target���� �Ÿ��� 3���� �̸��̸� ������¿��� �����̰�..
/*	if(m_tNpcLongType)		// ��Ÿ� ������ �����Ѱ��� ���ݰŸ��� �Ǵ�..
	{
		if((int)fDis > nRange) return false; 
	}	
	else					// �ܰŸ�(��������)
	{
		if(fDis > 2.5) return false;			// �۾� :���ݰ��ɰŸ��� 2.5�� �ӽ� ������..
	}	*/

	vDistance = vEnd22 - vNpc;
	fDis = vDistance.Magnitude();	
	m_fCurX = vEnd22.x;
	m_fCurZ = vEnd22.z;

	if(m_fCurX < 0 || m_fCurZ < 0)
	{
		TRACE("Npc-MoveAttack : nid=(%d, %s), x=%.2f, z=%.2f\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fCurX, m_fCurZ);
	}


	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result	<< uint16(m_sNid + NPC_BAND)
			<< m_fCurX << m_fCurZ << m_fCurY;

	// This seems really dumb, but for reasons unbeknownst to me
	// we're sending the packet twice -- first with the distance/speed,
	// second without. 
	int wpos = result.wpos(); 
	result << fDis;
	g_pMain->Send(&result); // send the first packet

	result.put(wpos, 0.0f); // replace the distance/speed with 0
	g_pMain->Send(&result); // send it again

	SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND);
	
	m_fEndPoint_X = m_fCurX;
	m_fEndPoint_Y = m_fCurZ;
}

int CNpc::GetNFinalDamage(CNpc *pNpc)
{
	short damage = 0;
	float Attack = 0;
	float Avoid = 0;
	short Hit = 0;
	short Ac = 0;
	int random = 0;
	uint8 result;

	if(pNpc == nullptr)	return damage;

	// ���ݹ�ø
	Attack = (float)m_sHitRate;

	// ����ø
	Avoid = (float)pNpc->m_sEvadeRate;

	//������ Hit 
	Hit = m_sDamage;
	
	// ����� Ac 
	Ac = (short)pNpc->m_sDefense;

	// Ÿ�ݺ� ���ϱ�
	result = GetHitRate(Attack/Avoid);		

	switch(result)
	{
//	case GREAT_SUCCESS:
//		damage = (short)(0.6 * (2 * Hit));
//		if(damage <= 0){
//			damage = 0;
//			break;
//		}
//		damage = myrand(0, damage);
//		damage += (short)(0.7 * (2 * Hit));
//		break;
	case GREAT_SUCCESS:
		damage = (short)(0.6 * Hit);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.7 * Hit);
		break;
	case SUCCESS:
	case NORMAL:
		if(Hit - Ac > 0)
		{
			damage = (short)(0.6 * (Hit - Ac));
			if(damage <= 0){
				damage = 0;
			break;
			}
			damage = myrand(0, damage);
			damage += (short)(0.7 * (Hit - Ac));
		}
		else
			damage = 0;
		break;
	case FAIL:
		damage = 0;
		break;
	}
	
	return damage;	
}

bool CNpc::IsCompStatus(CUser* pUser)
{
	
	if(IsHPCheck(pUser->m_sHP) == true)
	{
		if(RandomBackMove() == false)
			return false;
		else 
			return true;
	}

	return false;
}

//	Target �� ��ġ�� �ٽ� ��ã�⸦ �� ������ ���ߴ��� �Ǵ�
bool CNpc::IsChangePath()
{
	// �н����ε��� ������ ��ǥ�� ������,, Target�� �� ���ݰŸ��� �ִ����� �Ǵ�,,
//	if(!m_pPath) return true;

	float fCurX=0.0f, fCurZ=0.0f;
	GetTargetPos(fCurX, fCurZ);

	__Vector3 vStart, vEnd;
	vStart.Set(m_fEndPoint_X, 0, m_fEndPoint_Y);
	vEnd.Set(fCurX, 0, fCurZ);
	
	float fDis = GetDistance(vStart, vEnd);
	float fCompDis = 3.0f;

	//if(fDis <= m_byAttackRange)
	if(fDis < fCompDis)
	{
		//TRACE("#### Npc-IsChangePath() : [nid=%d] -> attack range in #####\n", m_sNid);
		return false;
	}

	 //TRACE("#### IsChangePath() - [�� - cur:x=%.2f, z=%.2f, ��ǥ��:x=%.2f, z=%.2f], [target : x=%.2f, z=%.2f]\n", 
	//	 m_fCurX, m_fCurZ, m_fEndPoint_X, m_fEndPoint_Y, fCurX, fCurZ);
	return true;
}

//	Target �� ���� ��ġ�� ��´�.
bool CNpc::GetTargetPos(float& x, float& z)
{
	if(m_Target.id >= USER_BAND && m_Target.id < NPC_BAND)	// Target �� User �� ���
	{
		CUser* pUser = g_pMain->GetUserPtr(m_Target.id - USER_BAND);

		if(!pUser) return false;

		x = pUser->m_curx;
		z = pUser->m_curz;
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)
	{
		CNpc* pNpc = g_pMain->m_arNpc.GetData(m_Target.id - NPC_BAND);
		//CNpc* pNpc = g_pMain->m_arNpc[m_Target.id - NPC_BAND];
		if(!pNpc) return false;

		x = pNpc->m_fCurX;
		z = pNpc->m_fCurZ;

	}

	return true;
}

//	Target �� NPC ���� ��ã�⸦ �ٽ��Ѵ�.
bool CNpc::ResetPath()
{
	float cur_x, cur_z;
	GetTargetPos(cur_x, cur_z);

//	TRACE("ResetPath : user pos ,, x=%.2f, z=%.2f\n", cur_x, cur_z);

	m_Target.x = cur_x;
	m_Target.z = cur_z;

	int nValue = GetTargetPath();
	if(nValue == -1)		// Ÿ���� �������ų�,, �־���������...
	{
		TRACE("Npc-ResetPath Fail - target_x = %.2f, z=%.2f, value=%d\n", m_Target.x, m_Target.z, nValue);
		return false;
	}
	else if(nValue == 0)	// Ÿ�� �������� �ٷ� ����..
	{
		m_fSecForMetor = m_fSpeed_2;	// �����϶��� �ٴ� �ӵ���... 
		IsNoPathFind(m_fSecForMetor);
	}

	//TRACE("Npc-ResetPath - target_x = %.2f, z=%.2f, value=%d\n", m_Target.x, m_Target.z, nValue);

	return true;	
}

int CNpc::GetFinalDamage(CUser *pUser, int type)
{
	short damage = 0;
	float Attack = 0;
	float Avoid = 0;
	short Hit = 0;
	short Ac = 0;
	short HitB = 0;
	int random = 0;
	uint8 result;

	if(pUser == nullptr)	return damage;
	
	Attack = (float)m_sHitRate;		// ���ݹ�ø
	Avoid = (float)pUser->m_fAvoidrate;		// ����ø	
	Hit = m_sDamage;	// ������ Hit 		
//	Ac = (short)pUser->m_sAC ;	// ����� Ac 

//	Ac = (short)pUser->m_sItemAC + (short)pUser->m_sLevel ;	// ����� Ac 
//	Ac = (short)pUser->m_sAC - (short)pUser->m_sLevel ;	// ����� Ac. ��...������ �̿� ��.��
	Ac = (short)pUser->m_sItemAC + (short)pUser->m_bLevel + (short)(pUser->m_sAC - pUser->m_bLevel - pUser->m_sItemAC);

//	ASSERT(Ac != 0);
//	short kk = (short)pUser->m_sItemAC;
//	short tt = (short)pUser->m_sLevel;
//	Ac = kk + tt;

	HitB = (int)((Hit * 200) / (Ac + 240)) ;

	int nMaxDamage = (int)(2.6 * m_sDamage);

	// Ÿ�ݺ� ���ϱ�
	result = GetHitRate(Attack/Avoid);	
	
//	TRACE("Hitrate : %d     %f/%f\n", result, Attack, Avoid);

	switch(result)
	{
	case GREAT_SUCCESS:
/*
		damage = (short)(0.6 * (2 * Hit));
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.7 * (2 * Hit));
		break;
*/	
//		damage = 0;
//		break;

		damage = (short)HitB;
		if(damage <= 0){
			damage = 0;
			break;
		}

		damage = (int)(0.3f * myrand(0, damage));
		damage += (short)(0.85f * (float)HitB);
//		damage = damage * 2;
		damage = (damage * 3) / 2;
		break;

	case SUCCESS:
/*
		damage = (short)(0.6f * Hit);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.7f * Hit);
		break;
*/
/*
		damage = (short)(0.3f * (float)HitB);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.85f * (float)HitB);
*/
/*
		damage = (short)HitB;
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = (int)(0.3f * myrand(0, damage));
		damage += (short)(0.85f * (float)HitB);
		damage = damage * 2;

		break;
*/
	case NORMAL:
		/*
		if(Hit - Ac > 0){
			damage = (short)(0.6f * (Hit - Ac));
			if(damage <= 0){
				damage = 0;
				break;
			}
			damage = myrand(0, damage);
			damage += (short)(0.7f * (Hit - Ac));

		}
		else
			damage = 0;
		*/
/*
		damage = (short)(0.3f * (float)HitB);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.85f * (float)HitB);
*/
		damage = (short)HitB;
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = (int)(0.3f * myrand(0, damage));
		damage += (short)(0.85f * (float)HitB);

		break;

	case FAIL:
		damage = 0;

		break;
	}
	
	if(damage > nMaxDamage)	{
		TRACE("#### Npc-GetFinalDamage Fail : nid=%d, result=%d, damage=%d, maxdamage=%d\n", m_sNid+NPC_BAND, result, damage, nMaxDamage);
		damage = nMaxDamage;
	}

	return damage;	
}

//	���� ������ ������ Ÿ������ ��´�.(���� : ���� HP�� �������� ����)
void CNpc::ChangeTarget(int nAttackType, CUser *pUser)
{
	int preDamage, lastDamage;
	__Vector3 vUser, vNpc;
	float fDistance1 = 0.0f, fDistance2 = 0.0f;
	int iRandom = myrand(0, 100);

	if (pUser == nullptr
		|| pUser->m_bLive == USER_DEAD
		|| pUser->m_bNation == m_byGroup
		|| pUser->m_bInvisibilityType
		|| pUser->m_byIsOP == MANAGER_USER
		|| m_NpcState == NPC_FAINTING
		|| m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT 
		|| m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER 
		|| m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE 
		|| m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT)
		return;

	CUser *preUser = nullptr;
	if(m_Target.id >= 0 && m_Target.id < NPC_BAND)	
		preUser = g_pMain->GetUserPtr(m_Target.id - USER_BAND);

	if(pUser == preUser)	{
		if(m_tNpcGroupType)	 {			// ����Ÿ���̸� �þ߾ȿ� ���� Ÿ�Կ��� ��ǥ ����
			m_Target.failCount = 0;
			if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	FindFriend(1);
			else		FindFriend();
		}
		else	{
			if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{
				m_Target.failCount = 0;
				FindFriend(1);
			}
		}
		return;
	}
	if(preUser != nullptr/* && preUser->m_state == GAME_STATE_INGAME */)
	{
		preDamage = 0; lastDamage = 0;

		if(iRandom >= 0 && iRandom < 50)	{			// ���� �ڽ��� ���� ���ϰ� Ÿ���� ����
			preDamage = preUser->GetDamage(m_sNid+NPC_BAND);
			lastDamage = pUser->GetDamage(m_sNid+NPC_BAND);
			//TRACE("Npc-changeTarget 111 - iRandom=%d, pre=%d, last=%d\n", iRandom, preDamage, lastDamage);
			if(preDamage > lastDamage) return;
		}
		else if(iRandom >= 50 && iRandom < 80)	{		// ���� ����� �÷��̾�
			vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
			vUser.Set(preUser->m_curx, 0, preUser->m_curz);
			fDistance1 = GetDistance(vNpc, vUser);
			vUser.Set(pUser->m_curx, 0, pUser->m_curz);
			fDistance2 = GetDistance(vNpc, vUser);
			//TRACE("Npc-changeTarget 222 - iRandom=%d, preDis=%.2f, lastDis=%.2f\n", iRandom, fDistance1, fDistance2);
			if(fDistance2 > fDistance1)	return;
		}
		if(iRandom >= 80 && iRandom < 95)		{		// ���Ͱ� �������� ���� ���� Ÿ���� �� �� �ִ� ����
			preDamage = GetFinalDamage(preUser, 0);
			lastDamage = GetFinalDamage(pUser, 0); 
			//TRACE("Npc-changeTarget 333 - iRandom=%d, pre=%d, last=%d\n", iRandom, preDamage, lastDamage);
			if(preDamage > lastDamage) return;
		}
		if(iRandom >= 95 && iRandom < 101)		{		// Heal Magic�� ����� ����
		}
	}
	else if(preUser == nullptr && nAttackType == 1004)		return;		// Heal magic�� �������� �ʵ���..
		
	m_Target.id	= pUser->m_iUserId + USER_BAND;
	m_Target.x	= pUser->m_curx;
	m_Target.y	= pUser->m_cury;
	m_Target.z  = pUser->m_curz;

	//TRACE("Npc-changeTarget - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);

	int nValue = 0;
	// ��� �Ÿ��µ� �����ϸ� �ٷ� �ݰ�
	if(m_NpcState == NPC_STANDING || m_NpcState == NPC_MOVING || m_NpcState == NPC_SLEEPING)
	{									// ������ ������ �ݰ����� �̾�����
		if(IsCloseTarget(pUser, m_byAttackRange) == true)
		{
			m_NpcState = NPC_FIGHTING;
			m_Delay = 0;
		}
		else							// �ٷ� �������� ��ǥ�� �����ϰ� ����	
		{
			nValue = GetTargetPath(1);
			if(nValue == 1)	// �ݰ� ������ �ణ�� ������ �ð��� ����	
			{
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
			else if(nValue == -1)
			{
				m_NpcState = NPC_STANDING;
				m_Delay = 0;
			}
			else if(nValue == 0)
			{
				m_fSecForMetor = m_fSpeed_2;	// �����϶��� �ٴ� �ӵ���... 
				IsNoPathFind(m_fSecForMetor);
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
		}
	}
//	else m_NpcState = NPC_ATTACKING;	// ���� �����ϴµ� ���� �����ϸ� ��ǥ�� �ٲ�

	if(m_tNpcGroupType)	 {			// ����Ÿ���̸� �þ߾ȿ� ���� Ÿ�Կ��� ��ǥ ����
		m_Target.failCount = 0;
		if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	FindFriend(1);
		else		FindFriend();
	}
	else	{
		if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{
			m_Target.failCount = 0;
			FindFriend(1);
		}
	}
}

//	���� ������ Npc�� Ÿ������ ��´�.(���� : ���� HP�� �������� ����)
void CNpc::ChangeNTarget(CNpc *pNpc)
{
	int preDamage, lastDamage;
	__Vector3 vMonster, vNpc;
	float fDist = 0.0f;

	if(pNpc == nullptr) return;
	if(pNpc->m_NpcState == NPC_DEAD) return;

	CNpc *preNpc = nullptr;
	if(m_Target.id >= 0 && m_Target.id < NPC_BAND)
	{
	}
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)
	{
		preNpc = g_pMain->m_arNpc.GetData(m_Target.id - NPC_BAND);
	}

	if(pNpc == preNpc) return;

	if(preNpc != nullptr)
	{
		preDamage = 0; lastDamage = 0;
		preDamage = GetNFinalDamage(preNpc);
		lastDamage = GetNFinalDamage(pNpc); 

		// ������ �˻�,,   (�Ÿ��� ������ ���ݷ��� �Ǵ��ؼ�,,)
		vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
		vMonster.Set(preNpc->m_fCurX, 0, preNpc->m_fCurZ);
		fDist = GetDistance(vNpc, vMonster);
		preDamage = (int)((double)preDamage/fDist + 0.5);
		vMonster.Set(pNpc->m_fCurX, 0, pNpc->m_fCurZ);
		fDist = GetDistance(vNpc, vMonster);
		lastDamage = (int)((double)lastDamage/fDist + 0.5);

		if(preDamage > lastDamage) return;
	}
		
	m_Target.id	= pNpc->m_sNid + NPC_BAND;
	m_Target.x	= pNpc->m_fCurX;
	m_Target.y	= pNpc->m_fCurZ;
	m_Target.z  = pNpc->m_fCurZ;

	int nValue = 0;
	// ��� �Ÿ��µ� �����ϸ� �ٷ� �ݰ�
	if(m_NpcState == NPC_STANDING || m_NpcState == NPC_MOVING || m_NpcState == NPC_SLEEPING)
	{									// ������ ������ �ݰ����� �̾�����
		if(IsCloseTarget(m_byAttackRange) == 1)
		{
			m_NpcState = NPC_FIGHTING;
			m_Delay = 0;
		}
		else							// �ٷ� �������� ��ǥ�� �����ϰ� ����	
		{
			nValue = GetTargetPath();
			if(nValue == 1)	// �ݰ� ������ �ణ�� ������ �ð��� ����	
			{
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
			else if(nValue == -1)
			{
				m_NpcState = NPC_STANDING;
				m_Delay = 0;
			}
			else if(nValue == 0)
			{
				m_fSecForMetor = m_fSpeed_2;	// �����϶��� �ٴ� �ӵ���... 
				IsNoPathFind(m_fSecForMetor);
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
		}
	}
//	else m_NpcState = NPC_ATTACKING;	// ���� �����ϴµ� ���� �����ϸ� ��ǥ�� �ٲ�

	if(m_tNpcGroupType)					// ����Ÿ���̸� �þ߾ȿ� ���� Ÿ�Կ��� ��ǥ ����
	{
		m_Target.failCount = 0;
		FindFriend();
	}
}


void CNpc::ToTargetMove(CUser* pUser)
{
	TRACE("### ToTargetMove() ���� ��ã�� ���� ### \n");
}

//	NPC �� ������ ���´�.
int CNpc::GetDefense()
{
	return m_sDefense;
}

//	Damage ���, ���� m_iHP �� 0 �����̸� ���ó��
bool CNpc::SetDamage(int nAttackType, int nDamage, TCHAR *id, int uid)
{
	int i=0, len=0;
	int userDamage = 0;
	bool bFlag = false;
	_ExpUserList *tempUser = nullptr;

	if(m_NpcState == NPC_DEAD) return true;
	if(m_iHP <= 0) return true;
	if(nDamage < 0) return true;

	CUser* pUser = nullptr;
	CNpc* pNpc = nullptr;
	char strDurationID[MAX_ID_SIZE+1];

	if(uid >= USER_BAND && uid < NPC_BAND)	{	// Target �� User �� ���
		pUser = g_pMain->GetUserPtr(uid);	// �ش� ��������� ����
		if(pUser == nullptr) return true;
	}
	else if(uid >= NPC_BAND && m_Target.id < INVALID_BAND)	{	// Target �� mon �� ���
		pNpc = g_pMain->m_arNpc.GetData(uid - NPC_BAND);
		if(pNpc == nullptr) return true;
		userDamage = nDamage;		
		goto go_result;
	}

	userDamage = nDamage;		
													// �׿� �������� �ҿ����.		
	if( (m_iHP - nDamage) < 0 ) userDamage = m_iHP;

	for(i = 0; i < NPC_HAVE_USER_LIST; i++)	{
		if(m_DamagedUserList[i].iUid == uid)	{
			if(_stricmp("**duration**", id) == 0) {
				bFlag = true;
				strcpy_s(strDurationID, sizeof(strDurationID), pUser->m_strUserID);
				if(_stricmp(m_DamagedUserList[i].strUserID, strDurationID) == 0)	{
					m_DamagedUserList[i].nDamage += userDamage; 
					goto go_result;
				}
			}
			else if(_stricmp(m_DamagedUserList[i].strUserID, id) == 0) 
			{ 
				m_DamagedUserList[i].nDamage += userDamage; 
				goto go_result;
			}
		}
	}

	for(i = 0; i < NPC_HAVE_USER_LIST; i++)				// �ο� ������ ���� ������� ������ ��ġ��?
	{
		if(m_DamagedUserList[i].iUid == -1)
		{
			if(m_DamagedUserList[i].nDamage <= 0)
			{
				len = strlen(id);
				if( len > MAX_ID_SIZE || len <= 0 ) {
					TRACE("###  Npc SerDamage Fail ---> uid = %d, name=%s, len=%d, id=%s  ### \n", m_sNid+NPC_BAND, m_proto->m_strName, len, id);
					continue;
				}
				if(bFlag == true)	strcpy_s(m_DamagedUserList[i].strUserID, sizeof(m_DamagedUserList[i].strUserID), strDurationID);
				else	{
					if(_stricmp("**duration**", id) == 0) {
						strcpy(m_DamagedUserList[i].strUserID, pUser->m_strUserID);
					}
					else strcpy(m_DamagedUserList[i].strUserID, id);
				}
				m_DamagedUserList[i].iUid = uid;
				m_DamagedUserList[i].nDamage = userDamage;
				m_DamagedUserList[i].bIs = false;
				break;
			}
		}
	}

go_result:
	m_TotalDamage += userDamage;
	m_iHP -= nDamage;	

	if( m_iHP <= 0 )
	{
	//	m_ItemUserLevel = pUser->m_sLevel;
		m_iHP = 0;
		Dead();
		return false;
	}

	int iRandom = myrand(1, 100);
	int iLightningR = 0;

	if(uid >= USER_BAND && uid < NPC_BAND)	// Target �� User �� ���
	{
		if(nAttackType == 3 && m_NpcState != NPC_FAINTING)	{			// ���� ��Ű�� ��ų�� ����ߴٸ�..
			// Ȯ�� ���..
			iLightningR = (int)(10 + (40 - 40 * ( (double)m_byLightningR / 80)));
			if( COMPARE(iRandom, 0, iLightningR) )	{
				m_NpcState = NPC_FAINTING;
				m_Delay = 0;
				m_tFaintingTime = UNIXTIME;
			}
			else	ChangeTarget(nAttackType, pUser);
		}
		else	{
			ChangeTarget(nAttackType, pUser);
		}
	}
	if(uid >= NPC_BAND && m_Target.id < INVALID_BAND)	// Target �� mon �� ���
	{
		ChangeNTarget(pNpc);
	}

	return true;
}

// Heal�迭 ��������
bool CNpc::SetHMagicDamage(int nDamage)
{
	if (m_NpcState == NPC_DEAD
		|| m_iHP <= 0
		|| nDamage <= 0)
		return false;

	int oldHP = 0;

	oldHP = m_iHP;
	m_iHP += nDamage;
	if( m_iHP < 0 )
		m_iHP = 0;
	else if ( m_iHP > m_iMaxHP )
		m_iHP = m_iMaxHP;

	TRACE("Npc - SetHMagicDamage(), nid=%d,%s, oldHP=%d -> curHP=%d\n", m_sNid+NPC_BAND, m_proto->m_strName, oldHP, m_iHP);

	Packet result(AG_USER_SET_HP);
	result << uint16(m_sNid + NPC_BAND) << m_iHP;
	g_pMain->Send(&result);

	return true;
}

//	NPC ���ó���� ����ġ �й踦 ����Ѵ�.(�Ϲ� ������ ���� ����ڱ���)
void CNpc::SendExpToUserList()
{
	int i=0;
	int nExp = 0;
	int nPartyExp = 0;
	int nLoyalty = 0;
	int nPartyLoyalty = 0;
	double totalDamage = 0;
	double CompDamage = 0;
	double TempValue = 0;
	int nPartyNumber = -1;
	int nUid = -1;
	CUser* pUser = nullptr;
	CUser* pPartyUser = nullptr;
	CUser* pMaxDamageUser = nullptr;
	_PARTY_GROUP* pParty = nullptr;
	char strMaxDamageUser[MAX_ID_SIZE+1];
	MAP* pMap = GetMap();
	if (pMap == nullptr) return;

	IsUserInSight();	// �þ߱ǳ��� �ִ� ���� ����..
				
	for(i = 0; i < NPC_HAVE_USER_LIST; i++)				// �ϴ� ����Ʈ�� �˻��Ѵ�.
	{
		if(m_DamagedUserList[i].iUid < 0 || m_DamagedUserList[i].nDamage<= 0) continue;
		if(m_DamagedUserList[i].bIs == true) pUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
		if(pUser == nullptr) continue;
		
		if(pUser->m_byNowParty == 1)			// ��Ƽ �Ҽ�
		{	
			totalDamage = GetPartyDamage(pUser->m_sPartyNumber);
			if(totalDamage == 0 || m_TotalDamage == 0)
				nPartyExp = 0;
			else	{
				if( CompDamage < totalDamage )	{	// 
					CompDamage = totalDamage;
					m_sMaxDamageUserid = m_DamagedUserList[i].iUid;
					pMaxDamageUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
					if(pMaxDamageUser == nullptr)	{
						m_byMaxDamagedNation = pUser->m_bNation;
						strcpy_s( strMaxDamageUser, sizeof(strMaxDamageUser), pUser->m_strUserID );
					}
					else	{
						m_byMaxDamagedNation = pMaxDamageUser->m_bNation;
						strcpy_s( strMaxDamageUser, sizeof(strMaxDamageUser), pMaxDamageUser->m_strUserID );
					}
				}

				TempValue = m_proto->m_iExp * (totalDamage / m_TotalDamage);
				nPartyExp = (int)TempValue;
				if(TempValue > nPartyExp)	nPartyExp = nPartyExp + 1;
			}
			if(m_proto->m_iLoyalty == 0 || totalDamage == 0 || m_TotalDamage == 0)
				nPartyLoyalty = 0;
			else	{
				TempValue = m_proto->m_iLoyalty * (totalDamage / m_TotalDamage);
				nPartyLoyalty = (int)TempValue;
				if(TempValue > nPartyLoyalty)	nPartyLoyalty = nPartyLoyalty + 1;
			}
			// ��Ƽ�� ��ü�� ���鼭 ����ġ �й�
			if(i != 0)
			{
				bool bFlag = false;
				int count = 0;
				for(int j=0; j<i; j++)
				{
					if(m_DamagedUserList[j].iUid < 0 || m_DamagedUserList[j].nDamage<= 0) continue;
					if(m_DamagedUserList[j].bIs == true) pPartyUser = g_pMain->GetUserPtr(m_DamagedUserList[j].iUid);
					if(pPartyUser == nullptr) continue;
					if(pUser->m_sPartyNumber == pPartyUser->m_sPartyNumber)	continue;
					count++;
				}

				if(count == i)	bFlag = true;

				// ���⿡�� �� �۾�...
				if(bFlag == true)	{
					int uid = 0;
					pParty = g_pMain->m_arParty.GetData( pUser->m_sPartyNumber );
					if( pParty ) {	
						int nTotalMan = 0;
						int nTotalLevel = 0;
						for(int j=0; j<8; j++)	{
							uid = pParty->uid[j];
							pPartyUser = g_pMain->GetUserPtr(uid);
							if(pPartyUser)	{
								nTotalMan++;
								nTotalLevel += pPartyUser->m_bLevel;
							}
						}

						nPartyExp = GetPartyExp( nTotalLevel, nTotalMan, nPartyExp );
						//TRACE("* PartyUser GetPartyExp total_level=%d, total_man = %d, exp=%d *\n", nTotalLevel, nTotalMan, nPartyExp);

						for(int k=0; k<8; k++)	{
							uid = pParty->uid[k];
							pPartyUser = g_pMain->GetUserPtr(uid);
							if(pPartyUser)
							{
								// monster�� �Ÿ��� �Ǵ�
								if( IsInExpRange(pPartyUser) == true)
								{
									TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
									//TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) );
									nExp = (int)TempValue;
									if(TempValue > nExp)	nExp = nExp + 1;
									if(nPartyLoyalty <= 0)
										nLoyalty = 0;
									else	{
										TempValue = ( nPartyLoyalty * ( 1+0.2*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
										nLoyalty = (int)TempValue;
										if(TempValue > nLoyalty)	nLoyalty = nLoyalty + 1;
									}
									//TRACE("* PartyUser Exp id=%s, damage=%d, total=%d, exp=%d, loral=%d, level=%d/%d *\n", pPartyUser->m_strUserID, (int)totalDamage, m_TotalDamage, nExp, nLoyalty, pPartyUser->m_sLevel, nTotalLevel);
									//pPartyUser->SetExp(nExp, nLoyalty, m_proto->m_sLevel);
									pPartyUser->SetPartyExp(nExp, nLoyalty, nTotalLevel, nTotalMan);
								}
							}
						}	
					}
				}
			}
			else if(i==0)
			{
				int uid = 0;
				pParty = g_pMain->m_arParty.GetData( pUser->m_sPartyNumber );
				if( pParty ) {	
					int nTotalMan = 0;
					int nTotalLevel = 0;
					for(int j=0; j<8; j++)	{
						uid = pParty->uid[j];
						pPartyUser = g_pMain->GetUserPtr(uid);
						if(pPartyUser)	{
							nTotalMan++;
							nTotalLevel += pPartyUser->m_bLevel;
						}
					}

					nPartyExp = GetPartyExp( nTotalLevel, nTotalMan, nPartyExp );
					//TRACE("* PartyUser GetPartyExp total_level=%d, total_man = %d, exp=%d *\n", nTotalLevel, nTotalMan, nPartyExp);

					for(int k=0; k<8; k++)	{
						uid = pParty->uid[k];
						pPartyUser = g_pMain->GetUserPtr(uid);
						if(pPartyUser)
						{
							// monster�� �Ÿ��� �Ǵ�
							if( IsInExpRange(pPartyUser) == true)
							{
								TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
								//TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) );
								nExp = (int)TempValue;
								if(TempValue > nExp)	nExp = nExp + 1;
								if(nPartyLoyalty <= 0)
									nLoyalty = 0;
								else	{
									TempValue = ( nPartyLoyalty * ( 1+0.2*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
									nLoyalty = (int)TempValue;
									if(TempValue > nLoyalty)	nLoyalty = nLoyalty + 1;
								}
								//TRACE("* PartyUser Exp id=%s, damage=%d, total=%d, exp=%d, loral=%d, level=%d/%d *\n", pPartyUser->m_strUserID, (int)totalDamage, m_TotalDamage, nExp, nLoyalty, pPartyUser->m_sLevel, nTotalLevel);
								//pPartyUser->SetExp(nExp, nLoyalty, m_proto->m_sLevel);
								pPartyUser->SetPartyExp(nExp, nLoyalty, nTotalLevel, nTotalMan);
							}
						}
					}	
				}
			}
			//nExp = 
		}	
		else if(pUser->m_byNowParty == 2)		// �δ� �Ҽ�
		{	
			
		}	
		else									// ����
		{
			totalDamage = m_DamagedUserList[i].nDamage;
			
			if(totalDamage == 0 || m_TotalDamage == 0)	{
				nExp = 0;
				nLoyalty = 0;
			}
			else	{

				if( CompDamage < totalDamage )	{	// 
					CompDamage = totalDamage;
					m_sMaxDamageUserid = m_DamagedUserList[i].iUid;
					pMaxDamageUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
					if(pMaxDamageUser == nullptr)	{
						m_byMaxDamagedNation = pUser->m_bNation;
						strcpy_s( strMaxDamageUser, sizeof(strMaxDamageUser), pUser->m_strUserID );
					}
					else	{
						m_byMaxDamagedNation = pMaxDamageUser->m_bNation;
						strcpy_s( strMaxDamageUser, sizeof(strMaxDamageUser), pMaxDamageUser->m_strUserID );
					}
				}

				TempValue = m_proto->m_iExp * ( totalDamage / m_TotalDamage );
				nExp = (int)TempValue;
				if(TempValue > nExp)	nExp = nExp + 1;

				if(m_proto->m_iLoyalty == 0) nLoyalty = 0;
				else	{
					TempValue = m_proto->m_iLoyalty * ( totalDamage / m_TotalDamage );
					nLoyalty = (int)TempValue;
					if(TempValue > nLoyalty)	nLoyalty = nLoyalty + 1;
				}

				//TRACE("* User Exp id=%s, damage=%d, total=%d, exp=%d, loral=%d *\n", pUser->m_strUserID, (int)totalDamage, m_TotalDamage, nExp, nLoyalty);
				pUser->SetExp(nExp, nLoyalty, m_proto->m_sLevel);
			}
		}
	}

	if( g_pMain->m_byBattleEvent == BATTLEZONE_OPEN )	{	// ������
		if( m_bySpecialType >= 90 && m_bySpecialType <= 100 )	{					// �׾����� �������� ���� ���� ������ ����� �ּ���
			if( strlen( strMaxDamageUser) != 0 )	{		// ���Ϳ��� ���� �������� ���� ���� ������ �̸��� ����
				Packet result(AG_BATTLE_EVENT, uint8(BATTLE_EVENT_MAX_USER));

				switch (m_bySpecialType)
				{
				case 100:	result << uint8(1); break;
				case 90:	result << uint8(3); break;
				case 91:	result << uint8(4); break;
				case 92:	result << uint8(5);	break;
				case 93:	result << uint8(6); break;
				case 98:	result << uint8(7); break;
				case 99:	result << uint8(8); break;
				}

				if (m_bySpecialType == 90 || m_bySpecialType == 91 || m_bySpecialType == 98)
					g_pMain->m_sKillKarusNpc++;
				else if (m_bySpecialType == 92 || m_bySpecialType == 93 || m_bySpecialType == 99)
					g_pMain->m_sKillElmoNpc++;

				result.SByte();
				result << strMaxDamageUser;
				g_pMain->Send(&result);

				bool	bKarusComplete = (g_pMain->m_sKillKarusNpc == pMap->m_sKarusRoom),
						bElMoradComplete = (g_pMain->m_sKillElmoNpc == pMap->m_sElmoradRoom);
				if (bKarusComplete || bElMoradComplete)
				{
					result.clear();
					result	<< uint8(BATTLE_EVENT_RESULT) 
							<< uint8(bKarusComplete ? KARUS_ZONE : ELMORAD_ZONE)
							<< strMaxDamageUser;
					g_pMain->Send(&result);
				}
			}
		}
	}
}

int CNpc::SendDead(int type)
{
	if(m_NpcState != NPC_DEAD || m_iHP > 0) return 0;

	if(type) GiveNpcHaveItem();	// ������ ������(�����̸� �ȶ���Ʈ��)

	return m_sRegenTime;
}

//	NPC�� Target ���� �Ÿ��� ���� �������� ������ �Ǵ�
bool CNpc::IsCloseTarget(CUser *pUser, int nRange)
{
	if(pUser == nullptr)
	{
		return false;
	}
	if(pUser->m_sHP <= 0 ||/* pUser->m_state != GAME_STATE_INGAME ||*/ pUser->m_bLive == false)
	{
		return false;
	}

	__Vector3 vUser;
	__Vector3 vNpc;
	float fDis = 0.0f;
	vNpc.Set(m_fCurX, m_fCurY, m_fCurZ);
	vUser.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz); 
	fDis = GetDistance(vNpc, vUser);
	
	// ���ݹ��� ���±� ������ 2���� �Ÿ���������,,
	if((int)fDis > nRange * 2) return false; 

	//InitTarget();

	m_Target.id = pUser->m_iUserId + USER_BAND;
	m_Target.x = pUser->m_curx;
	m_Target.y = pUser->m_cury;
	m_Target.z = pUser->m_curz;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// �þ� �������� �����Ḧ ã�´�.
// type = 0: ���� �׷��̸鼭 ���� �йи� Ÿ�Ը� ����, 1:�׷��̳� �йи��� ������� ����, 
//        2:����NPC�� ���� �Ʊ��� ���¸� üũ�ؼ� ġ���� ��������,, �������� ġ��� �Ʊ��� NID�� �����Ѵ�
int CNpc::FindFriend(int type)
{
	CNpc* pNpc = nullptr;
	MAP* pMap = GetMap();
	if (pMap == nullptr) return 0;

	if(m_bySearchRange == 0) return 0;
	if(type != 2)	{
		if(m_Target.id == -1) return 0;
	}

	int min_x = (int)(m_fCurX - m_bySearchRange)/VIEW_DIST;	if(min_x < 0) min_x = 0;
	int min_z = (int)(m_fCurZ - m_bySearchRange)/VIEW_DIST;	if(min_z < 0) min_z = 0;
	int max_x = (int)(m_fCurX + m_bySearchRange)/VIEW_DIST;	if(max_x > pMap->GetXRegionMax()) max_x = pMap->GetXRegionMax();
	int max_z = (int)(m_fCurZ + m_bySearchRange)/VIEW_DIST;	if(min_z > pMap->GetZRegionMax()) min_z = pMap->GetZRegionMax();

	int search_x = max_x - min_x + 1;		
	int search_z = max_z - min_z + 1;	
	
	int i, j, count = 0;
	_TargetHealer arHealer[9];
	for(i=0; i<9; i++)	{
		arHealer[i].sNID = -1;
		arHealer[i].sValue = 0;
	}

	for(i = 0; i < search_x; i++)	{
		for(j = 0; j < search_z; j++)	{
			FindFriendRegion(min_x+i, min_z+j, pMap, &arHealer[count], type);
			//FindFriendRegion(min_x+i, min_z+j, pMap, type);
		}
	}

	int iValue = 0, iMonsterNid = 0;
	for(i=0; i<9; i++)	{
		if(iValue < arHealer[i].sValue)	{
			iValue = arHealer[i].sValue;
			iMonsterNid = arHealer[i].sNID;
		}
	}

	if(iMonsterNid != 0)	{
		m_Target.id = iMonsterNid;

		return iMonsterNid;
	}

	return 0;
}

void CNpc::FindFriendRegion(int x, int z, MAP* pMap, _TargetHealer* pHealer, int type)
//void CNpc::FindFriendRegion(int x, int z, MAP* pMap, int type)
{
	// �ڽ��� region�� �ִ� UserArray�� ���� �˻��Ͽ�,, ����� �Ÿ��� ������ �ִ����� �Ǵ�..
	if(x < 0 || z < 0 || x > pMap->GetXRegionMax() || z > pMap->GetZRegionMax())	{
		TRACE("#### Npc-FindFriendRegion() Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, x, z);
		return;
	}

	int* pNpcIDList = nullptr, total_mon, count = 0;

	pMap->m_lock.Acquire();
	CRegion *pRegion = &pMap->m_ppRegion[x][z];
	total_mon = pRegion->m_RegionNpcArray.GetSize();
	pNpcIDList = new int[total_mon];

	foreach_stlmap (itr, pRegion->m_RegionNpcArray)
		pNpcIDList[count++] = *itr->second;
	pMap->m_lock.Release();

	CNpc* pNpc = nullptr;
	__Vector3 vStart, vEnd;
	float fDis = 0.0f;
	// ���� ���� �����̱⶧����.. searchrange�� 2���..
	float fSearchRange = 0.0f;
	if( type == 2)	fSearchRange = (float)m_byAttackRange;
	else fSearchRange = (float)m_byTracingRange;
	vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
	int iValue = 0, iCompValue = 0, iHP = 0;

	for(int i=0 ; i<total_mon; i++ ) {
		int nid = pNpcIDList[i];
		if( nid < NPC_BAND )	continue;
		pNpc = (CNpc*)g_pMain->m_arNpc.GetData(nid - NPC_BAND);

		if( pNpc != nullptr && pNpc->m_NpcState != NPC_DEAD && pNpc->m_sNid != m_sNid)	{
			vEnd.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 
			fDis = GetDistance(vStart, vEnd);

			// ���⿡�� ���� ���ݰŸ��� �ִ� ���������� �Ǵ�
			if(fDis <= fSearchRange)	{
				if(type == 1)	{
					if(m_sNid != pNpc->m_sNid)	{
						if(pNpc->m_Target.id > -1 && pNpc->m_NpcState == NPC_FIGHTING) continue;
						pNpc->m_Target.id = m_Target.id;		// ��� ���ῡ�� ������ ��û�Ѵ�.
						pNpc->m_Target.x = m_Target.x;			// ���� ��ǥ�� �������ڰ�...
						pNpc->m_Target.y = m_Target.y;
						pNpc->m_Target.z = m_Target.z;
						pNpc->m_Target.failCount = 0;
						pNpc->NpcStrategy(NPC_ATTACK_SHOUT);
					}
				}
				else if(type == 0)	{
					if(pNpc->m_tNpcGroupType && m_sNid != pNpc->m_sNid && pNpc->m_proto->m_byFamilyType == m_proto->m_byFamilyType)	{
						if(pNpc->m_Target.id > -1 && pNpc->m_NpcState == NPC_FIGHTING) continue;
						pNpc->m_Target.id = m_Target.id;		// ���� Ÿ���� ���ῡ�� ������ ��û�Ѵ�.
						pNpc->m_Target.x = m_Target.x;			// ���� ��ǥ�� �������ڰ�...
						pNpc->m_Target.y = m_Target.y;
						pNpc->m_Target.z = m_Target.z;
						pNpc->m_Target.failCount = 0;
						pNpc->NpcStrategy(NPC_ATTACK_SHOUT);
					}
				}
				else if(type == 2)	{
					if(pHealer == nullptr) continue;
					// HP���¸� üũ
					iHP = (int)(pNpc->m_iMaxHP * 0.9);
					if(pNpc->m_iHP <= iHP)	{		// HP üũ
						iCompValue = (int)((pNpc->m_iMaxHP - pNpc->m_iHP) / (pNpc->m_iMaxHP * 0.01));
						if(iValue < iCompValue)		{
							iValue = iCompValue;
							pHealer->sNID = pNpc->m_sNid+NPC_BAND;
							pHealer->sValue = iValue;
						}
					}
				}
			}	
			else continue;
		}
	}

	if(pNpcIDList)	{
		delete [] pNpcIDList;
		pNpcIDList = nullptr;
	}
}

void CNpc::NpcStrategy(uint8 type)
{
	switch(type)
	{
	case NPC_ATTACK_SHOUT:
		m_NpcState = NPC_TRACING;
		m_Delay = m_sSpeed;//STEP_DELAY;
		m_fDelayTime = getMSTime();
		break;
	}
}

void CNpc::FillNpcInfo(Packet & result)
{
	if (m_bySpecialType == 5 && m_byChangeType == 0)	
		result << uint8(0);
	else
		result << uint8(1);

	result	<< uint16(m_sNid + NPC_BAND) << m_proto->m_sSid << m_proto->m_sPid
			<< m_sSize << m_iWeapon_1 << m_iWeapon_2
			<< m_bCurZone << m_proto->m_strName
			<< m_byGroup << uint8(m_proto->m_sLevel)
			<< m_fCurX << m_fCurZ << m_fCurY << m_byDirection
			<< bool(m_iHP > 0) // are we alive?
			<< m_proto->m_tNpcType
			<< m_iSellingGroup << m_iMaxHP << m_iHP
			<< m_byGateOpen 
			<< float(m_sHitRate) << float(m_sEvadeRate) << m_sDefense
			<< m_byObjectType << m_byTrapNumber;
}

int CNpc::GetDir(float x1, float z1, float x2, float z2)
{
	int nDir;					//	3 4 5
								//	2 8 6
								//	1 0 7

	int nDirCount = 0;

	int x11 = (int)x1 / TILE_SIZE;
	int y11 = (int)z1 / TILE_SIZE; 
	int x22 = (int)x2 / TILE_SIZE;
	int y22 = (int)z2 / TILE_SIZE; 

	int deltax = x22 - x11;
	int deltay = y22 - y11;

	int fx = ((int)x1/TILE_SIZE) * TILE_SIZE;
	int fy = ((int)z1/TILE_SIZE) * TILE_SIZE;

	float add_x = x1 - fx;
	float add_y = z1 - fy;

	if (deltay==0) {
		if (x22>x11) nDir = DIR_RIGHT;		
		else nDir = DIR_LEFT;	
		goto result_value;
	}
	if (deltax==0) 
	{
		if (y22>y11) nDir = DIR_DOWN;	
		else nDir = DIR_UP;		
		goto result_value;
	}
	else 
	{
		if (y22>y11) 
		{
			if (x22>x11) 
			{
				nDir = DIR_DOWNRIGHT;		// -> 
			}
			else 
			{
				nDir = DIR_DOWNLEFT;		// -> 
			}
		}
		else
		{
			if (x22 > x11) 
			{
				nDir = DIR_UPRIGHT;		
			}
			else 
			{
				nDir = DIR_UPLEFT;
			}
		}
	}

result_value:

	switch(nDir)
	{
	case DIR_DOWN:
		m_fAdd_x = add_x;
		m_fAdd_z = 3;
		break;
	case DIR_DOWNLEFT:
		m_fAdd_x = 1;
		m_fAdd_z = 3;
		break;
	case DIR_LEFT:
		m_fAdd_x = 1;
		m_fAdd_z = add_y;
		break;
	case DIR_UPLEFT:
		m_fAdd_x = 1;
		m_fAdd_z = 1;
		break;
	case DIR_UP:
		m_fAdd_x = add_x;
		m_fAdd_z = 1;
		break;
	case DIR_UPRIGHT:
		m_fAdd_x = 3;
		m_fAdd_z = 1;
		break;
	case DIR_RIGHT:
		m_fAdd_x = 3;
		m_fAdd_z = add_y;
		break;
	case DIR_DOWNRIGHT:
		m_fAdd_x = 3;
		m_fAdd_z = 3;
		break;
	}

	return nDir;
}

float CNpc::RandomGenf(float max, float min)
{
	if ( max == min ) return max;
	if ( min > max ) { float b = min; min = max; max = b; }
	int k = rand()%(int)((max*100-min*100));	

	return (float)((float)(k*0.01f)+min);
}

void CNpc::NpcMoveEnd()
{
	SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND);

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result	<< uint16(m_sNid + NPC_BAND)
			<< m_fCurX << m_fCurZ << m_fCurY << float(0.0f);

	g_pMain->Send(&result);
}

void CNpc::GetVectorPosition(__Vector3 & vOrig, __Vector3 & vDest, float fDis, __Vector3 * vResult)
{
	*vResult = vDest - vOrig;
	vResult->Magnitude();
	vResult->Normalize();
	*vResult *= fDis;
	*vResult += vOrig;
}

float CNpc::GetDistance(__Vector3 & vOrig, __Vector3 & vDest)
{
	return (vOrig - vDest).Magnitude();
}

bool CNpc::GetUserInView()
{
	MAP* pMap = GetMap();
	if (pMap == nullptr)	return false;
	//if( m_ZoneIndex > 5 || m_ZoneIndex < 0) return false;		// �ӽ��ڵ� ( 2002.03.24 )
	int min_x = (int)(m_fCurX - NPC_VIEW_RANGE)/VIEW_DIST;	if(min_x < 0) min_x = 0;
	int min_z = (int)(m_fCurZ - NPC_VIEW_RANGE)/VIEW_DIST;	if(min_z < 0) min_z = 0;
	int max_x = (int)(m_fCurX + NPC_VIEW_RANGE)/VIEW_DIST;	if(max_x > pMap->GetXRegionMax()) max_x = pMap->GetXRegionMax();
	int max_z = (int)(m_fCurZ + NPC_VIEW_RANGE)/VIEW_DIST;	if(max_z > pMap->GetZRegionMax()) max_z = pMap->GetZRegionMax();

	int search_x = max_x - min_x + 1;		
	int search_z = max_z - min_z + 1;	
	
	bool bFlag = false;
	int i, j;

	for(i = 0; i < search_x; i++)	{
		for(j = 0; j < search_z; j++)	{
			bFlag = GetUserInViewRange(min_x+i, min_z+j);
			if(bFlag == true)	return true;
		}
	}

	return false;
}

bool CNpc::GetUserInViewRange(int x, int z)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr || x < 0 || z < 0 || x > pMap->GetXRegionMax() || z > pMap->GetZRegionMax())	{
		TRACE("#### Npc-GetUserInViewRange() Fail : [nid=%d, sid=%d], x1=%d, z1=%d #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, x, z);
		return false;
	}

	FastGuard lock(pMap->m_lock);
	__Vector3 vStart, vEnd;
	vStart.Set(m_fCurX, 0, m_fCurZ);
	float fDis = 0.0f; 

	foreach_stlmap (itr, pMap->m_ppRegion[x][z].m_RegionUserArray)
	{
		CUser *pUser = g_pMain->GetUserPtr(*itr->second);
		if (pUser == nullptr)
			continue;

		vEnd.Set(pUser->m_curx, 0, pUser->m_curz);
		fDis = GetDistance(vStart, vEnd);
		if(fDis <= NPC_VIEW_RANGE)
			return true;		
	}
	
	return false;
}

void CNpc::SendAttackSuccess(uint8 byResult, int tuid, short sDamage, int nHP, uint8 byFlag, short sAttack_type)
{
	uint16 sid, tid;
	uint8 type;

	if (byFlag == 0)
	{
		type = 2;
		sid = m_sNid+NPC_BAND;
		tid = tuid;
	}
	else	
	{
		type = 1;
		sid = tuid;
		tid = m_sNid+NPC_BAND;
	}

	Packet result(AG_ATTACK_RESULT, type);
	result << byResult << sid << tid << sDamage << nHP << uint8(sAttack_type);
	g_pMain->Send(&result);
}

void CNpc::CalcAdaptivePosition(__Vector3 & vPosOrig, __Vector3 & vPosDest, float fAttackDistance, __Vector3 * vResult)
{
	*vResult = vPosOrig - vPosDest;	
	vResult->Normalize();	
	*vResult *= fAttackDistance;
	*vResult += vPosDest;
}

//	���� ���� �������� �� ȭ�� �����ȿ� �ִ��� �Ǵ�
void CNpc::IsUserInSight()
{
	CUser* pUser = nullptr;
	// Npc�� User���� �Ÿ��� 50���� �ȿ� �ִ� ������Ը�,, ����ġ�� �ش�..
	int iSearchRange = NPC_EXP_RANGE;		

	int i,j;
	__Vector3 vStart, vEnd;
	float fDis = 0.0f;

	vStart.Set(m_fCurX, m_fCurY, m_fCurZ);

	for(j = 0; j < NPC_HAVE_USER_LIST; j++)
	{
		m_DamagedUserList[j].bIs = false;
	}

	for(i = 0; i < NPC_HAVE_USER_LIST; i++)
	{
		pUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
		if(pUser == nullptr)	continue;

		vEnd.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz);
		fDis = GetDistance(vStart, vEnd);

		if((int)fDis <= iSearchRange)
		{
			// �����ִ� ����Ʈ���� ������ ���ٸ�
			if(m_DamagedUserList[i].iUid == pUser->m_iUserId)
			{
				// ���� ID�� ���ؼ� �����ϸ�	
				if(_stricmp(m_DamagedUserList[i].strUserID, pUser->m_strUserID) == 0) 
				{ 
					// �̶����� �����Ѵٴ� ǥ�ø� �Ѵ�
					m_DamagedUserList[i].bIs = true;
				}
			}
		}
	}
}

uint8 CNpc::GetHitRate(float rate)
{
	uint8 result;
	int random = 0;
	random = myrand(1, 10000);

	if( rate >= 5.0 )
	{
		if( random >= 1 && random <= 3500)
			result = GREAT_SUCCESS;
		else if( random >= 3501 && random <= 7500)
			result = SUCCESS;
		else if( random >= 7501 && random <= 9800)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 5.0 && rate >= 3.0)
	{
		if( random >= 1 && random <= 2500)
			result = GREAT_SUCCESS;
		else if( random >= 2501 && random <= 6000)
			result = SUCCESS;
		else if( random >= 6001 && random <= 9600)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 3.0 && rate >= 2.0)
	{
		if( random >= 1 && random <= 2000)
			result = GREAT_SUCCESS;
		else if( random >= 2001 && random <= 5000)
			result = SUCCESS;
		else if( random >= 5001 && random <= 9400)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 2.0 && rate >= 1.25)
	{
		if( random >= 1 && random <= 1500)
			result = GREAT_SUCCESS;
		else if( random >= 1501 && random <= 4000)
			result = SUCCESS;
		else if( random >= 4001 && random <= 9200)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 1.25 && rate >= 0.8)
	{
		if( random >= 1 && random <= 1000)
			result = GREAT_SUCCESS;
		else if( random >= 1001 && random <= 3000)
			result = SUCCESS;
		else if( random >= 3001 && random <= 9000)
			result = NORMAL;
		else
			result = FAIL;
	}	
	else if ( rate < 0.8 && rate >= 0.5)
	{
		if( random >= 1 && random <= 800)
			result = GREAT_SUCCESS;
		else if( random >= 801 && random <= 2500)
			result = SUCCESS;
		else if( random >= 2501 && random <= 8000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 0.5 && rate >= 0.33)
	{
		if( random >= 1 && random <= 600)
			result = GREAT_SUCCESS;
		else if( random >= 601 && random <= 2000)
			result = SUCCESS;
		else if( random >= 2001 && random <= 7000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 0.33 && rate >= 0.2)
	{
		if( random >= 1 && random <= 400)
			result = GREAT_SUCCESS;
		else if( random >= 401 && random <= 1500)
			result = SUCCESS;
		else if( random >= 1501 && random <= 6000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else
	{
		if( random >= 1 && random <= 200)
			result = GREAT_SUCCESS;
		else if( random >= 201 && random <= 1000)
			result = SUCCESS;
		else if( random >= 1001 && random <= 5000)
			result = NORMAL;
		else
			result = FAIL;
	}
	
	return result;	
}

bool CNpc::IsLevelCheck(int iLevel)
{
	// ������ �������� ������,,,  �ٷ� ����
	if(iLevel <= m_proto->m_sLevel)
		return false;

	int compLevel = 0;

	compLevel = iLevel - m_proto->m_sLevel;

	// ������ ���ؼ� 8�̸��̸� �ٷ� ����
	if(compLevel < 8)	
		return false;

	return true;
}

bool CNpc::IsHPCheck(int iHP)
{
	return (m_iHP < (m_iMaxHP*0.2));
}

// �н� ���ε带 �Ұ������� üũ�ϴ� ��ƾ..
bool CNpc::IsPathFindCheck(float fDistance)
{
	int nX = 0, nZ = 0;
	__Vector3 vStart, vEnd, vDis, vOldDis;
	float fDis = 0.0f;
	vStart.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	vEnd.Set(m_fEndPoint_X, 0, m_fEndPoint_Y);
	vDis.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	int count = 0;
	int nError = 0;

	MAP* pMap = GetMap();

	nX = (int)(vStart.x / TILE_SIZE);
	nZ = (int)(vStart.z / TILE_SIZE);
	if(pMap->IsMovable(nX, nZ) == true)
	{
		nError = -1;
		return false;
	}
	nX = (int)(vEnd.x / TILE_SIZE);
	nZ = (int)(vEnd.z / TILE_SIZE);
	if(pMap->IsMovable(nX, nZ) == true)
	{
		nError = -1;
		return false;
	}

	do
	{
		vOldDis.Set(vDis.x, 0, vDis.z);
		GetVectorPosition(vDis, vEnd, fDistance, &vDis);
		fDis = GetDistance(vOldDis, vEnd);

		if(fDis > NPC_MAX_MOVE_RANGE)
		{
			nError = -1;
			break;
		}
		
		nX = (int)(vDis.x / TILE_SIZE);
		nZ = (int)(vDis.z / TILE_SIZE);

		if(pMap->IsMovable(nX, nZ) == true
			|| count >= MAX_PATH_LINE)
		{
			nError = -1;
			break;
		}

		m_pPoint[count].fXPos = vEnd.x;
		m_pPoint[count++].fZPos = vEnd.z;

	} while (fDis <= fDistance);

	m_iAniFrameIndex = count;

	if(nError == -1)
		return false;

	return true;
}

// �н� ���ε带 ���� �ʰ� ���ݴ������ ���� ��ƾ..
void CNpc::IsNoPathFind(float fDistance)
{
	ClearPathFindData();
	m_bPathFlag = true;

	int nX = 0, nZ = 0;
	__Vector3 vStart, vEnd, vDis, vOldDis;
	float fDis = 0.0f;
	vStart.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	vEnd.Set(m_fEndPoint_X, 0, m_fEndPoint_Y);
	vDis.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	int count = 0;
	int nError = 0;

	fDis = GetDistance(vStart, vEnd);	
	if(fDis > NPC_MAX_MOVE_RANGE)	{						// 100���� ���� ������ ���ĵ����·�..
		ClearPathFindData();
		TRACE("#### Npc-IsNoPathFind Fail : NPC_MAX_MOVE_RANGE overflow  .. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f, fDis=%.2f#####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_fStartPoint_X, m_fStartPoint_Y, m_fEndPoint_X, m_fEndPoint_Y, fDis);
		return;
	}

	if (GetMap() == nullptr)
	{
		ClearPathFindData();
		TRACE("#### Npc-IsNoPathFind No map : [nid=%d, name=%s], zone=%d #####\n", m_sNid+NPC_BAND, m_proto->m_strName, m_bCurZone);
		return;
	}
	MAP* pMap = GetMap();

	do
	{
		vOldDis.Set(vDis.x, 0, vDis.z);
		GetVectorPosition(vDis, vEnd, fDistance, &vDis);
		fDis = GetDistance(vOldDis, vEnd);
		
		nX = (int)(vDis.x / TILE_SIZE);
		nZ = (int)(vDis.z / TILE_SIZE);
		if(count < 0 || count >= MAX_PATH_LINE)	{	
			ClearPathFindData();
			TRACE("#### Npc-IsNoPathFind index overflow Fail 1 :  count=%d ####\n", count);
			return;
		}	

		m_pPoint[count].fXPos = vEnd.x;
		m_pPoint[count++].fZPos = vEnd.z;
	} while (fDis <= fDistance);

	if(count <= 0 || count >= MAX_PATH_LINE)	{	
		ClearPathFindData();
		TRACE("#### IsNoPtahfind Fail : nid=%d,%s, count=%d ####\n", m_sNid+NPC_BAND, m_proto->m_strName, count);
		return;
	}
	m_iAniFrameIndex = count;

}

//	NPC �� ���� �������� ������.
void CNpc::GiveNpcHaveItem()
{
	int temp = 0, iPer = 0, iMakeItemCode = 0, iMoney = 0, iRandom, nCount = 1, i =0;

/*	if( m_byMoneyType == 1 )	{
		SetByte(pBuf, AG_NPC_EVENT_ITEM, index);
		SetShort(pBuf, m_sMaxDamageUserid, index);	
		SetShort(pBuf, m_sNid+NPC_BAND, index);
		SetDWORD(pBuf, TYPE_MONEY_SID, index);
		SetDWORD(pBuf, m_iMoney, index);
		return;
	}	*/

	iRandom = myrand(70, 100);
	iMoney = m_iMoney * iRandom / 100;
	//m_iMoney, m_iItem;
	_NpcGiveItem m_GiveItemList[NPC_HAVE_ITEM_LIST];			// Npc�� ItemList
	if( iMoney <= 0 )	{
		nCount = 0;
	}
	else	{
		m_GiveItemList[0].sSid = TYPE_MONEY_SID;
		if( iMoney >= SHRT_MAX ) {
			iMoney = 32000;	
			m_GiveItemList[0].count = iMoney;
		}
		else	m_GiveItemList[0].count = iMoney;
	}
	
	_K_MONSTER_ITEM * pItem = g_pMain->m_NpcItemArray.GetData(m_iItem);
	if (pItem != nullptr)
	{
		// j = iItem
		for (int j = 0; j < 5; j++)
		{
			if (pItem->iItem[j] == 0
				|| pItem->sPercent[j] == 0)
				continue;

			iRandom = myrand(1, 10000);
			iPer = pItem->sPercent[j];
			if (iRandom > iPer)
				continue;

			if (j < 2)
			{
				if (pItem->iItem[j] < 100)
				{
					iMakeItemCode = ItemProdution(pItem->iItem[j]);
				}
				else 
				{
					_MAKE_ITEM_GROUP * pGroup = g_pMain->m_MakeItemGroupArray.GetData(pItem->iItem[j]);
					if (pGroup == nullptr
						|| pGroup->iItems.size() != 30)
						continue;

					iMakeItemCode = pGroup->iItems[myrand(1, 30) - 1];
				}

				if (iMakeItemCode == 0) 
					continue;

				m_GiveItemList[nCount].sSid = iMakeItemCode;
				m_GiveItemList[nCount].count = 1;
			}
			else	
			{
				m_GiveItemList[nCount].sSid = pItem->iItem[j];
				if (COMPARE(m_GiveItemList[nCount].sSid, ARROW_MIN, ARROW_MAX))
					m_GiveItemList[nCount].count = 20;
				else	
					m_GiveItemList[nCount].count = 1;
			}
			nCount++;
		}
	}

	if( m_sMaxDamageUserid < 0 || m_sMaxDamageUserid > MAX_USER )	{
		//TRACE("### Npc-GiveNpcHaveItem() User Array Fail : [nid - %d,%s], userid=%d ###\n", m_sNid+NPC_BAND, m_proto->m_strName, m_sMaxDamageUserid);
		return;
	}

	Packet result(AG_NPC_GIVE_ITEM);
	result	<< m_sMaxDamageUserid << uint16(m_sNid + NPC_BAND)
			<< m_bCurZone << m_iRegion_X << m_iRegion_Z
			<< m_fCurX << m_fCurZ << m_fCurY
			<< uint8(nCount);

	for (i = 0; i < nCount; i++)
		result << m_GiveItemList[i].sSid << m_GiveItemList[i].count;

	g_pMain->Send(&result);
}


void CNpc::Yaw2D(float fDirX, float fDirZ, float& fYawResult)
{
	if ( fDirX >= 0.0f ) 
	{ 
		if ( fDirZ >= 0.0f ) 
			fYawResult = (float)(asin(fDirX)); 
		else 
			fYawResult = D3DXToRadian(90.0f) + (float)(acos(fDirX)); 
	}
	else 
	{ 
		if ( fDirZ >= 0.0f ) 
			fYawResult = D3DXToRadian(270.0f) + (float)(acos(-fDirX)); 
		else 
			fYawResult = D3DXToRadian(180.0f) + (float)(asin(-fDirX)); 
	}
}
  
void CNpc::ComputeDestPos(__Vector3 & vCur, float fDegree, float fDegreeOffset, float fDistance, __Vector3 * vResult)
{
	__Matrix44 mtxRot; 
	vResult->Zero();
	mtxRot.RotationY(D3DXToRadian(fDegree+fDegreeOffset));
	*vResult *= mtxRot;
	*vResult *= fDistance;
	*vResult += vCur;
}

int	CNpc::GetPartyDamage(int iNumber)
{
	int i=0;
	int nDamage = 0;
	CUser* pUser = nullptr;
	for(i = 0; i < NPC_HAVE_USER_LIST; i++)				// �ϴ� ����Ʈ�� �˻��Ѵ�.
	{
		if(m_DamagedUserList[i].iUid < 0 || m_DamagedUserList[i].nDamage<= 0) continue;
		if(m_DamagedUserList[i].bIs == true) pUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
		if(pUser == nullptr) continue;
		
		if(pUser->m_sPartyNumber != iNumber)	continue;

		nDamage += m_DamagedUserList[i].nDamage;
	}

	return nDamage;
}

void CNpc::NpcTypeParser()
{
	// �������� �İ������� �����Ѵ�
	switch(m_byActType)
	{
	case 1:
		m_tNpcAttType = m_tNpcOldAttType = 0;
		break;
	case 2:
		m_tNpcAttType = m_tNpcOldAttType = 0;
		m_byNpcEndAttType = 0;			
		break;
	case 3:
		m_tNpcGroupType = 1;
		m_tNpcAttType = m_tNpcOldAttType = 0;
		break;
	case 4:
		m_tNpcGroupType = 1;
		m_tNpcAttType = m_tNpcOldAttType = 0;
		m_byNpcEndAttType = 0;			
		break;
	case 6:
		m_byNpcEndAttType = 0;			
		break;
	case 5:
	case 7:
		m_tNpcAttType = m_tNpcOldAttType = 1;
		break;
	default :
		m_tNpcAttType = m_tNpcOldAttType = 1;
	}
}

void CNpc::HpChange()
{
	m_fHPChangeTime = getMSTime();

	//if(m_NpcState == NPC_FIGHTING || m_NpcState == NPC_DEAD)	return;
	if(m_NpcState == NPC_DEAD)	return;
	if( m_iHP < 1 )	return;	// �ױ������϶��� ȸ�� �ȵ�...
	if( m_iHP == m_iMaxHP)  return;	// HP�� �����̱� ������.. 
	
	//int amount =  (int)(m_sLevel*(1+m_sLevel/60.0) + 1) ;
	int amount =  (int)(m_iMaxHP / 20) ;

	m_iHP += amount;
	if( m_iHP < 0 )
		m_iHP = 0;
	else if ( m_iHP > m_iMaxHP )
		m_iHP = m_iMaxHP;

	Packet result(AG_USER_SET_HP);
	result << uint16(m_sNid + NPC_BAND) << m_iHP << m_iMaxHP;
	g_pMain->Send(&result);
}

bool CNpc::IsInExpRange(CUser* pUser)
{
	// Npc�� User���� �Ÿ��� 50���� �ȿ� �ִ� ������Ը�,, ����ġ�� �ش�..
	int iSearchRange = NPC_EXP_RANGE;		
	__Vector3 vStart, vEnd;
	float fDis = 0.0f;

	vStart.Set(m_fCurX, m_fCurY, m_fCurZ);
	vEnd.Set(pUser->m_curx, pUser->m_cury, pUser->m_curz);
	fDis = GetDistance(vStart, vEnd);
	if((int)fDis <= iSearchRange)
	{
		if(m_bCurZone == pUser->m_curZone)
			return true;
	}

	return false;
}

bool CNpc::CheckFindEnermy()
{
	// ����� ���͵� �����ϹǷ� ����
	if(m_proto->m_tNpcType == NPC_GUARD || m_proto->m_tNpcType == NPC_PATROL_GUARD || m_proto->m_tNpcType == NPC_STORE_GUARD ) // || m_proto->m_tNpcType == NPCTYPE_MONSTER)
		return true;

	MAP* pMap = GetMap();

	if (pMap == nullptr
		|| m_iRegion_X > pMap->GetXRegionMax() || m_iRegion_Z > pMap->GetZRegionMax() || m_iRegion_X < 0 || m_iRegion_Z < 0)
	{
		//TRACE("#### CheckFindEnermy Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", m_sNid+NPC_BAND, m_proto->m_sSid, m_iRegion_X, m_iRegion_Z);
		return false;
	}

	if (pMap->m_ppRegion[m_iRegion_X][m_iRegion_Z].m_byMoving == 1)
		return true;

	return false;
}

void CNpc::MSpChange(int type, int amount)
{
	if( type == 2 ) {
		m_sMP += amount;
		if( m_sMP < 0 )
			m_sMP = 0;
		else if ( m_sMP > m_sMaxMP )
			m_sMP = m_sMaxMP;
	}
	else if( type == 3 ) {	// monster�� SP�� ����..
	}
}

void CNpc::ItemWoreOut( int type, int damage )
{
	// ������ �� ������ ���ҷ���..
}

int	CNpc::ItemProdution(int item_number)							// ������ ����
{
	int iItemNumber = 0, iRandom = 0, i=0, iItemGrade = 0, iItemLevel = 0;
	int iDefault = 0, iItemCode=0, iItemKey=0, iRand2=0, iRand3=0, iRand4=0, iRand5=0;
	int iTemp1 = 0, iTemp2 = 0, iTemp3 = 0;

	iRandom = myrand(1, 10000);
	//iRandom = myrand(1, 4000);

	//TRACE("ItemProdution : nid=%d, sid=%d, name=%s, item_number = %d\n", m_sNid+NPC_BAND, m_proto->m_sSid, m_proto->m_strName, item_number);
	iItemGrade = GetItemGrade(item_number);
	//TRACE("ItemProdution : GetItemGrade() = %d\n", iItemGrade);
	if(iItemGrade == 0)		return 0;
	iItemLevel = m_proto->m_sLevel / 5;

	if( COMPARE( iRandom, 1, 4001) )	{			// ���ⱸ ������
		iDefault = 100000000;
		iRandom = myrand( 1, 10000 );				// ������ ������ ����(�ܰ�, ��, ����,,,,)
		if( COMPARE ( iRandom, 1, 701 ) )			iRand2 = 10000000;
		else if( COMPARE ( iRandom, 701, 1401 ) )	iRand2 = 20000000;
		else if( COMPARE ( iRandom, 1401, 2101 ) )	iRand2 = 30000000;
		else if( COMPARE ( iRandom, 2101, 2801 ) )	iRand2 = 40000000;
		else if( COMPARE ( iRandom, 2801, 3501 ) )	iRand2 = 50000000;
		else if( COMPARE ( iRandom, 3501, 5501 ) )	iRand2 = 60000000;
		else if( COMPARE ( iRandom, 5501, 6501 ) )	iRand2 = 70000000;
		else if( COMPARE ( iRandom, 6501, 8501 ) )	iRand2 = 80000000;
		else if( COMPARE ( iRandom, 8501, 10001 ) )	iRand2 = 90000000;

		iTemp1 = GetWeaponItemCodeNumber( 1 );
		//TRACE("ItemProdution : GetWeaponItemCodeNumber() = %d, iRand2=%d\n", iTemp1, iRand2);
		if( iTemp1 == 0 )	return 0;
		iItemCode = iTemp1 * 100000;	// ���ú���ǥ ����

		iRand3 = myrand(1, 10000);					// ����(����, ī�罺)
		if( COMPARE( iRand3, 1, 5000) )	iRand3 = 10000;
		else	iRand3 = 50000;
		iRand4 = myrand(1, 10000);					// �Ѽ�, ��չ��������� ����
		if( COMPARE( iRand4, 1, 5000) )	iRand4 = 0;
		else	iRand4 = 5000000;
		
		iRandom = GetItemCodeNumber(iItemLevel, 1);	// ���̸���ǥ ����
		//TRACE("ItemProdution : GetItemCodeNumber() = %d, iRand2=%d, iRand3=%d, iRand4=%d\n", iRandom, iRand2, iRand3, iRand4);
		if(iRandom == -1)	{						// �߸��� ������ ��������
			return 0;
		}
		iRand5 = iRandom * 10;
		iItemNumber = iDefault + iItemCode + iRand2 + iRand3 + iRand4 + iRand5 + iItemGrade;

		//TRACE("ItemProdution : Weapon Success item_number = %d, default=%d, itemcode=%d, iRand2=%d, iRand3=%d, iRand4=%d, iRand5, iItemGrade=%d\n", iItemNumber, iDefault, iItemCode, iRand2, iRand3, iRand4, iRand5, iItemGrade);
	}
	else if( COMPARE( iRandom, 4001, 8001) )	{		// �� ������
		iDefault = 200000000;			
		
		iTemp1 = GetWeaponItemCodeNumber( 2 );
		//TRACE("ItemProdution : GetWeaponItemCodeNumber() = %d\n", iTemp1 );
		if( iTemp1 == 0 )	return 0;
		iItemCode = iTemp1 * 1000000;		// ���ú���ǥ ����

		if( m_byMaxDamagedNation == KARUS )	{		// ����
			iRandom = myrand(0, 10000);					// ������ ������ ����		
			if( COMPARE( iRandom, 0, 2000) )	{		
				iRand2 = 0;	
				iRand3 = 10000;							// ���簩���� ��ũ���Ʒ��� ��������
			}
			else if( COMPARE( iRandom, 2000, 4000) )	{
				iRand2 = 40000000;
				iRand3 = 20000;							// �αװ����� ���Ʒ��� ��������
			}
			else if( COMPARE( iRandom, 4000, 6000) )	{
				iRand2 = 60000000;
				iRand3 = 30000;							// �����簩���� ��Ŭ ���Ʒ��� ��������
			}
			else if( COMPARE( iRandom, 6000, 10001) )	{
				iRand2 = 80000000;
				iRandom = myrand(0, 10000);
				if( COMPARE( iRandom, 0, 5000) )	iRand3 = 20000;	// ���������� ���Ʒ�
				else								iRand3 = 40000;	// ���������� ǻ�����Ʒ�
			}
		}
		else if( m_byMaxDamagedNation == ELMORAD )	{
			iRandom = myrand(0, 10000);					// ������ ������ ����		
			if( COMPARE( iRandom, 0, 3300) )	{		
				iRand2 = 0;	
				iItemKey = myrand(0, 10000);			// ���簩���� ��� ������ ����
				if( COMPARE( iItemKey, 0, 3333) )			iRand3 = 110000;
				else if( COMPARE( iItemKey, 3333, 6666) )	iRand3 = 120000;
				else if( COMPARE( iItemKey, 6666, 10001) )	iRand3 = 130000;
			}
			else if( COMPARE( iRandom, 3300, 5600) )	{
				iRand2 = 40000000;
				iItemKey = myrand(0, 10000);			// �αװ����� ���ڿ� ���ڸ� ����
				if( COMPARE( iItemKey, 0, 5000) )	iRand3 = 120000;
				else								iRand3 = 130000;
			}
			else if( COMPARE( iRandom, 5600, 7800) )	{
				iRand2 = 60000000;
				iItemKey = myrand(0, 10000);			// �����簩���� ���ڿ� ���ڸ� ����
				if( COMPARE( iItemKey, 0, 5000) )	iRand3 = 120000;
				else								iRand3 = 130000;
			}
			else if( COMPARE( iRandom, 7800, 10001) )	{
				iRand2 = 80000000;
				iItemKey = myrand(0, 10000);			// ���������� ���ڿ� ���ڸ� ����
				if( COMPARE( iItemKey, 0, 5000) )	iRand3 = 120000;
				else								iRand3 = 130000;
			}
			
		}
		
		iTemp2 = myrand(0, 10000);					// ���� ���� ������ ����
		if( COMPARE( iTemp2, 0, 2000) )				iRand4 = 1000;
		else if( COMPARE( iTemp2, 2000, 4000) )		iRand4 = 2000;
		else if( COMPARE( iTemp2, 4000, 6000) )		iRand4 = 3000;
		else if( COMPARE( iTemp2, 6000, 8000) )		iRand4 = 4000;
		else if( COMPARE( iTemp2, 8000, 10001) )	iRand4 = 5000;
		iRandom = GetItemCodeNumber(iItemLevel, 2);				// ���̸���ǥ ����
		if(iRandom == -1)	{		// �߸��� ������ ��������
			return 0;
		}
		iRand5 = iRandom * 10;
		iItemNumber = iDefault + iRand2 + iItemCode + iRand3 + iRand4 + iRand5 + iItemGrade;	// iItemGrade : ������ ��޻���ǥ ����
		//TRACE("ItemProdution : Defensive Success item_number = %d, default=%d, iRand2=%d, itemcode=%d, iRand3=%d, iRand4=%d, iRand5, iItemGrade=%d\n", iItemNumber, iDefault, iRand2, iItemCode, iRand3, iRand4, iRand5, iItemGrade);
	}
	else if( COMPARE( iRandom, 8001, 10001) )	{       // �Ǽ��縮 ������
		iDefault = 300000000;
		iRandom = myrand(0, 10000);					// �Ǽ��縮 ��������(�Ͱ�, �����, ����, ��Ʈ)
		if( COMPARE( iRandom, 0, 2500) )			iRand2 = 10000000;
		else if( COMPARE( iRandom, 2500, 5000) )	iRand2 = 20000000;
		else if( COMPARE( iRandom, 5000, 7500) )	iRand2 = 30000000;
		else if( COMPARE( iRandom, 7500, 10001) )	iRand2 = 40000000;
		iRand3 = myrand(1, 10000);					// ����(������, ī�罺)
		if( COMPARE( iRand3, 1, 5000) )	iRand3 = 110000;
		else	iRand3 = 150000;
		iRandom = GetItemCodeNumber(iItemLevel, 3);	// ���̸���ǥ ����
		//TRACE("ItemProdution : GetItemCodeNumber() = %d\n", iRandom);
		if(iRandom == -1)	{		// �߸��� ������ ��������
			return 0;
		}
		iRand4 = iRandom * 10;
		iItemNumber = iDefault + iRand2 + iRand3 + iRand4 + iItemGrade;
		//TRACE("ItemProdution : Accessary Success item_number = %d, default=%d, iRand2=%d, iRand3=%d, iRand4=%d, iItemGrade=%d\n", iItemNumber, iDefault, iRand2, iRand3, iRand4, iItemGrade);
	}
	
	return iItemNumber;
}

int  CNpc::GetItemGrade(int item_grade)
{
	int iPercent = 0, iRandom = 0, i=0;
	_MAKE_ITEM_GRADE_CODE* pItemData = nullptr;

	iRandom = myrand(1, 1000);
	pItemData = g_pMain->m_MakeGradeItemArray.GetData(item_grade); 
	if(pItemData == nullptr)	return 0;


	for(i=0; i<9; i++)	{
		if(i == 0)	{
			if(pItemData->sGrade[i] == 0)	{
				iPercent += pItemData->sGrade[i];
				continue;
			}
			if( COMPARE( iRandom, 0, pItemData->sGrade[i]) )	return i+1;
			else	{
				iPercent += pItemData->sGrade[i];
				continue;
			}
		}
		else	{
			if(pItemData->sGrade[i] == 0)	{
				iPercent += pItemData->sGrade[i];
				continue;
			}

			if( COMPARE( iRandom, iPercent, iPercent+pItemData->sGrade[i]) )	return i+1;
			else	{
				iPercent += pItemData->sGrade[i];
				continue;
			}
		}
		
	}

	return 0;
}

int  CNpc::GetWeaponItemCodeNumber(int item_type)
{
	int iPercent = 0, iRandom = 0, i=0, iItem_level = 0;
	_MAKE_WEAPON* pItemData = nullptr;

	iRandom = myrand(0, 1000);
	if( item_type == 1 )	{		// ���ⱸ
		iItem_level = m_proto->m_sLevel / 10;
		pItemData = g_pMain->m_MakeWeaponItemArray.GetData(iItem_level); 
	}
	else if( item_type == 2 )	{	// ��
		iItem_level = m_proto->m_sLevel / 10;
		pItemData = g_pMain->m_MakeDefensiveItemArray.GetData(iItem_level); 
	}

	if(pItemData == nullptr)	return 0;

	for(i=0; i<MAX_UPGRADE_WEAPON; i++)	{
		if(i == 0)	{
			if(pItemData->sClass[i] == 0)	{
				iPercent += pItemData->sClass[i];
				continue;
			}
			if( COMPARE( iRandom, 0, pItemData->sClass[i]) )	return i+1;
			else	{
				iPercent += pItemData->sClass[i];
				continue;
			}
		}
		else	{
			if(pItemData->sClass[i] == 0)	{
				iPercent += pItemData->sClass[i];
				continue;
			}

			if( COMPARE( iRandom, iPercent, iPercent+pItemData->sClass[i]) )	return i+1;
			else	{
				iPercent += pItemData->sClass[i];
				continue;
			}
		}
	}

	return 0;
}

int  CNpc::GetItemCodeNumber(int level, int item_type)
{
	int iItemCode = 0, iRandom = 0, i=0, iItemType = 0, iPercent = 0;
	int iItemPercent[3];
	_MAKE_ITEM_LARE_CODE* pItemData = nullptr;

	iRandom = myrand(0, 1000);
	pItemData = g_pMain->m_MakeLareItemArray.GetData(level); 
	if(pItemData == nullptr)	return -1;
	iItemPercent[0] = pItemData->sLareItem;
	iItemPercent[1] = pItemData->sMagicItem;
	iItemPercent[2] = pItemData->sGeneralItem;

	for(i=0; i<3; i++)	{
		if(i == 0)	{
			if( COMPARE( iRandom, 0, iItemPercent[i]) )	{
				iItemType = i+1;
				break;
			}
			else	{
				iPercent += iItemPercent[i];
				continue;
			}
		}
		else	{
			if( COMPARE( iRandom, iPercent, iPercent+iItemPercent[i]) )	{
				iItemType = i+1;
				break;
			}
			else	{
				iPercent += iItemPercent[i];
				continue;
			}
		}
	}

	switch(iItemType)	{
		case 0:						// �߸��� ������
			iItemCode = 0;
			break;
		case 1:						// lare item
			if(item_type == 1)	{			// ���ⱸ
				iItemCode = myrand(16, 24);
			}
			else if(item_type == 2)	{		// ��
				iItemCode = myrand(12, 24);
			}
			else if(item_type == 3)	{		// �Ǽ��縮
				iItemCode = myrand(0, 10);
			}
			break;
		case 2:						// magic item
			if(item_type == 1)	{			// ���ⱸ
				iItemCode = myrand(6, 15);
			}
			else if(item_type == 2)	{		// ��
				iItemCode = myrand(6, 11);
			}
			else if(item_type == 3)	{		// �Ǽ��縮
				iItemCode = myrand(0, 10);
			}
			break;
		case 3:						// general item
			if(item_type == 1)	{			// ���ⱸ
				iItemCode = 5;
			}
			else if(item_type == 2)	{		// ��
				iItemCode = 5;
			}
			else if(item_type == 3)	{		// �Ǽ��縮
				iItemCode = myrand(0, 10);
			}
			break;	
	}

	return iItemCode;
}

void CNpc::DurationMagic_4()
{
	MAP* pMap = GetMap();
	if (pMap == nullptr)	
		return;

	if (m_byDungeonFamily > 0)
	{
		CRoomEvent* pRoom = pMap->m_arRoomEventArray.GetData(m_byDungeonFamily);
		if (pRoom == nullptr)
		{
			// If it doesn't exist, there's no point continually assuming it exists. Just unset it.
			// We'll only throw the message once, so that the user knows they need to make sure the room event exists.
			m_byDungeonFamily = 0;
			TRACE("#### Npc-DurationMagic_4() failed: room event does not exist : [nid=%d, name=%s], m_byDungeonFamily(event)=%d #####\n", 
				m_sNid+NPC_BAND, m_proto->m_strName, m_byDungeonFamily);
		}
		else
		{
			if( pRoom->m_byStatus == 3 )	{	// ���� Ŭ���� �Ȱ��
				if( m_NpcState != NPC_DEAD )	{
					if( m_byRegenType == 0 )	{		
						m_byRegenType = 2;      // ������ ���� �ʵ���,,
						Dead(1);
						return;
					}
				}
				//return;
			}
		}
	}

	for (int i = 0; i < MAX_MAGIC_TYPE4; i++)	
	{
		if (m_MagicType4[i].sDurationTime)
		{
			if (UNIXTIME > (m_MagicType4[i].tStartTime + m_MagicType4[i].sDurationTime))
			{
				m_MagicType4[i].sDurationTime = 0;		
				m_MagicType4[i].tStartTime = 0;
				m_MagicType4[i].byAmount = 0;
				if(i == 5)	{					// �ӵ� ����... �ɷ�ġ..
					m_fSpeed_1 = m_fOldSpeed_1;
					m_fSpeed_2 = m_fOldSpeed_2;
				}
			}
		}
	}
}

// ��ȭ�Ǵ� ������ ������ �ٲپ��ش�...
void CNpc::ChangeMonsterInfomation(int iChangeType)
{
	if( m_sChangeSid == 0 || m_byChangeType == 0 )	return;			// ������ �ʴ� ����
	if( m_NpcState != NPC_DEAD )	return;		// ���� ����
	
	CNpcTable*	pNpcTable = m_proto;
	if(m_byInitMoveType >= 0 && m_byInitMoveType < 100)	{
		if (iChangeType == 1)	// �ٸ� ���ͷ� ��ȯ..
			pNpcTable = g_pMain->m_arMonTable.GetData(m_sChangeSid);
		if(pNpcTable == nullptr)	{
			TRACE("##### ChangeMonsterInfomation Sid Fail -- Sid = %d #####\n", m_sChangeSid);
		}
	}
	else if(m_byInitMoveType >= 100)	{
		if(iChangeType == 1)	// �ٸ� ���ͷ� ��ȯ..
			pNpcTable = g_pMain->m_arNpcTable.GetData(m_sChangeSid);
		if(pNpcTable == nullptr)	{
			TRACE("##### ChangeMonsterInfomation Sid Fail -- Sid = %d #####\n", m_sChangeSid);
		}
	}

	// ��������......
	m_proto = pNpcTable;
	m_sSize			= pNpcTable->m_sSize;		// ĳ������ ����(100 �ۼ�Ʈ ����)
	m_iWeapon_1		= pNpcTable->m_iWeapon_1;	// ���빫��
	m_iWeapon_2		= pNpcTable->m_iWeapon_2;	// ���빫��
	m_byGroup		= pNpcTable->m_byGroup;		// �Ҽ�����
	m_byActType		= pNpcTable->m_byActType;	// �ൿ����
	m_byRank		= pNpcTable->m_byRank;		// ����
	m_byTitle		= pNpcTable->m_byTitle;		// ����
	m_iSellingGroup = pNpcTable->m_iSellingGroup;
	m_iHP			= pNpcTable->m_iMaxHP;		// �ִ� HP
	m_iMaxHP		= pNpcTable->m_iMaxHP;		// ���� HP
	m_sMP			= pNpcTable->m_sMaxMP;		// �ִ� MP
	m_sMaxMP		= pNpcTable->m_sMaxMP;		// ���� MP
	m_sAttack		= pNpcTable->m_sAttack;		// ���ݰ�
	m_sDefense		= pNpcTable->m_sDefense;	// ��
	m_sHitRate		= pNpcTable->m_sHitRate;	// Ÿ�ݼ�����
	m_sEvadeRate	= pNpcTable->m_sEvadeRate;	// ȸ�Ǽ�����
	m_sDamage		= pNpcTable->m_sDamage;		// �⺻ ������
	m_sAttackDelay	= pNpcTable->m_sAttackDelay;// ���ݵ�����
	m_sSpeed		= pNpcTable->m_sSpeed;		// �̵��ӵ�
	m_fSpeed_1		= (float)pNpcTable->m_bySpeed_1;	// �⺻ �̵� Ÿ��
	m_fSpeed_2		= (float)pNpcTable->m_bySpeed_2;	// �ٴ� �̵� Ÿ��..
	m_fOldSpeed_1	= (float)pNpcTable->m_bySpeed_1;	// �⺻ �̵� Ÿ��
	m_fOldSpeed_2	= (float)pNpcTable->m_bySpeed_2;	// �ٴ� �̵� Ÿ��..
	m_sStandTime	= pNpcTable->m_sStandTime;	// ���ִ� �ð�
	m_byFireR		= pNpcTable->m_byFireR;		// ȭ�� ���׷�
	m_byColdR		= pNpcTable->m_byColdR;		// �ñ� ���׷�
	m_byLightningR	= pNpcTable->m_byLightningR;	// ���� ���׷�
	m_byMagicR		= pNpcTable->m_byMagicR;	// ���� ���׷�
	m_byDiseaseR	= pNpcTable->m_byDiseaseR;	// ���� ���׷�
	m_byPoisonR		= pNpcTable->m_byPoisonR;	// �� ���׷�
	m_bySearchRange	= pNpcTable->m_bySearchRange;	// �� Ž�� ����
	m_byAttackRange	= pNpcTable->m_byAttackRange;	// �����Ÿ�
	m_byTracingRange	= pNpcTable->m_byTracingRange;	// �߰ݰŸ�
	m_iMoney		= pNpcTable->m_iMoney;			// �������� ��
	m_iItem			= pNpcTable->m_iItem;			// �������� ������
	m_tNpcLongType	= pNpcTable->m_byDirectAttack;	
	m_byWhatAttackType = pNpcTable->m_byMagicAttack;
}

void CNpc::DurationMagic_3()
{
	int duration_damage = 0;

	for(int i=0; i<MAX_MAGIC_TYPE3; i++)	{
		if (m_MagicType3[i].byHPDuration) {
			if (UNIXTIME >= (m_MagicType3[i].tStartTime + m_MagicType3[i].byHPInterval)) {		// 2�ʰ�������
				m_MagicType3[i].byHPInterval += 2;
				//TRACE("DurationMagic_3,, [%d] curtime = %.2f, dur=%.2f, nid=%d, damage=%d\n", i, currenttime, m_MagicType3[i].fStartTime, m_sNid+NPC_BAND, m_MagicType3[i].sHPAmount);

				if( m_MagicType3[i].sHPAmount >= 0 )	{				// healing
				}
				else {
					// damage ����...
					duration_damage = m_MagicType3[i].sHPAmount;
					duration_damage = abs(duration_damage);
					if( SetDamage(0, duration_damage, "**duration**", m_MagicType3[i].sHPAttackUserID  ) == false )	{
						// Npc�� ���� ���,,
						SendExpToUserList(); // ����ġ �й�!!
						SendDead();
						SendAttackSuccess(MAGIC_ATTACK_TARGET_DEAD, m_MagicType3[i].sHPAttackUserID, duration_damage, m_iHP, 1, DURATION_ATTACK);
						//TRACE("&&&& Duration Magic attack .. pNpc->m_byHPInterval[%d] = %d &&&& \n", i, m_MagicType3[i].byHPInterval);
						m_MagicType3[i].tStartTime = 0;
						m_MagicType3[i].byHPDuration = 0;
						m_MagicType3[i].byHPInterval = 2;
						m_MagicType3[i].sHPAmount = 0;
						m_MagicType3[i].sHPAttackUserID = -1; 
						duration_damage = 0;
					}
					else	{
						SendAttackSuccess(ATTACK_SUCCESS, m_MagicType3[i].sHPAttackUserID, duration_damage, m_iHP, 1, DURATION_ATTACK);	
						//TRACE("&&&& Duration Magic attack .. pNpc->m_byHPInterval[%d] = %d &&&& \n", i, m_MagicType3[i].byHPInterval);
					}
				}

				if (UNIXTIME >= (m_MagicType3[i].tStartTime + m_MagicType3[i].byHPDuration)) {	// �� ���ݽð�..
					m_MagicType3[i].tStartTime = 0;
					m_MagicType3[i].byHPDuration = 0;
					m_MagicType3[i].byHPInterval = 2;
					m_MagicType3[i].sHPAmount = 0;
					m_MagicType3[i].sHPAttackUserID = -1;
					duration_damage = 0;
				}
			}
		}
	}	
}

/////////////////////////////////////////////////////////////////////////////
//	NPC�� ���ڴ°��.
//
time_t CNpc::NpcSleeping()
{
	if (g_pMain->m_byNight == 1)	{	// ��
		m_NpcState = NPC_STANDING;
		return m_Delay;
	}

	m_NpcState = NPC_SLEEPING;
	return m_sStandTime;
}

/////////////////////////////////////////////////////////////////////////////
// ���Ͱ� �������·�..........
time_t CNpc::NpcFainting()
{
	if (UNIXTIME > (m_tFaintingTime + FAINTING_TIME)) {
		m_NpcState = NPC_STANDING;
		m_tFaintingTime = 0;
		return 0;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// ���Ͱ� ġ����·�..........
time_t CNpc::NpcHealing()
{
	if( m_proto->m_tNpcType != NPC_HEALER )	{
		InitTarget();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	// ġ������ ġ�ᰡ �� �ƴ����� �Ǵ�.. 
	CNpc* pNpc = nullptr;
	int nID = m_Target.id;
	bool bFlag = false;
	int iHP = 0;

	int ret = 0;
	int nStandingTime = m_sStandTime;

	ret = IsCloseTarget(m_byAttackRange, 2);

	if(ret == 0)   {
		if(m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT ) // ���� ����� ������ ���� �ʵ���..
		{
			m_NpcState = NPC_STANDING;
			InitTarget();
			return 0;
		}	
		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;			// �����ϰ� �������� ������ ���� �������(������ ���� ������) 
		return 0; // IsCloseTarget()�� ���� x, y���� �����ϰ� Delay = 0���� ��
	}	
	else if( ret == 2 )	{
		//if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{		// ���� �����̸�.....
		if(m_tNpcLongType == 2)	{		// ����, ����(��)������ ������ ���� �̹Ƿ� ��Ÿ� ������ �� �� �ִ�.
			return LongAndMagicAttack();
		}
		else	{
			if(m_proto->m_tNpcType == NPC_DOOR || m_proto->m_tNpcType == NPC_ARTIFACT || m_proto->m_tNpcType == NPC_PHOENIX_GATE || m_proto->m_tNpcType == NPC_GATE_LEVER || m_proto->m_tNpcType == NPC_DOMESTIC_ANIMAL || m_proto->m_tNpcType == NPC_SPECIAL_GATE || m_proto->m_tNpcType == NPC_DESTORY_ARTIFACT ) // ���� ����� ������ ���� �ʵ���..
			{
				m_NpcState = NPC_STANDING;
				InitTarget();
				return 0;
			}	
			m_sStepCount = 0;
			m_byActionFlag = ATTACK_TO_TRACE;
			m_NpcState = NPC_TRACING;			// �����ϰ� �������� ������ ���� �������(������ ���� ������) 
			return 0; // IsCloseTarget()�� ���� x, y���� �����ϰ� Delay = 0���� ��
		}
	}
	else if( ret == -1 )	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}

	if(nID >= NPC_BAND && nID < INVALID_BAND)	{
		pNpc = g_pMain->m_arNpc.GetData(nID - NPC_BAND);

		if(pNpc == nullptr)	{				// User �� Invalid �� ���
			InitTarget();
		}

		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			InitTarget();
		}

		// ġ�� üũ���� 
		iHP = (int)(pNpc->m_iMaxHP * 0.9);		// 90�ۼ�Ʈ�� HP
		if( pNpc->m_iHP >= iHP)	{		// Heal �Ϸ����..
			InitTarget();
		}
		else	{						// Heal ���ֱ�
			CNpcMagicProcess::MagicPacket(MAGIC_EFFECTING, m_proto->m_iMagic3, m_sNid + NPC_BAND, nID);
			return m_sAttackDelay;
		}
	}


	// ���ο� ġ������ ã�Ƽ� �����ش�
	int iMonsterNid = FindFriend( 2 );

	if(iMonsterNid == 0)	{
		InitTarget();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	CNpcMagicProcess::MagicPacket(MAGIC_EFFECTING, m_proto->m_iMagic3, m_sNid + NPC_BAND, iMonsterNid);
	return m_sAttackDelay;
}

int CNpc::GetPartyExp( int party_level, int man, int nNpcExp )
{
	int nPartyExp = 0;
	int nLevel = party_level / man;
	double TempValue = 0;
	nLevel = m_proto->m_sLevel - nLevel;

	//TRACE("GetPartyExp ==> party_level=%d, man=%d, exp=%d, nLevle=%d, mon=%d\n", party_level, man, nNpcExp, nLevel, m_proto->m_sLevel);

	if(nLevel < 2)	{
		nPartyExp = nNpcExp * 1;
	}
	else if(nLevel >= 2 && nLevel < 5)	{
		TempValue = nNpcExp * 1.2;
		nPartyExp = (int)TempValue;
		if(TempValue > nPartyExp)  nPartyExp=nPartyExp+1;
	}
	else if(nLevel >= 5 && nLevel < 8)	{
		TempValue = nNpcExp * 1.5;
		nPartyExp = (int)TempValue;
		if(TempValue > nPartyExp)  nPartyExp=nPartyExp+1;
	}
	else if(nLevel >= 8)	{
		nPartyExp = nNpcExp * 2;
	}	

	return nPartyExp;
}

void CNpc::ChangeAbility(int iChangeType)	// iChangeType - 0:�ɷ�ġ �ٿ�, 1:�ɷ�ġ ȸ��
{
	if( iChangeType > 2 )	return;			// 

	int nHP = 0, nAC=0, nDamage=0, nMagicR=0, nDiseaseR=0, nPoisonR=0, nLightningR=0, nFireR=0, nColdR=0;
	CNpcTable*	pNpcTable = m_proto;

	// ��������......
	if( iChangeType == BATTLEZONE_OPEN )	{		// �ɷ�ġ �ٿ�
		nHP = (int)(pNpcTable->m_iMaxHP / 2);
		nAC = (int)(pNpcTable->m_sDefense * 0.2);
		nDamage = (int)(pNpcTable->m_sDamage * 0.3);
		nMagicR = (int)(pNpcTable->m_byMagicR / 2);
		nDiseaseR = (int)(pNpcTable->m_byDiseaseR / 2);
		nPoisonR = (int)(pNpcTable->m_byPoisonR / 2);
		nLightningR = (int)(pNpcTable->m_byLightningR / 2);
		nFireR = (int)(pNpcTable->m_byFireR / 2);
		nColdR = (int)(pNpcTable->m_byColdR / 2);
		m_iMaxHP = nHP;
		if( m_iHP > nHP )	{	// HP�� �ٲ�� �ڱ�,,
			HpChange();
		}
		m_sDefense = nAC;
		m_sDamage = nDamage;
		m_byFireR		= nFireR;		// ȭ�� ���׷�
		m_byColdR		= nColdR;		// �ñ� ���׷�
		m_byLightningR	= nLightningR;	// ���� ���׷�
		m_byMagicR		= nMagicR;		// ���� ���׷�
		m_byDiseaseR	= nDiseaseR;	// ���� ���׷�
		m_byPoisonR		= nPoisonR;		// �� ���׷�
		//TRACE("-- ChangeAbility down : nid=%d, name=%s, hp:%d->%d, damage=%d->%d\n", m_sNid+NPC_BAND, m_proto->m_strName, pNpcTable->m_iMaxHP, nHP, pNpcTable->m_sDamage, nDamage); 
	}
	else if( iChangeType == BATTLEZONE_CLOSE )	{	// �ɷ�ġ ȸ��
		m_iMaxHP		= pNpcTable->m_iMaxHP;		// ���� HP
		//TRACE("++ ChangeAbility up : nid=%d, name=%s, hp:%d->%d, damage=%d->%d\n", m_sNid+NPC_BAND, m_proto->m_strName, m_iHP, m_iMaxHP, pNpcTable->m_sDamage, nDamage); 
		if( m_iMaxHP > m_iHP )	{	// HP�� �ٲ�� �ڱ�,,
			m_iHP = m_iMaxHP - 50;
			HpChange();
		}
		m_sDamage		= pNpcTable->m_sDamage;		// �⺻ ������
		m_sDefense		= pNpcTable->m_sDefense;	// ��
		m_byFireR		= pNpcTable->m_byFireR;		// ȭ�� ���׷�
		m_byColdR		= pNpcTable->m_byColdR;		// �ñ� ���׷�
		m_byLightningR	= pNpcTable->m_byLightningR;	// ���� ���׷�
		m_byMagicR		= pNpcTable->m_byMagicR;	// ���� ���׷�
		m_byDiseaseR	= pNpcTable->m_byDiseaseR;	// ���� ���׷�
		m_byPoisonR		= pNpcTable->m_byPoisonR;	// �� ���׷�
	}
}

bool CNpc::Teleport()
{
	int i=0;
	int nX=0, nZ=0, nTileX=0, nTileZ=0;
	MAP* pMap = GetMap();
	if (pMap == nullptr)	return false;

	while(1)	{
		i++;
		nX = myrand(0, 10);
		nX = myrand(0, 10);
		nX = (int)m_fCurX + nX;
		nZ = (int)m_fCurZ + nZ;
		nTileX = nX / TILE_SIZE;
		nTileZ = nZ / TILE_SIZE;

		if(nTileX > pMap->GetMapSize())		nTileX = pMap->GetMapSize();
		if(nTileZ > pMap->GetMapSize())		nTileZ = pMap->GetMapSize();

		if(nTileX < 0 || nTileZ < 0)	{
			TRACE("#### Npc-SetLive() Fail : nTileX=%d, nTileZ=%d #####\n", nTileX, nTileZ);
			return false;
		}
		break;
	}	

	Packet result(AG_NPC_INOUT);
	result << uint8(NPC_OUT) << uint16(m_sNid + NPC_BAND) << m_fCurX << m_fCurZ << m_fCurY;
	g_pMain->Send(&result);

	m_fCurX = (float)nX;	m_fCurZ = (float)nZ;

	result.clear();
	result << uint8(NPC_IN) << uint16(m_sNid + NPC_BAND) << m_fCurX << m_fCurZ << float(0.0f);
	g_pMain->Send(&result);

	SetUid(m_fCurX, m_fCurZ, m_sNid + NPC_BAND);
	return true;
}