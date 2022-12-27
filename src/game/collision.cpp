/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include <game/animation.h>

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
	m_AnimationTime = 0.0;
}

int CCollision::GetZoneHandle(const char *pName)
{
	if (!m_pLayers->ZoneGroup())
		return -1;

	int Handle = m_Zones.size();
	m_Zones.add(array<int>());

	array<int> &LayerList = m_Zones[Handle];

	char aLayerName[12];
	for (int l = 0; l < m_pLayers->ZoneGroup()->m_NumLayers; l++)
	{
		CMapItemLayer *pLayer = m_pLayers->GetLayer(m_pLayers->ZoneGroup()->m_StartLayer + l);

		if (pLayer->m_Type == LAYERTYPE_TILES)
		{
			CMapItemLayerTilemap *pTLayer = (CMapItemLayerTilemap *)pLayer;
			IntsToStr(pTLayer->m_aName, sizeof(aLayerName) / sizeof(int), aLayerName);
			if (str_comp(pName, aLayerName) == 0)
				LayerList.add(l);
		}
		else if (pLayer->m_Type == LAYERTYPE_QUADS)
		{
			CMapItemLayerQuads *pQLayer = (CMapItemLayerQuads *)pLayer;
			IntsToStr(pQLayer->m_aName, sizeof(aLayerName) / sizeof(int), aLayerName);
			if (str_comp(pName, aLayerName) == 0)
				LayerList.add(l);
		}
	}

	return Handle;
}

/* TEEUNIVERSE  BEGIN *************************************************/

inline bool SameSide(const vec2 &l0, const vec2 &l1, const vec2 &p0, const vec2 &p1)
{
	vec2 l0l1 = l1 - l0;
	vec2 l0p0 = p0 - l0;
	vec2 l0p1 = p1 - l0;

	return sign(l0l1.x * l0p0.y - l0l1.y * l0p0.x) == sign(l0l1.x * l0p1.y - l0l1.y * l0p1.x);
}

// t0, t1 and t2 are position of triangle vertices
inline vec3 BarycentricCoordinates(const vec2 &t0, const vec2 &t1, const vec2 &t2, const vec2 &p)
{
	vec2 e0 = t1 - t0;
	vec2 e1 = t2 - t0;
	vec2 e2 = p - t0;

	float d00 = dot(e0, e0);
	float d01 = dot(e0, e1);
	float d11 = dot(e1, e1);
	float d20 = dot(e2, e0);
	float d21 = dot(e2, e1);
	float denom = d00 * d11 - d01 * d01;

	vec3 bary;
	bary.x = (d11 * d20 - d01 * d21) / denom;
	bary.y = (d00 * d21 - d01 * d20) / denom;
	bary.z = 1.0f - bary.x - bary.y;

	return bary;
}

// t0, t1 and t2 are position of triangle vertices
inline bool InsideTriangle(const vec2 &t0, const vec2 &t1, const vec2 &t2, const vec2 &p)
{
	vec3 bary = BarycentricCoordinates(t0, t1, t2, p);
	return (bary.x >= 0.0f && bary.y >= 0.0f && bary.x + bary.y < 1.0f);
}

// t0, t1 and t2 are position of quad vertices
inline bool InsideQuad(const vec2 &q0, const vec2 &q1, const vec2 &q2, const vec2 &q3, const vec2 &p)
{
	if (SameSide(q1, q2, p, q0))
		return InsideTriangle(q0, q1, q2, p);
	else
		return InsideTriangle(q1, q2, q3, p);
}

/* TEEUNIVERSE END ****************************************************/

static void Rotate(vec2 *pCenter, vec2 *pPoint, float Rotation)
{
	float x = pPoint->x - pCenter->x;
	float y = pPoint->y - pCenter->y;
	pPoint->x = (x * cosf(Rotation) - y * sinf(Rotation) + pCenter->x);
	pPoint->y = (x * sinf(Rotation) + y * cosf(Rotation) + pCenter->y);
}

