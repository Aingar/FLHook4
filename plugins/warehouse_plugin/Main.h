#pragma once

// Includes
#include <FLHook.h>
#include <plugin.h>
#include "../rename_plugin/Main.h"

namespace Plugins::Warehouse
{
	struct WarehouseItem : Reflectable
	{
		uint hashID;
		uint quantity;
	};

	struct Warehouse : Reflectable
	{
		std::string warehouseNickName;
		uint warehouseHash;
		std::vector<WarehouseItem> storedItems;
	};

	struct WhPlayer : Reflectable
	{
		std::wstring accID;
		std::vector<Warehouse> warehouses;
	};

	struct AllPlayers : Reflectable
	{
		std::string File() override
		{
			char path[MAX_PATH];
			GetUserDataPath(path);
			return std::string(path) + "\\warehouseData.json";
		}

		std::vector<WhPlayer> players;
	};

	struct Config final : Reflectable
	{
		std::string File() override { return "flhook_plugins/warehouse.json"; }

		std::vector<std::string> restrictedBases;
		std::vector<uint> restrictedBasesHashed;

		std::vector<std::string> restrictedItems;
		std::vector<uint> restrictedItemsHashed;

		uint costPerStackWithdraw = 0;
		uint costPerStackStore = 0;
	};

	//! Global data for this plugin
	struct Global final
	{
		// Other fields
		ReturnCode returncode = ReturnCode::Default;

		AllPlayers allPlayers;
		Config config;
	};
}