/*
	Copyright 2008-2009 Ambient.5

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "ServerSocket.h"
#include "ItemManager.h"
#include "inventory.h"
#include "object.h"
#include "classdef.h"
#include "PositionHandler.h"

enum PLAYER_STATUS
{
	PLAYER_EMPTY = 0,
	PLAYER_CONNECTED = 1,
	PLAYER_LOGGED = 2,
	PLAYER_PLAYING = 3,
	PLAYER_LOGGING_OUT = 4,
};

enum OBJECTS
{
	_OBJECT_EMPTY	= -1,
	_OBJECT_USER	= 1,
	_OBJECT_MONSTER	= 2,
	_OBJECT_NPC		= 3,
	_OBJECT_DUMMY	= 4,
	_OBJECT_ITEM		= 5,
};

enum PLAYER_WARDROBE
{
	RIGHT_HAND	= 0,	// <--
	LEFT_HAND	= 1,	// <--
	HELMET		= 2,	// <--
	ARMOR		= 3,	// <--
	PANTS		= 4,	// <--
	GLOVES		= 5,	// <--
	BOOTS		= 6,	// <--
	WINGS		= 7,	// <--
	GUARDIAN	= 8,	// <--
	AMULET		= 9,	// <--
	RING_01		= 10,	// <--
	RING_02		= 11,	// <--
};

struct SC_CHARINFO
{
	std::string name;
	uint8 Class;
	uint8 ChangeUp;
	uint16 level;
	//std::vector<uint32> item_guids;
	//DATA_ITEM temp_inv[12];
};

struct DATA_CHARACTER
{
	uint32 Position; //mapx mapy map dir
	uint64 Exp;
	uint8 LevelUpPoint;
	uint16 Str;
	uint16 Dex;
	uint16 Vit;
	uint16 Energy;
	uint16 Leadership;
	uint32 Life;
	uint32 Mana;
	uint32 Shield;
	uint32 BP;
	uint32 Money;
	uint8 PkLevel;
	uint8 AddPoint;
	uint8 MaxAddPoint;
	uint8 MinusPoint;
	uint8 MaxMinusPoint;
	std::vector<uint32> item_guids;
	//std::vector<uint8> spell_data;
};

class CPlayer : public CObject
{
public:
	uint32 dbuid;
	ServerSocket* socket;
	PLAYER_STATUS status;
	uint32 tick_count;
	uint32 last_save_time;
	uint32 check_time;

	CPositionHandler position;
	/* obsolete */
	size_t path_count;
	std::vector<MovePoint> path;
	/* obsolete */

	char account[10];
	char name[10];
	unsigned char charset[18];
	unsigned char failed_attempts;
	SC_CHARINFO sc_charinfo[5];
	CInventory inventory;

	size_t send_serial;

	unsigned char rest;

	uint64 experience;
	uint16 leveluppoint;
	uint32 money;
	uint8 pklevel;
	uint32 gmlevel;
	uint8 addpoint;
	uint8 maxaddpoint;
	uint8 minuspoint;
	uint8 maxminuspoint;
	uint8 Class;
	uint8 changeup;

	/*data from calc start*/
	bool armed;
	unsigned char SkillLongSpearChange;

	//here goes specific stat bonuses as it made by webzen, better implement it later by mod auras
	uint32 add_life;
	uint32 add_mana;
	uint32 extra_gold;
	uint32 life_steal;
	uint32 mana_steal;
	uint32 damage_reflect;
	uint32 damage_absorb;
	uint32 _SkillLongSpearChange;
	uint32 addstrength;
	uint32 adddexterity;
	uint32 addvitality;
	uint32 addenergy;
	uint32 addbp;
	uint32 addshield;
	uint8 res[7];

	uint32 ad_left_min;
	uint32 ad_left_max;
	uint32 ad_right_min;
	uint32 ad_right_max;
	uint32 spell_damage_min;
	uint32 spell_damage_max;
	uint32 crit_damage;
	uint32 excellent_damage;
	/*data from calc end*/

	CPlayer();
	void Send(unsigned char* buffer, size_t len);
	void Close();
	void SetStatus(unsigned char status);
	void SendInventory();
	//int LoadCharacters();
	uint32 LoadSelectionScreen();
	bool LoadCharacterData(SC_CHARINFO* info);
	//void DeleteFromViewport(void* obj);
	void AssignItem(DATA_ITEM* item);
	bool CheckPacketTime();
	bool CheckPosition();
	void SetPosition(uint8 x, uint8 y);
	bool SavePlayer();
	void CookCharset();
	void LoadItemToInventory(DATA_ITEM * ditem);
	void Calculate();
	const CPositionHandler::MovePoint GetCurrentPosition();
};

#endif