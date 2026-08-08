static string SilverBarterSanitizeJsonError(string error)
{
	error.Replace("\r", " ");
	error.Replace("\n", " ");
	return error;
}

class SilverBarterConfigVersionProbe
{
	string CONFIG_VERSION;
};

static bool SilverBarterJsonHasKey(string path, string key)
{
	FileHandle handle = OpenFile(path, FileMode.READ);
	if (handle == 0)
		return false;

	string line;
	string token = "\"" + key + "\"";
	while (FGets(handle, line) >= 0)
	{
		if (line.IndexOf(token) >= 0)
		{
			CloseFile(handle);
			return true;
		}
	}

	CloseFile(handle);
	return false;
}

static bool SilverBarterCreateConfigBackup(string path, string suffix)
{
	string backupPath = path + suffix;
	if (!FileExist(backupPath) && !CopyFile(path, backupPath))
	{
		Print("[SilverBarter] ERROR: Config backup could not be created: " + backupPath);
		return false;
	}
	return true;
}

// Prueft ob ein Classname als Item/Magazin/Waffe in der Config existiert
static bool SilverBarterIsValidItemClass(string classname)
{
	if (!g_Game || classname == "")
		return false;
	if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
		return true;
	if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
		return true;
	if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
		return true;
	return false;
}

// SilverBarter Haupt-Konfiguration
class SilverBarterConfig
{
	// Config-Pfade
	private const static string MOD_FOLDER = "$profile:\\SilverBarter\\";
	private const static string CONFIG_NAME = "SilverBarterConfig.json";
	private const static string CURRENT_VERSION = "3";

	string CONFIG_VERSION = CURRENT_VERSION;

	// Globale Einstellungen
	bool m_debugMode = false;
	bool m_zenSkillsXPEnabled = true;
	ref array<string> m_quantityPriceClassnames = new array<string>;
	ref map<string, float> m_categoryValueMultipliers = new map<string, float>;

	// Trader-Konfiguration
	ref array<ref SilverTrader_ServerConfig> m_traders;

	void SilverBarterConfig()
	{
		m_traders = new array<ref SilverTrader_ServerConfig>;
	}

