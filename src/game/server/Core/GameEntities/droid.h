#ifndef GAME_SERVER_ENTITIES_DROID_H
#define GAME_SERVER_ENTITIES_DROID_H

#include <game/server/entity.h>

const int DroidPhysSize = 60;

enum
{
	DROIDSTATUS_IDLE=0,
	DROIDSTATUS_HURT,
	DROIDSTATUS_ELECTRIC,
	DROIDSTATUS_TERMINATED,
	NUM_DROIDSTATUSS
};

enum
{
	DROIDTYPE_WALKER=0,
	DROIDTYPE_STAR,
	DROIDTYPE_CRAWLER,
	DROIDTYPE_BOSSCRAWLER,
	DROIDTYPE_FLY,
	NUM_DROIDTYPES
};

enum
{
	DROIDANIM_IDLE=0,
	DROIDANIM_MOVE,
	DROIDANIM_ATTACK,
	DROIDANIM_JUMPATTACK,
	NUM_DROIDANIMS
};

class CDroid : public CEntity
{
public:
	CDroid(CGameWorld *pGameWorld, vec2 Pos, int Type);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	virtual void TakeDamage(vec2 Force, int Dmg, int From, vec2 Pos, int Weapon);
	int m_Health;

	vec2 m_Center;
	int m_Type;

	enum State
	{
		IDLE,
		MOVE,
		TURN,
		TAKEOFF,
		FLY,
	};

protected:
	int m_State;
	int m_NextState;
	int m_StateChangeTick;

	int m_Anim;
	int m_Mode;

	int m_AttackTimer;

	vec2 m_Vel;

	int m_FlyTargetTick;

	vec2 m_Target;
	vec2 m_NewTarget;

	int m_Status;
	int m_Dir;

	vec2 m_StartPos;

	int m_DamageTakenTick;
	int m_DeathTick;

	int m_FireDelay;
	int m_FireCount;

	void SetState(int State);

	virtual bool FindTarget();
	virtual bool Target();
	virtual void Fire();

	int m_TargetIndex;
	int m_AttackTick;
	int m_TargetTimer;

	int m_ReloadTimer;
};

#endif
