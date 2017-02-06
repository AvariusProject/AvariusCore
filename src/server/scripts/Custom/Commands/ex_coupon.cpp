#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Common.h"
#include "Player.h"
#include "WorldSession.h"
#include "Language.h"
#include "Log.h"
#include "SpellAuras.h"
#include "World.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Config.h"
#include <iostream>
#include <iterator>
#include <vector>
#include <random>
#include <algorithm>

#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "SocialMgr.h"




class coupun : public CommandScript
{
public:
	coupun() : CommandScript("coupun") { }

	std::vector<ChatCommand> GetCommands()  const override
	{

		static std::vector<ChatCommand> coupontable =
		{


			{ "reedem", SEC_PLAYER, false, &HandleGutscheinCommand, "" },


			{ "generate", SEC_ADMINISTRATOR, false, &HandlegutscheinerstellenCommand, "" }

		};

		static std::vector<ChatCommand> commandTable =
		{
			{ "coupon", SEC_ADMINISTRATOR , false, NULL, "" , coupontable },

		};

		return commandTable;
	}





	//Gibt dem Eventteam die Moeglichkeit Gutscheine fuer Spieler zu erstellen.
	static bool HandlegutscheinerstellenCommand(ChatHandler* handler, const char* args)
	{

		Player* player = handler->GetSession()->GetPlayer();

		char* itemid = strtok((char*)args, " ");
		if (!itemid) {
			player->GetSession()->SendNotification("Ohne ItemID geht das leider nicht!");
			return false;
		}

		uint32 item = atoi((char*)itemid);

		char* itemanzahl = strtok(NULL, " ");
		if (!itemanzahl || !atoi(itemanzahl)) {
			player->GetSession()->SendNotification("Ohne Anzahl geht das leider nicht!");
			return false;
		}


		char* anzahlnutzer = strtok(NULL, " ");
		if (!anzahlnutzer) {
			player->GetSession()->SendNotification("Ohne Anzahl wie oft der Code eingesetzt werden kann, geht das nicht!");
			return false;
		}



		PreparedStatement * itemquery = WorldDatabase.GetPreparedStatement(WORLD_SEL_ITEM_NR);
		itemquery->setUInt32(0, item);
		PreparedQueryResult ergebnis = WorldDatabase.Query(itemquery);


		if (!ergebnis) {
			player->GetSession()->SendNotification("Item existiert nicht");
			return true;
		}


		uint32 anzahlint = atoi((char*)itemanzahl);
		uint32 codebenutztbar = atoi((char*)anzahlnutzer);
		//uint32 item = atoi((char*)args);


		if (!item)
		{
			player->GetSession()->SendNotification("Ohne Itemid geht das leider nicht!");
			return true;
		}

		if (!itemanzahl)
		{
			player->GetSession()->SendNotification("Ohne Anzahl geht das leider nicht!");
			return true;
		}

		if (!anzahlnutzer) {
			player->GetSession()->SendNotification("Ohne Anzahl der Nutzung geht das leider nicht!");
			return true;
		}


		if (item == 49623) {
			player->GetSession()->SendNotification("Schattengram als Belohnung zu generieren ist verboten, wird geloggt und Exitare informiert.");
			CharacterDatabase.PExecute("INSERT INTO eventteamlog "
				"(player,guid, itemid,gutscheincode,anzahl)"
				"VALUES ('%s', '%u', '%u', '%s','%u')",
				player->GetSession()->GetPlayerName(), player->GetGUID(), item, "Schattemgram", 0);
			return true;
		}




		auto randchar = []() -> char
		{
			const char charset[] =
				"0123456789"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz";
			const size_t max_index = (sizeof(charset) - 1);
			return charset[rand() % max_index];
		};
		std::string str(10, 0);
		std::generate_n(str.begin(), 10, randchar);

		/*CharacterDatabase.PExecute("INSERT INTO `item_codes` (code,belohnung,anzahl,benutzt,benutztbar) Values ('%s','%u','%u','%u','%u')", str, item, anzahlint, 0,1);*/

		PreparedStatement * inscode = CharacterDatabase.GetPreparedStatement(CHAR_INS_NOPLAYERITEMCODE);
		inscode->setString(0, str);
		inscode->setUInt32(1, item);
		inscode->setUInt32(2, anzahlint);
		inscode->setUInt32(3, 0);
		inscode->setUInt32(4, codebenutztbar);
		CharacterDatabase.Execute(inscode);


		std::ostringstream ss;
		std::ostringstream tt;


		QueryResult itemsql = WorldDatabase.PQuery("SELECT `name` FROM `item_template` WHERE `entry` = '%u'", item);
		Field *fields = itemsql->Fetch();
		std::string itemname = fields[0].GetCString();

		ss << "Der Code fuer das Item: " << itemname << " mit der Anzahl " << anzahlint << " lautet " << str << " . Wir wuenschen dir weiterhin viel Spass auf MMOwning. Dein MMOwning-Team";
		player->GetSession()->SendNotification("Dein Code wurde generiert und dir zugesendet.");

		tt << str << " ist der generierte Gutscheincode fuer das Item " << itemname << " mit der Itemanzahl " << anzahlint << ". Der Code kann " << codebenutztbar << " benutzt werden.";
		handler->PSendSysMessage(tt.str().c_str(), player->GetName());
		SQLTransaction trans = CharacterDatabase.BeginTransaction();
		MailDraft("Dein Gutscheincode", ss.str().c_str())
			.SendMailTo(trans, MailReceiver(player, player->GetGUID()), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM));
		CharacterDatabase.CommitTransaction(trans);



