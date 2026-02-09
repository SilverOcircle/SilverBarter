// SilverBarter Haupt-Konfiguration
class SilverBarterConfig
{
	// Config-Pfade
	private const static string MOD_FOLDER = "$profile:\\SilverBarter\\";
	private const static string CONFIG_NAME = "SilverBarterConfig.json";
	private const static string CURRENT_VERSION = "1";

	string CONFIG_VERSION;

	// Globale Einstellungen
	bool m_debugMode;

	// Trader-Konfiguration
	ref array<ref SilverTrader_ServerConfig> m_traders;

	void SilverBarterConfig()
	{
		m_traders = new array<ref SilverTrader_ServerConfig>;
	}

	void ~SilverBarterConfig()
	{
		if (m_traders)
		{
			m_traders.Clear();
			delete m_traders;
		}
	}

	void Load()
	{
		if (!g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string path = MOD_FOLDER + CONFIG_NAME;
		if (FileExist(path))
		{
			JsonFileLoader<SilverBarterConfig>.JsonLoadFile(path, this);

			if (CONFIG_VERSION != CURRENT_VERSION)
			{
				JsonFileLoader<SilverBarterConfig>.JsonSaveFile(path + "_backup", this);
				CONFIG_VERSION = CURRENT_VERSION;
				Save();
			}
		}
		else
		{
			SetDefaultValues();
			CONFIG_VERSION = CURRENT_VERSION;
			Save();
		}
	}

	void Save()
	{
		if (!g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		JsonFileLoader<SilverBarterConfig>.JsonSaveFile(MOD_FOLDER + CONFIG_NAME, this);
	}

	void SetDefaultValues()
	{
		m_debugMode = false;
		m_traders = new array<ref SilverTrader_ServerConfig>;

		// Beispiel-Trader
		SilverTrader_ServerConfig exampleTrader = new SilverTrader_ServerConfig();
		exampleTrader.m_traderId = 0;
		exampleTrader.m_classname = "SurvivorM_Mirek";
		exampleTrader.m_position = "6618.94 41.62 7151.61";
		exampleTrader.m_rotation = -43.268715;
		exampleTrader.m_storageMaxSize = 5000;
		exampleTrader.m_storageCommission = 0.65;
		exampleTrader.m_dumpingByAmountAlgorithm = "linear";
		exampleTrader.m_dumpingByAmountModifier = 0.65;
		exampleTrader.m_dumpingByBadQuality = 0.5;
		exampleTrader.m_sellMaxQuantityPercent = 0.8;
		exampleTrader.m_buyMaxQuantityPercent = 0.9;

		exampleTrader.m_buyFilter = new array<string>;
		exampleTrader.m_buyFilter.Insert("Inventory_Base");
		exampleTrader.m_buyFilter.Insert("Weapon_Base");
		exampleTrader.m_buyFilter.Insert("Magazine_Base");
		exampleTrader.m_buyFilter.Insert("Ammunition_Base");
		exampleTrader.m_buyFilter.Insert("Box_Base");

		exampleTrader.m_sellFilter = new array<string>;
		exampleTrader.m_sellFilter.Insert("Inventory_Base");
		exampleTrader.m_sellFilter.Insert("Weapon_Base");
		exampleTrader.m_sellFilter.Insert("Magazine_Base");
		exampleTrader.m_sellFilter.Insert("Ammunition_Base");
		exampleTrader.m_sellFilter.Insert("!Container_Base");
		exampleTrader.m_sellFilter.Insert("Box_Base");
		exampleTrader.m_sellFilter.Insert("FirstAidKit");
		exampleTrader.m_sellFilter.Insert("SmallProtectorCase");
		exampleTrader.m_sellFilter.Insert("PlateCarrierPouches");
		exampleTrader.m_sellFilter.Insert("!SyringeFull");
		exampleTrader.m_sellFilter.Insert("!Paper");
		exampleTrader.m_sellFilter.Insert("!Zen_EmptyFood");
		exampleTrader.m_sellFilter.Insert("!FenceKit");
		exampleTrader.m_sellFilter.Insert("!WatchtowerKit");
		exampleTrader.m_sellFilter.Insert("!ShelterKit");
		exampleTrader.m_sellFilter.Insert("!TerritoryFlagKit");
		exampleTrader.m_sellFilter.Insert("!Flag_Base");
		exampleTrader.m_sellFilter.Insert("!Rag");
		exampleTrader.m_sellFilter.Insert("!BurlapStrip");
		exampleTrader.m_sellFilter.Insert("!Stone");
		exampleTrader.m_sellFilter.Insert("!SmallStone");
		exampleTrader.m_sellFilter.Insert("!Firewood");
		exampleTrader.m_sellFilter.Insert("!BoneHook");
		exampleTrader.m_sellFilter.Insert("!WoodenHook");
		exampleTrader.m_sellFilter.Insert("!Bark_ColorBase");
		exampleTrader.m_sellFilter.Insert("!Bone");
		exampleTrader.m_sellFilter.Insert("!Bait");
		exampleTrader.m_sellFilter.Insert("!BoneBait");
		exampleTrader.m_sellFilter.Insert("!Barrel_ColorBase");
		exampleTrader.m_sellFilter.Insert("!FireplaceBase");
		exampleTrader.m_sellFilter.Insert("!CookingStand");
		exampleTrader.m_sellFilter.Insert("!WoodenStick");
		exampleTrader.m_sellFilter.Insert("!Torch");
		exampleTrader.m_sellFilter.Insert("!LongWoodenStick");
		exampleTrader.m_sellFilter.Insert("!SharpWoodenStick");
		exampleTrader.m_sellFilter.Insert("!HandDrillKit");
		exampleTrader.m_sellFilter.Insert("!Spear");
		exampleTrader.m_sellFilter.Insert("!SmallGuts");
		exampleTrader.m_sellFilter.Insert("!Guts");
		exampleTrader.m_sellFilter.Insert("!Worm");
		exampleTrader.m_sellFilter.Insert("!ImprovisedFishingRod");
		exampleTrader.m_sellFilter.Insert("!TripwireTrap");
		exampleTrader.m_sellFilter.Insert("!RabbitSnareTrap");
		exampleTrader.m_sellFilter.Insert("!FishNetTrap");
		exampleTrader.m_sellFilter.Insert("!SmallFishTrap");
		exampleTrader.m_sellFilter.Insert("!Empty_ZenJameson");
		exampleTrader.m_sellFilter.Insert("!Zen_CamoShelterKit");
		exampleTrader.m_sellFilter.Insert("!Empty_Honey");
		exampleTrader.m_sellFilter.Insert("!Empty_Marmalade");
		exampleTrader.m_sellFilter.Insert("!Empty_Can_Opened");
		exampleTrader.m_sellFilter.Insert("!BF_DoorBarricadeKit");
		exampleTrader.m_sellFilter.Insert("!BF_WindowBarricadeKit");
		exampleTrader.m_sellFilter.Insert("!BF_WindowBarricadeMedKit");
		exampleTrader.m_sellFilter.Insert("!BF_DoubleDoorBarricadeKit");
		exampleTrader.m_sellFilter.Insert("!bl_improvised_whetstone");
		exampleTrader.m_sellFilter.Insert("!bl_improvised_sewing_kit");
		exampleTrader.m_sellFilter.Insert("!Single_Match");

		exampleTrader.m_attachments = new array<string>;
		exampleTrader.m_attachments.Insert("DownJacket_Orange");
		exampleTrader.m_attachments.Insert("Jeans_Black");
		exampleTrader.m_attachments.Insert("Slingbag_Black");
		exampleTrader.m_attachments.Insert("WoolGloves_Black");
		exampleTrader.m_attachments.Insert("BeanieHat_Black");
		exampleTrader.m_attachments.Insert("HikingBootsLow_Black");
		exampleTrader.m_attachments.Insert("Shemag_Green");

		// Item-spezifische Commission-Overrides (wertvolle/seltene Items)
		exampleTrader.m_commissionOverrides = new array<ref SilverCommissionOverride>;

		SilverCommissionOverride leatherOverride = new SilverCommissionOverride();
		leatherOverride.classname = "TannedLeather";
		leatherOverride.commission = 0.2;
		exampleTrader.m_commissionOverrides.Insert(leatherOverride);

		// Limitierte Items (werden bei jedem Restart auf maxQuantity zurueckgesetzt)
		exampleTrader.m_limitedItems = new array<ref SilverTrader_LimitedItem>;

		SilverTrader_LimitedItem limitedBook1 = new SilverTrader_LimitedItem();
		limitedBook1.classname = "ZenSkills_Book_Survival";
		limitedBook1.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook1);

		SilverTrader_LimitedItem limitedBook2 = new SilverTrader_LimitedItem();
		limitedBook2.classname = "ZenSkills_Book_Crafting";
		limitedBook2.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook2);

		SilverTrader_LimitedItem limitedBook3 = new SilverTrader_LimitedItem();
		limitedBook3.classname = "ZenSkills_Book_Hunting";
		limitedBook3.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook3);

