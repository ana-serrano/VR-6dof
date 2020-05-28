function [ out ] = compute_smoothness_weight( edgemap, params )
%COMPUTE_DEPTH_WEIGHT Compute weights of the data term
%   Compute weights of the data term
% 
%   [ out ] = COMPUTE_DEPTH_WEIGHT( depth, window_size )
% 
%   Inputs
%       depth: depth map (h,w)
%       windows_size: the size of a local patch
%   Output
%       out: weight map (h,w)
%           out = exp( - local_variance )


%% 1. Compute local variance
sigma_d_squared = stdfilt(edgemap, ones(params.smweight_windowsize)).^2;

out = exp(- params.scale_factor_smoothness * sigma_d_squared);

end