		/*CharacterDatabase.PExecute("INSERT INTO firstnpc_log "
		"(grund,spieler, guid)"
		"VALUES ('%s', '%s', '%u')",
		"Eventteamgutschein", player->GetSession()->GetPlayerName(),player->GetGUID()); */

		PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_FIRSTLOG);
		stmt->setString(0, "Eventteamgutschein");
		stmt->setString(1, player->GetSession()->GetPlayerName());
		stmt->setUInt32(2, player->GetGUID());
		CharacterDatabase.Execute(stmt);

		PreparedStatement* eventlog = CharacterDatabase.GetPreparedStatement(CHAR_INS_EVENTLOG);
		eventlog->setString(0, player->GetSession()->GetPlayerName());
		eventlog->setUInt32(1, player->GetGUID());
		eventlog->setUInt32(2, item);
		eventlog->setString(3, str);
		eventlog->setUInt32(4, anzahlint);
		CharacterDatabase.Execute(eventlog);

		/*CharacterDatabase.PExecute("INSERT INTO eventteamlog "
		"(player,guid, itemid,gutscheincode,anzahl)"
		"VALUES ('%s', '%u', '%u', '%s','%u')",
		player->GetSession()->GetPlayerName(),player->GetGUID(),item,str,anzahlint); */

		return true;

	};



	static bool HandleGutscheinCommand(ChatHandler* handler, const char* args)
	{
		Player *player = handler->GetSession()->GetPlayer();

		std::string itemCode = std::string((char*)args);

		if (itemCode == "")
		{
			player->GetSession()->SendNotification("Ohne Code geht das leider nicht!");
			return true;
		}

		if (itemCode == "GOLD") {
			return true;
		}



		PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEMCODEGES);
		stmt->setString(0, itemCode);
		PreparedQueryResult result = CharacterDatabase.Query(stmt);

		//QueryResult result = CharacterDatabase.PQuery("SELECT `code`, `belohnung`, `anzahl`, `benutzt`, `benutztbar` FROM `item_codes` WHERE `code` = '%s'", itemCode);


		if (result)
		{

			Field* fields = result->Fetch();
			std::string code = fields[0].GetCString();
			uint32 belohnung = fields[1].GetUInt32();
			uint32 anzahl = fields[2].GetUInt32();
			uint8 benutzt = fields[3].GetUInt8();
			uint32 benutztbar = fields[4].GetUInt32();

			QueryResult accvorhanden = CharacterDatabase.PQuery("SELECT `accid`,`code` FROM `item_codes_account` WHERE `accid` = '%u' AND code = '%s' ", player->GetSession()->GetAccountId(), code);

			if (!accvorhanden) {
				QueryResult itemid = WorldDatabase.PQuery("SELECT `entry` FROM `item_template` WHERE `entry` = '%u'", belohnung);

				if (!itemid) {
					player->GetSession()->SendNotification("Das Item scheint nicht zu existieren. Der Code wird daher abgelehnt");
					return true;
				}



				if (benutzt < benutztbar)
				{
					benutzt++;
					Item* item = Item::CreateItem(belohnung, anzahl);
					player->GetSession()->SendNotification("Dein Code wurde akzeptiert!");
					SQLTransaction trans = CharacterDatabase.BeginTransaction();
					item->SaveToDB(trans);
					MailDraft("Dein Gutscheincode", "Dein Code wurde erfolgreich eingeloest. Wir wuenschen dir weiterhin viel Spass auf MMOwning. Dein MMOwning-Team").AddItem(item)
						.SendMailTo(trans, MailReceiver(player, player->GetGUID()), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM));
					CharacterDatabase.CommitTransaction(trans);

					CharacterDatabase.PExecute("UPDATE item_codes SET name = '%s' WHERE code = '%s'", player->GetName().c_str(), itemCode);
					CharacterDatabase.PExecute("UPDATE item_codes SET benutzt = '%u' WHERE code = '%s'", benutzt, itemCode);

					PreparedStatement* itemcodeaccount = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEMCODEACCOUNT);
					itemcodeaccount->setString(0, player->GetSession()->GetPlayerName());
					itemcodeaccount->setUInt32(1, player->GetSession()->GetAccountId());
					itemcodeaccount->setString(2, itemCode);
					CharacterDatabase.Execute(itemcodeaccount);

					//CharacterDatabase.PExecute("INSERT INTO item_codes_account (name,accid,code) Values('%s','%u','%s')", player->GetSession()->GetPlayerName(), player->GetSession()->GetAccountId(), itemCode);

					char msg[250];
					snprintf(msg, 250, "Dein Code wurde akzeptiert.");
					ChatHandler(player->GetSession()).PSendSysMessage(msg,
						player->GetName());
					return true;

				}
				else {
					char msg[250];
					snprintf(msg, 250, "Der Code hat keine weitere Aufladung und wird daher abgelehnt.");
					ChatHandler(player->GetSession()).PSendSysMessage(msg,
						player->GetName());
					return true;
				}
			}

			else {
				char msg[250];
				snprintf(msg, 250, "Du hast den Code bereits verwendet.");
				ChatHandler(player->GetSession()).PSendSysMessage(msg,
					player->GetName());
				return true;

			}
		}
		else {
			char msg[250];
			snprintf(msg, 250, "Der eingegebene Code exisitert nicht.");
			ChatHandler(player->GetSession()).PSendSysMessage(msg,
				player->GetName());
			return true;
		}
		return true;
	};


};

	
void AddSC_coupon()
{
	new coupun();
}