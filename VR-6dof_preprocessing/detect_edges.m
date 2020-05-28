function [ edgemap ] = detect_edges( img, type )
%DETECT_EDGES Edge detection function
%   Edge detection function
%   
%   edgemap = DETECT_EDGES( img, type )
%   
%   Inputs
%       img: an input image in [0,1] scale (h,w,d)
%           d==1: depth map
%           d==3: rgb image
%           d==4: rgbd image
%       type: type of edge dectectors (string)
%           'multiscale': multiscale edge detector (default)
%               reference: Dollar and Zitnick, 'Fast edge detection using structured forests', TPAMI 2015
%           'canny': canny edge detector
%   Output
%       edgemap: edge map (h,w)

if ~exist('type', 'var')
    type = 'multiscale';
end

switch type
    case 'multiscale'
        addpath(genpath('piotr_edge'));

        opts=edgesTrain();                % default options (good settings)
        opts.modelDir='models/';          % model will be in models/forest
        switch size(img,3)
            case 4
                opts.modelFnm='modelNyuRgbd';
            case 3
                opts.modelFnm='modelBsdsBig';        % model name
            case 1
                opts.modelFnm='modelNyuD';        % model name
        end        
        opts.nPos=5e5; opts.nNeg=5e5;     % decrease to speedup training
        opts.useParfor=1;                 % parallelize if sufficient memory

        % train edge detector (~20m/8Gb per tree, proportional to nPos/nNeg)
        model=edgesTrain(opts); % will load model if already trained
        addpath(genpath('piotr_cv_toolbox'));

        % set detection parameters (can set after training)
        model.opts.multiscale=2;          % for top accuracy set multiscale=1
        model.opts.sharpen=2;             % for top speed set sharpen=0
        model.opts.nTreesEval=16;          % for top speed set nTreesEval=1
        model.opts.nThreads=8;            % max number threads for evaluation
        model.opts.nms=0;                 % set to true to enable nms

        % detect edge and visualize results
        edgemap = edgesDetect(uint8(img *255),model);
        % figure; im(1-E);
        cd('../');
    case 'canny'
        edgemap = edge(rgb2gray(img), 'Canny');
    otherwise
end


end