	void Load()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string path = MOD_FOLDER + CONFIG_NAME;
		if (FileExist(path))
		{
			Print("[SilverBarter] Loading config: " + path);
			bool configChanged = false;
			SilverBarterConfigVersionProbe versionProbe = new SilverBarterConfigVersionProbe();
			string versionError;
			if (!JsonFileLoader<SilverBarterConfigVersionProbe>.LoadFile(path, versionProbe, versionError))
			{
				Print("[SilverBarter] ERROR: Config version could not be read, file preserved: " + SilverBarterSanitizeJsonError(versionError));
				return;
			}

			if (versionProbe.CONFIG_VERSION == "1")
			{
				SilverBarterLegacyConfig legacyConfig = new SilverBarterLegacyConfig();
				string legacyError;
				if (!JsonFileLoader<SilverBarterLegacyConfig>.LoadFile(path, legacyConfig, legacyError))
				{
					Print("[SilverBarter] ERROR: Legacy config could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(legacyError));
					return;
				}
				if (!CreateMigrationBackup(path))
					return;
				MigrateLegacy(legacyConfig);
				configChanged = true;
				Print("[SilverBarter] Config migrated from array format to map format.");
			}
			else
			{
				if (versionProbe.CONFIG_VERSION != "" && versionProbe.CONFIG_VERSION != "2" && versionProbe.CONFIG_VERSION != CURRENT_VERSION)
				{
					Print("[SilverBarter] ERROR: Unsupported config version, file preserved: " + versionProbe.CONFIG_VERSION);
					return;
				}

				SilverBarterConfig loadedConfig = new SilverBarterConfig();
				loadedConfig.CONFIG_VERSION = "";
				loadedConfig.m_quantityPriceClassnames = null;
				loadedConfig.m_categoryValueMultipliers = null;
				loadedConfig.m_traders = null;
				string loadError;
				if (!JsonFileLoader<SilverBarterConfig>.LoadFile(path, loadedConfig, loadError))
				{
					Print("[SilverBarter] ERROR: Config could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(loadError));
					return;
				}
				if (versionProbe.CONFIG_VERSION == "2" && !SilverBarterCreateConfigBackup(path, ".v2.bak"))
					return;
				if (versionProbe.CONFIG_VERSION == "" || versionProbe.CONFIG_VERSION == "2")
					configChanged = true;
				ApplyLoadedConfig(loadedConfig);
			}

			if (EnsureDefaults())
				configChanged = true;
			CONFIG_VERSION = CURRENT_VERSION;
			if (configChanged)
			{
				Save();
				Print("[SilverBarter] Config load and update finished.");
			}
			else
			{
				Print("[SilverBarter] Config load finished, no update required.");
			}
		}
		else
		{
			SetDefaultValues();
			CONFIG_VERSION = CURRENT_VERSION;
			Save();
		}
	}

	private bool EnsureDefaults()
	{
		bool changed = false;
		if (!m_quantityPriceClassnames)
		{
			m_quantityPriceClassnames = new array<string>;
			changed = true;
		}
		if (!m_traders)
		{
			m_traders = new array<ref SilverTrader_ServerConfig>;
			changed = true;
		}
		if (!m_categoryValueMultipliers)
		{
			m_categoryValueMultipliers = new map<string, float>;
			changed = true;
		}
		if (NormalizeCategoryValueMultipliers())
			changed = true;

		if (EnsureCategoryValueMultiplier("weapons", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("attachments", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("magazines", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("ammo", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("tools", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("food", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("clothing", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("medical", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("electronic", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("base_building", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("vehicle_parts", 1.0)) changed = true;
		if (EnsureCategoryValueMultiplier("other", 1.0)) changed = true;

		foreach (SilverTrader_ServerConfig trader : m_traders)
		{
			if (trader)
				trader.ValidateAndNormalize();
		}
		return changed;
	}

	private bool NormalizeCategoryValueMultipliers()
	{
		bool changed = false;
		map<string, float> normalized = new map<string, float>;
		foreach (string category, float multiplier : m_categoryValueMultipliers)
		{
			string normalizedCategory = category;
			normalizedCategory.ToLower();
			if (normalizedCategory != "")
				normalized.Set(normalizedCategory, multiplier);
			if (normalizedCategory != category)
				changed = true;
		}
		if (normalized.Count() != m_categoryValueMultipliers.Count())
			changed = true;
		m_categoryValueMultipliers = normalized;
		return changed;
	}

	private void ApplyLoadedConfig(SilverBarterConfig loadedConfig)
	{
		CONFIG_VERSION = loadedConfig.CONFIG_VERSION;
		m_debugMode = loadedConfig.m_debugMode;
		m_zenSkillsXPEnabled = loadedConfig.m_zenSkillsXPEnabled;
		m_quantityPriceClassnames = loadedConfig.m_quantityPriceClassnames;
		m_categoryValueMultipliers = loadedConfig.m_categoryValueMultipliers;
		m_traders = loadedConfig.m_traders;
		loadedConfig.m_quantityPriceClassnames = null;
		loadedConfig.m_categoryValueMultipliers = null;
		loadedConfig.m_traders = null;
	}

	private void MigrateLegacy(SilverBarterLegacyConfig legacyConfig)
	{
		m_debugMode = legacyConfig.m_debugMode;
		m_zenSkillsXPEnabled = legacyConfig.m_zenSkillsXPEnabled;
		m_quantityPriceClassnames = legacyConfig.m_quantityPriceClassnames;
		legacyConfig.m_quantityPriceClassnames = null;

		m_categoryValueMultipliers = new map<string, float>;
		if (legacyConfig.m_categoryValueMultipliers)
		{
			foreach (SilverCategoryValueMultiplier legacyMultiplier : legacyConfig.m_categoryValueMultipliers)
			{
				if (legacyMultiplier && legacyMultiplier.category != "")
					m_categoryValueMultipliers.Set(legacyMultiplier.category, legacyMultiplier.multiplier);
			}
		}

		m_traders = new array<ref SilverTrader_ServerConfig>;
		if (legacyConfig.m_traders)
		{
			foreach (SilverTraderLegacyConfig legacyTrader : legacyConfig.m_traders)
			{
				if (legacyTrader)
					m_traders.Insert(MigrateLegacyTrader(legacyTrader));
			}
		}
	}

	private SilverTrader_ServerConfig MigrateLegacyTrader(SilverTraderLegacyConfig legacyTrader)
	{
		SilverTrader_ServerConfig trader = new SilverTrader_ServerConfig();
		trader.m_traderId = legacyTrader.m_traderId;
		trader.m_position = legacyTrader.m_position;
		trader.m_storageMaxSize = legacyTrader.m_storageMaxSize;
		trader.m_storageCommission = legacyTrader.m_storageCommission;
		trader.m_dumpingByAmountAlgorithm = legacyTrader.m_dumpingByAmountAlgorithm;
		trader.m_dumpingByAmountModifier = legacyTrader.m_dumpingByAmountModifier;
		trader.m_dumpingByBadQuality = legacyTrader.m_dumpingByBadQuality;
		trader.m_sellMaxQuantityPercent = legacyTrader.m_sellMaxQuantityPercent;
		trader.m_buyMaxQuantityPercent = legacyTrader.m_buyMaxQuantityPercent;
		trader.m_classname = legacyTrader.m_classname;
		trader.m_orientation = legacyTrader.m_orientation;
		trader.m_buyFilter = legacyTrader.m_buyFilter;
		trader.m_sellFilter = legacyTrader.m_sellFilter;
		trader.m_attachments = legacyTrader.m_attachments;
		legacyTrader.m_buyFilter = null;
		legacyTrader.m_sellFilter = null;
		legacyTrader.m_attachments = null;

		trader.m_commissionOverrides = ConvertLegacyCommissionOverrides(legacyTrader.m_commissionOverrides);
		trader.m_categoryValueMultipliers = ConvertLegacyCategoryMultipliers(legacyTrader.m_categoryValueMultipliers);
		trader.m_defaultItems = ConvertLegacyDefaultItems(legacyTrader.m_defaultItems);
		trader.m_limitedItems = ConvertLegacyLimitedItems(legacyTrader.m_limitedItems);
		return trader;
	}

	private map<string, float> ConvertLegacyCommissionOverrides(array<ref SilverCommissionOverride> entries)
	{
		map<string, float> result = new map<string, float>;
		if (!entries)
			return result;
		foreach (SilverCommissionOverride entry : entries)
		{
			if (entry && entry.classname != "")
				result.Set(entry.classname, entry.commission);
		}
		return result;
	}

	private map<string, float> ConvertLegacyCategoryMultipliers(array<ref SilverCategoryValueMultiplier> entries)
	{
		map<string, float> result = new map<string, float>;
		if (!entries)
			return result;
		foreach (SilverCategoryValueMultiplier entry : entries)
		{
			if (entry && entry.category != "")
				result.Set(entry.category, entry.multiplier);
		}
		return result;
	}

	private map<string, float> ConvertLegacyDefaultItems(array<ref SilverTrader_ItemEntry> entries)
	{
		map<string, float> result = new map<string, float>;
		if (!entries)
			return result;
		foreach (SilverTrader_ItemEntry entry : entries)
		{
			if (entry && entry.classname != "")
				result.Set(entry.classname, entry.quantity);
		}
		return result;
	}

	private map<string, int> ConvertLegacyLimitedItems(array<ref SilverTrader_LimitedItem> entries)
	{
		map<string, int> result = new map<string, int>;
		if (!entries)
			return result;
		foreach (SilverTrader_LimitedItem entry : entries)
		{
			if (entry && entry.classname != "")
				result.Set(entry.classname, entry.maxQuantity);
		}
		return result;
	}

	private bool CreateMigrationBackup(string path)
	{
		string backupPath = path + ".v1.bak";
		if (!FileExist(backupPath) && !CopyFile(path, backupPath))
		{
			Print("[SilverBarter] WARNING: Could not create migration backup: " + backupPath);
			return false;
		}
		return true;
	}

	void Save()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string saveError;
		if (!JsonFileLoader<SilverBarterConfig>.SaveFile(MOD_FOLDER + CONFIG_NAME, this, saveError))
			Print("[SilverBarter] ERROR: Config could not be saved: " + SilverBarterSanitizeJsonError(saveError));
	}

	void SetDefaultValues()
	{
		m_debugMode = false;
		m_zenSkillsXPEnabled = true;
		m_traders = new array<ref SilverTrader_ServerConfig>;
		SetDefaultCategoryValueMultipliers();

		// Beispiel-Trader
		SilverTrader_ServerConfig exampleTrader = new SilverTrader_ServerConfig();
		exampleTrader.m_traderId = 0;
		exampleTrader.m_classname = "SurvivorM_Mirek";
		exampleTrader.m_position = "6618.94 41.62 7151.61";
		exampleTrader.m_orientation = -43.268715;
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
		exampleTrader.m_sellFilter.Insert("!dzn_snowball_base");
		exampleTrader.m_sellFilter.Insert("!SeedBase");
		exampleTrader.m_sellFilter.Insert("!Splint");
		exampleTrader.m_sellFilter.Insert("!CookZ_EmptyCan");

		exampleTrader.m_attachments = new array<string>;
		exampleTrader.m_attachments.Insert("DownJacket_Orange");
		exampleTrader.m_attachments.Insert("Jeans_Black");
		exampleTrader.m_attachments.Insert("Slingbag_Black");
		exampleTrader.m_attachments.Insert("WoolGloves_Black");
		exampleTrader.m_attachments.Insert("BeanieHat_Black");
		exampleTrader.m_attachments.Insert("HikingBootsLow_Black");
		exampleTrader.m_attachments.Insert("Shemag_Green");

		// Item-spezifische Commission-Overrides (wertvolle/seltene Items)
		exampleTrader.m_commissionOverrides = new map<string, float>;

		SilverCommissionOverride leatherOverride = new SilverCommissionOverride();
		leatherOverride.classname = "TannedLeather";
		leatherOverride.commission = 0.2;
		exampleTrader.m_commissionOverrides.Insert(leatherOverride.classname, leatherOverride.commission);

		// Limitierte Items (werden bei jedem Restart auf maxQuantity zurueckgesetzt)
		exampleTrader.m_limitedItems = new map<string, int>;

		SilverTrader_LimitedItem limitedBook1 = new SilverTrader_LimitedItem();
		limitedBook1.classname = "ZenSkills_Book_Survival";
		limitedBook1.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook1.classname, limitedBook1.maxQuantity);

		SilverTrader_LimitedItem limitedBook2 = new SilverTrader_LimitedItem();
		limitedBook2.classname = "ZenSkills_Book_Crafting";
		limitedBook2.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook2.classname, limitedBook2.maxQuantity);

		SilverTrader_LimitedItem limitedBook3 = new SilverTrader_LimitedItem();
		limitedBook3.classname = "ZenSkills_Book_Hunting";
		limitedBook3.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook3.classname, limitedBook3.maxQuantity);

		SilverTrader_LimitedItem limitedBook4 = new SilverTrader_LimitedItem();
		limitedBook4.classname = "ZenSkills_Book_Gathering";
		limitedBook4.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook4.classname, limitedBook4.maxQuantity);

		// Standard-Items die der Trader von Anfang an hat
		exampleTrader.m_defaultItems = new map<string, float>;

		SilverTrader_ItemEntry item1 = new SilverTrader_ItemEntry();
		item1.classname = "AmmoBox_308Win_20Rnd";
		item1.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item1.classname, item1.quantity);

		SilverTrader_ItemEntry item2 = new SilverTrader_ItemEntry();
		item2.classname = "AmmoBox_22_50Rnd";
		item2.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item2.classname, item2.quantity);

		SilverTrader_ItemEntry item3 = new SilverTrader_ItemEntry();
		item3.classname = "AmmoBox_45ACP_25rnd";
		item3.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item3.classname, item3.quantity);

		SilverTrader_ItemEntry item4 = new SilverTrader_ItemEntry();
		item4.classname = "ZenSkills_Injector_ExpBoost";
		item4.quantity = 8;
		exampleTrader.m_defaultItems.Insert(item4.classname, item4.quantity);

		SilverTrader_ItemEntry item5 = new SilverTrader_ItemEntry();
		item5.classname = "ZenSkills_Injector_PerkReset";
		item5.quantity = 2;
		exampleTrader.m_defaultItems.Insert(item5.classname, item5.quantity);

		SilverTrader_ItemEntry item6 = new SilverTrader_ItemEntry();
		item6.classname = "MeatTenderizer";
		item6.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item6.classname, item6.quantity);

		SilverTrader_ItemEntry item11 = new SilverTrader_ItemEntry();
		item11.classname = "NailBox";
		item11.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item11.classname, item11.quantity);

		SilverTrader_ItemEntry item12 = new SilverTrader_ItemEntry();
		item12.classname = "Battery9V";
		item12.quantity = 30;
		exampleTrader.m_defaultItems.Insert(item12.classname, item12.quantity);

		SilverTrader_ItemEntry item13 = new SilverTrader_ItemEntry();
		item13.classname = "Ammo_45ACP";
		item13.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item13.classname, item13.quantity);

		SilverTrader_ItemEntry item14 = new SilverTrader_ItemEntry();
		item14.classname = "Ammo_380";
		item14.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item14.classname, item14.quantity);

