local Color = color(Var "Color1");
local children = {
	LoadActor(Var "File1") .. {
		OnCommand=cmd(scale_or_crop_background;diffuse,Color;loop,false);
		GainFocusCommand=cmd(play);
		LoseFocusCommand=cmd(pause);
	};
};

return Def.ActorFrame { children = children };