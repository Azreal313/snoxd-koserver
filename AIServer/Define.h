#pragma once

//
//	Defines About Communication
//
#define AI_KARUS_SOCKET_PORT		10020
#define AI_ELMO_SOCKET_PORT			10030
#define AI_BATTLE_SOCKET_PORT		10040
#define MAX_SOCKET					100
#define CLIENT_SOCKSIZE		10
#define MAX_PATH_LINE		100

#define MAX_NPC_SIZE		30
#define MAX_WEAPON_NAME_SIZE	40
#define VIEW_DIST			48		// ���ðŸ�
#define MAX_UPGRADE_WEAPON	12

///////////////// NATION ///////////////////////////////////
//
#define UNIFY_ZONE			0
#define KARUS_ZONE			1
#define ELMORAD_ZONE		2
#define BATTLE_ZONE			3

//enum MOVE_SPEED {SPEED_SLOW=0, SPEED_NORMAL, SPEED_FAST};
//enum USER_TYPE {TYPE_USER=0, TYPE_MONSTER, TYPE_NPC, TYPE_DOOR, TYPE_GUARD};

//
//	User Authority
//
#define MANAGER_USER	0	// ���, ������
#define GENERAL_USER	1	// �Ϲ�����

// Npc InOut
#define NPC_IN					0X01
#define NPC_OUT					0X02

#define TILE_SIZE		4
#define CELL_SIZE		4

#define COMPARE(x,min,max) ((x>=min)&&(x<max))

struct _NpcPosition
{
	BYTE	byType;			// type
	BYTE	bySpeed;		// speed
	POINT	pPoint;			// position
	float fXPos;
	float fZPos;
};

//
//	About USER
//
#define USER_DEAD				0X00
#define USER_LIVE				0X01

//
//	About USER Log define 
//
#define USER_LOGIN				0X01
#define USER_LOGOUT				0X02
#define USER_LEVEL_UP			0X03


//
//	About NPC
//
#define NPC_NUM					20
#define MAX_DUNGEON_BOSS_MONSTER	20
	
#define NPC_DEAD				0X00
#define NPC_LIVE				0X01
#define NPC_ATTACKING			0X02
#define NPC_ATTACKED			0X03
#define NPC_ESCAPE				0X04
#define NPC_STANDING			0X05
#define NPC_MOVING				0X06
#define NPC_TRACING				0X07
#define NPC_FIGHTING			0X08
#define NPC_STRATEGY			0x09
#define NPC_BACK				0x0A
#define NPC_SLEEPING			0x0B
#define NPC_FAINTING			0x0C
#define NPC_HEALING				0x0D

#define NPC_PASSIVE				150
#define NPC_MAX_MOVE_RANGE		100

//
//	About Map Object
//
#define USER_BAND				0			// Map ���� ������ �ִ�.
#define NPC_BAND				10000		// Map ���� NPC(������)�� �ִ�.
#define INVALID_BAND			30000		// �߸��� ID BAND

//
//  Item
//
#define TYPE_MONEY				0
#define TYPE_ITEM				1

////////////////////////////////////////////////////////////
// Durability Type
#define ATTACK				0x01
#define DEFENCE				0x02
////////////////////////////////////////////////////////////

//
//	Attack
//

const BYTE	ATTACK_FAIL		=	0;
const BYTE	ATTACK_SUCCESS	=	1;
const BYTE	ATTACK_TARGET_DEAD	= 2;
const BYTE	ATTACK_TARGET_DEAD_OK = 3;
const BYTE	MAGIC_ATTACK_TARGET_DEAD	= 4;

#define GREAT_SUCCESS			0X01		// �뼺��
#define SUCCESS					0X02		// ����
#define NORMAL					0X03		// ����
#define	FAIL					0X04		// ����

#define DIR_DOWN			0			// �� �����ִ� ������ �����Ѵ�.
#define	DIR_DOWNLEFT		1
#define DIR_LEFT			2
#define	DIR_UPLEFT			3
#define DIR_UP				4
#define DIR_UPRIGHT			5
#define DIR_RIGHT			6
#define	DIR_DOWNRIGHT		7

