/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <base/tl/array.h>
#include "pathfinding.h"

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	double m_AnimationTime;

	array< array<int> > m_Zones;

	int m_WaypointCount;
	int m_ConnectionCount;
	
	void ClearWaypoints();
	
	void RemoveClosedAreas();
	void AddWaypoint(vec2 Position, bool InnerCorner = false);
	CWaypoint *GetWaypointAt(int x, int y);
	void ConnectWaypoints();
	CWaypoint *GetClosestWaypoint(vec2 Pos);

	CWaypoint *m_apWaypoint[MAX_WAYPOINTS];
	CWaypoint *m_pCenterWaypoint;
	
	CWaypointPath *m_pPath;
	
	int m_LowestPoint;

public:
	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);
	
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,

		COLFLAG_RAMP_LEFT=8,
		COLFLAG_RAMP_RIGHT=16,
		COLFLAG_ROOFSLOPE_LEFT=32,
		COLFLAG_ROOFSLOPE_RIGHT=64,
		COLFLAG_DAMAGEFLUID=128,
	};

	vec2 GetClosestWaypointPos(vec2 Pos);

	void GenerateWaypoints();
	bool GenerateSomeMoreWaypoints();
	int WaypointCount() { return m_WaypointCount; }
	int ConnectionCount() { return m_ConnectionCount; }

	CCollision();

	void SetWaypointCenter(vec2 Position);
	void AddWeight(vec2 Pos, int Weight);
	
	vec2 GetRandomWaypointPos();
	
	int m_Time;
	bool m_GlobalAcid;
	
	int GetLowestPoint();
	float GetGlobalAcidLevel();
	
	//CWaypointPath *AStar(vec2 Start, vec2 End);
	bool AStar(vec2 Start, vec2 End);
	
	CWaypointPath *GetPath(){ return m_pPath; }
	void ForgetAboutThePath(){ m_pPath = 0; }

	// for testing
	vec2 m_aPath[99];
	int m_PathLen;

	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round_to_int(x), round_to_int(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int FastIntersectLine(vec2 Pos0, vec2 Pos1);
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);

	void SetTime(double Time) { m_AnimationTime = Time; }

	//This function return an Handle to access all zone layers with the name "pName"
	int GetZoneHandle(const char* pName);
	int GetZoneValueAt(int ZoneHandle, float x, float y);

	// MapGen
	bool ModifTile(ivec2 pos, int group, int layer, int tile, int flags, int reserved);
};

#endif
