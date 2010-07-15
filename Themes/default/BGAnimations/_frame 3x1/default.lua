local File, Width = ...
assert( File );
assert( Width );

local FullFile = THEME:GetPathB('','_frame files 3x1/'..File )
local Frame = LoadActor( FullFile )

return Def.ActorFrame {
	Frame .. { InitCommand=cmd(setstate,0;pause;halign,1;x,-Width/2) };
	Frame .. { InitCommand=cmd(setstate,1;pause;zoomtowidth,Width) };
	Frame .. { InitCommand=cmd(setstate,2;pause;halign,0;x,Width/2) };
};
