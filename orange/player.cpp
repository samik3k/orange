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

#include "stdafx.h"
#include "Deathway/SimpleModulus/SimpleModulus.h"
#include "DataBase.h"
#include "packets.h"
#include "ItemManager.h"
#include "objectmanager.h"
#include "ItemTemplate.h"
#include "classdef.h"
#include "player.h"

CPlayer::CPlayer()
{
	this->dbuid = 0;
	this->socket = NULL;
	this->status = PLAYER_EMPTY;
	this->type = OBJECT_PLAYER;
	this->tick_count = NULL;
	this->last_save_time = GetTicks();
	this->failed_attempts = NULL;
	this->send_serial = NULL;
	this->viewport.resize(100);
	this->viewport.clear();
	this->last_move_time = GetTicks();
	this->check_time = GetTicks();
	this->pklevel = 0;
	this->rest = 0;

	this->move_speed = 800;

	this->experience = 0;
	this->leveluppoint = 0;
	this->money = 0;
	this->pklevel = 0;
	this->gmlevel = 0;
	this->addpoint = 0;
	this->maxaddpoint = 0;
	this->minuspoint = 0;
	this->maxminuspoint = 0;
	this->Class = 0;
	this->changeup = 0;

	ZeroMemory(this->account, sizeof(this->account));
	ZeroMemory(this->name, sizeof(this->name));
	ZeroMemory(this->charset, sizeof(this->charset));

	/*data from calc start*/
	this->armed = false;
	this->SkillLongSpearChange = 0;
	this->crit_damage = 0;
	this->excellent_damage = 0;
	this->spell_damage_min = 0;
	this->spell_damage_max = 0;
	/*data from calc end*/
}

void CPlayer::Send(unsigned char* buffer, size_t len)
{
	if(this->status == PLAYER_EMPTY)
	{
		return;
	}
	unsigned char in_buffer[16384];
	unsigned char send_buffer[16384];
	ZeroMemory(in_buffer, sizeof(in_buffer));
	ZeroMemory(send_buffer, sizeof(send_buffer));
	memcpy(in_buffer, buffer, len);

	int ret;
	unsigned char btsize;

	unsigned char code = buffer[0];
	switch(code)
	{
	case 0xC1:
		{
			this->socket->SendBuf((const char*)in_buffer, len);
			break;
		}
	case 0xC2:
		{
			this->socket->SendBuf((const char*)in_buffer, len);
			break;
		}
	case 0xC3:
		{
			btsize = in_buffer[1];
			in_buffer[1] = this->send_serial;
			this->send_serial++;

			ret = g_SimpleModulusSC.Encrypt(&send_buffer[2], &in_buffer[1], len - 1);
			send_buffer[0] = 0xC3;
			send_buffer[1] = ret + 2;
			size_t size = ret + 2;
			in_buffer[1] = btsize;
			this->socket->SendBuf((const char*)send_buffer, size);
			break;
		}
	case 0xC4:
		{
			btsize = in_buffer[2];
			in_buffer[2] = this->send_serial;
			this->send_serial++;
			ret = g_SimpleModulusSC.Encrypt(&send_buffer[3], &in_buffer[2], len - 2);
			send_buffer[0] = 0xC4;
			send_buffer[1] = HIBYTE(ret + 3);
			send_buffer[2] = LOBYTE(ret + 3);
			size_t size = ret + 3;
			in_buffer[2] = btsize;
			this->socket->SendBuf((const char*)send_buffer, size);
			break;
		}
	default:
		{
			break;
		}
	}
}

void CPlayer::Close()
{
	ServerSocket * s = this->socket;
	TcpSocket * tcp = dynamic_cast<TcpSocket *>(s);
	if(tcp)
	{
		tcp->SetCloseAndDelete();
	}
}

void CPlayer::SetStatus(unsigned char status)
{
	QSqlQuery q(accounts_db.db);
	accounts_db.LockForWrite();
	q.exec(Query("UPDATE `accounts` SET `status` = %d WHERE `account` = '%s'", status, this->account).c_str());
	accounts_db.Unlock();
}

