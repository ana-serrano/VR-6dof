%clear all 
%close all

%folder = ['_triangle_orientations/' filename];
files = dir([folder '/*.jpg']);

alpha = VideoWriter([folder '/' filename_in '_alphaproc.mp4'],'MPEG-4');
alpha.FrameRate = 30;
open(alpha);
    
nF = length(files)/6;
for f=1:nF
    fprintf('Loading video frame %04d / %04d \n', f, nF);
    %%Read faces and project them to equirectangular
    bname = [folder '/' filename_in '_frame_' sprintf('%04d',f-1)];    
    bottom = imrotate((imread([bname '_face_3.jpg'])),-90);
    top = imrotate((imread([bname '_face_2.jpg'])),90);    
    left = ((imread([bname '_face_1.jpg'])));
    back = ((imread([bname '_face_5.jpg'])));    
    right = ((imread([bname '_face_0.jpg'])));
    front = ((imread([bname '_face_4.jpg'])));    
    out = cubic2equi(top, bottom, left, right, front, back);    
    out = flip(out(:,:,1),1);    
    out = im2double(imresize(out,2));

    %Closing operation
    se = strel('disk',2);
    aa = imerode(imcomplement(out),se);
    bb = imdilate(aa,se);

    %Thresholding
    cc = bb.*double(bb>0.8);
    
    %Gaussian blur
    dd = imgaussfilt(cc,11,'FilterSize',7);
    
    %Logistic function
    c = 0.5;
    k = 8.0;
    Scurve = 1 ./ (1 +  exp(-k.*(dd-c)));   
    Scurve(Scurve<0.1) = 0;
    Scurve(Scurve>0.8) = 1;
    Scurve = imcomplement(Scurve);
    writeVideo(alpha,Scurve);    

end
close(alpha);

