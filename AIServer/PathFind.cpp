#include "stdafx.h"
#include "MAP.h"
#include "PathFind.h"
#include "math.h"
#include "Serverdlg.h"

#define LEVEL_ONE_FIND_CROSS			2
#define LEVEL_ONE_FIND_DIAGONAL			3
#define LEVEL_TWO_FIND_CROSS			11
#define LEVEL_TWO_FIND_DIAGONAL			10

CPathFind::CPathFind()
{
	m_pStack = (STACK *)calloc(1, sizeof(STACK));
	m_pOpen = NULL;
	m_pClosed = NULL;
	m_pMap = NULL;
	m_lMapUse = 0;
}

CPathFind::~CPathFind()
{
	ClearData();
	free(m_pStack);
}

void CPathFind::ClearData()
{
	_PathNode *t_node1, *t_node2;

	if(m_pOpen)
	{
		t_node1 = m_pOpen->NextNode;
		while(t_node1)
		{
			t_node2 = t_node1->NextNode;
			free(t_node1);
			t_node1 = t_node2;
		}
		free(m_pOpen);
		m_pOpen = NULL;
	}

	if(m_pClosed)
	{
		t_node1 = m_pClosed->NextNode;
		while(t_node1)
		{
			t_node2 = t_node1->NextNode;
			free(t_node1);
			t_node1 = t_node2;
		}
		free(m_pClosed);
		m_pClosed = NULL;
	}
}

void CPathFind::SetMap(int x, int y, short *pMap, uint32 nMapSize, int16 min_x, int16 min_y)
{
	m_vMapSize.cx = x;
	m_vMapSize.cy = y;
	m_nMapSize = nMapSize; // event array requires the full map size
	m_pMap = pMap;
	this->min_x = min_x;
	this->min_y = min_y;

/*	if(InterlockedCompareExchange(&m_lMapUse, (LONG)1, (LONG)0) == 0)
	{
		m_vMapSize.cx = x;
		m_vMapSize.cy = y;
		m_pMap = map;
		::InterlockedExchange(&m_lMapUse, 0);
	}
	else TRACE("�߸��� �ʼ���\n");	*/
}

_PathNode *CPathFind::FindPath(int start_x, int start_y, int dest_x, int dest_y)
{
	_PathNode *t_node, *r_node = NULL;
	
//	if(!m_pMap) return NULL;

	ClearData();
	m_pOpen = (_PathNode *)calloc(1, sizeof(_PathNode));
	m_pClosed = (_PathNode *)calloc(1, sizeof(_PathNode));
	
	t_node = (_PathNode *)calloc(1, sizeof(_PathNode));
	t_node->g = 0;
	t_node->h = (int)sqrt((double)((start_x-dest_x)*(start_x-dest_x) + (start_y-dest_y)*(start_y-dest_y)));
//	t_node->h = (int)max( start_x-dest_x, start_y-dest_y );
	t_node->f = t_node->g + t_node->h;
	t_node->x = start_x;
	t_node->y = start_y;

	m_pOpen->NextNode = t_node;
	r_node = (_PathNode *)ReturnBestNode();
	if(r_node == NULL) return r_node;
	if(r_node->x == dest_x && r_node->y == dest_y)
		return r_node;

	FindChildPath(r_node, dest_x, dest_y);
	return r_node;
}

_PathNode *CPathFind::ReturnBestNode()
{
	_PathNode *tmp;
	
	if(m_pOpen->NextNode == NULL) {
		return NULL;
	}
	
	tmp=m_pOpen->NextNode;   // point to first node on m_pOpen
	m_pOpen->NextNode=tmp->NextNode;    // Make m_pOpen point to nextnode or NULL.
	
	tmp->NextNode=m_pClosed->NextNode;
	m_pClosed->NextNode=tmp;
	
	return(tmp);
}

void CPathFind::FindChildPath(_PathNode *node, int dx, int dy)
{
	int x, y;
	// UpperLeft
	if(IsBlankMap(x=node->x-1,y=node->y-1))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_CROSS		);
	// Upper
	if(IsBlankMap(x=node->x,y=node->y-1))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_DIAGONAL	);
	// UpperRight
	if(IsBlankMap(x=node->x+1,y=node->y-1))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_CROSS		);
	// Right
	if(IsBlankMap(x=node->x+1,y=node->y))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_DIAGONAL	);
	// LowerRight
	if(IsBlankMap(x=node->x+1,y=node->y+1))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_CROSS		);
	// Lower
	if(IsBlankMap(x=node->x,y=node->y+1))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_DIAGONAL	);
	// LowerLeft
	if(IsBlankMap(x=node->x-1,y=node->y+1))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_CROSS		);
	// Left
	if(IsBlankMap(x=node->x-1,y=node->y))
		FindChildPathSub(node, x, y, dx, dy, LEVEL_TWO_FIND_DIAGONAL	);
}