uint32 CPlayer::LoadSelectionScreen()
{
	ZeroMemory(this->sc_charinfo, sizeof(SC_CHARINFO) * 5);
	QSqlQuery q(accounts_db.db);
	accounts_db.LockForRead();
	//q.exec(Query("SELECT `name`, `class`, `changeup`, `level`, `inventory_guids` FROM `characters` WHERE `account` = '%s';", this->account).c_str());
	q.exec(Query("SELECT `name`, `class`, `changeup`, `level` FROM `characters` WHERE `account` = '%s';", this->account).c_str());
	accounts_db.Unlock();
	SC_CHARINFO * info = NULL;
	uint32 count = 0;
	for(uint32 i = 0; (i < 5) && (q.next()); ++i)
	{
		info = &(this->sc_charinfo[i]);
		info->name.clear();
		info->name = q.value(0).toString().toStdString();
		info->Class = q.value(1).toUInt();
		info->ChangeUp = q.value(2).toUInt();
		info->level = q.value(3).toUInt();
		count++;
	}
	return count;
}

bool CPlayer::LoadCharacterData(SC_CHARINFO *info)
{
	if(!info)
	{
		return false;
	}
	QSqlQuery q(accounts_db.db);
	accounts_db.LockForRead();
	bool result = q.exec(Query("SELECT `position`, `experience`, `leveluppoint`, `strength`, `dexterity`, `vitality`, `energy`, `leadership`, `life`, `mana`, `shield`, `bp`, `money`, `pklevel`, `addpoint`, `maxaddpoint`, `minuspoint`, `maxminuspoint`, `dbuid` FROM `characters` WHERE `account` = '%s' AND `name` = '%s';", this->account, info->name.c_str()).c_str());
	accounts_db.Unlock();
	if(!result)
	{
		return false;
	}
	q.next();
	strcpy_s(this->name, sizeof(this->name), info->name.c_str());
	this->Class = info->Class;
	this->changeup = info->ChangeUp;
	uint32 position = q.value(0).toUInt();
	/*this->x = (uint8)((position >> 24) & 0x00ffffff);
	this->x_old = this->x;
	this->target_x = this->x;
	this->y = (uint8)((position >> 16) & 0x0000ffff);
	this->y_old = this->y;
	this->target_y = this->y;
	this->path.clear();
	MovePoint pt;
	pt.time = GetTicks();
	pt.x = this->x;
	pt.y = this->y;
	this->path.push_back(pt);*/
	this->map = (uint8)((position >> 8) & 0x000000ff);
	uint8 tempx = ((position >> 24) & 0x00ffffff);
	uint8 tempy = ((position >> 16) & 0x0000ffff);
	if(!WorldMap[this->map].FreeToMove(tempx, tempy))
	{
		this->position.SetPosition(130, 130); /* need to make safe zones */
	}
	this->position.SetPosition(tempx, tempy);
	this->dir = (uint8)((position) & 0x000000ff);
	this->experience = q.value(1).toULongLong();
	this->level = info->level;
	this->leveluppoint = q.value(2).toUInt();
	this->strength = q.value(3).toUInt();
	this->dexterity = q.value(4).toUInt();
	this->vitality = q.value(5).toUInt();
	this->energy = q.value(6).toUInt();
	this->leadership = q.value(7).toUInt();
	this->life = q.value(8).toInt();
	this->maxlife = DCInfo.DefClass[this->Class].VitalityToLife * this->vitality + DCInfo.DefClass[this->Class].LevelLife * this->level;
	this->mana = q.value(9).toInt();
	this->maxmana = DCInfo.DefClass[this->Class].EnergyToMana * this->energy + DCInfo.DefClass[this->Class].LevelMana * this->level;
	this->shield = q.value(10).toInt();
	this->maxshield = this->shield; //todo
	this->bp = q.value(11).toInt();
	this->maxbp = this->bp; //todo
	this->money = q.value(12).toUInt();
	this->pklevel = q.value(13).toUInt();
	this->addpoint = q.value(14).toUInt();
	this->maxaddpoint = q.value(15).toUInt();
	this->minuspoint = q.value(16).toUInt();
	this->maxminuspoint = q.value(17).toUInt();
	this->dbuid = q.value(18).toUInt();

	return true;
}

