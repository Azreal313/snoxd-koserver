#pragma once

#include "../N3Base/N3ShapeMgr.h"
#include "../shared/STLMap.h"
#include "Extern.h"
#include "RoomEvent.h"

typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <CRoomEvent>		RoomEventArray;

class CRegion;
class CNpc;
class CUser;

// temporary
struct CSize
{
	CSize() : cx(0), cy(0) {}
	CSize(int cx, int cy) : cx(cx), cy(cy) {}
	int cx, cy;
};

class SMDFile;
class MAP  
{
public:
	// Passthru methods
	int GetMapSize();
	float GetUnitDistance();
	int GetXRegionMax();
	int GetZRegionMax();
	short * GetEventIDs();
	int GetEventID(int x, int z);

	CRegion**		m_ppRegion;				// 64������ Ÿ������..
	int m_nZoneNumber;						// zone number
	int	m_nServerNo;
	std::string m_MapName;
	float*		m_fHeight;
	uint8		m_byRoomType;		// ���� �ʱ�ȭ����( 0:�ڵ����� �ʱ�ȭ, 1:�����̺�Ʈ ����(Ư�������� �Ϸ�� �ʱ�ȭ)
	uint8		m_byRoomEvent;		// event room(0:empty, 1:use)
	uint8		m_byRoomStatus;		// room status(1:������, 2:���� �ʱ�ȭ��, 3:���ʱ�ȭ �Ϸ�)
	uint8		m_byInitRoomCount;	// room �ʱ�ȭ �ð��� ����(���Ϳ� ����ȭ�� ���߱� ����)
	ObjectEventArray m_ObjectEventArray;
	RoomEventArray	 m_arRoomEventArray;
	short	m_sKarusRoom;			// karus�� ������
	short	m_sElmoradRoom;			// elmorad�� ������

	FastMutex m_lock;

public:
	MAP();
	virtual ~MAP();

	bool Initialize(_ZONE_INFO *pZone);

	bool LoadRoomEvent();
	bool ObjectIntersect(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float z );

	bool RegionNpcRemove( int rx, int rz, int nid );
	void RegionNpcAdd( int rx, int rz, int nid );
	bool RegionUserRemove( int rx, int rz, int uid );
	void RegionUserAdd( int rx, int rz, int uid );

	int IsRoomCheck(float fx, float fz);	// �������� ���, ������ ������ġ�� ������ ��� ��ġ�� �ִ����� �Ǵ�
	bool IsRoomStatusCheck();

	bool IsMovable(int dest_x, int dest_y);
	void InitializeRoom();

	CRoomEvent* SetRoomEvent( int number );

protected:
	void RemoveMapData();

	SMDFile * m_smdFile;
};