int CCollision::GetZoneValueAt(int ZoneHandle, float x, float y)
{
	if (!m_pLayers->ZoneGroup())
		return 0;

	if (ZoneHandle < 0 || ZoneHandle >= m_Zones.size())
		return 0;

	int Index = 0;

	for (int i = 0; i < m_Zones[ZoneHandle].size(); i++)
	{
		int l = m_Zones[ZoneHandle][i];

		CMapItemLayer *pLayer = m_pLayers->GetLayer(m_pLayers->ZoneGroup()->m_StartLayer + l);
		if (pLayer->m_Type == LAYERTYPE_TILES)
		{
			CMapItemLayerTilemap *pTLayer = (CMapItemLayerTilemap *)pLayer;

			CTile *pTiles = (CTile *)m_pLayers->Map()->GetData(pTLayer->m_Data);

			int Nx = clamp(round_to_int(x) / 32, 0, pTLayer->m_Width - 1);
			int Ny = clamp(round_to_int(y) / 32, 0, pTLayer->m_Height - 1);

			int TileIndex = (pTiles[Ny * pTLayer->m_Width + Nx].m_Index > 128 ? 0 : pTiles[Ny * pTLayer->m_Width + Nx].m_Index);
			if (TileIndex > 0)
				Index = TileIndex;
		}
		else if (pLayer->m_Type == LAYERTYPE_QUADS)
		{
			CMapItemLayerQuads *pQLayer = (CMapItemLayerQuads *)pLayer;

			const CQuad *pQuads = (const CQuad *)m_pLayers->Map()->GetDataSwapped(pQLayer->m_Data);

			for (int q = 0; q < pQLayer->m_NumQuads; q++)
			{
				vec2 Position(0.0f, 0.0f);
				float Angle = 0.0f;
				if (pQuads[q].m_PosEnv >= 0)
				{
					GetAnimationTransform(m_AnimationTime, pQuads[q].m_PosEnv, m_pLayers, Position, Angle);
				}

				vec2 p0 = Position + vec2(fx2f(pQuads[q].m_aPoints[0].x), fx2f(pQuads[q].m_aPoints[0].y));
				vec2 p1 = Position + vec2(fx2f(pQuads[q].m_aPoints[1].x), fx2f(pQuads[q].m_aPoints[1].y));
				vec2 p2 = Position + vec2(fx2f(pQuads[q].m_aPoints[2].x), fx2f(pQuads[q].m_aPoints[2].y));
				vec2 p3 = Position + vec2(fx2f(pQuads[q].m_aPoints[3].x), fx2f(pQuads[q].m_aPoints[3].y));

				if (Angle != 0)
				{
					vec2 center(fx2f(pQuads[q].m_aPoints[4].x), fx2f(pQuads[q].m_aPoints[4].y));
					Rotate(&center, &p0, Angle);
					Rotate(&center, &p1, Angle);
					Rotate(&center, &p2, Angle);
					Rotate(&center, &p3, Angle);
				}

				if (InsideQuad(p0, p1, p2, p3, vec2(x, y)))
				{
					Index = pQuads[q].m_ColorEnvOffset;
				}
			}
		}
	}

	return Index;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	for (int i = 0; i < m_Width * m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;

		if (Index > 128)
			continue;

		switch (Index)
		{
		case TILE_DEATH:
			m_pTiles[i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pTiles[i].m_Index = COLFLAG_SOLID | COLFLAG_NOHOOK;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x / 32, 0, m_Width - 1);
	int Ny = clamp(y / 32, 0, m_Height - 1);

	return m_pTiles[Ny * m_Width + Nx].m_Index > 128 ? 0 : m_pTiles[Ny * m_Width + Nx].m_Index;
}

bool CCollision::IsTileSolid(int x, int y)
{
	return GetTile(x, y) & COLFLAG_SOLID;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance + 1);
	vec2 Last = Pos0;

	for (int i = 0; i < End; i++)
	{
		float a = i / Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if (CheckPoint(Pos.x, Pos.y))
		{
			if (pOutCollision)
				*pOutCollision = Pos;
			if (pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if (pOutCollision)
		*pOutCollision = Pos1;
	if (pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if (pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if (CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if (CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if (pBounces)
				(*pBounces)++;
			Affected++;
		}

		if (CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if (pBounces)
				(*pBounces)++;
			Affected++;
		}

		if (Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if (CheckPoint(Pos.x - Size.x, Pos.y - Size.y))
		return true;
	if (CheckPoint(Pos.x + Size.x, Pos.y - Size.y))
		return true;
	if (CheckPoint(Pos.x - Size.x, Pos.y + Size.y))
		return true;
	if (CheckPoint(Pos.x + Size.x, Pos.y + Size.y))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if (Distance > 0.00001f)
	{
		// vec2 old_pos = pos;
		float Fraction = 1.0f / (float)(Max + 1);
		for (int i = 0; i <= Max; i++)
		{
			// float amount = i/(float)max;
			// if(max == 0)
			// amount = 0;

			vec2 NewPos = Pos + Vel * Fraction; // TODO: this row is not nice

			if (TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if (TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if (TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if (Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}

bool CCollision::ModifTile(ivec2 pos, int group, int layer, int tile, int flags, int reserved)
{
	CMapItemGroup *pGroup = m_pLayers->GetGroup(group);
	CMapItemLayer *pLayer = m_pLayers->GetLayer(pGroup->m_StartLayer + layer);
	if (pLayer->m_Type != LAYERTYPE_TILES)
		return false;

	CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
	int TotalTiles = pTilemap->m_Width * pTilemap->m_Height;
	int tpos = (int)pos.y * pTilemap->m_Width + (int)pos.x;
	if (tpos < 0 || tpos >= TotalTiles)
		return false;

	if (pTilemap != m_pLayers->GameLayer())
	{
		CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(pTilemap->m_Data));
		pTiles[tpos].m_Flags = flags;
		pTiles[tpos].m_Index = tile;
		pTiles[tpos].m_Reserved = reserved;
	}
	else
	{
		m_pTiles[tpos].m_Index = tile;
		m_pTiles[tpos].m_Flags = flags;
		m_pTiles[tpos].m_Reserved = reserved;

		switch (tile)
		{
		case TILE_DEATH:
			m_pTiles[tpos].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[tpos].m_Index = COLFLAG_SOLID;
			break;
		case TILE_DAMAGEFLUID:
			m_pTiles[tpos].m_Index = COLFLAG_DAMAGEFLUID;
			break;
		case TILE_RAMP_LEFT:
			m_pTiles[tpos].m_Index = COLFLAG_RAMP_LEFT;
			break;
		case TILE_RAMP_RIGHT:
			m_pTiles[tpos].m_Index = COLFLAG_RAMP_RIGHT;
			break;
		case TILE_ROOFSLOPE_LEFT:
			m_pTiles[tpos].m_Index = COLFLAG_ROOFSLOPE_LEFT;
			break;
		case TILE_ROOFSLOPE_RIGHT:
			m_pTiles[tpos].m_Index = COLFLAG_ROOFSLOPE_RIGHT;
			break;
		default:
			if (tile <= 128)
				m_pTiles[tpos].m_Index = 0;
		}
	}

	return true;
}

int CCollision::GetLowestPoint()
{
	if (!m_pLayers || !m_Height)
		return 0;

	if (!m_LowestPoint)
		for (int y = m_Height - 1; y > 0; y--)
			for (int x = 0; x < m_Width; x++)
			{
				if (!IsTileSolid(x * 32, y * 32))
				{
					m_LowestPoint = (y + 1) * 32;
					return m_LowestPoint;
				}
			}

	return m_LowestPoint;
}

float CCollision::GetGlobalAcidLevel()
{
	return GetLowestPoint() + m_Time;
}

vec2 CCollision::GetClosestWaypointPos(vec2 Pos)
{
	CWaypoint *Wp = GetClosestWaypoint(Pos);

	if (Wp)
		return Wp->m_Pos;

	return vec2(0, 0);
}

CWaypoint *CCollision::GetClosestWaypoint(vec2 Pos)
{
	CWaypoint *W = NULL;
	float Dist = 9000;

	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i])
		{
			if (m_GlobalAcid && GetGlobalAcidLevel() < Pos.y)
				continue;

			int d = distance(m_apWaypoint[i]->m_Pos, Pos);

			if (d < Dist && d < 800)
			{
				if (!FastIntersectLine(m_apWaypoint[i]->m_Pos, Pos) || Dist == 9000)
				{
					W = m_apWaypoint[i];
					Dist = d;
				}
			}
		}
	}

	return W;
}

void CCollision::SetWaypointCenter(vec2 Position)
{
	m_pCenterWaypoint = GetClosestWaypoint(Position);

	// clear path weights
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i])
			m_apWaypoint[i]->m_PathDistance = 0;
	}

	if (m_pCenterWaypoint)
		m_pCenterWaypoint->SetCenter();
}

void CCollision::GenerateWaypoints()
{
	ClearWaypoints();

	for (int x = 2; x < m_Width - 2; x++)
	{
		for (int y = 2; y < m_Height - 2; y++)
		{
			if (m_pTiles[y * m_Width + x].m_Index == 214)
			{
				AddWaypoint(vec2(x, y));
				continue;
			}

			if (m_pTiles[y * m_Width + x].m_Index && m_pTiles[y * m_Width + x].m_Index < 130)
				continue;

			// find all outer corners
			if ((IsTileSolid((x - 1) * 32, (y - 1) * 32) && !IsTileSolid((x - 1) * 32, (y - 0) * 32) && !IsTileSolid((x - 0) * 32, (y - 1) * 32)) ||
				(IsTileSolid((x - 1) * 32, (y + 1) * 32) && !IsTileSolid((x - 1) * 32, (y - 0) * 32) && !IsTileSolid((x - 0) * 32, (y + 1) * 32)) ||
				(IsTileSolid((x + 1) * 32, (y + 1) * 32) && !IsTileSolid((x + 1) * 32, (y - 0) * 32) && !IsTileSolid((x - 0) * 32, (y + 1) * 32)) ||
				(IsTileSolid((x + 1) * 32, (y - 1) * 32) && !IsTileSolid((x + 1) * 32, (y - 0) * 32) && !IsTileSolid((x - 0) * 32, (y - 1) * 32)))
			{
				// outer corner found -> create a waypoint

				// check validity (solid tiles under the corner)
				/*
				bool Found = false;
				for (int i = 0; i < 20; ++i)
					if (IsTileSolid(x*32, (y+i)*32))
						Found = true;
					*/

				bool Found = true;

				// count slopes
				int Slopes = 0;

				for (int xx = -2; xx <= 2; xx++)
					for (int yy = -2; yy <= 2; yy++)
						if (GetTile((x + xx) * 32, (y + yy) * 32) >= COLFLAG_RAMP_LEFT)
							Slopes++;

				if (Found && Slopes < 3)
					AddWaypoint(vec2(x, y));
			}
			else
				// find all inner corners
				if ((IsTileSolid((x + 1) * 32, y * 32) || IsTileSolid((x - 1) * 32, y * 32)) && (IsTileSolid(x * 32, (y - 1) * 32) || IsTileSolid(x * 32, (y + 1) * 32)))
				{
					// inner corner found -> create a waypoint
					// AddWaypoint(vec2(x, y), true);

					// check validity (solid tiles under the corner)
					bool Found = false;
					for (int i = 0; i < 20; ++i)
						if (IsTileSolid(x * 32, (y + i) * 32))
							Found = true;

					// count slopes
					int Slopes = 0;

					for (int xx = -2; xx <= 2; xx++)
						for (int yy = -2; yy <= 2; yy++)
							if (GetTile((x + xx) * 32, (y + yy) * 32) >= COLFLAG_RAMP_LEFT)
								Slopes++;

					// too tight spots to go
					if ((IsTileSolid((x)*32, (y - 1) * 32) && IsTileSolid((x)*32, (y + 1) * 32)) ||
						(IsTileSolid((x - 1) * 32, (y)*32) && IsTileSolid((x + 1) * 32, (y)*32)))
						Found = false;

					if (Found && Slopes < 3)
						AddWaypoint(vec2(x, y));
				}
		}
	}

	bool KeepGoing = true;
	int i = 0;

	while (KeepGoing && i++ < 10)
	{
		ConnectWaypoints();
		KeepGoing = GenerateSomeMoreWaypoints();
	}
	ConnectWaypoints();

	RemoveClosedAreas();
}

// create a new waypoints between connected, far apart ones
bool CCollision::GenerateSomeMoreWaypoints()
{
	bool Result = false;

	for (int i = 0; i < m_WaypointCount; i++)
	{
		for (int j = 0; j < m_WaypointCount; j++)
		{
			if (m_apWaypoint[i] && m_apWaypoint[j] && m_apWaypoint[i]->Connected(m_apWaypoint[j]))
			{
				if (abs(m_apWaypoint[i]->m_X - m_apWaypoint[j]->m_X) > 20 && m_apWaypoint[i]->m_Y == m_apWaypoint[j]->m_Y)
				{
					int x = (m_apWaypoint[i]->m_X + m_apWaypoint[j]->m_X) / 2;

					if (IsTileSolid(x * 32, (m_apWaypoint[i]->m_Y + 1) * 32) || IsTileSolid(x * 32, (m_apWaypoint[i]->m_Y - 1) * 32))
					{
						AddWaypoint(vec2(x, m_apWaypoint[i]->m_Y));
						Result = true;
					}
				}

				if (abs(m_apWaypoint[i]->m_Y - m_apWaypoint[j]->m_Y) > 30 && m_apWaypoint[i]->m_X == m_apWaypoint[j]->m_X)
				{
					int y = (m_apWaypoint[i]->m_Y + m_apWaypoint[j]->m_Y) / 2;

					if (IsTileSolid((m_apWaypoint[i]->m_X + 1) * 32, y * 32) || IsTileSolid((m_apWaypoint[i]->m_X - 1) * 32, y * 32))
					{
						AddWaypoint(vec2(m_apWaypoint[i]->m_X, y));
						Result = true;
					}
				}

				m_apWaypoint[i]->Unconnect(m_apWaypoint[j]);
			}
		}
	}

	return Result;
}

void CCollision::AddWeight(vec2 Pos, int Weight)
{
	CWaypoint *Wp = GetClosestWaypoint(Pos);

	if (Wp)
		Wp->AddWeight(Weight);
}

vec2 CCollision::GetRandomWaypointPos()
{
	int n = 0;
	while (n++ < 10)
	{
		int i = rand() % m_WaypointCount;

		if (m_apWaypoint[i])
			return m_apWaypoint[i]->m_Pos;
	}

	return vec2(0, 0);
}

int CCollision::FastIntersectLine(vec2 Pos0, vec2 Pos1)
{
	// float Distance = distance(Pos0, Pos1) / 4.0f;
	float Distance = distance(Pos0, Pos1);
	int End(Distance + 1);

	for (int i = 0; i < End; i++)
	{
		float a = i / Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if (CheckPoint(Pos.x, Pos.y))
			return GetCollisionAt(Pos.x, Pos.y);
	}
	return 0;
}

void CCollision::ClearWaypoints()
{
	m_WaypointCount = 0;

	for (int i = 0; i < MAX_WAYPOINTS; i++)
	{
		if (m_apWaypoint[i])
			delete m_apWaypoint[i];

		m_apWaypoint[i] = NULL;
	}

	m_pCenterWaypoint = NULL;
}

void CCollision::RemoveClosedAreas()
{
	return;
	for (int i = 0; i < MAX_WAYPOINTS; i++)
	{
		if (m_apWaypoint[i])
		{
			m_apWaypoint[i]->CalculateAreaSize(0);

			if (m_apWaypoint[i]->GetAreaSize() < 1000)
			{
				m_apWaypoint[i]->m_ToBeDeleted = true;
			}
		}
	}

	for (int i = 0; i < MAX_WAYPOINTS; i++)
	{
		if (m_apWaypoint[i] && m_apWaypoint[i]->m_ToBeDeleted)
		{
			m_WaypointCount--;
			m_apWaypoint[i]->ClearConnections();
			delete m_apWaypoint[i];
			m_apWaypoint[i] = NULL;
		}
	}
}

void CCollision::AddWaypoint(vec2 Position, bool InnerCorner)
{
	if (m_WaypointCount >= MAX_WAYPOINTS)
		return;

	m_apWaypoint[m_WaypointCount] = new CWaypoint(Position, InnerCorner);
	m_WaypointCount++;
}

CWaypoint *CCollision::GetWaypointAt(int x, int y)
{
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i])
		{
			if (m_apWaypoint[i]->m_X == x && m_apWaypoint[i]->m_Y == y)
				return m_apWaypoint[i];
		}
	}
	return NULL;
}

void CCollision::ConnectWaypoints()
{
	m_ConnectionCount = 0;

	// clear existing connections
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (!m_apWaypoint[i])
			continue;

		m_apWaypoint[i]->ClearConnections();
	}

	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (!m_apWaypoint[i])
			continue;

		int x, y;

		x = m_apWaypoint[i]->m_X - 1;
		y = m_apWaypoint[i]->m_Y;

		// find waypoints at left
		while (!m_pTiles[y * m_Width + x].m_Index || m_pTiles[y * m_Width + x].m_Index >= 128)
		{
			CWaypoint *W = GetWaypointAt(x, y);

			if (W)
			{
				if (m_apWaypoint[i]->Connect(W))
					m_ConnectionCount++;
				break;
			}

			// if (!IsTileSolid(x*32, (y-1)*32) && !IsTileSolid(x*32, (y+1)*32))
			if (!IsTileSolid(x * 32, (y + 1) * 32))
				break;

			x--;
		}

		x = m_apWaypoint[i]->m_X;
		y = m_apWaypoint[i]->m_Y - 1;

		int n = 0;

		// find waypoints at up
		// bool SolidFound = false;
		while ((!m_pTiles[y * m_Width + x].m_Index || m_pTiles[y * m_Width + x].m_Index >= 128) && n++ < 10)
		{
			CWaypoint *W = GetWaypointAt(x, y);

			// if (IsTileSolid((x+1)*32, y*32) || IsTileSolid((x+1)*32, y*32))
			//	SolidFound = true;

			// if (W && SolidFound)
			if (W)
			{
				if (m_apWaypoint[i]->Connect(W))
					m_ConnectionCount++;
				break;
			}

			y--;
		}
	}

	// connect to near, visible waypoints
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i] && m_apWaypoint[i]->m_InnerCorner)
			continue;

		for (int j = 0; j < m_WaypointCount; j++)
		{
			if (m_apWaypoint[j] && m_apWaypoint[j]->m_InnerCorner)
				continue;

			if (m_apWaypoint[i] && m_apWaypoint[j] && !m_apWaypoint[i]->Connected(m_apWaypoint[j]))
			{
				float Dist = distance(m_apWaypoint[i]->m_Pos, m_apWaypoint[j]->m_Pos);

				if (Dist < 1000 && !IntersectLine(m_apWaypoint[i]->m_Pos, m_apWaypoint[j]->m_Pos, NULL, NULL) &&
					m_apWaypoint[i]->m_Pos.y != m_apWaypoint[j]->m_Pos.y)
				{
					if (m_apWaypoint[i]->Connect(m_apWaypoint[j]))
						m_ConnectionCount++;
				}
			}
		}
	}
}