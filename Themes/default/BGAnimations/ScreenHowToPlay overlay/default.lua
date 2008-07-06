return Def.ActorFrame {
	Def.ActorFrame {
		OnCommand=cmd(x,SCREEN_CENTER_X-20);

		-- Initial glow around receptors
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,85;y,95;zoom,0.7;rotationz,90;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,6;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,275;y,95;zoom,0.7;rotationz,270;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,6;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,212;y,95;zoom,0.7;rotationz,180;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,6;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,148;y,95;zoom,0.7;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,6;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
	        
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,148;y,95;zoom,0.7;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,9.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
	        
		-- 2nd step UP
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,212;y,95;zoom,0.7;rotationz,180;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,12.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
	        
		-- 3rd step UP
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,84;y,95;zoom,0.7;rotationz,90;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,15.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
	
		-- 4th step jump
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,85;y,95;zoom,0.7;rotationz,90;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,18.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
		LoadActor("tapglow") .. {
			OnCommand=cmd(x,275;y,95;zoom,0.7;rotationz,270;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,18.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
	        
		-- miss step
		LoadActor("healthhilight") .. {
			OnCommand=cmd(x,180;y,40;diffuseshift;effectcolor1,1,0.93333,0.266666,0.4;effectcolor2,1,1,1,1;effectperiod,0.25;effectmagnitude,0,1,0;diffusealpha,0;sleep,22.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
		};
	};

	-- messages
	LoadFont("common normal") .. {
		Text = "How To Play StepMania",
		InitCommand=cmd(zbuffer,1;z,20;x,SCREEN_CENTER_X;y,SCREEN_CENTER_Y;shadowlength,0);
		OnCommand=cmd(diffusealpha,0;zoom,4;sleep,0.0;linear,0.3;diffusealpha,1;zoom,1;sleep,1.8;linear,0.3;zoom,0.75;x,170;y,60);
	};
	LoadActor("feet") .. {
		OnCommand=cmd(zbuffer,1;z,20;x,SCREEN_CENTER_X;y,SCREEN_CENTER_Y;addx,-SCREEN_WIDTH;sleep,2.4;decelerate,0.3;addx,SCREEN_WIDTH;sleep,2;linear,0.3;zoomy,0);
	};
	LoadActor("tapmessage") .. {
		OnCommand=cmd(x,SCREEN_HEIGHT;y,280;diffusealpha,0;sleep,6;linear,0;diffusealpha,1;sleep,2;linear,0;diffusealpha,0);
	};
	LoadActor("tapmessage") .. {
		OnCommand=cmd(x,SCREEN_HEIGHT;y,280;diffusealpha,0;sleep,9.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
	};
	LoadActor("tapmessage") .. {
		OnCommand=cmd(x,SCREEN_HEIGHT;y,280;diffusealpha,0;sleep,12.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
	};
	LoadActor("tapmessage") .. {
		OnCommand=cmd(x,SCREEN_HEIGHT;y,280;diffusealpha,0;sleep,15.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
	};
	LoadActor("jumpmessage") .. {
		OnCommand=cmd(x,SCREEN_HEIGHT;y,280;diffusealpha,0;sleep,18.7;linear,0;diffusealpha,1;sleep,1.7;linear,0;diffusealpha,0);
	};
	LoadActor("missmessage") .. {
		OnCommand=cmd(x,SCREEN_HEIGHT;y,280;diffusealpha,0;sleep,22.7;linear,0;diffusealpha,1;sleep,22.7;linear,0;diffusealpha,0);
	};
};