		SilverTrader_ItemEntry item15 = new SilverTrader_ItemEntry();
		item15.classname = "Ammo_22";
		item15.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item15.classname, item15.quantity);

		SilverTrader_ItemEntry item16 = new SilverTrader_ItemEntry();
		item16.classname = "Ammo_12gaSlug";
		item16.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item16.classname, item16.quantity);

		SilverTrader_ItemEntry item17 = new SilverTrader_ItemEntry();
		item17.classname = "Ammo_12gaPellets";
		item17.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item17.classname, item17.quantity);

		SilverTrader_ItemEntry item18 = new SilverTrader_ItemEntry();
		item18.classname = "AmmoBox_12gaSlug_10Rnd";
		item18.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item18.classname, item18.quantity);

		SilverTrader_ItemEntry item19 = new SilverTrader_ItemEntry();
		item19.classname = "AmmoBox_00buck_10rnd";
		item19.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item19.classname, item19.quantity);

		SilverTrader_ItemEntry item20 = new SilverTrader_ItemEntry();
		item20.classname = "Hatchet";
		item20.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item20.classname, item20.quantity);

		SilverTrader_ItemEntry item21 = new SilverTrader_ItemEntry();
		item21.classname = "B95";
		item21.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item21.classname, item21.quantity);

		SilverTrader_ItemEntry item22 = new SilverTrader_ItemEntry();
		item22.classname = "MKII";
		item22.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item22.classname, item22.quantity);

		SilverTrader_ItemEntry item23 = new SilverTrader_ItemEntry();
		item23.classname = "P1";
		item23.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item23.classname, item23.quantity);

		SilverTrader_ItemEntry item24 = new SilverTrader_ItemEntry();
		item24.classname = "WaterBottle";
		item24.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item24.classname, item24.quantity);

		SilverTrader_ItemEntry item25 = new SilverTrader_ItemEntry();
		item25.classname = "Crackers";
		item25.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item25.classname, item25.quantity);

		SilverTrader_ItemEntry item26 = new SilverTrader_ItemEntry();
		item26.classname = "SaltySticks";
		item26.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item26.classname, item26.quantity);

		SilverTrader_ItemEntry item27 = new SilverTrader_ItemEntry();
		item27.classname = "Zagorky";
		item27.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item27.classname, item27.quantity);

		SilverTrader_ItemEntry item28 = new SilverTrader_ItemEntry();
		item28.classname = "Pate";
		item28.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item28.classname, item28.quantity);

		SilverTrader_ItemEntry item29 = new SilverTrader_ItemEntry();
		item29.classname = "BandageDressing";
		item29.quantity = 35;
		exampleTrader.m_defaultItems.Insert(item29.classname, item29.quantity);

		SilverTrader_ItemEntry item30 = new SilverTrader_ItemEntry();
		item30.classname = "Whetstone";
		item30.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item30.classname, item30.quantity);

		SilverTrader_ItemEntry item31 = new SilverTrader_ItemEntry();
		item31.classname = "Screwdriver";
		item31.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item31.classname, item31.quantity);

		SilverTrader_ItemEntry item32 = new SilverTrader_ItemEntry();
		item32.classname = "WoolGloves_Black";
		item32.quantity = 8;
		exampleTrader.m_defaultItems.Insert(item32.classname, item32.quantity);

		SilverTrader_ItemEntry item33 = new SilverTrader_ItemEntry();
		item33.classname = "Shemag_Green";
		item33.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item33.classname, item33.quantity);

		SilverTrader_ItemEntry item34 = new SilverTrader_ItemEntry();
		item34.classname = "HikingBootsLow_Black";
		item34.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item34.classname, item34.quantity);

		m_traders.Insert(exampleTrader);
	}

	private void SetDefaultCategoryValueMultipliers()
	{
		m_categoryValueMultipliers = new map<string, float>;
		InsertCategoryValueMultiplier("weapons", 1.0);
		InsertCategoryValueMultiplier("attachments", 1.0);
		InsertCategoryValueMultiplier("magazines", 1.0);
		InsertCategoryValueMultiplier("ammo", 1.0);
		InsertCategoryValueMultiplier("tools", 1.0);
		InsertCategoryValueMultiplier("food", 1.0);
		InsertCategoryValueMultiplier("clothing", 1.0);
		InsertCategoryValueMultiplier("medical", 1.0);
		InsertCategoryValueMultiplier("electronic", 1.0);
		InsertCategoryValueMultiplier("base_building", 1.0);
		InsertCategoryValueMultiplier("vehicle_parts", 1.0);
		InsertCategoryValueMultiplier("other", 1.0);
	}

	private void InsertCategoryValueMultiplier(string category, float multiplier)
	{
		m_categoryValueMultipliers.Insert(category, multiplier);
	}

	private bool EnsureCategoryValueMultiplier(string category, float multiplier)
	{
		if (!m_categoryValueMultipliers.Contains(category))
		{
			m_categoryValueMultipliers.Insert(category, multiplier);
			return true;
		}
		return false;
	}
};

// Kategorie-Wert-Multiplikator (wirkt auf BuyPrice je nach Item-Kategorie aus dem Cache)
class SilverCategoryValueMultiplier
{
	string category;
	float multiplier;
};

// Trader-Daten Klasse (Inventar eines Traders)
class SilverTrader_Data
{
	ref map<string, float> m_items;
	ref map<string, int> m_attachmentSurcharge;   // transient: eingefrorener Stueckaufschlag je classname (nur Rotating)
	ref map<string, ref array<ref SilverPreviewAttachment>> m_previewAttachments;   // transient: Preview-Baum je Waffen-classname
	int m_rotationRevision;                        // transient: erhoeht sich bei jeder Rotation, gegen TOCTOU beim Kauf

	void SilverTrader_Data()
	{
		m_items = new map<string, float>;
		m_attachmentSurcharge = new map<string, int>;
		m_previewAttachments = new map<string, ref array<ref SilverPreviewAttachment>>;
		m_rotationRevision = 0;
	}

