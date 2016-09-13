function pylontest_tracking()

cam = PylonMEX.getInstance();

cam.setupAxes([],true);
title('press space to add tracking window');

ud = get(cam.hfigImageFig,'userdata');
ud.win_set = false;
ud.hWind = gobjects(0);
set(cam.hfigImageFig,'userdata',ud);
set(cam.hfigImageFig,'KeyPressFcn',@(s,e) KeyPress(s,e,cam));
%pause();
cam.StartLiveMode();

set(cam.hfigImageFig,'DeleteFcn',@(~,~)delete(cam));

%pause;
%error('test error');

function KeyPress(src,evt,cam)
ud = get(src,'userdata');
if strcmpi(evt.Key,'space') %&& ~ud.win_set
    cam.StopLiveMode();
    %add tracking window
   
    ud.hWind(end+1) = imrect2('Parent',cam.haxImageAxes,...
                'LimMode','manual',...
                'HandleVisibility','callback',...
                'Color','r');

    cam.setFrameCallback( @(c) trackwind(c,ud.hWind));
    ud.win_set = true;

    set(cam.hfigImageFig,'userdata',ud);
    cam.StartLiveMode();
end

function trackwind(cam,hWind)
persistent hLine;
WIND = zeros(numel(hWind),4);
for n=1:numel(hWind)
WIND(n,:) = [hWind(n).Position(1),hWind(n).Position(1)+hWind(n).Position(3)-1,...
        hWind(n).Position(2),hWind(n).Position(2)+hWind(n).Position(4)-1];
end
[Xc,Yc] = radialcenter_mex(cam.ImageData,WIND);

if ishghandle(cam.haxImageAxes)
    if isempty(hLine)||~ishghandle(hLine)
        hLine=line(cam.haxImageAxes,'Xdata',Xc,'YData',Yc,'LineStyle','none','color','r','marker','+','MarkerSize',15);
    else
        set(hLine,'Xdata',Xc,'YData',Yc);
    end
end