void CPlayer::SendInventory()
{
	PMSG_INVENTORYLISTCOUNT phead;
	ZeroMemory(&phead, sizeof(PMSG_INVENTORYLISTCOUNT));
	unsigned char buffer[4096];
	ZeroMemory(buffer, sizeof(buffer));
	size_t offs = sizeof(PMSG_INVENTORYLISTCOUNT);
	int count = 0;
	for(int i = 0; i < 108; ++i)
	{
		CItem * item = this->inventory[i];
		if(item->IsItem())
		{
			count++;
			PMSG_INVENTORYLIST data;
			data.Pos = i;
			ItemByteConvert(data.ItemInfo, item->type, item->option1, item->option2, item->option3, item->level, (unsigned char)item->durability, item->option_new, item->m_SetOption, item->m_JewelOfHarmonyOption, item->m_ItemOptionEx);
			memcpy(&buffer[offs], &data, sizeof(PMSG_INVENTORYLIST));
			offs += sizeof(PMSG_INVENTORYLIST);
		}
	}
	phead.h.c = 0xC4;
	phead.h.sizeH = HIBYTE(offs);
	phead.h.sizeL = LOBYTE(offs);
	phead.h.headcode = 0xF3;
	phead.subcode = 0x10;
	phead.Count = count;
	memcpy(buffer, &phead, sizeof(PMSG_INVENTORYLISTCOUNT));
	this->Send(buffer, offs);
}

/*void CPlayer::AssignItem(DATA_ITEM *data)
{
	CItem * item =  this->inventory[data->slot];
	item->guid = data->guid;
	item->type = data->type;
	item->level = data->level;
	item->durability = (float)data->durability;
	item->m_Option1 = data->option1;
	item->m_Option2 = data->option2;
	item->m_Option3 = data->option3;
	item->m_NewOption = data->newoption;
	item->m_SetOption = data->setoption;
	item->m_JewelOfHarmonyOption = data->joh_option;
	item->m_ItemOptionEx = data->optionex;
	item->m_PetItem_Exp = data->petitem_exp;
	item->m_PetItem_Level = data->petitem_level;
}*/

bool CPlayer::CheckPosition()
{
	if((abs(this->target_x - this->x_old) < 15) && (abs(this->target_y - this->y_old) < 15))
	{
		return true;
	}
	return false;
}

bool CPlayer::CheckPacketTime()
{
	if((GetTickDiff(this->check_time)) >= 300)
	{
		return true;
	}
	return false;
}

void CPlayer::SetPosition(uint8 _x, uint8 _y)
{
	if(this->type != OBJECT_PLAYER && this->teleporting)
	{
		return;
	}
	PMSG_POSITION_SET pPacket;
	PMSG_RECV_POSITION_SET vPacket;
	pPacket.h.c = 0xC1;
	pPacket.h.size = sizeof(PMSG_POSITION_SET);
	pPacket.h.headcode = 0xD6;
	pPacket.X = _x;
	pPacket.Y = _y;
	vPacket.h.c = 0xC1;
	vPacket.h.headcode = 0xD6;
	vPacket.h.size = sizeof(PMSG_RECV_POSITION_SET);
	vPacket.X = _x;
	vPacket.Y = _y;
	vPacket.NumberH = HIBYTE(this->guid.lo);
	vPacket.NumberL = LOBYTE(this->guid.lo);
	MovePoint pt;
	pt.time = GetTicks();
	pt.x = _x;
	pt.y = _y;
	this->path.clear();
	this->path.push_back(pt);
	this->x = _x;
	this->y = _y;
	this->target_x = _x;
	this->target_y = _y;
	this->x_old = _x;
	this->y_old = _y;
	this->last_move_time = pt.time;
	this->Send((uint8*)&pPacket, pPacket.h.size);
	this->SendToViewport((uint8*)&vPacket, vPacket.h.size);
}

