/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <teeuniverses/components/localization.h>
#include <engine/storage.h> // MapGen

#include <game/layers.h>
#include <game/voting.h>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"
#include "mapgen.h"

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/

struct CPlayerSpecData
{
	CPlayerSpecData()
	{
		m_Kits = 0;
		m_WeaponSlot = 0;
		
		for (int i = 0; i < 4; i++)
			m_aWeapon[i] = 0;
	}
	
	int m_WeaponSlot;
	int m_aWeapon[4];
	int m_Kits;
};


class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;
	// MapGen
	CMapGen m_MapGen;
	IStorage *m_pStorage;

	static void ConsoleOutputCallback_Chat(const char *pLine, void *pUser);

	static void ConLanguage(IConsole::IResult *pResult, void *pUserData);
	static void ConAbout(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;

public:
	int m_ChatResponseTargetID;
	int m_ChatPrintCBIndex;
public:
	int m_ZoneHandle_TeeWorlds;

public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	virtual class CLayers *Layers() { return &m_Layers; }
	// MapGen
	IStorage *Storage() const { return m_pStorage; }
	CMapGen *MapGen() { return &m_MapGen; }

	CGameContext();
	~CGameContext();

	void Clear();

	void ReloadMap();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
	CGameWorld m_World;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, int64_t Mask=-1LL);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, int64_t Mask=-1LL);
	void CreateHammerHit(vec2 Pos, int64_t Mask=-1LL);
	void CreatePlayerSpawn(vec2 Pos, int64_t Mask=-1LL);
	void CreateDeath(vec2 Pos, int Who, int64_t Mask=-1LL);
	void CreateSound(vec2 Pos, int Sound, int64_t Mask=-1LL);
	void CreateSoundGlobal(int Sound, int Target=-1);

	class CWeapon *NewWeapon(int Part1, int Part2);
	class CWeapon *NewWeapon(int Weapon);

	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendMotd(int To, const char* pText);
	void SendChatTarget(int To, const char *pText, ...);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);
	void SendBroadcast_VL(const char *pText, int ClientID, ...);
	void SetClientLanguage(int ClientID, const char *pLanguage);
	void CreateFlameHit(vec2 Pos);
	void CreateBuildingHit(vec2 Pos);


	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	//
	void SwapTeams();

	//
	void UpdateAI();

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID);
	virtual void OnClientEnter(int ClientID);
	virtual void OnClientDrop(int ClientID, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);

	virtual void OnSetAuthed(int ClientID,int Level);
	
	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();

	/* Ninslash Start */
	// MapGen
	virtual void SaveMap(const char *path);

	vec2 GetNearHumanSpawnPos(bool AllowVision = false);
	vec2 GetFarHumanSpawnPos(bool AllowVision = false);
	int DistanceToHuman(vec2 Pos);
	
	void AddBot();
	void KickBots();
	void KickBot(int ClientID);
	
	bool IsBot(int ClientID);
	bool IsHuman(int ClientID);
	
	int m_BroadcastLockTick;
	
	bool m_ShowWaypoints;
	bool m_ShowAiState;

	virtual void GetAISkin(int Skin, bool PVP, int Level = 1);
	virtual void AddZombie();
	virtual bool AIInputUpdateNeeded(int ClientID);
	virtual void AIUpdateInput(int ClientID, int *Data);

	int CountBots(bool SkipSpecialTees = false);
	int CountBotsAlive(bool SkipSpecialTees = false);
	//int CountHumans();
	int CountHumansAlive();
};

inline int64_t CmaskAll() { return -1LL; }
inline int64_t CmaskOne(int ClientID) { return 1LL<<ClientID; }
inline int64_t CmaskAllExceptOne(int ClientID) { return CmaskAll()^CmaskOne(ClientID); }
inline bool CmaskIsSet(int64_t Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }

#endif