		SilverTrader_LimitedItem limitedBook4 = new SilverTrader_LimitedItem();
		limitedBook4.classname = "ZenSkills_Book_Gathering";
		limitedBook4.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook4);

		// Standard-Items die der Trader von Anfang an hat
		exampleTrader.m_defaultItems = new array<ref SilverTrader_ItemEntry>;

		SilverTrader_ItemEntry item1 = new SilverTrader_ItemEntry();
		item1.classname = "AmmoBox_308Win_20Rnd";
		item1.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item1);

		SilverTrader_ItemEntry item2 = new SilverTrader_ItemEntry();
		item2.classname = "AmmoBox_22_50Rnd";
		item2.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item2);

		SilverTrader_ItemEntry item3 = new SilverTrader_ItemEntry();
		item3.classname = "AmmoBox_45ACP_25rnd";
		item3.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item3);

		SilverTrader_ItemEntry item4 = new SilverTrader_ItemEntry();
		item4.classname = "ZenSkills_Injector_ExpBoost";
		item4.quantity = 200;
		exampleTrader.m_defaultItems.Insert(item4);

		SilverTrader_ItemEntry item5 = new SilverTrader_ItemEntry();
		item5.classname = "ZenSkills_Injector_PerkReset";
		item5.quantity = 2;
		exampleTrader.m_defaultItems.Insert(item5);

		SilverTrader_ItemEntry item6 = new SilverTrader_ItemEntry();
		item6.classname = "MeatTenderizer";
		item6.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item6);

		SilverTrader_ItemEntry item11 = new SilverTrader_ItemEntry();
		item11.classname = "NailBox";
		item11.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item11);

		SilverTrader_ItemEntry item12 = new SilverTrader_ItemEntry();
		item12.classname = "Battery9V";
		item12.quantity = 30;
		exampleTrader.m_defaultItems.Insert(item12);

		SilverTrader_ItemEntry item13 = new SilverTrader_ItemEntry();
		item13.classname = "Ammo_45ACP";
		item13.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item13);

		SilverTrader_ItemEntry item14 = new SilverTrader_ItemEntry();
		item14.classname = "Ammo_380";
		item14.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item14);

		SilverTrader_ItemEntry item15 = new SilverTrader_ItemEntry();
		item15.classname = "Ammo_22";
		item15.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item15);

		SilverTrader_ItemEntry item16 = new SilverTrader_ItemEntry();
		item16.classname = "Ammo_12gaSlug";
		item16.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item16);

		SilverTrader_ItemEntry item17 = new SilverTrader_ItemEntry();
		item17.classname = "Ammo_12gaPellets";
		item17.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item17);

		SilverTrader_ItemEntry item18 = new SilverTrader_ItemEntry();
		item18.classname = "AmmoBox_12gaSlug_10Rnd";
		item18.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item18);

		SilverTrader_ItemEntry item19 = new SilverTrader_ItemEntry();
		item19.classname = "AmmoBox_00buck_10rnd";
		item19.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item19);

		SilverTrader_ItemEntry item20 = new SilverTrader_ItemEntry();
		item20.classname = "Hatchet";
		item20.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item20);

		SilverTrader_ItemEntry item21 = new SilverTrader_ItemEntry();
		item21.classname = "B95";
		item21.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item21);

		SilverTrader_ItemEntry item22 = new SilverTrader_ItemEntry();
		item22.classname = "MKII";
		item22.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item22);

		SilverTrader_ItemEntry item23 = new SilverTrader_ItemEntry();
		item23.classname = "P1";
		item23.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item23);

		SilverTrader_ItemEntry item24 = new SilverTrader_ItemEntry();
		item24.classname = "WaterBottle";
		item24.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item24);

		SilverTrader_ItemEntry item25 = new SilverTrader_ItemEntry();
		item25.classname = "Crackers";
		item25.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item25);

		SilverTrader_ItemEntry item26 = new SilverTrader_ItemEntry();
		item26.classname = "SaltySticks";
		item26.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item26);

		SilverTrader_ItemEntry item27 = new SilverTrader_ItemEntry();
		item27.classname = "Zagorky";
		item27.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item27);

		SilverTrader_ItemEntry item28 = new SilverTrader_ItemEntry();
		item28.classname = "Pate";
		item28.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item28);

		SilverTrader_ItemEntry item29 = new SilverTrader_ItemEntry();
		item29.classname = "BandageDressing";
		item29.quantity = 35;
		exampleTrader.m_defaultItems.Insert(item29);

		SilverTrader_ItemEntry item30 = new SilverTrader_ItemEntry();
		item30.classname = "Whetstone";
		item30.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item30);

		SilverTrader_ItemEntry item31 = new SilverTrader_ItemEntry();
		item31.classname = "Screwdriver";
		item31.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item31);

		SilverTrader_ItemEntry item32 = new SilverTrader_ItemEntry();
		item32.classname = "WoolGloves_Black";
		item32.quantity = 8;
		exampleTrader.m_defaultItems.Insert(item32);

		SilverTrader_ItemEntry item33 = new SilverTrader_ItemEntry();
		item33.classname = "Shemag_Green";
		item33.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item33);

		SilverTrader_ItemEntry item34 = new SilverTrader_ItemEntry();
		item34.classname = "HikingBootsLow_Black";
		item34.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item34);

		m_traders.Insert(exampleTrader);
	}
};