bool CPlayer::SavePlayer()
{
	if(this->type != OBJECT_PLAYER)
	{
		return false;
	}
	this->last_save_time = GetTicks();
	uint32 position = this->dir;
	position |= this->map * 0x100;
	position |= this->y * 0x10000;
	position |= this->x * 0x1000000;
	QSqlQuery q(accounts_db.db);
	accounts_db.LockForWrite();
	bool result = q.exec(Query("UPDATE `characters` SET `class` = %u, `changeup` = %u, `position` = %u, `experience` = %I64u, `leveluppoint` = %u, `level` = %u, `strength` = %u, `dexterity` = %u, `vitality` = %u, `energy` = %u, `leadership` = %u, `life` = %u, `mana` = %u, `shield` = %u, `bp` = %u, `money` = %u, `pklevel` = %u, `addpoint` = %u, `maxaddpoint` = %u, `minuspoint` = %u, `maxminuspoint` = %u WHERE `dbuid` = %u;", this->Class, this->changeup, position, this->experience, this->leveluppoint, this->level, this->strength, this->dexterity, this->vitality, this->energy, this->leadership, (uint32)this->life, (uint32)this->mana, (uint32)this->shield, (uint32)this->bp, this->money, this->pklevel, this->addpoint, this->maxaddpoint, this->minuspoint, this->maxminuspoint, this->dbuid).c_str());
	accounts_db.Unlock();
	assert(result);
	/*TODO: saving inventory and co. here */
	return true;
}