	bool LoadFromJson(string path)
	{
		if (!FileExist(path))
			return false;

		Print("[SilverBarter] Loading trader data: " + path);

		SilverBarterConfigVersionProbe versionProbe = new SilverBarterConfigVersionProbe();
		string versionError;
		if (!JsonFileLoader<SilverBarterConfigVersionProbe>.LoadFile(path, versionProbe, versionError))
		{
			Print("[SilverBarter] ERROR: Trader data version could not be read, file preserved: " + SilverBarterSanitizeJsonError(versionError));
			return false;
		}

		if (versionProbe.CONFIG_VERSION != "" && versionProbe.CONFIG_VERSION != "2")
		{
			Print("[SilverBarter] ERROR: Unsupported trader data version, file preserved: " + versionProbe.CONFIG_VERSION);
			return false;
		}

		SilverTrader_DataJson jsonData = new SilverTrader_DataJson();
		jsonData.CONFIG_VERSION = "";
		jsonData.m_items = null;

		// m_items erst anfassen wenn jsonData gültig ist
		if (versionProbe.CONFIG_VERSION == "2")
		{
			string loadError;
			if (!JsonFileLoader<SilverTrader_DataJson>.LoadFile(path, jsonData, loadError) || !jsonData.m_items)
			{
				Print("[SilverBarter] ERROR: Trader data could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(loadError));
				return false;
			}
		}
		else if (SilverBarterJsonHasKey(path, "m_itemList"))
		{
			SilverTrader_DataJsonLegacy legacyData = new SilverTrader_DataJsonLegacy();
			string legacyError;
			if (!JsonFileLoader<SilverTrader_DataJsonLegacy>.LoadFile(path, legacyData, legacyError) || !legacyData.m_itemList)
			{
				Print("[SilverBarter] ERROR: Legacy trader data could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(legacyError));
				return false;
			}

			jsonData.CONFIG_VERSION = "2";
			jsonData.m_items = new map<string, float>;
			foreach (SilverTrader_ItemEntry legacyEntry : legacyData.m_itemList)
			{
				if (legacyEntry && legacyEntry.classname != "" && IsValidClassname(legacyEntry.classname))
					jsonData.m_items.Set(legacyEntry.classname, legacyEntry.quantity);
			}

			string backupPath = path + ".v1.bak";
			if (!FileExist(backupPath) && !CopyFile(path, backupPath))
			{
				Print("[SilverBarter] ERROR: Trader data migration backup could not be created: " + backupPath);
				return false;
			}
			if (!SaveMigratedTraderData(path, jsonData))
				return false;
			Print("[SilverBarter] Trader data migrated from array format to map format: " + path);
		}
		else if (SilverBarterJsonHasKey(path, "m_items"))
		{
			string unversionedError;
			if (!JsonFileLoader<SilverTrader_DataJson>.LoadFile(path, jsonData, unversionedError) || !jsonData.m_items)
			{
				Print("[SilverBarter] ERROR: Unversioned trader data could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(unversionedError));
				return false;
			}

			jsonData.CONFIG_VERSION = "2";
			string unversionedBackupPath = path + ".unversioned.bak";
			if (!FileExist(unversionedBackupPath) && !CopyFile(path, unversionedBackupPath))
			{
				Print("[SilverBarter] ERROR: Unversioned trader data backup could not be created: " + unversionedBackupPath);
				return false;
			}
			if (!SaveMigratedTraderData(path, jsonData))
				return false;
			Print("[SilverBarter] Unversioned trader data updated to format version 2: " + path);
		}
		else
		{
			Print("[SilverBarter] ERROR: Unknown trader data format, file preserved: " + path);
			return false;
		}

		if (!m_items)
			m_items = new map<string, float>;
		else
			m_items.Clear();

		foreach (string classname, float quantity : jsonData.m_items)
		{
			if (classname != "" && IsValidClassname(classname))
				m_items.Insert(classname, quantity);
		}
		Print("[SilverBarter] Trader data load finished.");
		return true;
	}

	void SaveToJson(string path)
	{
		SilverTrader_DataJson jsonData = new SilverTrader_DataJson();
		jsonData.CONFIG_VERSION = "2";
		jsonData.m_items = new map<string, float>;
		if (m_items)
		{
			foreach (string classname, float quantity : m_items)
				jsonData.m_items.Insert(classname, quantity);
		}

		string saveError;
		if (!JsonFileLoader<SilverTrader_DataJson>.SaveFile(path, jsonData, saveError))
			Print("[SilverBarter] ERROR: Trader data could not be saved: " + path + " | " + SilverBarterSanitizeJsonError(saveError));
	}

	private bool SaveMigratedTraderData(string path, SilverTrader_DataJson jsonData)
	{
		string migrationSaveError;
		if (!JsonFileLoader<SilverTrader_DataJson>.SaveFile(path, jsonData, migrationSaveError))
		{
			Print("[SilverBarter] ERROR: Migrated trader data could not be saved: " + SilverBarterSanitizeJsonError(migrationSaveError));
			return false;
		}
		return true;
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
	string CONFIG_VERSION = "2";
	ref map<string, float> m_items;
};

class SilverTrader_DataJsonLegacy
{
	ref array<ref SilverTrader_ItemEntry> m_itemList;
};

class SilverTrader_ItemEntry
{
	string classname;
	float quantity;
};

// Basis-Trader-Info Klasse (für Client/Server)
class SilverTrader_Info
{
	int m_traderId = -1;
	vector m_position;
	ref array<string> m_buyFilter = new array<string>;
	ref array<string> m_sellFilter = new array<string>;
	ref map<string, float> m_commissionOverrides = new map<string, float>; // Item-spezifische Commission
	ref map<string, float> m_categoryValueMultipliers = new map<string, float>; // Trader-spezifische Kategorie-Multiplikatoren (optional)
	int m_storageMaxSize = 5000;
	float m_storageCommission = 0.65;
	string m_dumpingByAmountAlgorithm = "linear";
	float m_dumpingByAmountModifier = 0.65;
	float m_dumpingByBadQuality = 0.5;
	float m_sellMaxQuantityPercent = 0.8;
	float m_buyMaxQuantityPercent = 0.9;

	// Ermittelt Commission fuer ein Item (Override oder Fallback)
	float GetCommissionForItem(string classname)
	{
		if (m_commissionOverrides)
		{
			float exactCommission;
			if (m_commissionOverrides.Find(classname, exactCommission))
				return exactCommission;

			string classnameLower = classname;
			classnameLower.ToLower();
			foreach (string overrideClassname, float overrideCommission : m_commissionOverrides)
			{
				string overrideLower = overrideClassname;
				overrideLower.ToLower();
				if (overrideLower == classnameLower)
					return overrideCommission;
			}

			string bestParentClassname;
			float bestParentCommission;
			foreach (string parentClassname, float parentCommission : m_commissionOverrides)
			{
				if (g_Game.IsKindOf(classname, parentClassname))
				{
					if (bestParentClassname == "" || g_Game.IsKindOf(parentClassname, bestParentClassname))
					{
						bestParentClassname = parentClassname;
						bestParentCommission = parentCommission;
					}
				}
			}
			if (bestParentClassname != "")
				return bestParentCommission;
		}

		// Fallback auf Standard-Commission
		return m_storageCommission;
	}

	// Grenzen pruefen und Config-Werte normalisieren. Liefert false, wenn der Trader verworfen werden muss.
	bool ValidateAndNormalize()
	{
		if (m_traderId < 0)
			return false;

		if (!m_buyFilter)
			m_buyFilter = new array<string>;
		if (!m_sellFilter)
			m_sellFilter = new array<string>;
		if (!m_commissionOverrides)
			m_commissionOverrides = new map<string, float>;
		if (!m_categoryValueMultipliers)
			m_categoryValueMultipliers = new map<string, float>;

		if (m_storageMaxSize <= 0)
			m_storageMaxSize = 5000;

		m_storageCommission = Math.Clamp(m_storageCommission, 0, 1);
		m_sellMaxQuantityPercent = Math.Clamp(m_sellMaxQuantityPercent, 0.01, 1);
		m_buyMaxQuantityPercent = Math.Clamp(m_buyMaxQuantityPercent, 0.01, 1);
		m_dumpingByAmountModifier = Math.Clamp(m_dumpingByAmountModifier, 0, 1);
		m_dumpingByBadQuality = Math.Clamp(m_dumpingByBadQuality, 0, 1);

		if (m_commissionOverrides)
		{
			for (int commissionIndex = 0; commissionIndex < m_commissionOverrides.Count(); commissionIndex++)
			{
				string commissionClassname = m_commissionOverrides.GetKey(commissionIndex);
				float commissionValue = m_commissionOverrides.GetElement(commissionIndex);
				m_commissionOverrides.Set(commissionClassname, Math.Clamp(commissionValue, 0, 1));
			}
		}

		map<string, float> normalizedMultipliers = new map<string, float>;
		foreach (string multiplierCategory, float multiplierValue : m_categoryValueMultipliers)
		{
			string normalizedCategory = multiplierCategory;
			normalizedCategory.ToLower();
			if (normalizedCategory != "" && multiplierValue > 0)
				normalizedMultipliers.Set(normalizedCategory, multiplierValue);
		}
		m_categoryValueMultipliers = normalizedMultipliers;

		return true;
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

// Rekursive Attachment-Spec fuer Pool-Waffen (verschachtelt moeglich: Optik->Batterie, Waffe->Magazin)
// Arbeitspaket fuer die iterative Attachment-Validierung (Spec-Liste + Tiefe).
class SilverAttachValidateJob
{
	array<ref SilverAttachmentSpec> m_Specs;   // plain Handle -> zeigt auf Config-Array
	int m_Depth;
};

class SilverAttachmentSpec
{
	// Maximale Verschachtelungstiefe (Ebene 0 = direkte Waffenaufsaetze). Begrenzt Preis, Sync und Spawn einheitlich.
	static const int MAX_DEPTH = 3;

	string classname;
	string slot;        // optional; leer = automatische Slot-Wahl, sonst fester Slot ueber aufgeloeste Slot-ID
	float fill = -1;    // Spawn-Fuellgrad 0..1 (Magazin/Quantity); negativ = Config-Default belassen, preisneutral
	ref array<ref SilverAttachmentSpec> attachments;

	// Bereinigt einen Attachment-Baum: ungueltige Classnames und Slot-Namen werden verworfen, die Tiefe auf
	// MAX_DEPTH begrenzt. Bewusst iterativ ueber eine Queue statt rekursiv - ein rekursiver Aufruf im Schleifen-
	// koerper zerstoert in diesem Enforce-Build den Schleifenzustand des Aufrufers. Parent-Kompatibilitaet kann
	// statisch nicht geprueft werden, dafuer greift der Runtime-Check beim Spawn.
	static void ValidateList(array<ref SilverAttachmentSpec> rootSpecs, int depth = 0)
	{
		if (!rootSpecs)
			return;

		array<ref SilverAttachValidateJob> queue = new array<ref SilverAttachValidateJob>;
		SilverAttachValidateJob rootJob = new SilverAttachValidateJob();
		rootJob.m_Specs = rootSpecs;
		rootJob.m_Depth = depth;
		queue.Insert(rootJob);

		while (queue.Count() > 0)
		{
			SilverAttachValidateJob job = queue.Get(0);
			queue.Remove(0);
			if (!job || !job.m_Specs)
				continue;

			array<ref SilverAttachmentSpec> specs = job.m_Specs;
			int jobDepth = job.m_Depth;

			for (int i = specs.Count() - 1; i >= 0; i--)
			{
				SilverAttachmentSpec spec = specs.Get(i);
				if (!spec || spec.classname == "" || !SilverBarterIsValidItemClass(spec.classname))
				{
					specs.Remove(i);
					continue;
				}
				if (spec.slot != "" && InventorySlots.GetSlotIdFromString(spec.slot) == InventorySlots.INVALID)
				{
					Print("[SilverBarter] WARNING: Invalid attachment slot removed: " + spec.slot + " (" + spec.classname + ")");
					specs.Remove(i);
					continue;
				}
				if (!spec.attachments)
				{
					spec.attachments = new array<ref SilverAttachmentSpec>;
				}
				else if (jobDepth + 1 >= MAX_DEPTH)
				{
					spec.attachments.Clear();
				}
				else if (spec.attachments.Count() > 0)
				{
					SilverAttachValidateJob childJob = new SilverAttachValidateJob();
					childJob.m_Specs = spec.attachments;
					childJob.m_Depth = jobDepth + 1;
					queue.Insert(childJob);
				}
			}
		}
	}
};

// Pool-Item fuer rotierende Haendler (mit Gewichtung)
class SilverTrader_PoolItem
{
	string classname;
	int quantity;       // Menge pro Rotation
	float weight;       // Gewichtung (1.0 = normal, 0.1 = sehr selten)
	ref array<ref SilverAttachmentSpec> attachments;   // optional: Aufsaetze die an der Waffe spawnen
};

// Flache, gesyncte Beschreibung eines Attachment-Knotens fuer die Client-Preview (transient, kein JSON).
// m_ParentIndex = -1 -> direkt an der Waffe, sonst Index eines frueheren Knotens in derselben Liste.
class SilverPreviewAttachment
{
	string m_Classname;
	string m_Slot;
	int m_ParentIndex;
};

// Rotierender Haendler Config (komplett isoliert vom normalen Trader)
class SilverRotatingTrader_Config : SilverTrader_Info
{
	string m_classname;
	ref array<string> m_attachments = new array<string>;
	ref array<string> m_spawnPositions = new array<string>;                        // Mehrere Spawn-Positionen (zufaellig bei Restart)
	float m_orientation;
	int m_rotationIntervalMinutes = 60;                        // Rotationsintervall in Minuten
	int m_activeSlots = 5;                                    // Wie viele Items pro Rotation aktiv
	ref array<ref SilverTrader_PoolItem> m_poolItems = new array<ref SilverTrader_PoolItem>;     // Gesamtkatalog
	bool m_enableZenMapMarker = false;                            // ZenMap Marker auf Karte anzeigen
	string m_zenMapMarkerName;                            // Name des Markers auf der Karte
	string m_zenMapMarkerIcon;                            // Icon-Pfad (leer = Standard)

	override bool ValidateAndNormalize()
	{
		if (!super.ValidateAndNormalize())
			return false;

		if (m_classname == "")
			return false;

		if (!m_attachments)
			m_attachments = new array<string>;
		if (!m_spawnPositions)
			m_spawnPositions = new array<string>;
		if (!m_poolItems)
			m_poolItems = new array<ref SilverTrader_PoolItem>;

		foreach (SilverTrader_PoolItem poolItem : m_poolItems)
		{
			if (!poolItem)
				continue;
			if (!poolItem.attachments)
				poolItem.attachments = new array<ref SilverAttachmentSpec>;
			else
				SilverAttachmentSpec.ValidateList(poolItem.attachments);
		}

		if (m_rotationIntervalMinutes <= 0)
			m_rotationIntervalMinutes = 60;

		if (m_activeSlots <= 0)
			m_activeSlots = 5;

		return true;
	}
};

// Server-spezifische Trader-Config (erweitert Info um Spawn-Daten)
class SilverTrader_ServerConfig : SilverTrader_Info
{
	string m_classname;
	ref array<string> m_attachments = new array<string>;
	ref map<string, float> m_defaultItems = new map<string, float>; // Start-Items fuer Trader
	ref map<string, int> m_limitedItems = new map<string, int>; // Bei Restart auf fixen Wert setzen
	float m_orientation;

	override bool ValidateAndNormalize()
	{
		if (!super.ValidateAndNormalize())
			return false;

		if (m_classname == "")
			return false;

		if (!m_attachments)
			m_attachments = new array<string>;
		if (!m_defaultItems)
			m_defaultItems = new map<string, float>;
		if (!m_limitedItems)
			m_limitedItems = new map<string, int>;

		return true;
	}
};

// Nur fuer die einmalige Migration der bisherigen Array-Konfigurationen.
class SilverTraderLegacyInfo
{
	int m_traderId;
	vector m_position;
	ref array<string> m_buyFilter;
	ref array<string> m_sellFilter;
	ref array<ref SilverCommissionOverride> m_commissionOverrides;
	ref array<ref SilverCategoryValueMultiplier> m_categoryValueMultipliers;
	int m_storageMaxSize;
	float m_storageCommission;
	string m_dumpingByAmountAlgorithm;
	float m_dumpingByAmountModifier;
	float m_dumpingByBadQuality;
	float m_sellMaxQuantityPercent;
	float m_buyMaxQuantityPercent;
};

class SilverTraderLegacyConfig : SilverTraderLegacyInfo
{
	string m_classname;
	ref array<string> m_attachments;
	ref array<ref SilverTrader_ItemEntry> m_defaultItems;
	ref array<ref SilverTrader_LimitedItem> m_limitedItems;
	float m_orientation;
};

class SilverRotatingTraderLegacyConfig : SilverTraderLegacyInfo
{
	string m_classname;
	ref array<string> m_attachments;
	ref array<string> m_spawnPositions;
	float m_orientation;
	int m_rotationIntervalMinutes;
	int m_activeSlots;
	ref array<ref SilverTrader_PoolItem> m_poolItems;
	bool m_enableZenMapMarker;
	string m_zenMapMarkerName;
	string m_zenMapMarkerIcon;
};

class SilverBarterLegacyConfig
{
	string CONFIG_VERSION;
	bool m_debugMode;
	bool m_zenSkillsXPEnabled = true;
	ref array<string> m_quantityPriceClassnames;
	ref array<ref SilverCategoryValueMultiplier> m_categoryValueMultipliers;
	ref array<ref SilverTraderLegacyConfig> m_traders;
};

class SilverRotatingTradersLegacyConfig
{
	string CONFIG_VERSION;
	ref array<ref SilverRotatingTraderLegacyConfig> m_rotatingTraders;
};

// Separate Config fuer rotierende Haendler
class SilverRotatingTradersConfig
{
	private const static string MOD_FOLDER = "$profile:\\SilverBarter\\";
	private const static string CONFIG_NAME = "SilverBarterRotatingTraders.json";
	private const static string CURRENT_VERSION = "3";

	string CONFIG_VERSION = CURRENT_VERSION;
	ref array<ref SilverRotatingTrader_Config> m_rotatingTraders;

	void SilverRotatingTradersConfig()
	{
		m_rotatingTraders = new array<ref SilverRotatingTrader_Config>;
	}

	void Load()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string path = MOD_FOLDER + CONFIG_NAME;
		if (FileExist(path))
		{
			Print("[SilverBarter] Loading rotating traders config: " + path);
			bool configChanged = false;
			SilverBarterConfigVersionProbe versionProbe = new SilverBarterConfigVersionProbe();
			string versionError;
			if (!JsonFileLoader<SilverBarterConfigVersionProbe>.LoadFile(path, versionProbe, versionError))
			{
				Print("[SilverBarter] ERROR: Rotating traders config version could not be read, file preserved: " + SilverBarterSanitizeJsonError(versionError));
				return;
			}

			if (versionProbe.CONFIG_VERSION == "1")
			{
				SilverRotatingTradersLegacyConfig legacyConfig = new SilverRotatingTradersLegacyConfig();
				string legacyError;
				if (!JsonFileLoader<SilverRotatingTradersLegacyConfig>.LoadFile(path, legacyConfig, legacyError))
				{
					Print("[SilverBarter] ERROR: Legacy rotating traders config could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(legacyError));
					return;
				}
				if (!CreateMigrationBackup(path))
					return;
				MigrateLegacy(legacyConfig);
				configChanged = true;
				Print("[SilverBarter] Rotating traders config migrated from array format to map format.");
			}
			else
			{
				if (versionProbe.CONFIG_VERSION != "" && versionProbe.CONFIG_VERSION != "2" && versionProbe.CONFIG_VERSION != CURRENT_VERSION)
				{
					Print("[SilverBarter] ERROR: Unsupported rotating traders config version, file preserved: " + versionProbe.CONFIG_VERSION);
					return;
				}

				SilverRotatingTradersConfig loadedConfig = new SilverRotatingTradersConfig();
				loadedConfig.CONFIG_VERSION = "";
				loadedConfig.m_rotatingTraders = null;
				string loadError;
				if (!JsonFileLoader<SilverRotatingTradersConfig>.LoadFile(path, loadedConfig, loadError))
				{
					Print("[SilverBarter] ERROR: Rotating traders config could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(loadError));
					return;
				}
				if (versionProbe.CONFIG_VERSION == "2" && !SilverBarterCreateConfigBackup(path, ".v2.bak"))
					return;
				if (versionProbe.CONFIG_VERSION == "" || versionProbe.CONFIG_VERSION == "2")
					configChanged = true;
				CONFIG_VERSION = loadedConfig.CONFIG_VERSION;
				m_rotatingTraders = loadedConfig.m_rotatingTraders;
				loadedConfig.m_rotatingTraders = null;
			}

			if (!m_rotatingTraders)
			{
				m_rotatingTraders = new array<ref SilverRotatingTrader_Config>;
				configChanged = true;
			}
			foreach (SilverRotatingTrader_Config rotatingTrader : m_rotatingTraders)
			{
				if (rotatingTrader)
					rotatingTrader.ValidateAndNormalize();
			}
			CONFIG_VERSION = CURRENT_VERSION;
			if (configChanged)
			{
				Save();
				Print("[SilverBarter] Rotating traders config load and update finished.");
			}
			else
			{
				Print("[SilverBarter] Rotating traders config load finished, no update required.");
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
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string saveError;
		if (!JsonFileLoader<SilverRotatingTradersConfig>.SaveFile(MOD_FOLDER + CONFIG_NAME, this, saveError))
			Print("[SilverBarter] ERROR: Rotating traders config could not be saved: " + SilverBarterSanitizeJsonError(saveError));
	}

	private void MigrateLegacy(SilverRotatingTradersLegacyConfig legacyConfig)
	{
		m_rotatingTraders = new array<ref SilverRotatingTrader_Config>;
		if (!legacyConfig.m_rotatingTraders)
			return;

		foreach (SilverRotatingTraderLegacyConfig legacyTrader : legacyConfig.m_rotatingTraders)
		{
			if (!legacyTrader)
				continue;

			SilverRotatingTrader_Config trader = new SilverRotatingTrader_Config();
			trader.m_traderId = legacyTrader.m_traderId;
			trader.m_position = legacyTrader.m_position;
			trader.m_storageMaxSize = legacyTrader.m_storageMaxSize;
			trader.m_storageCommission = legacyTrader.m_storageCommission;
			trader.m_dumpingByAmountAlgorithm = legacyTrader.m_dumpingByAmountAlgorithm;
			trader.m_dumpingByAmountModifier = legacyTrader.m_dumpingByAmountModifier;
			trader.m_dumpingByBadQuality = legacyTrader.m_dumpingByBadQuality;
			trader.m_sellMaxQuantityPercent = legacyTrader.m_sellMaxQuantityPercent;
			trader.m_buyMaxQuantityPercent = legacyTrader.m_buyMaxQuantityPercent;
			trader.m_classname = legacyTrader.m_classname;
			trader.m_orientation = legacyTrader.m_orientation;
			trader.m_rotationIntervalMinutes = legacyTrader.m_rotationIntervalMinutes;
			trader.m_activeSlots = legacyTrader.m_activeSlots;
			trader.m_enableZenMapMarker = legacyTrader.m_enableZenMapMarker;
			trader.m_zenMapMarkerName = legacyTrader.m_zenMapMarkerName;
			trader.m_zenMapMarkerIcon = legacyTrader.m_zenMapMarkerIcon;
			trader.m_buyFilter = legacyTrader.m_buyFilter;
			trader.m_sellFilter = legacyTrader.m_sellFilter;
			trader.m_attachments = legacyTrader.m_attachments;
			trader.m_spawnPositions = legacyTrader.m_spawnPositions;
			trader.m_poolItems = legacyTrader.m_poolItems;
			legacyTrader.m_buyFilter = null;
			legacyTrader.m_sellFilter = null;
			legacyTrader.m_attachments = null;
			legacyTrader.m_spawnPositions = null;
			legacyTrader.m_poolItems = null;

			trader.m_commissionOverrides = new map<string, float>;
			if (legacyTrader.m_commissionOverrides)
			{
				foreach (SilverCommissionOverride commissionEntry : legacyTrader.m_commissionOverrides)
				{
					if (commissionEntry && commissionEntry.classname != "")
						trader.m_commissionOverrides.Set(commissionEntry.classname, commissionEntry.commission);
				}
			}

			trader.m_categoryValueMultipliers = new map<string, float>;
			if (legacyTrader.m_categoryValueMultipliers)
			{
				foreach (SilverCategoryValueMultiplier multiplierEntry : legacyTrader.m_categoryValueMultipliers)
				{
					if (multiplierEntry && multiplierEntry.category != "")
						trader.m_categoryValueMultipliers.Set(multiplierEntry.category, multiplierEntry.multiplier);
				}
			}

			m_rotatingTraders.Insert(trader);
		}
	}

	private bool CreateMigrationBackup(string path)
	{
		string backupPath = path + ".v1.bak";
		if (!FileExist(backupPath) && !CopyFile(path, backupPath))
		{
			Print("[SilverBarter] WARNING: Could not create rotating config migration backup: " + backupPath);
			return false;
		}
		return true;
	}

	void SetDefaultValues()
	{
		m_rotatingTraders = new array<ref SilverRotatingTrader_Config>;

		SilverRotatingTrader_Config rotatingTrader = new SilverRotatingTrader_Config();
		rotatingTrader.m_traderId = 100;
		rotatingTrader.m_classname = "SurvivorM_Boris";
		rotatingTrader.m_spawnPositions = new array<string>;
		rotatingTrader.m_spawnPositions.Insert("6744.79 51.09 11380.01");
		rotatingTrader.m_spawnPositions.Insert("3634.65 99.05 7496.00");
		rotatingTrader.m_spawnPositions.Insert("4152.52 74.094 7759.84");
		rotatingTrader.m_spawnPositions.Insert("5226.59 35.481 8586.15");
		rotatingTrader.m_spawnPositions.Insert("6294.12 20.80 9524.32");
		rotatingTrader.m_spawnPositions.Insert("8172.09 31.48 10659.29");
		rotatingTrader.m_orientation = 0;
		rotatingTrader.m_rotationIntervalMinutes = 60;
		rotatingTrader.m_activeSlots = 5;
		rotatingTrader.m_storageMaxSize = 5000;
		rotatingTrader.m_storageCommission = 0.8;
		rotatingTrader.m_dumpingByAmountAlgorithm = "linear";
		rotatingTrader.m_dumpingByAmountModifier = 0.65;
		rotatingTrader.m_dumpingByBadQuality = 0.5;
		rotatingTrader.m_sellMaxQuantityPercent = 0.8;
		rotatingTrader.m_buyMaxQuantityPercent = 0.9;
		rotatingTrader.m_enableZenMapMarker = true;
		rotatingTrader.m_zenMapMarkerName = "Yuri";
		rotatingTrader.m_zenMapMarkerIcon = "";

		rotatingTrader.m_buyFilter = new array<string>;
		rotatingTrader.m_buyFilter.Insert("Inventory_Base");
		rotatingTrader.m_buyFilter.Insert("Weapon_Base");
		rotatingTrader.m_buyFilter.Insert("Magazine_Base");
		rotatingTrader.m_buyFilter.Insert("Ammunition_Base");

		rotatingTrader.m_sellFilter = new array<string>;
		rotatingTrader.m_sellFilter.Insert("Inventory_Base");
		rotatingTrader.m_sellFilter.Insert("Weapon_Base");
		rotatingTrader.m_sellFilter.Insert("Magazine_Base");
		rotatingTrader.m_sellFilter.Insert("Ammunition_Base");
		rotatingTrader.m_sellFilter.Insert("!SyringeFull");
		rotatingTrader.m_sellFilter.Insert("!Paper");
		rotatingTrader.m_sellFilter.Insert("!Zen_EmptyFood");
		rotatingTrader.m_sellFilter.Insert("!FenceKit");
		rotatingTrader.m_sellFilter.Insert("!WatchtowerKit");
		rotatingTrader.m_sellFilter.Insert("!ShelterKit");
		rotatingTrader.m_sellFilter.Insert("!TerritoryFlagKit");
		rotatingTrader.m_sellFilter.Insert("!Flag_Base");
		rotatingTrader.m_sellFilter.Insert("!Rag");
		rotatingTrader.m_sellFilter.Insert("!BurlapStrip");
		rotatingTrader.m_sellFilter.Insert("!Stone");
		rotatingTrader.m_sellFilter.Insert("!SmallStone");
		rotatingTrader.m_sellFilter.Insert("!Firewood");
		rotatingTrader.m_sellFilter.Insert("!BoneHook");
		rotatingTrader.m_sellFilter.Insert("!WoodenHook");
		rotatingTrader.m_sellFilter.Insert("!Bark_ColorBase");
		rotatingTrader.m_sellFilter.Insert("!Bone");
		rotatingTrader.m_sellFilter.Insert("!Bait");
		rotatingTrader.m_sellFilter.Insert("!BoneBait");
		rotatingTrader.m_sellFilter.Insert("!Barrel_ColorBase");
		rotatingTrader.m_sellFilter.Insert("!FireplaceBase");
		rotatingTrader.m_sellFilter.Insert("!CookingStand");
		rotatingTrader.m_sellFilter.Insert("!WoodenStick");
		rotatingTrader.m_sellFilter.Insert("!Torch");
		rotatingTrader.m_sellFilter.Insert("!LongWoodenStick");
		rotatingTrader.m_sellFilter.Insert("!SharpWoodenStick");
		rotatingTrader.m_sellFilter.Insert("!HandDrillKit");
		rotatingTrader.m_sellFilter.Insert("!Spear");
		rotatingTrader.m_sellFilter.Insert("!SmallGuts");
		rotatingTrader.m_sellFilter.Insert("!Guts");
		rotatingTrader.m_sellFilter.Insert("!Worm");
		rotatingTrader.m_sellFilter.Insert("!ImprovisedFishingRod");
		rotatingTrader.m_sellFilter.Insert("!TripwireTrap");
		rotatingTrader.m_sellFilter.Insert("!RabbitSnareTrap");
		rotatingTrader.m_sellFilter.Insert("!FishNetTrap");
		rotatingTrader.m_sellFilter.Insert("!SmallFishTrap");
		rotatingTrader.m_sellFilter.Insert("!Empty_ZenJameson");
		rotatingTrader.m_sellFilter.Insert("!Zen_CamoShelterKit");
		rotatingTrader.m_sellFilter.Insert("!Empty_Honey");
		rotatingTrader.m_sellFilter.Insert("!Empty_Marmalade");
		rotatingTrader.m_sellFilter.Insert("!Empty_Can_Opened");
		rotatingTrader.m_sellFilter.Insert("!BF_DoorBarricadeKit");
		rotatingTrader.m_sellFilter.Insert("!BF_WindowBarricadeKit");
		rotatingTrader.m_sellFilter.Insert("!BF_WindowBarricadeMedKit");
		rotatingTrader.m_sellFilter.Insert("!BF_DoubleDoorBarricadeKit");
		rotatingTrader.m_sellFilter.Insert("!bl_improvised_whetstone");
		rotatingTrader.m_sellFilter.Insert("!bl_improvised_sewing_kit");
		rotatingTrader.m_sellFilter.Insert("!Single_Match");

		rotatingTrader.m_commissionOverrides = new map<string, float>;

		rotatingTrader.m_attachments = new array<string>;
		rotatingTrader.m_attachments.Insert("WoolCoat_Black");
		rotatingTrader.m_attachments.Insert("CargoPants_Black");
		rotatingTrader.m_attachments.Insert("MilitaryBoots_Black");
		rotatingTrader.m_attachments.Insert("Slingbag_Black");
		rotatingTrader.m_attachments.Insert("WoolGloves_Black");
		rotatingTrader.m_attachments.Insert("GP5GasMask");

		rotatingTrader.m_poolItems = new array<ref SilverTrader_PoolItem>;

		SilverTrader_PoolItem poolItem1 = new SilverTrader_PoolItem();
		poolItem1.classname = "AKM";
		poolItem1.quantity = 1;
		poolItem1.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem1);

		SilverTrader_PoolItem poolItem2 = new SilverTrader_PoolItem();
		poolItem2.classname = "M4A1";
		poolItem2.quantity = 1;
		poolItem2.weight = 0.05;
		rotatingTrader.m_poolItems.Insert(poolItem2);

		SilverTrader_PoolItem poolItem3 = new SilverTrader_PoolItem();
		poolItem3.classname = "UMP45";
		poolItem3.quantity = 2;
		poolItem3.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem3);

		SilverTrader_PoolItem poolItem4 = new SilverTrader_PoolItem();
		poolItem4.classname = "MP5K";
		poolItem4.quantity = 2;
		poolItem4.weight = 0.7;
		rotatingTrader.m_poolItems.Insert(poolItem4);

		SilverTrader_PoolItem poolItem5 = new SilverTrader_PoolItem();
		poolItem5.classname = "Ammo_762x39";
		poolItem5.quantity = 5;
		poolItem5.weight = 1.0;
		rotatingTrader.m_poolItems.Insert(poolItem5);

		SilverTrader_PoolItem poolItem6 = new SilverTrader_PoolItem();
		poolItem6.classname = "M68Optic";
		poolItem6.quantity = 1;
		poolItem6.weight = 0.3;
		rotatingTrader.m_poolItems.Insert(poolItem6);

		SilverTrader_PoolItem poolItem7 = new SilverTrader_PoolItem();
		poolItem7.classname = "FAMAS";
		poolItem7.quantity = 1;
		poolItem7.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem7);

		SilverTrader_PoolItem poolItem8 = new SilverTrader_PoolItem();
		poolItem8.classname = "AugShort";
		poolItem8.quantity = 1;
		poolItem8.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem8);

		SilverTrader_PoolItem poolItem9 = new SilverTrader_PoolItem();
		poolItem9.classname = "SVD";
		poolItem9.quantity = 1;
		poolItem9.weight = 0.05;
		rotatingTrader.m_poolItems.Insert(poolItem9);

		SilverTrader_PoolItem poolItem10 = new SilverTrader_PoolItem();
		poolItem10.classname = "VSS";
		poolItem10.quantity = 1;
		poolItem10.weight = 0.08;
		rotatingTrader.m_poolItems.Insert(poolItem10);

		SilverTrader_PoolItem poolItem11 = new SilverTrader_PoolItem();
		poolItem11.classname = "Saiga";
		poolItem11.quantity = 1;
		poolItem11.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem11);

		SilverTrader_PoolItem poolItem12 = new SilverTrader_PoolItem();
		poolItem12.classname = "M16A2";
		poolItem12.quantity = 1;
		poolItem12.weight = 0.12;
		rotatingTrader.m_poolItems.Insert(poolItem12);

		SilverTrader_PoolItem poolItem13 = new SilverTrader_PoolItem();
		poolItem13.classname = "Deagle";
		poolItem13.quantity = 1;
		poolItem13.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem13);

		SilverTrader_PoolItem poolItem14 = new SilverTrader_PoolItem();
		poolItem14.classname = "AmmoBox_556x45_20Rnd";
		poolItem14.quantity = 3;
		poolItem14.weight = 0.8;
		rotatingTrader.m_poolItems.Insert(poolItem14);

		SilverTrader_PoolItem poolItem15 = new SilverTrader_PoolItem();
		poolItem15.classname = "AmmoBox_308Win_20Rnd";
		poolItem15.quantity = 2;
		poolItem15.weight = 0.6;
		rotatingTrader.m_poolItems.Insert(poolItem15);

		SilverTrader_PoolItem poolItem16 = new SilverTrader_PoolItem();
		poolItem16.classname = "AmmoBox_357_20Rnd";
		poolItem16.quantity = 3;
		poolItem16.weight = 0.7;
		rotatingTrader.m_poolItems.Insert(poolItem16);

		SilverTrader_PoolItem poolItem17 = new SilverTrader_PoolItem();
		poolItem17.classname = "AmmoBox_9x39_20Rnd";
		poolItem17.quantity = 2;
		poolItem17.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem17);

		SilverTrader_PoolItem poolItem18 = new SilverTrader_PoolItem();
		poolItem18.classname = "AmmoBox_762x39_20Rnd";
		poolItem18.quantity = 3;
		poolItem18.weight = 0.8;
		rotatingTrader.m_poolItems.Insert(poolItem18);

		SilverTrader_PoolItem poolItem19 = new SilverTrader_PoolItem();
		poolItem19.classname = "Mag_MP5_30Rnd";
		poolItem19.quantity = 2;
		poolItem19.weight = 0.6;
		rotatingTrader.m_poolItems.Insert(poolItem19);

		SilverTrader_PoolItem poolItem20 = new SilverTrader_PoolItem();
		poolItem20.classname = "Mag_AKM_Drum75Rnd";
		poolItem20.quantity = 1;
		poolItem20.weight = 0.05;
		rotatingTrader.m_poolItems.Insert(poolItem20);

		SilverTrader_PoolItem poolItem21 = new SilverTrader_PoolItem();
		poolItem21.classname = "Mag_STANAG_60Rnd";
		poolItem21.quantity = 1;
		poolItem21.weight = 0.08;
		rotatingTrader.m_poolItems.Insert(poolItem21);

		SilverTrader_PoolItem poolItem22 = new SilverTrader_PoolItem();
		poolItem22.classname = "Mag_STANAG_30Rnd";
		poolItem22.quantity = 2;
		poolItem22.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem22);

		SilverTrader_PoolItem poolItem23 = new SilverTrader_PoolItem();
		poolItem23.classname = "Mag_Aug_30Rnd";
		poolItem23.quantity = 2;
		poolItem23.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem23);

		SilverTrader_PoolItem poolItem24 = new SilverTrader_PoolItem();
		poolItem24.classname = "Mag_FAMAS_25Rnd";
		poolItem24.quantity = 2;
		poolItem24.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem24);