// Trader-Daten Klasse (Inventar eines Traders)
class SilverTrader_Data
{
	ref map<string, float> m_items;

	void SilverTrader_Data()
	{
		m_items = new map<string, float>;
	}

	void ~SilverTrader_Data()
	{
		if (m_items)
		{
			delete m_items;
		}
	}

	void LoadFromJson(string path)
	{
		if (!m_items)
		{
			m_items = new map<string, float>;
		}
		else
		{
			m_items.Clear();
		}

		if (FileExist(path))
		{
			SilverTrader_DataJson jsonData = new SilverTrader_DataJson();
			JsonFileLoader<SilverTrader_DataJson>.JsonLoadFile(path, jsonData);

			if (jsonData && jsonData.m_itemList)
			{
				foreach (SilverTrader_ItemEntry entry : jsonData.m_itemList)
				{
					if (entry && entry.classname != "" && IsValidClassname(entry.classname))
					{
						m_items.Insert(entry.classname, entry.quantity);
					}
				}
			}
		}
	}

	void SaveToJson(string path)
	{
		SilverTrader_DataJson jsonData = new SilverTrader_DataJson();
		jsonData.m_itemList = new array<ref SilverTrader_ItemEntry>;

		if (m_items)
		{
			foreach (string classname, float quantity : m_items)
			{
				SilverTrader_ItemEntry entry = new SilverTrader_ItemEntry();
				entry.classname = classname;
				entry.quantity = quantity;
				jsonData.m_itemList.Insert(entry);
			}
		}

		JsonFileLoader<SilverTrader_DataJson>.JsonSaveFile(path, jsonData);
	}

