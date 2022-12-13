﻿#include "Main.h"

namespace Plugins::Tempban
{
	const std::unique_ptr<Global> global = std::make_unique<Global>();

	/**************************************************************************************************************
	Check if TempBans exceeded
	**************************************************************************************************************/

	void TimerCheckKick()
	{
		// timed out tempbans get deleted here

		global->returncode = ReturnCode::Default;

		for (auto it = global->TempBans.begin(); it != global->TempBans.end(); ++it)
		{
			if (((*it).banStart + (*it).banDuration) < timeInMS())
			{
				global->TempBans.erase(it);
				break; // fix to not overflow the list
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> __stdcall TempBan(const std::wstring& wscCharname, uint _duration)
	{
		const auto client = Hk::Client::GetClientIdFromCharName(wscCharname);

		mstime duration = 1000 * _duration * 60;
		TempbanInfo tempban;
		tempban.banStart = timeInMS();
		tempban.banDuration = duration;

		CAccount* acc;
		if (client != -1)
			acc = Players.FindAccountFromClientID(client.value());
		else
		{
			if (!(acc = Hk::Client::GetAccountByCharName(wscCharname).value()))
				return cpp::fail(Error::CharacterDoesNotExist);
		}
		const auto wscId = Hk::Client::GetAccountID(acc);

		tempban.accountId = wscId.value();
		global->TempBans.push_back(tempban);

		if (client != -1 && Hk::Player::Kick(client.value()).has_error())
		{
			AddLog(LogType::Kick, LogLevel::Info, wscCharname + L" could not be kicked (TempBan Plugin)");
			Console::ConInfo(wscCharname + L" could not be kicked (TempBan Plugin)");
		}
		
		return {};
	}

	bool TempBannedCheck(ClientId client)
	{
		CAccount* acc;
		acc = Players.FindAccountFromClientID(client);

		const auto wscId = Hk::Client::GetAccountID(acc);

		for (auto& ban : global->TempBans)
		{
			if (ban.accountId == wscId.value())
				return true;
		}

		return false;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void __stdcall Login(struct SLoginInfo const& li [[maybe_unused]], unsigned int& client)
	{
		global->returncode = ReturnCode::Default;

		// check for tempban
		if (TempBannedCheck(client))
		{
			global->returncode = ReturnCode::SkipAll;
			Hk::Player::Kick(client);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void CmdTempBan(CCmds* classptr, const std::wstring& wscCharname, uint iDuration)
	{
		// right check
		if (!(classptr->rights & RIGHT_KICKBAN))
		{
			classptr->Print(L"ERR No permission");
			return;
		}

		if (const auto err = TempBan(wscCharname, iDuration); err.has_error())
		{
			classptr->PrintError(err.error());
		}
			
		classptr->Print(L"OK");
	}

	bool ExecuteCommandString(CCmds* classptr, const std::wstring& wscCmd)
	{
		if (wscCmd == L"tempban")
		{
			global->returncode = ReturnCode::SkipAll; // do not let other plugins kick in
			                                          // since we now handle the command

			CmdTempBan(classptr, classptr->ArgCharname(1), classptr->ArgInt(2));

			return true;
		}

		return false;
	}

	void CmdHelp(CCmds* classptr) { classptr->Print(L"tempban <charname>"); }

	TempBanCommunicator::TempBanCommunicator(std::string plug) : PluginCommunicator(plug) { this->TempBan = TempBan; }
} // namespace Plugins::Tempban

using namespace Plugins::Tempban;

DefaultDllMain();

extern "C" EXPORT void ExportPluginInfo(PluginInfo* pi)
{
	pi->name(TempBanCommunicator::pluginName);
	pi->shortName("tempban");
	pi->mayUnload(true);
	pi->returnCode(&global->returncode);
	pi->versionMajor(PluginMajorVersion::VERSION_04);
	pi->versionMinor(PluginMinorVersion::VERSION_00);
	pi->emplaceHook(HookedCall::FLHook__TimerCheckKick, &TimerCheckKick);
	pi->emplaceHook(HookedCall::IServerImpl__Login, &Login, HookStep::After);
	pi->emplaceHook(HookedCall::FLHook__AdminCommand__Process, &ExecuteCommandString);
	pi->emplaceHook(HookedCall::FLHook__AdminCommand__Help, &CmdHelp);

	// Register IPC
	global->communicator = new TempBanCommunicator(TempBanCommunicator::pluginName);
	PluginCommunicator::ExportPluginCommunicator(global->communicator);
}