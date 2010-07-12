return Def.ActorFrame{
	LoadActor("frame");
	LoadFont("_terminator two 13px")..{
		InitCommand=cmd(y,-46;shadowlength,0;strokecolor,color("#333333FF");settext,ScreenString("SongsSurvived"););
	};
};