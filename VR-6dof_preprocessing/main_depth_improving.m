% Reference: Levin et al., "Colorization using Optimization", SIGGRAPH 2004
% Implemented in MATLAB R2017a environment
%clear all;
%close all;

addpath(genpath('optical_flow'));
addpath(genpath('bgu_matlab'));

%%%%%%%%%%%%%%%%%%%%%%%%% Liu's optical flow parameters %%%%%%%%%%%%%%%%%%%%%%%%%
% Reference: C. Liu. Beyond Pixels: Exploring New Representations and Applications for Motion Analysis. Doctoral Thesis. Massachusetts Institute of Technology. May 2009.
% set optical flow parameters (see Coarse2FineTwoFrames.m for the definition of the parameters)
% Parameters are set by the author. Using the default set is recommended.
alpha = 0.012;
ratio = 0.75;
minWidth = 20;
nOuterFPIterations = 7;
nInnerFPIterations = 1;
nSORIterations = 30;
para = [alpha,ratio,minWidth,nOuterFPIterations,nInnerFPIterations,nSORIterations];

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Parameter Setting %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%filename = 'batuca_1';
texture_path = '_input_videos/';  
depth_path = '_input_videos/';
addpath(texture_path);  addpath(depth_path);
% params_ad.maxiter = 60;     params_ad.sigma = 0.1;  params_ad.lambda = 1.2; params_ad.func = 'tukey';
upscale_size = [1024 2048];
downscale_size = upscale_size*0.8;
%Increase downscaling to decrease processing time
%downscale_size = upscale_size*0.6;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
params.lambda_data = 4e-2; %data
params.lambda2 = 1; %edge
params.gamma = 1e-2;  %temporal
params.smoothness = 1e-2; %smoothness
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
params.smweight_windowsize = 3; 
params.scale_factor_smoothness = 1e+3;
params.tol = 1e-6;     
params.maxiter = 30;  
params.solver = 'bicgstab';
pad_size = 65;      % padding size
min_meter_in_depth = 0.29999999999999999;
weight_type = 'tukey';
window_size = 3;
params.pad_size = pad_size; params.upscale_size = upscale_size; params.downscale_size = downscale_size;
params.bilateral_sigma = 2; params.svweight_patchsize = 7; params.scale_factor = 1e+5;
params.video_duration = 20; params.upsampling = 'bilinear';
params.starting_point_in_sec = 0;
params.left_right = 'none';    params.weight_type = weight_type;   params.wrs_window_size = 3;
params.filename = filename; params.extended_filename = '1';
%{
lambda_data: fidelity of input depth values (the higher, the more credible the input data is)
lambda2: weight term of the edge-awareness (set to 1)
gamma: temporal consistency term (the higher, the smooother the transition is)
tol: tolerance of the internal solver
maxiter: max itertation of the internal solver
solver: internal solver type
pad_size: size of padding pixels
upscale_size, downscale_size: size of upscaling/downsizing
bilateral_sigma: sigma value in bilateral filtering
svweight_patchsize: local window size in computing spatially-varying data weight
scale_factor: steepness of exponential in computing the spatially-varying data weight (the higher, the steeper the slope is)
video_duration: desirable duration of the output video (in sec)
upsampling: method of upsampling ('bilinear', 'bgu')
%}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Debug output path %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
debugpath = ['_improved_depth\' filename '\'];
flowpath = [debugpath 'flow\']; videopath = [debugpath 'videos\'];  edgepath = [debugpath 'edges\'];
w_smpath = [debugpath 'w_sm\']; w_datapath = [debugpath 'w_data\'];
if ~exist(debugpath, 'dir')
    mkdir(debugpath);
end
if ~exist(flowpath, 'dir')
    mkdir(flowpath);
end
if ~exist(videopath, 'dir')
    mkdir(videopath);
end
if ~exist(edgepath, 'dir')
    mkdir(edgepath);
end
% if ~exist(w_smpath, 'dir')
%     mkdir(w_smpath);
% end
if ~exist(w_datapath, 'dir')
    mkdir(w_datapath);
end
save([debugpath 'params.mat'], 'params');

%% 1. Video reader/writer
tvobj =  VideoReader([texture_path filename '.mp4']);
dvobj =  VideoReader([depth_path filename '_depth.mp4']);

tvwriter = VideoWriter([videopath filename '.mp4'], 'MPEG-4');
tvwriter.FrameRate = tvobj.FrameRate;
open(tvwriter);
dvwriter = VideoWriter([videopath filename '_depth.mp4'], 'MPEG-4');
dvwriter.FrameRate = tvobj.FrameRate;
open(dvwriter);
% dorigwriter = VideoWriter([videopath filename '_2048_2048_depth.mp4'], 'MPEG-4');
% dorigwriter.FrameRate = tvobj.FrameRate;
% open(dorigwriter);

num_frames = 1;
prev_depth_frame = [];
prev_img = [];
total_num_frames = floor(dvobj.Duration * dvobj.FrameRate) - 1;

for t = floor(dvobj.FrameRate * params.starting_point_in_sec+1):total_num_frames
    disp(' ');
    disp(sprintf('Processing depth for frame %05d',t));   
    img = im2double(read(tvobj, t));
    img = imcut(img, params.left_right);
    
    depth = im2double(read(dvobj, t));
    depth = imcut(depth, params.left_right);
    
    origimg = img;    
    
    if size(depth,3) ~= 1
        depth = depth(:,:,1);
    end
    origdepth = imresize(depth, upscale_size);        
    
    % Image padding to handle artifacts around boundaries
    img = padarray(img, [0 pad_size], 'circular');
    img = padarray(img, [pad_size 0], 'symmetric');
    origimg_pad = img;
    depth = padarray(depth, [0 pad_size], 'circular');
    depth = padarray(depth, [pad_size 0], 'symmetric');
    
    padarray_size = size(depth);
    edgemap_img = [];   depth_edges = [];
    img = imresize(img, downscale_size);  depth = imresize(depth, downscale_size);
    
    %% 2. Edge detection    
    edgemap_img = detect_edges(img);
    depth_edges = detect_edges(repmat(depth,[1,1,3]));
    edgemap = (edgemap_img + depth_edges);        % edge voting
 
    % Inverse depth encoding
    %depth = min_meter_in_depth ./ min(depth, 1e-3);
    maskimg = ones(size(edgemap));

    % Spatially-varying weight of the data term
    weight_data = compute_depth_weight(depth, params);
    edgemap = imresize(edgemap, downscale_size);  maskimg = imresize(maskimg, downscale_size);
    params.lambda = vectorize_any(medfilt2(imresize(weight_data, downscale_size))) * params.lambda_data;
    
    %% 3. Compute edge-aware weights
    weights.w_rs = weight_compute(edgemap(:,:,1), params.wrs_window_size, params.weight_type);
    weights.w_sm = eight_neighbour_extract(compute_smoothness_weight(depth(:,:,1), params));

    if num_frames > 1
        %% 4. Optimization
        % Flow estimation to handle temporal consistency
        [vx,vy,~] = Coarse2FineTwoFrames(prev_img,img,para);
        flows = cat(3, vx, vy);
        %flow_coded = flowToColor(flows);    imwrite(flow_coded, [flowpath sprintf('flow_%d.png', num_frames)]);
        
        % Depth cleaning
        depth_propagated = optimize_objective_temporal(depth, weights, maskimg, flows, prev_depth_frame, params);
    else
        depth_propagated = optimize_objective(depth, weights, maskimg, params);
    end
    prev_depth_frame = depth_propagated; 

    depth_propagated = imresize(depth_propagated, padarray_size);
    %depth_propagated = L0Smoothing(depth_propagated, 1e-3, 1.5);
    %img_upscaled = imresize(img, padarray_size);
    
    % depth encoding
    %depth_propagated =  depth_propagated / min_meter_in_depth;
    depth_propagated = clip01(depth_propagated);
    
    % bilateral filtering
    greyimg_pad = rgb2gray(origimg_pad);
    depth_bilateral = bilateralFilter(double(depth_propagated), rgb2gray(origimg_pad),min(greyimg_pad(:)),max(greyimg_pad(:)),params.bilateral_sigma);
    depth_bilateral = depth_bilateral((pad_size+1):(end-pad_size+1),(pad_size+1):(end-pad_size+1));
    
    switch params.upsampling
        case 'bilinear'
            depth_bilateral = imresize(depth_bilateral, upscale_size);
        case 'bgu'
            [gamma_bgu, A_bgu, b_bgu, lambda_s_bgu, intensity_options] = bguFit(depth_bilateral, depth_bilateral, depth_bilateral, []);
            depth_bilateral = bguSlice(gamma_bgu, origimg, rgb2gray(origimg));
    end
    
    
    writeVideo(tvwriter, clip01(imresize(origimg, upscale_size)));
    writeVideo(dvwriter, clip01(depth_bilateral));
%     writeVideo(dorigwriter, clip01(origdepth));
    
    imwrite(depth_bilateral, [debugpath sprintf('%d.jpg', num_frames)]);
%     imwrite(edgemap, [edgepath sprintf('edge_%d.png', num_frames)]);
    
    prev_img = img;
    num_frames = num_frames + 1;
    if num_frames > tvobj.FrameRate*params.video_duration
        break;
    end
%     imwrite(weight_data, [w_datapath sprintf('%d.png', num_frames)]);
end


clear dvwriter tvwriter dorigwriter;