void CPlayer::CookCharset()
{
	ZeroMemory(charset, 18);
	this->charset[0] = (this->Class * 0x20) & 0xE0; //0x100 - 0x20 = 0xE0
	this->charset[0] |= (this->changeup * 0x10) & 0x10;
	if(this->action == 0x80)
	{
		this->charset[0] |= 0x02;
	}
	else if(this->action == 0x81)
	{
		this->charset[0] |= 0x03;
	}
	if(this->inventory[RIGHT_HAND]->type >= 0)
	{
		this->charset[12] |= (this->inventory[RIGHT_HAND]->type & 0x0f00) / 0x10;  //12 char - highest 4 bits
		this->charset[1] = (this->inventory[RIGHT_HAND]->type & 0xff); //1 char both 4-bit fields
	}
	else //or -1;
	{
		this->charset[12] |= 0xf0;
		this->charset[1] = 0xff;
	}
	if(this->inventory[LEFT_HAND]->type >= 0)
	{
		this->charset[13] |= (this->inventory[LEFT_HAND]->type & 0x0f00) / 0x10;
		this->charset[2] = (this->inventory[LEFT_HAND]->type & 0xff);
	}
	else
	{
		this->charset[13] |= 0xf0;
		this->charset[2] = 0xff;
	}
	if(this->inventory[HELMET]->type >= 0)
	{
		this->charset[13] |= (this->inventory[HELMET]->type & 0x01E0) / 0x20;
		this->charset[9] |= (this->inventory[HELMET]->type & 0x10) * 0x08;
		this->charset[3] |= (this->inventory[HELMET]->type & 0x0f) * 0x10;
	}
	else
	{
		this->charset[13] |= 0x0f;
		this->charset[9] |= 0x80;
		this->charset[3] |= 0xf0;
	}
	if(this->inventory[ARMOR]->type >= 0)
	{
		this->charset[14] |= (this->inventory[ARMOR]->type & 0x01E0) / 0x02;
		this->charset[9] |= (this->inventory[ARMOR]->type & 0x10) * 0x04;
		this->charset[3] |= (this->inventory[ARMOR]->type &0x0f);
	}
	else
	{
		this->charset[14] |= 0xf0;
		this->charset[9] |= 0x40;
		this->charset[3] |= 0x0f;
	}
	if(this->inventory[PANTS]->type >= 0)
	{
		this->charset[14] |= (this->inventory[PANTS]->type & 0x01E0) / 0x20;
		this->charset[9] |= (this->inventory[PANTS]->type & 0x10) * 0x02;
		this->charset[4] |= (this->inventory[PANTS]->type & 0x0f) * 0x10;
	}
	else
	{
		this->charset[14] |= 0x0f;
		this->charset[9] |= 0x20;
		this->charset[4] |= 0xf0;
	}
	if(this->inventory[GLOVES]->type >= 0)
	{
		this->charset[15] |= (this->inventory[GLOVES]->type & 0x01E0) / 0x02;
		this->charset[9] |= (this->inventory[GLOVES]->type & 0x10);
		this->charset[4] |= (this->inventory[GLOVES]->type & 0x0f);
	}
	else
	{
		this->charset[15] |= 0xf0;
		this->charset[9] |= 0x10;
		this->charset[4] |= 0x0f;
	}
	if(this->inventory[BOOTS]->type >= 0)
	{
		this->charset[15] |= (this->inventory[BOOTS]->type & 0x01E0) / 0x20;
		this->charset[9] |= (this->inventory[BOOTS]->type & 0x10) / 0x02;
		this->charset[5] |= (this->inventory[BOOTS]->type & 0x0f) * 0x10;
	}
	else
	{
		this->charset[15] |= 0x0f;
		this->charset[9] |= 0x08;
		this->charset[5] |= 0xf0;
	}
	uint8 index = 0;
	if(this->inventory[WINGS]->type >= 0)
	{
		index |= (this->inventory[WINGS]->type & 0x03) * 0x04;
	}
	else
	{
		index |= 0x0c;
	}
	if((this->inventory[GUARDIAN]->type >= 0) && !(this->inventory[GUARDIAN]->type == 6660))
	{
		index |= (this->inventory[GUARDIAN]->type & 0x03);
	}
	else
	{
		index |= 0x03;
	}
	this->charset[5] |= index;
	uint32 levelindex = 0;
	levelindex = LevelConvert(this->inventory[RIGHT_HAND]->level) & 0xff;
	levelindex |= (LevelConvert(this->inventory[LEFT_HAND]->level) & 0xff) * 0x08;
	levelindex |= (LevelConvert(this->inventory[HELMET]->level) & 0xff) * 0x40;
	levelindex |= (LevelConvert(this->inventory[ARMOR]->level) & 0xff) * 0x200;
	levelindex |= (LevelConvert(this->inventory[PANTS]->level) & 0xff) * 0x1000;
	levelindex |= (LevelConvert(this->inventory[GLOVES]->level) & 0xff) * 0x8000;
	levelindex |= (LevelConvert(this->inventory[BOOTS]->level) & 0xff) * 0x40000;
	this->charset[6] = (levelindex / 0x10000) & 0xff;
	this->charset[7] = (levelindex / 0x100) & 0xff;
	this->charset[8] = (levelindex) & 0xff;
	if(((this->inventory[WINGS]->type >= (12 * 512 + 3)) && (this->inventory[WINGS]->type <= (12 * 512 + 6))) || (this->inventory[WINGS]->type == (13 * 512 + 30)))
	{
		this->charset[5] |= 0x0C;
		if(this->inventory[WINGS]->type == (13 * 512 + 30))
		{
			this->charset[5] |= 0x05;
		}
		else
		{
			this->charset[9] |= (this->inventory[WINGS]->type - 2) & 0x07;
		}
	}
	this->charset[10] = 0;
	if(this->inventory[HELMET]->IsExtItem())
	{
		this->charset[10] = 0x80;
	}
	if(this->inventory[ARMOR]->IsExtItem())
	{
		this->charset[10] |= 0x40;
	}
	if(this->inventory[PANTS]->IsExtItem())
	{
		this->charset[10] |= 0x20;
	}
	if(this->inventory[GLOVES]->IsExtItem())
	{
		this->charset[10] |= 0x10;
	}
	if(this->inventory[BOOTS]->IsExtItem())
	{
		this->charset[10] |= 0x8;
	}
	if(this->inventory[RIGHT_HAND]->IsExtItem())
	{
		this->charset[10] |= 0x4;
	}
	if(this->inventory[LEFT_HAND]->IsExtItem())
	{
		this->charset[10] |= 0x2;
	}
	this->charset[11] = 0;
	if(this->inventory[HELMET]->IsSetItem())
	{
		this->charset[11] = 0x80;
	}
	if(this->inventory[ARMOR]->IsSetItem())
	{
		this->charset[11] |= 0x40;
	}
	if(this->inventory[PANTS]->IsSetItem())
	{
		this->charset[11] |= 0x20;
	}
	if(this->inventory[GLOVES]->IsSetItem())
	{
		this->charset[11] |= 0x10;
	}
	if(this->inventory[BOOTS]->IsSetItem())
	{
		this->charset[11] |= 0x8;
	}
	if(this->inventory[RIGHT_HAND]->IsSetItem())
	{
		this->charset[11] |= 0x4;
	}
	if(this->inventory[LEFT_HAND]->IsSetItem())
	{
		this->charset[11] |= 0x2;
	}
	/*if(lpObj->IsFullSetItem)
	{
		lpObj->CharSet[11] |= 0x01;
	}*/
	if((this->inventory[GUARDIAN]->type & 3) && (this->inventory[GUARDIAN]->type >= 0))
	{
		this->charset[10] |= 0x01;
	}
	this->charset[16] = 0;
	this->charset[17] = 0;
	if(this->inventory[GUARDIAN]->type == (13 * 512 + 4)) //1a04
	{
		this->charset[12] |= 0x01;
	}
	else if(this->inventory[GUARDIAN]->type == 0x1A25) //horn of fenrir
	{
		this->charset[10] &= 0xFE;
		this->charset[12] &= 0xFE;
		this->charset[12] |= 0x04;
		//todo: unique fenrir checks
	}
}