		SilverTrader_PoolItem poolItem25 = new SilverTrader_PoolItem();
		poolItem25.classname = "Mag_SVD_10Rnd";
		poolItem25.quantity = 1;
		poolItem25.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem25);

		SilverTrader_PoolItem poolItem26 = new SilverTrader_PoolItem();
		poolItem26.classname = "Mag_Saiga_Drum20Rnd";
		poolItem26.quantity = 1;
		poolItem26.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem26);

		SilverTrader_PoolItem poolItem27 = new SilverTrader_PoolItem();
		poolItem27.classname = "Mag_Saiga_8Rnd";
		poolItem27.quantity = 2;
		poolItem27.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem27);

		SilverTrader_PoolItem poolItem28 = new SilverTrader_PoolItem();
		poolItem28.classname = "M4_Suppressor";
		poolItem28.quantity = 1;
		poolItem28.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem28);

		SilverTrader_PoolItem poolItem29 = new SilverTrader_PoolItem();
		poolItem29.classname = "AK_Suppressor";
		poolItem29.quantity = 1;
		poolItem29.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem29);

		SilverTrader_PoolItem poolItem30 = new SilverTrader_PoolItem();
		poolItem30.classname = "PistolSuppressor";
		poolItem30.quantity = 1;
		poolItem30.weight = 0.25;
		rotatingTrader.m_poolItems.Insert(poolItem30);

		SilverTrader_PoolItem poolItem31 = new SilverTrader_PoolItem();
		poolItem31.classname = "SewingKit";
		poolItem31.quantity = 2;
		poolItem31.weight = 0.6;
		rotatingTrader.m_poolItems.Insert(poolItem31);

		SilverTrader_PoolItem poolItem32 = new SilverTrader_PoolItem();
		poolItem32.classname = "LeatherSewingKit";
		poolItem32.quantity = 2;
		poolItem32.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem32);

		SilverTrader_PoolItem poolItem33 = new SilverTrader_PoolItem();
		poolItem33.classname = "PlateCarrierVest_Black";
		poolItem33.quantity = 1;
		poolItem33.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem33);

		SilverTrader_PoolItem poolItem34 = new SilverTrader_PoolItem();
		poolItem34.classname = "PlateCarrierVest_Camo";
		poolItem34.quantity = 1;
		poolItem34.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem34);

		SilverTrader_PoolItem poolItem35 = new SilverTrader_PoolItem();
		poolItem35.classname = "BallisticHelmet_Black";
		poolItem35.quantity = 1;
		poolItem35.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem35);

		SilverTrader_PoolItem poolItem36 = new SilverTrader_PoolItem();
		poolItem36.classname = "dzn_module_card";
		poolItem36.quantity = 1;
		poolItem36.weight = 0.3;
		rotatingTrader.m_poolItems.Insert(poolItem36);

		SilverTrader_PoolItem poolItem37 = new SilverTrader_PoolItem();
		poolItem37.classname = "dzn_module_lantia";
		poolItem37.quantity = 1;
		poolItem37.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem37);

		SilverTrader_PoolItem poolItem38 = new SilverTrader_PoolItem();
		poolItem38.classname = "dzn_module_surge";
		poolItem38.quantity = 1;
		poolItem38.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem38);

		SilverTrader_PoolItem poolItem39 = new SilverTrader_PoolItem();
		poolItem39.classname = "dzn_module_ext";
		poolItem39.quantity = 1;
		poolItem39.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem39);

		SilverTrader_PoolItem poolItem40 = new SilverTrader_PoolItem();
		poolItem40.classname = "dzn_module_ext2";
		poolItem40.quantity = 1;
		poolItem40.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem40);

		SilverTrader_PoolItem poolItem41 = new SilverTrader_PoolItem();
		poolItem41.classname = "dzn_detector";
		poolItem41.quantity = 1;
		poolItem41.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem41);

		SilverTrader_PoolItem poolItem42 = new SilverTrader_PoolItem();
		poolItem42.classname = "dzn_printer_filament_abs";
		poolItem42.quantity = 1;
		poolItem42.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem42);

		SilverTrader_PoolItem poolItem43 = new SilverTrader_PoolItem();
		poolItem43.classname = "dzn_printer_filament_tpc";
		poolItem43.quantity = 1;
		poolItem43.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem43);

		SilverTrader_PoolItem poolItem44 = new SilverTrader_PoolItem();
		poolItem44.classname = "dzn_printer_filament_nylon";
		poolItem44.quantity = 1;
		poolItem44.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem44);

		m_rotatingTraders.Insert(rotatingTrader);
	}
};

