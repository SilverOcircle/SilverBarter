class CfgPatches {
	class SilverBarter {
		units[] = {"TraderPoint"};
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods {
	class SilverBarter {
		type = "mod";
		author = "SilverOcircle";

		class defs {
			class gameScriptModule {
				value = "";
				files[] = {"SilverBarter/scripts/3_Game"};
			};

			class worldScriptModule {
				value = "";
				files[] = {"SilverBarter/scripts/4_World"};
			};

			class missionScriptModule {
				value = "";
				files[] = {"SilverBarter/scripts/5_Mission"};
			};
		};
	};
};

class CfgVehicles
{
	// TraderPoint Entity Definition (unsichtbar - nur fuer Interaktionslogik)
	class HouseNoDestruct;
	class TraderPoint: HouseNoDestruct
	{
		scope = 2;
		model = "\dz\data\lightpoint.p3d";
	};
};