/*void CPlayer::LoadItemToInventory(DATA_ITEM *ditem)
{
	if(ditem->type < 0)
	{
		return;
	}
	ITEM_TEMPLATE const * it = ItemTemplate.Get(ditem->type);
	if(!it)
	{
		return;
	}
	if((ditem->slot >= 0) && (ditem->slot < 12))
	{
		int8 required_slot = (ItemTemplate.Get(ditem->type))->slot;
		switch(GetCategory(ditem->type))
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			{
				if(!(ditem->slot == RIGHT_HAND) && !(ditem->slot == RIGHT_HAND))
				{
					ItemManager.DeleteFromDB(ditem->guid);
					return;
				}
				break;
			}
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			{
				if(ditem->slot != required_slot)
				{
					ItemManager.DeleteFromDB(ditem->guid);
					return;
				}
				break;
			}
		default:
			{
				ItemManager.DeleteFromDB(ditem->guid);
				return;
			}
		}
	}
	//need here a proper checks for two hand weapons, atm i don't know anything about them :D
	CItem* item = ItemManager.InsertItem(ditem->guid);
	if(item)
	{
		this->inventory[ditem->slot] = item;
		this->inventory[ditem->slot]->ApplyTemplate(it);
		this->inventory[ditem->slot]->Assign(ditem);
	}
}*/

