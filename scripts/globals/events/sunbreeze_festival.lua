---------------------------------------------------------
---------------------------------------------------------
----------    Summerfest/Sunbreeze Festival    ----------
----------           Author: Foodz             ----------
---------------------------------------------------------
---------------------------------------------------------

require("scripts/globals/status");
require("scripts/globals/settings");

---------------------------------------------------------
---------------------------------------------------------


function isSummerfestEnabled()
	local option = 0;
	local month = tonumber(os.date("%m"));
	local day = tonumber(os.date("%d"));
	if(month == 7 and day >= 25 or month == 8 and day <= 21 or SUMMERFEST_YEAR_ROUND ~= 0) then -- According to wiki Harvest Fest is Oct 20 - Nov 1.
		if(SUMMERFEST_2004 == 1) then
			option = 1;
		elseif(SUNBREEZE_2009 == 1) then
			option = 2;
		elseif(SUNBREEZE_2011 == 1) then
			option = 3;
		end
	end
	return option;
end;