// Globaler Override fuer die Filter-Kategorie eines Classnames
class SilverCategoryOverride
{
	string pattern;    // z.B. "ZenSkills_" oder exakter Classname
	string category;   // Ziel-Kategorie, z.B. "other", "base_building"
	bool   prefixOnly; // true = IndexOf(pattern) == 0, false = exakter Classname-Vergleich
};

// Separate Config fuer Kategorie-Overrides (standardmaessig deaktiviert)
class SilverCategoryOverridesConfig
{
	private const static string MOD_FOLDER = "$profile:\\SilverBarter\\";
	private const static string CONFIG_NAME = "SilverBarterCategoryOverrides.json";
	private const static string CURRENT_VERSION = "1";

	string CONFIG_VERSION = CURRENT_VERSION;
	bool m_enabled = false;
	ref array<ref SilverCategoryOverride> m_categoryOverrides = new array<ref SilverCategoryOverride>;

	void SilverCategoryOverridesConfig()
	{
		m_categoryOverrides = new array<ref SilverCategoryOverride>;
	}

	void Load()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string path = MOD_FOLDER + CONFIG_NAME;
		if (FileExist(path))
		{
			Print("[SilverBarter] Loading category overrides config: " + path);
			SilverCategoryOverridesConfig loadedConfig = new SilverCategoryOverridesConfig();
			loadedConfig.CONFIG_VERSION = "";
			loadedConfig.m_categoryOverrides = null;
			string loadError;
			if (!JsonFileLoader<SilverCategoryOverridesConfig>.LoadFile(path, loadedConfig, loadError))
			{
				Print("[SilverBarter] ERROR: Category overrides config could not be loaded, file preserved: " + SilverBarterSanitizeJsonError(loadError));
				return;
			}
			if (loadedConfig.CONFIG_VERSION != "" && loadedConfig.CONFIG_VERSION != CURRENT_VERSION)
			{
				Print("[SilverBarter] ERROR: Unsupported category overrides config version, file preserved: " + loadedConfig.CONFIG_VERSION);
				return;
			}
			bool configChanged = loadedConfig.CONFIG_VERSION == "";
			CONFIG_VERSION = loadedConfig.CONFIG_VERSION;
			m_enabled = loadedConfig.m_enabled;
			m_categoryOverrides = loadedConfig.m_categoryOverrides;
			loadedConfig.m_categoryOverrides = null;
			if (!m_categoryOverrides)
			{
				m_categoryOverrides = new array<ref SilverCategoryOverride>;
				configChanged = true;
			}
			CONFIG_VERSION = CURRENT_VERSION;
			if (configChanged)
			{
				Save();
				Print("[SilverBarter] Category overrides config load and update finished.");
			}
			else
			{
				Print("[SilverBarter] Category overrides config load finished, no update required.");
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
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string saveError;
		if (!JsonFileLoader<SilverCategoryOverridesConfig>.SaveFile(MOD_FOLDER + CONFIG_NAME, this, saveError))
			Print("[SilverBarter] ERROR: Category overrides config could not be saved: " + SilverBarterSanitizeJsonError(saveError));
	}

	void SetDefaultValues()
	{
		m_enabled = false;
		m_categoryOverrides = new array<ref SilverCategoryOverride>;

		// Mustereintraege als Vorlage (greifen erst nach Aktivierung via m_enabled = true)
		SilverCategoryOverride exampleSkills = new SilverCategoryOverride();
		exampleSkills.pattern = "ZenSkills_";
		exampleSkills.category = "other";
		exampleSkills.prefixOnly = true;
		m_categoryOverrides.Insert(exampleSkills);

		SilverCategoryOverride exampleTerjeBook = new SilverCategoryOverride();
		exampleTerjeBook.pattern = "TerjeBook";
		exampleTerjeBook.category = "other";
		exampleTerjeBook.prefixOnly = true;
		m_categoryOverrides.Insert(exampleTerjeBook);

		SilverCategoryOverride exampleSeaChest = new SilverCategoryOverride();
		exampleSeaChest.pattern = "SeaChest";
		exampleSeaChest.category = "base_building";
		exampleSeaChest.prefixOnly = false;
		m_categoryOverrides.Insert(exampleSeaChest);
	}
};

class SilverBarterConfigService
{
	private static ref SilverBarterConfig s_Config;
	private static ref SilverRotatingTradersConfig s_RotatingConfig;
	private static ref SilverCategoryOverridesConfig s_CategoryOverridesConfig;