////////////////////////////////////////////////////////////
// Npc Type
// Monster�� 0���� ���� 10������ Ÿ��
#define NPCTYPE_MONSTER				0	// monster
#define NPC_BOSS_MONSTER			3	// ���� ����
#define NPC_DUNGEON_MONSTER			4	// ���� ����
#define NPC_TRAP_MONSTER			5	// ���� ����
// NPC�� 11���� ����
#define NPC_GUARD					11	// �ٹ����� ���
#define NPC_PATROL_GUARD			12	// �Ϲ� �ʵ忡�� ������ ����ϴ� ������
#define NPC_STORE_GUARD				13	// �Ϲ� �ʵ忡�� �����ֺ��� ��ȣ�ϴ� ���
#define NPC_MERCHANT				21	// �������� NPC 
#define NPC_TINKER					22	// ��������
#define NPC_WAREHOUSE				23	// â������
#define NPC_CAPTAIN_NPC				35	// ���� �����ִ� NPC
#define NPC_KNIGHTS_NPC				36	// ���� ���� NPC
#define NPC_CLERIC					37	// ����� NPC
#define NPC_HEALER					40	// Healer
#define NPC_DOOR					50	// ���� (6->50)
#define NPC_PHOENIX_GATE			51	// ������ �ʴ� �� (8->51)
#define NPC_SPECIAL_GATE			52	// ������ �ʴ� ���̸鼭 2�и��� ���ȴ� ������ �ϴ� ��
#define NPC_GATE_LEVER				55	// ���� ����...	(9->55)	
#define NPC_ARTIFACT				60	// ��輮 (7->60)
#define NPC_DESTORY_ARTIFACT		61	// �ı��Ǵ� ��輮
#define NPC_DOMESTIC_ANIMAL			99	// ���� NPC
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Magic State
#define NONE				0x01
#define CASTING				0x02
////////////////////////////////////////////////////////////

#define MORAL_SELF				1		// �� �ڽ�..
#define MORAL_FRIEND_WITHME		2		// ���� ������ �츮��(����) �� �ϳ� ..
#define MORAL_FRIEND_EXCEPTME	3		// ���� �� �츮�� �� �ϳ� 
#define MORAL_PARTY				4		// ���� ������ �츮��Ƽ �� �ϳ�..
#define MORAL_NPC				5		// NPC�� �ϳ�.
#define MORAL_PARTY_ALL			6		// ���� ȣ���� ��Ƽ ���..
#define MORAL_ENEMY				7		// ������ ������ ��� ���� �ϳ�(NPC����)
#define MORAL_ALL				8		// �׻� �����ϴ� ��� ���� �ϳ�.
#define MORAL_AREA_ENEMY		10		// ������ ���Ե� ��
#define MORAL_AREA_FRIEND		11		// ������ ���Ե� �츮��
#define MORAL_AREA_ALL			12		// ������ ���Ե� ���
#define MORAL_SELF_AREA			13		// ���� �߽����� �� ����

////////////////////////////////////////////////////////////////
// Magic Packet sub define 
////////////////////////////////////////////////////////////////
#define MAGIC_CASTING			0x01
#define MAGIC_FLYING			0x02
#define MAGIC_EFFECTING			0x03
#define MAGIC_FAIL				0x04
#define MAGIC_TYPE4_END			0x05	// For type 4 durational spells.					

// Attack Type
#define DIRECT_ATTACK		0
#define LONG_ATTACK			1
#define MAGIC_ATTACK		2
#define DURATION_ATTACK		3

#define NORMAL_OBJECT		0
#define SPECIAL_OBJECT		1

// Battlezone Announcement
#define BATTLEZONE_OPEN         0x00
#define BATTLEZONE_CLOSE        0x01           
#define DECLARE_WINNER          0x02
#define DECLARE_BAN				0x03

////////////////////////////////////////////////////////////////
// weather define
////////////////////////////////////////////////////////////////
#define WEATHER_FINE			0x01
#define WEATHER_RAIN			0x02
#define WEATHER_SNOW			0x03
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// magic define
////////////////////////////////////////////////////////////////
#define MORAL_GOOD		0x01
#define MORAL_BAD		0x02
#define MORAL_NEUTRAL	0x03

#define NONE_R				0
#define	FIRE_R				1
#define	COLD_R				2
#define LIGHTNING_R			3
#define MAGIC_R				4
#define DISEASE_R			5
#define POISON_R			6

////////////////////////////////////////////////////////////////
// Type 3 Attribute define
////////////////////////////////////////////////////////////////
#define ATTRIBUTE_FIRE			 1
#define ATTRIBUTE_ICE			 2
#define ATTRIBUTE_LIGHTNING		 3

const int TYPE_MONEY_SID		=	900000000;