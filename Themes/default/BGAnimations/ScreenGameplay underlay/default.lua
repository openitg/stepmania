local children = {
	LoadActor("_warning") .. {
		InitCommand=cmd(x,SCREEN_CENTER_X;y,SCREEN_CENTER_Y;
			vertalign,top;
			wag;effectmagnitude,0,0,10;effectperiod,2;
		);
		OnCommand=cmd(diffusealpha,0);
		ShowDangerAllMessageCommand=cmd(stoptweening;accelerate,0.3;diffusealpha,1);
		HideDangerAllMessageCommand=cmd(stoptweening;accelerate,0.3;diffusealpha,0);
	};
};

if GAMESTATE:IsExtraStage() then
	children[#children+1] = LoadActor("_extra score frame") .. {
		InitCommand=cmd(x,SCREEN_CENTER_X+0;y,SCREEN_CENTER_Y-208);
		OnCommand=cmd(addy,-100;sleep,0.5;linear,1;addy,100);
		OffCommand=cmd(linear,1;addy,-100);
	};
else
	local sFile = GAMESTATE:GetPlayMode() == "PlayMode_Oni" and "_oni score frame" or "_score frame";
	children[#children+1] = LoadActor(sFile) .. {
		InitCommand=cmd(x,SCREEN_CENTER_X+0;y,SCREEN_CENTER_Y+208);
		OnCommand=cmd(addy,100;linear,0.5;addy,-100);
		OffCommand=cmd(linear,0.5;addy,100);
	};
end

return Def.ActorFrame { children = children };