void CPathFind::FindChildPathSub(_PathNode *node, int x, int y, int dx, int dy, int arg)
{
	int g, c=0;
	_PathNode *old_node,*t_node;
	
	g = node->g + arg;
	
	if((old_node = CheckOpen(x, y)) != NULL)
	{
		for(c = 0; c < 8; c++)
		{
			if(node->Child[c] == NULL)
			{
				break;
			}
		}
		node->Child[c] = old_node;
		if(g < old_node->g)
		{
			old_node->Parent = node;
			old_node->g = g;
			old_node->f = g + old_node->h;
		}
	}
	else if((old_node = CheckClosed(x, y)) != NULL)
	{
		for(c = 0; c < 8; c++)
		{
			if(node->Child[c] == NULL)
			{
				break;
			}
		}
		node->Child[c] = old_node;
		if(g < old_node->g)
		{
			old_node->Parent = node;
			old_node->g = g;
			old_node->f = g + old_node->h;
			PropagateDown(old_node);
		}
	}
	else
	{
		t_node = (_PathNode *)calloc(1, sizeof(_PathNode));
		t_node->Parent = node;
		t_node->g = g;
//		t_node->h = (int)sqrt((x-dx)*(x-dx) + (y-dy)*(y-dy));
		t_node->h = (int)max( x-dx, y-dy );
		t_node->f = g + t_node->h;
		t_node->x = x;
		t_node->y = y;
		Insert(t_node);
		for(c = 0; c < 8; c++)
		{
			if(node->Child[c] == NULL)
			{
				break;
			}
		}
		node->Child[c] = t_node;
	}
}

_PathNode *CPathFind::CheckOpen(int x, int y)
{
	_PathNode *tmp = m_pOpen->NextNode;
	while (tmp != NULL)
	{
		if (tmp->x == x && tmp->y == y)
			return tmp;

		tmp = tmp->NextNode;
	}

	return NULL;
}

_PathNode *CPathFind::CheckClosed(int x, int y)
{
	_PathNode *tmp;
	
	tmp = m_pClosed->NextNode;
	
	while(tmp != NULL)
	{
		if(tmp->x == x && tmp->y == y)
		{
			return tmp;
		}
		else
		{
			tmp = tmp->NextNode;
		}
	}

	return NULL;
}

void CPathFind::Insert(_PathNode *node)
{
	_PathNode *tmp1, *tmp2;
	int f;
	
	if(m_pOpen->NextNode == NULL)
	{
		m_pOpen->NextNode = node;
		return;
	}
	
	f = node->f;
	tmp1 = m_pOpen;
	tmp2 = m_pOpen->NextNode;
	while((tmp2 != NULL) && (tmp2->f < f))
	{
		tmp1 = tmp2;
		tmp2 = tmp2->NextNode;
	}
	node->NextNode = tmp2;
	tmp1->NextNode = node;
}

void CPathFind::PropagateDown(_PathNode *old)
{
	int c, g;
	_PathNode *child, *parent;
	
	g = old->g;
	for(c = 0; c < 8; c++)
	{
		if((child = old->Child[c]) == NULL)
		{
			break;
		}
		if(g+1 < child->g)
		{
			child->g = g+1;
			child->f = child->g + child->h;
			child->Parent = old;
			Push(child);
		}
	}
	
	while (m_pStack->NextStackPtr != NULL)
	{
		parent = Pop();
		for(c = 0; c < 8; c++)
		{
			if((child = parent->Child[c])==NULL)
			{
				break;
			}
			if(parent->g+1 < child->g)
			{
				child->g = parent->g+1;
				child->f = parent->g + parent->h;
				child->Parent = parent;
				Push(child);
			}
		}
	}
}

void CPathFind::Push(_PathNode *node)
{
	STACK *tmp;
	
	tmp = (STACK *)calloc(1, sizeof(STACK));
	tmp->NodePtr = node;
	tmp->NextStackPtr = m_pStack->NextStackPtr;
	m_pStack->NextStackPtr = tmp;
}

_PathNode *CPathFind::Pop()
{
	_PathNode *t_node;
	STACK *t_stack;
	
	t_stack = m_pStack->NextStackPtr;
	t_node = t_stack->NodePtr;
	
	m_pStack->NextStackPtr = t_stack->NextStackPtr;
	free(t_stack);

	return t_node;
}

bool CPathFind::IsBlankMap(int x, int y)
{
	if (x < 0 || y < 0 || x >= m_vMapSize.cx || y >= m_vMapSize.cy) 
		return false;

	if ((min_x + x) < 0 || (min_y + y) < 0)
		return false;

	return !m_pMap[(min_x + x) * m_nMapSize + (min_y + y)];
}
