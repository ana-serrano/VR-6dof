function [ out ] = optimize_objective_temporal( scribble, weights, maskimg, flows, prev_frame, params )
%OPTIMIZE_OBJECTIVE_TEMPORAL Optimization wrapper function for colorization with temporal consistency term
% Optimization wrapper function for colorization with temporal consistency term
% 
% [ out ] = OPTIMIZE_OBJECTIVE_TEMPORAL( scribble, weight, maskimg, flows, prev_frame, params )
% 
% Inputs
%   scribble: scribble data (h,w,d)
%   weight: weight matrix (h,w,num_neigh)
%   maskimg: binary mask map that indicates scribble (h,w)
%   flows: optical flow (h,w,2)
%   prev_frame: previous frame (h,w,d)
%   params: parameters (default: params.lambda = 1e-1;    params.tol = 1e-12;     params.maxiter = 100;  params.solver = 'bicgstabl';)
%       params.lambda: fidelity of the input scribble term
%       params.maxiter: maximum number of iteration in MATLAB internal solvers
%       params.tol: tolerance of MATLAB internal solvers
%       params.solver: type of a solver

global scribble_;   global w_rs; global maskimg_;    global params_;    global w_sm;
scribble_ = scribble;   w_rs = weights.w_rs;   maskimg_ = maskimg; params_ = params;    w_sm = weights.w_sm;

[h,w] = size(maskimg);
scribble_ = scribble_(:);  maskimg_ = maskimg_(:);

warped = vectorize_any(warp_image(prev_frame, flows));
global weight_temp; weight_temp = vectorize_any(sqrt(sum(abs(flows).^2, 3)));
weight_temp(weight_temp < 1e-4) = 1e-4;

% b = vectorize_any(params_.lambda .* maskimg_ .* scribble_ + params_.gamma .* warped);
b = vectorize_any(params_.lambda .* maskimg_ .* scribble_ + params_.gamma .* weight_temp .* warped);
% b = b(:);

fprintf('Linear system solving starts.\n');
tic;
% [out, ~, ~, ~, ~] = bicgstabl(@(x)compute_Ax(x), b(:), params.tol, params.maxiter, [], []);
warning off;
[out, flag, ~, ~, resvec] = eval([params.solver '(@(x)compute_Ax(x), b, params.tol, params.maxiter, [], [], warped);']);
warning on;
elapsed_time = toc;
fprintf('\n%f seconds consumed for optimization.\n', elapsed_time);
out = reshape(out, h,w);

% figure; plot(resvec);   title('Relative residual');
% xlabel('Iteration number');
% ylabel('Relative residual');


end

function [ Ax ] = compute_Ax( x )
global w_rs; global maskimg_;    global params_;    global w_sm;    global weight_temp;
[h,w,~] = size(w_rs);
fprintf('.');
eight_neighbours = eight_neighbour_extract(reshape(x,h,w));
Ax = params_.lambda2 .* x + params_.lambda.*maskimg_.*x - params_.lambda2 .* reshape(sum(w_rs.*eight_neighbours ,3), [],1)...
    + params_.gamma .* weight_temp .* x + params_.smoothness .* vectorize_any(sum(w_sm .* (repmat(reshape(x,h,w),[1,1,8]) - eight_neighbours), 3));

end