	static SilverBarterConfig GetConfig()
	{
		if (!s_Config)
		{
			Print("[SilverBarter] Initializing config...");
			s_Config = new SilverBarterConfig();
			s_Config.Load();
		}
		return s_Config;
	}

	static SilverRotatingTradersConfig GetRotatingConfig()
	{
		if (!s_RotatingConfig)
		{
			Print("[SilverBarter] Initializing rotating traders config...");
			s_RotatingConfig = new SilverRotatingTradersConfig();
			s_RotatingConfig.Load();
		}
		return s_RotatingConfig;
	}

	static SilverCategoryOverridesConfig GetCategoryOverridesConfig()
	{
		if (!s_CategoryOverridesConfig)
		{
			Print("[SilverBarter] Initializing category overrides config...");
			s_CategoryOverridesConfig = new SilverCategoryOverridesConfig();
			s_CategoryOverridesConfig.Load();
		}
		return s_CategoryOverridesConfig;
	}
};

[Obsolete()]
static SilverBarterConfig GetSilverBarterConfig()
{
	return SilverBarterConfigService.GetConfig();
}

[Obsolete()]
static SilverRotatingTradersConfig GetSilverRotatingTradersConfig()
{
	return SilverBarterConfigService.GetRotatingConfig();
}

[Obsolete()]
static SilverCategoryOverridesConfig GetSilverCategoryOverridesConfig()
{
	return SilverBarterConfigService.GetCategoryOverridesConfig();
}
