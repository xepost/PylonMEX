function pylontest2()
clc;
cam = PylonMEX.getInstance();

cam.setupAxes([]);
%pause();
cam.StartLiveMode();

set(cam.hfigImageFig,'DeleteFcn',@(~,~)delete(cam));

%pause;
%error('test error');