	private bool IsValidClassname(string classname)
	{
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
			return true;
		if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
			return true;
		if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
			return true;
		return false;
	}
};

// JSON-Serialisierungs-Hilfsklassen
class SilverTrader_DataJson
{
	ref array<ref SilverTrader_ItemEntry> m_itemList;

	void SilverTrader_DataJson()
	{
		m_itemList = new array<ref SilverTrader_ItemEntry>;
	}
};

class SilverTrader_ItemEntry
{
	string classname;
	float quantity;
};

// Basis-Trader-Info Klasse (für Client/Server)
class SilverTrader_Info
{
	int m_traderId;
	vector m_position;
	ref array<string> m_buyFilter;
	ref array<string> m_sellFilter;
	ref array<ref SilverCommissionOverride> m_commissionOverrides; // Item-spezifische Commission
	int m_storageMaxSize;
	float m_storageCommission;
	string m_dumpingByAmountAlgorithm;
	float m_dumpingByAmountModifier;
	float m_dumpingByBadQuality;
	float m_sellMaxQuantityPercent;
	float m_buyMaxQuantityPercent;

	void ~SilverTrader_Info()
	{
		if (m_buyFilter)
			delete m_buyFilter;
		if (m_sellFilter)
			delete m_sellFilter;
		if (m_commissionOverrides)
			delete m_commissionOverrides;
	}

