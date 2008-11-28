local t = Def.ActorFrame {
};

t[#t+1] = Def.ActorFrame {
};

local NumColumns = THEME:GetMetric(Var "LoadingScreen", "NumColumns");




for i=1,NumColumns do
	t[#t+1] = LoadActor("difficulty pill") .. {
		InitCommand=cmd(x,SCREEN_CENTER_X-60 + 80 * (i-1);y,SCREEN_CENTER_Y-154;);
	};
	
	local d = THEHE:GetMetric(Var "LoadingScreen","ColumnDifficulty" .. i);
	local st = THEHE:GetMetric(Var "LoadingScreen","ColumnStepsType" .. i);	
	t[#t+1] = LoadFont("_venacti Bold 13px") .. {
		InitCommand=cmd(settext,"EASY";diffuse,color("#FFFF00");x,SCREEN_CENTER_X-60 + 80 * (i-1);y,SCREEN_CENTER_Y-158;shadowlength,0;);
	};
end

t[#t+1] = LoadFont("_terminator two 24px") .. {
	InitCommand=cmd(settext,"SINGLE";diffuse,color("#a38b00");x,SCREEN_CENTER_X-220;y,SCREEN_CENTER_Y-160;shadowlength,0;strokecolor,color("#00000000"););
};

t[#t+1] = LoadFont("_terminator two 30px") .. {
	InitCommand=cmd(settext,"Best Ranking";uppercase,true;diffuse,color("#FFFFFF");x,SCREEN_CENTER_X-130;y,SCREEN_CENTER_Y-206;shadowlength,0;strokecolor,color("#3b009c44"););
};
return t;