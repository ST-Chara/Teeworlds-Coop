#include <engine/shared/config.h>

#include <game/server/ai.h>
#include <game/server/Core/GameEntities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#include "bunny1_ai.h"


CAIbunny1::CAIbunny1(CGameContext *pGameServer, CPlayer *pPlayer, int Level)
: CAI(pGameServer, pPlayer)
{
	m_SkipMoveUpdate = 0;
	m_StartPos = vec2(0, 0);
	m_ShockTimer = 0;
	m_Triggered = false;
	m_TriggerLevel = 5 + rand()%10;
	
	m_Skin = SKIN_BUNNY1+min(Level, 4);
	Player()->SetCustomSkin(m_Skin);
}


void CAIbunny1::OnCharacterSpawn(CCharacter *pChr)
{
	CAI::OnCharacterSpawn(pChr);
	
	int Level = g_Config.m_SvMapGenLevel;
	
	m_WaypointDir = vec2(0, 0);
	m_PowerLevel = 8;
	
	m_StartPos = Player()->GetCharacter()->m_Pos;
	m_TargetPos = Player()->GetCharacter()->m_Pos;
	
	if (frandom() < 0.4f)
		pChr->GetPlayer()->IncreaseGold(frandom()*4);
		
	if (m_Skin == SKIN_FOXY1)
	{
		pChr->GiveWeapon(GameServer()->NewWeapon(GetChargedWeapon(GetModularWeapon(4, 1), 2)));
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_SHIELD)));
		
		pChr->SetHealth(80+min(Level*5.0f, 320.0f));
		m_PowerLevel = 12;
		m_TriggerLevel = 15 + rand()%5;
	}
	else if (m_Skin == SKIN_BUNNY3)
	{
		if (frandom() < 0.5f)
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(1, 3)));
		else
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(3, 1)));
		
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_SHIELD)));
		
		pChr->SetHealth(80+min(Level*5.0f, 320.0f));
		m_PowerLevel = 12;
		m_TriggerLevel = 15 + rand()%5;
	}
	else if (m_Skin == SKIN_BUNNY4)
	{
		if (frandom() < 0.5f)
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(6, 6)));
		else
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(6, 7)));
		
		m_AttackOnDamage = true;
		
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_SHIELD)));
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_SHIELD)));
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_MASK2)));
		
		pChr->SetHealth(80+min(Level*5.0f, 320.0f));
		m_PowerLevel = 14;
		m_TriggerLevel = 15 + rand()%5;
	}
	else if (m_Skin == SKIN_BUNNY2)
	{
		if (frandom() < 0.5f)
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(1, 4)));
		else
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(5, 6)));
		
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_GRENADE2)));
		pChr->GiveWeapon(GameServer()->NewWeapon(GetStaticWeapon(SW_SHIELD)));
		
		pChr->SetHealth(80+min(Level*4.0f, 320.0f));
		m_PowerLevel = 12;
		m_TriggerLevel = 15 + rand()%5;
	}
	else
	{
		if (frandom() < 0.5f)
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(1, 4)));
		else
			pChr->GiveWeapon(GameServer()->NewWeapon(GetModularWeapon(5, 6)));
		
		pChr->SetHealth(40+min(Level*3.0f, 220.0f));
	}
	
	m_ShockTimer = 10;
		
	if (!m_Triggered)
		m_ReactionTime = 100;
}


void CAIbunny1::ReceiveDamage(int CID, int Dmg)
{
	//if (CID >= 0 && frandom() < Dmg*0.03f)
		m_Triggered = true;
	
	if (m_AttackOnDamage)
	{
		m_Attack = 1;
		m_InputChanged = true;
		m_AttackOnDamageTick = GameServer()->Server()->Tick() + GameServer()->Server()->TickSpeed();
	}
}


void CAIbunny1::DoBehavior()
{
	m_Attack = 0;
	
	if (m_ShockTimer > 0 && m_ShockTimer--)
	{
		m_ReactionTime = 1 + frandom()*3;
		return;
	}
	
	HeadToMovingDirection();
	SeekClosestEnemyInSight();
	//bool Shooting = false;
	
	// if we see a player
	if (m_EnemiesInSight > 0)
	{
		ReactToPlayer();
		//m_Triggered = true;
		
		if (!m_MoveReactTime)
			m_MoveReactTime++;
		
		if (ShootAtClosestEnemy())
		{
			//Shooting = true;
			
			if (WeaponShootRange() - m_PlayerDistance > 200)
			{
				m_TargetPos = normalize(m_Pos - m_PlayerPos) * WeaponShootRange();
				GameServer()->Collision()->IntersectLine(m_Pos, m_TargetPos, 0x0, &m_TargetPos);
			}
		}
		else
		{
			if (SeekClosestEnemy())
			{
				m_TargetPos = m_PlayerPos;

				if (WeaponShootRange() - m_PlayerDistance > 200)
				{
					m_TargetPos = normalize(m_Pos - m_PlayerPos) * WeaponShootRange();
					GameServer()->Collision()->IntersectLine(m_Pos, m_TargetPos, 0x0, &m_TargetPos);
				}
			}
		}
	}
	else if (!m_Triggered)
	{
		m_TargetPos = m_StartPos;
	}
	else
	{
		// triggered, but no enemies in sight
		ShootAtClosestBuilding();
		ShootAtBlocks();
		
		if (SeekClosestEnemy())
			m_TargetPos = m_PlayerPos;
	}

	if (abs(m_Pos.x - m_TargetPos.x) < 40 && abs(m_Pos.y - m_TargetPos.y) < 40)
	{
		// stand still
		m_Move = 0;
		m_Jump = 0;
		m_Hook = 0;
	}
	else
	{
		if (!m_MoveReactTime || m_MoveReactTime++ > 9)
		{
			if (UpdateWaypoint())
			{
				MoveTowardsWaypoint();
			}
			else
			{
				m_WaypointPos = m_TargetPos;
				MoveTowardsWaypoint(true);
			}
		}
	}
	
	Player()->GetCharacter()->m_SkipPickups = 999;
	RandomlyStopShooting();
	
	if (m_AttackOnDamageTick > GameServer()->Server()->Tick())
		m_Attack = 1;
	
	// next reaction in
	m_ReactionTime = 1 + rand()%3;
}
