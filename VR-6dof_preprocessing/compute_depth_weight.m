function [ out ] = compute_depth_weight( depth, params )
%COMPUTE_DEPTH_WEIGHT Compute weights of the data term
%   Compute weights of the data term
% 
%   [ out ] = COMPUTE_DEPTH_WEIGHT( depth, params )
% 
%   Inputs
%       depth: depth map (h,w)
%       params: parameters
%           params.svweight_patchsize: window size
%           params.scale_factor: determines the steepness of exponential
%   Output
%       out: weight map (h,w)
%           out = exp( - scale * local_variance )

% [h,w] = size(depth);

%% 1. Compute local variance
% pad_depth = padarray(depth, [floor(params.svweight_patchsize/2),floor(params.svweight_patchsize/2)], 'symmetric');
% col_depth = im2col(pad_depth, [params.svweight_patchsize,params.svweight_patchsize]);
% sigma_d_squared = reshape(var(col_depth), h,w);
sigma_d_squared = stdfilt(depth, ones(params.svweight_patchsize)).^2;

out = exp(- params.scale_factor * sigma_d_squared);

end

