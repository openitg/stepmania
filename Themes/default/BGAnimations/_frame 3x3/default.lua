local File, Width, Height = ...
assert( File );
assert( Width );
assert( Height );

local FullFile = THEME:GetPathB('','_frame files 3x3/'..File )
local Frame = LoadActor( FullFile )
return Def.ActorFrame {
	Frame .. { InitCommand=cmd(setstate,0;pause;halign,1;valign,1;x,-Width/2;y,-Height/2) };
	Frame .. { InitCommand=cmd(setstate,1;pause;zoomtowidth,Width;valign,1;zoomtowidth,Width;y,-Height/2) };
	Frame .. { InitCommand=cmd(setstate,2;pause;halign,0;valign,1;x,Width/2;y,-Height/2) };
	Frame .. { InitCommand=cmd(setstate,3;pause;halign,1;x,-Width/2;zoomtoheight,Height) };
	Frame .. { InitCommand=cmd(setstate,4;pause;zoomtowidth,Width;zoomtoheight,Height) };
	Frame .. { InitCommand=cmd(setstate,5;pause;halign,0;x,Width/2;zoomtoheight,Height) };
	Frame .. { InitCommand=cmd(setstate,6;pause;halign,1;valign,0;x,-Width/2;y,Height/2) };
	Frame .. { InitCommand=cmd(setstate,7;pause;zoomtowidth,Width;valign,0;zoomtowidth,Width;y,Height/2) };
	Frame .. { InitCommand=cmd(setstate,8;pause;halign,0;valign,0;x,Width/2;y,Height/2) };
};
