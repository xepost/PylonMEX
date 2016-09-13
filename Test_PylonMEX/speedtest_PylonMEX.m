

%clc

timeout = 5;

PylonMEX_Initialize();

times = NaN(1000,1);
dT = NaN(size(times));
hWait = waitbar(0,'capturing');

for n=1:numel(times)
    
    t1 = tic();
    while PylonMEX_BufferCount() <1 && toc(t1)<timeout
        %disp('wait');
        pause(0.001);
    end
    dT(n) = toc(t1);
    if dT(n)>=timeout
        error('Timeout');
    end
    waitbar(n/numel(times),hWait);
    
    t2 = tic();
    img = PylonMEX_GrabNoWait();
    %img = img';
    times(n) = toc(t2);
    
end
delete(hWait);
fprintf('Avg dT: %f\n',nanmean(dT));
fprintf('Avg FPS: %f\n',1/nanmean(times));
pause();
PylonMEX_Shutdown();
clear mex;
imshow(img)