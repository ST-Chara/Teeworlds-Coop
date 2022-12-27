#include <game/server/gamecontext.h>
#include "character.h"
#include "radar.h"

CRadar::CRadar(CGameWorld *pGameWorld, int Type, int ObjectiveID)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_RADAR)
{
	m_ProximityRadius = 12;
	m_Type = Type;
	m_ObjectiveID = ObjectiveID;
	m_TargetPos = vec2(0, 0);
	m_Active = false;
	m_ActiveTick = 0;
	
	GameWorld()->InsertEntity(this);
}

void CRadar::Reset()
{
	//GameServer()->m_World.DestroyEntity(this);
}

void CRadar::Tick()
{
	if (m_Type == RADAR_CHARACTER || m_Type == RADAR_HUMAN)
	{
		CCharacter *pCharacter = GameServer()->GetPlayerChar(m_ObjectiveID);
		
		if (pCharacter && (m_Type == RADAR_HUMAN && !pCharacter->m_IsBot))
		{
			m_TargetPos = pCharacter->m_Pos;
			m_Active = true;
		}
		else
			m_Active = false;
	}
	
	if (m_Type == RADAR_ENEMY)
	{
		if (m_ActiveTick > Server()->Tick())
			m_Active = true;
		else
			m_Active = false;
	}
}

void CRadar::Snap(int SnappingClient)
{
	if (m_Type == RADAR_CHARACTER || m_Type == RADAR_HUMAN)
		if (SnappingClient == m_ObjectiveID)
			return;
	
	CCharacter *pCharacter = GameServer()->GetPlayerChar(SnappingClient);

	if (pCharacter)
		m_Pos = pCharacter->m_Pos;
	else
		return;
	
	if (pCharacter && pCharacter->m_IsBot)
		return;
	
	if(NetworkClipped(SnappingClient) || !m_Active)
		return;

	if (m_Type == RADAR_CHARACTER || m_Type == RADAR_HUMAN)
	{
		CNetObj_Pickup *pRadar = (CNetObj_Pickup *)Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup));
		if(!pRadar)
			return;

		pRadar->m_X = (int)m_TargetPos.x;
		pRadar->m_Y = (int)m_TargetPos.y;
		pRadar->m_Type = POWERUP_WEAPON;
		pRadar->m_Subtype = pCharacter->GetWeaponType(pCharacter->GetWeaponSlot());
	}
	if (m_Type == RADAR_DOOR)
	{
		CNetObj_Pickup *pRadar = (CNetObj_Pickup *)Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup));
		if(!pRadar)
			return;

		pRadar->m_X = (int)m_TargetPos.x;
		pRadar->m_Y = (int)m_TargetPos.y;
		pRadar->m_Type = POWERUP_HEALTH;
	}
	if (m_Type == RADAR_ENEMY)
	{
		CNetObj_Pickup *pRadar = (CNetObj_Pickup *)Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup));
		if(!pRadar)
			return;

		pRadar->m_X = (int)m_TargetPos.x;
		pRadar->m_Y = (int)m_TargetPos.y;
		pRadar->m_Type = POWERUP_ARMOR;
	}
}
