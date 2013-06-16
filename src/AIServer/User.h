#pragma once

#include "Extern.h"

#include "../shared/STLMap.h"
#include "../Ebenezer/Unit.h"

class MAP;
class CUser : public Unit
{
public:
	virtual uint16 GetID() { return m_iUserId; }
	virtual std::string & GetName() { return m_strUserID; }

	virtual int32 GetHealth() { return m_sHP; }
	virtual int32 GetMaxHealth() { return m_sMaxHP; }
	virtual int32 GetMana() { return m_sMP; }
	virtual int32 GetMaxMana() { return m_sMaxMP; }

	virtual void GetInOut(Packet &, uint8) {}
	virtual void AddToRegion(int16 sRegionX, int16 sRegionZ) {}

	virtual void HpChange(int amount, Unit *pAttacker = nullptr, bool bSendToAI = true) {}
	virtual void MSpChange(int amount) {}

	virtual bool isDead() { return m_bLive == AI_USER_DEAD || GetHealth() <= 0; }

	std::string m_strUserID;
	short	m_iUserId;					// User�� ��ȣ
	uint8	m_bLive;					// �׾���? ��Ҵ�?

	float			m_fWill_x;			// ���� X ��ǥ
	float			m_fWill_y;			// ���� Y ��ǥ
	float			m_fWill_z;			// ���� Z ��ǥ
	short			m_sSpeed;			// ������ ���ǵ�	
	uint8 			m_curZone;			// ���� ��
	MAP *			m_pMap;

	short	m_sHP;							// HP
	short	m_sMP;							// MP
	short	m_sMaxHP;						// HP
	short	m_sMaxMP;						// MP

	uint8	m_state;						// User�� ����

	short	m_sOldRegionX;					// ���� ���� X ��ǥ
	short	m_sOldRegionZ;					// ���� ���� Z ��ǥ

	uint8	m_bResHp;						// ȸ����
	uint8	m_bResMp;
	uint8	m_bResSta;

	uint8    m_byNowParty;				// ��Ƽ���̸� 1, �δ����̸� 2, �Ѵ� �ƴϸ� 0
	uint8	m_byPartyTotalMan;			// ��Ƽ ���� �� ���� �ο��� 
	short   m_sPartyTotalLevel;			// ��Ƽ ���� ����� �� ����
	short	m_sPartyNumber;				// ��Ƽ ��ȣ

	short	m_sHitDamage;				// Hit
	float	m_fHitrate;					// ���� ��ø��
	float	m_fAvoidrate;				// ��� ��ø��
	short	m_sAC;						// �����
	short   m_sItemAC;                  // ������ ����
	

	short  m_sSurroundNpcNumber[8];		// Npc �ٱ�~

	uint8   m_byIsOP;					// ��������� �Ǵ�..
	uint8	m_bInvisibilityType;

	long   m_lUsed;						// ������ �������.. (1:�����.. ���� ���ġ ����. 0:����ص� ����)

	bool		m_bLogOut;				// Logout ���ΰ�?

	uint8    m_bMagicTypeLeftHand;			// The type of magic item in user's left hand  
	uint8    m_bMagicTypeRightHand;			// The type of magic item in user's right hand
	short   m_sMagicAmountLeftHand;         // The amount of magic item in user's left hand
	short	m_sMagicAmountRightHand;        // The amount of magic item in user's left hand

public:
	short GetMagicDamage(int damage, short tid);
	void Initialize();
	void InitNpcAttack();
	void Attack(int sid, int tid);	// ATTACK
	void SetDamage(int damage, int tid);				// user damage
	void Dead(int tid, int nDamage);					// user dead
	void SetExp(int iNpcExp, int iLoyalty, int iLevel);		// user exp
	void SetPartyExp(int iNpcExp, int iLoyalty, int iPartyLevel, int iMan);		// user exp
	short GetDamage(int tid, int magicid=0);
	uint8 GetHitRate(float rate);
	int IsSurroundCheck(float fX, float fY, float fZ, int NpcID);
	void HealMagic();
	void HealAreaCheck(int rx, int rz);

	void SendAttackSuccess(short tid, uint8 result, short sDamage, int nHP=0, short sAttack_type=1, uint8 type = 1, short sid = -1);
	void SendHP();												// user�� HP
	void SendExp(int iExp, int iLoyalty, int tType = 1);

	CUser();
	virtual ~CUser();
};
