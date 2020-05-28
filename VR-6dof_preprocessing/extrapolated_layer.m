%clear all
%close all

in_path = '_input_videos';
out_path = ['_extrapolated_layer/' filename];
mkdir(out_path);

name = [in_path '/' filename];
%Read RGB and Depth videos
fg_rgb_vid  =  VideoReader([name '.mp4']);
fg_depth_vid  =  VideoReader(['_improved_depth/' filename '/videos/' filename '_depth.mp4']);
rgb_tex = im2double(read(fg_rgb_vid, 1));
d_tex = im2double(read(fg_depth_vid,1));

%Frames to sample to calculate the median (inpractical to use the full
%video at once)
%Alternative: Compute this for several blocks and a final value among the
%different blocks
TotFrames1 = fg_rgb_vid.Duration*fg_rgb_vid.FrameRate;
TotFrames2 = fg_depth_vid.Duration*fg_depth_vid.FrameRate;
TotFrames = min(TotFrames1,TotFrames2);
nFrames = 300;
Fsamples = round(linspace(1,floor(TotFrames),nFrames));

% Read frames, save them into video blocks
depth_block = zeros(size(rgb_tex,1),size(rgb_tex,2),nFrames);
rgb_block_r = zeros(size(rgb_tex,1),size(rgb_tex,2),nFrames);
rgb_block_g = zeros(size(rgb_tex,1),size(rgb_tex,2),nFrames);
rgb_block_b = zeros(size(rgb_tex,1),size(rgb_tex,2),nFrames);
for k = 1:nFrames
    Frame = Fsamples(k);
    fprintf('Extrapolated layer -> Loading ideo: %s, Frame: %03d/%03d\n', name, Frame, floor(TotFrames));
    rgb_tex = im2double(read(fg_rgb_vid, Frame));
    rgb_block_r(:,:,k) = rgb_tex(:,:,1);
    rgb_block_g(:,:,k) = rgb_tex(:,:,2);
    rgb_block_b(:,:,k) = rgb_tex(:,:,3);
    d_encoded = im2double(read(fg_depth_vid,Frame));
    depth_block(:,:,k) = d_encoded(:,:,1);
end

%Median depth has a problem: When items move a bit but are mostly static,
%some pixels may end up with lower depth than the original, 
%i.e., they will be closer to the camera, and we will see those instead of the
%original video. Instead of a median, we take a robust median of the min
%values (max. depths). 
M = depth_block;
depth_out = zeros(size(M,1),size(M,2));
color_out = zeros(size(M,1),size(M,2));
for ii=1:size(M,1)-1
    for jj= 1:size(M,2)
        [aux aux_idx] = sort(M(ii,jj,:), 'ascend');           
        depth_out(ii,jj) = median(aux(1:15));
        color_out(ii,jj,1) = median(rgb_block_r(ii,jj,aux_idx(1:15)),3);
        color_out(ii,jj,2) = median(rgb_block_g(ii,jj,aux_idx(1:15)),3);
        color_out(ii,jj,3) = median(rgb_block_b(ii,jj,aux_idx(1:15)),3);
      % r_out(ii,jj) = rgb_block_r(ii,jj,idx(ii,jj));
    end
end

imwrite(color_out,[out_path '/' filename  '_BG.png']);
imwrite(depth_out,[out_path '/' filename '_BG_depth.png']);

%We write them also as videos with only one frame to facilitate computing
%the triangle orientations as we do for the full video (just for
%convenience)
vv = VideoWriter([out_path '/' filename '_BG.mp4'],'MPEG-4');
vv.FrameRate = 30;
open(vv);
writeVideo(vv,color_out);
close(vv);

vv = VideoWriter([out_path '/' filename '_BG_depth.mp4'],'MPEG-4');
vv.FrameRate = 30;
open(vv);
writeVideo(vv,depth_out);
close(vv);