	// Ermittelt Commission fuer ein Item (Override oder Fallback)
	float GetCommissionForItem(string classname)
	{
		if (m_commissionOverrides)
		{
			// Exakte Klasse pruefen (case-insensitive)
			string classnameLower = classname;
			classnameLower.ToLower();
			foreach (SilverCommissionOverride commOverride : m_commissionOverrides)
			{
				string overrideLower = commOverride.classname;
				overrideLower.ToLower();
				if (commOverride && overrideLower == classnameLower)
				{
					return commOverride.commission;
				}
			}

			// Parent-Klasse pruefen (fuer Vererbung wie "Goldnugget_Base")
			foreach (SilverCommissionOverride parentOverride : m_commissionOverrides)
			{
				if (parentOverride && g_Game.IsKindOf(classname, parentOverride.classname))
				{
					return parentOverride.commission;
				}
			}
		}

		// Fallback auf Standard-Commission
		return m_storageCommission;
	}
};

// Commission-Override Eintrag (fuer Item-spezifische Commission)
class SilverCommissionOverride
{
	string classname;
	float commission;
};

// Limited-Item Eintrag (wird bei jedem Restart auf maxQuantity zurueckgesetzt)
class SilverTrader_LimitedItem
{
	string classname;
	int maxQuantity;
};

// Server-spezifische Trader-Config (erweitert Info um Spawn-Daten)
class SilverTrader_ServerConfig : SilverTrader_Info
{
	string m_classname;
	ref array<string> m_attachments;
	ref array<ref SilverTrader_ItemEntry> m_defaultItems; // Start-Items fuer Trader
	ref array<ref SilverTrader_LimitedItem> m_limitedItems; // Bei Restart auf fixen Wert setzen
	float m_rotation;

	void ~SilverTrader_ServerConfig()
	{
		if (m_attachments)
			delete m_attachments;
		if (m_defaultItems)
			delete m_defaultItems;
		if (m_limitedItems)
			delete m_limitedItems;
	}
};

// Globaler Config-Accessor
static ref SilverBarterConfig g_SilverBarterConfig;

static SilverBarterConfig GetSilverBarterConfig()
{
	if (!g_SilverBarterConfig)
	{
		Print("[SilverBarter] Initializing config...");
		g_SilverBarterConfig = new SilverBarterConfig();
		g_SilverBarterConfig.Load();
	}
	return g_SilverBarterConfig;
}