void CPlayer::Calculate()
{
	pItem right = this->inventory[RIGHT_HAND];
	pItem left = this->inventory[LEFT_HAND];
	pItem gloves = this->inventory[GLOVES];
	pItem amulet = this->inventory[AMULET];
	pItem guardian = this->inventory[GUARDIAN];

	//armed is false by default, if player have any weapon in hand except bolts, arrows or shield we set it true, made by webzen, useless imo
	for(uint32 i = 0; i < 2; ++i)
	{
		if(this->armed)
		{
			break;
		}
		pItem it = this->inventory[i];
		if(it->IsItem())
		{
			if(it->type != 2055 && it->type != 2063 && (it->type / 512) != 6)
			{
				this->armed;
			}
		}
	}

	this->add_life = 0;
	this->add_mana = 0;
	this->extra_gold = 0;
	this->life_steal = 0;
	this->mana_steal = 0;
	this->damage_reflect = 0;
	this->damage_absorb = 0;
	this->_SkillLongSpearChange = 0;
	//periodic item specific skipped
	//setoptions specific skipped
	this->addstrength = 0;
	this->adddexterity = 0;
	this->addvitality = 0;
	this->addenergy = 0;
	this->addbp = 0;
	this->addshield = 0;
	ZeroMemory(this->res, sizeof(this->res));
	//setoption specific skipped
	uint32 _strength = this->strength + this->addstrength;
	uint32 _dexterity = this->dexterity + this->adddexterity;
	uint32 _vitality = this->vitality + this->addvitality;
	uint32 _energy = this->energy + this->addenergy;
	switch(this->Class)
	{
	case CLASS_WIZARD:
		{
			this->ad_right_min = _strength / 8;
			this->ad_right_max = _strength / 4;
			this->ad_left_min = _strength / 8;
			this->ad_left_max = _strength / 4;
			break;
		}
	case CLASS_KNIGHT:
		{
			this->ad_right_min = _strength / 6;
			this->ad_right_max = _strength / 4;
			this->ad_left_min = _strength / 6;
			this->ad_left_max = _strength / 4;
			break;
		}
	case CLASS_ELF:
		{
			if(this->armed)
			{
				this->ad_right_min = (_strength + _dexterity) / 7;
				this->ad_right_max = (_strength + _dexterity) / 4;
				this->ad_left_min = (_strength + _dexterity) / 7;
				this->ad_left_max = (_strength + _dexterity) / 4;
			}
			else
			{
				this->ad_right_min = _strength / 14 + _dexterity / 7;
				this->ad_right_max = _strength / 8 + _dexterity / 4;
				this->ad_left_min = _strength / 14 + _dexterity / 7;
				this->ad_left_max = _strength / 8 + _dexterity / 4;
			}
			break;
		}
	case CLASS_MAGUMSA:
		{
			this->ad_right_min = _energy / 12 + _strength / 6;
			this->ad_right_max = _energy / 8 + _strength / 4;
			this->ad_left_min = _energy / 12 + _strength / 6;
			this->ad_left_max = _energy / 8 + _strength / 4;
			break;
		}
	case CLASS_DARKLORD:
		{
			this->ad_right_min = _energy / 14 + _strength / 7;
			this->ad_right_max = _energy / 10 + _strength / 5;
			this->ad_left_min = _energy / 14 + _strength / 7;
			this->ad_left_max = _energy / 10 + _strength / 5;
			break;
		}
	case CLASS_SUMMONER:
		{
			this->ad_right_min = _energy / 12 + _strength / 6; //wrong but..
			this->ad_right_max = _energy / 8 + _strength / 4;
			this->ad_left_min = _energy / 12 + _strength / 6;
			this->ad_left_max = _energy / 8 + _strength / 4;
			break;
		}
	default:
		{
			this->ad_right_min = 0; //:E
			this->ad_right_max = 0;
			this->ad_left_min = 0;
			this->ad_left_max = 0;
			break;
		}
	}
	this->inventory[WINGS]->PlusSpecial(&this->ad_right_min, 80);
	this->inventory[WINGS]->PlusSpecial(&this->ad_right_max, 80);
	this->inventory[WINGS]->PlusSpecial(&this->ad_left_min, 80);
	this->inventory[WINGS]->PlusSpecial(&this->ad_left_max, 80);
	uint32 _addcharisma = 0;
	if(this->inventory[WINGS]->IsItem())
	{
		_addcharisma += this->inventory[WINGS]->m_Leadership;
	}
	if(right->IsItem())
	{
		if(right->GetItemType() != ITEM_STAFF && right->GetItemType() != ITEM_SHIELD) //strange, not only weapons
		{
			this->ad_right_min += right->damage_min;
			this->ad_right_max += right->damage_max;
		}
		else
		{
			this->ad_right_min += right->damage_min / 2;
			this->ad_right_max += right->damage_max / 2;
		}
		if(right->m_SkillChange)
		{
			this->SkillLongSpearChange = 1;
		}
		right->PlusSpecial(&this->ad_right_min, 80);
		right->PlusSpecial(&this->ad_right_max, 80);
	}
	if(left->IsItem())
	{
		this->ad_left_min += left->damage_min;
		this->ad_left_max += left->damage_max;
		left->PlusSpecial(&this->ad_left_min, 80);
		left->PlusSpecial(&this->ad_left_max, 80);
	}
	this->crit_damage = 0;
	this->excellent_damage = 0;
	this->inventory[RIGHT_HAND]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[LEFT_HAND]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[HELMET]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[ARMOR]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[PANTS]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[GLOVES]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[BOOTS]->PlusSpecial(&this->crit_damage, 84);
	this->inventory[WINGS]->PlusSpecial(&this->crit_damage, 84);

	this->spell_damage_min = _energy / 9;
	this->spell_damage_max = _energy / 4;
	this->inventory[WINGS]->PlusSpecial(&this->spell_damage_min, 81);
	this->inventory[WINGS]->PlusSpecial(&this->spell_damage_max, 81);
	if(right->IsItem())
	{
		uint32 it_type = right->type;
		if(it_type != 31 && it_type != 21 && it_type != 23 && it_type != 25)
		{
			right->PlusSpecial(&this->spell_damage_min, 81);
			right->PlusSpecial(&this->spell_damage_max, 81);
		}
		else
		{
			right->PlusSpecial(&this->spell_damage_min, 80);
			right->PlusSpecial(&this->spell_damage_max, 80);
		}
	}
}

const CPositionHandler::MovePoint CPlayer::GetCurrentPosition()
{
	return this->position.GetPosition();
}