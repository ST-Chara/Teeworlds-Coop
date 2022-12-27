#ifndef GAME_SERVER_AI_DEF_DEF_ALIEN1_AI_H
#define GAME_SERVER_AI_DEF_DEF_ALIEN1_AI_H
#include <game/server/ai.h>
#include <game/server/gamecontext.h>

class CAIdefalien1 : public CAI
{
public:
	CAIdefalien1(CGameContext *pGameServer, CPlayer *pPlayer);

	virtual void DoBehavior();
	void OnCharacterSpawn(class CCharacter *pChr);
	void ReceiveDamage(int CID, int Dmg);

private:
	int m_SkipMoveUpdate;
	bool m_Triggered;
	vec2 m_StartPos;
	
	int m_ShockTimer;
};

#endif
