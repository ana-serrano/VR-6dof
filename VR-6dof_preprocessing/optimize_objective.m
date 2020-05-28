function [ out ] = optimize_objective( scribble, weights, maskimg, params )
%OPTIMIZE_OBJECTIVE Optimization wrapper function for colorization
% Optimization wrapper function for colorization
% 
% [ out ] = OPTIMIZE_OBJECTIVE( scribble, weight, maskimg, params )
% 
% Inputs
%   scribble: scribble data (h,w,d)
%   weight: weight matrix (h,w,num_neigh)
%   maskimg: binary mask map that indicates scribble (h,w)
%   params: parameters (default: params.lambda = 1e-1;    params.tol = 1e-12;     params.maxiter = 100;  params.solver = 'bicgstabl';)
%       params.lambda: fidelity of the input scribble term
%       params.maxiter: maximum number of iteration in MATLAB internal solvers
%       params.tol: tolerance of MATLAB internal solvers
%       params.solver: type of a solver

global scribble_;   global w_rs; global maskimg_;    global params_;    global w_sm;
scribble_ = scribble;   w_rs = weights.w_rs;   maskimg_ = maskimg; params_ = params;    w_sm = weights.w_sm;

[h,w] = size(maskimg);
scribble_ = scribble_(:);  maskimg_ = maskimg_(:);


b = params_.lambda .* maskimg_ .* scribble_;
b = b(:);

fprintf('Linear system solving starts.\n');
tic;
% [out, ~, ~, ~, ~] = bicgstabl(@(x)compute_Ax(x), b(:), params.tol, params.maxiter, [], []);
[out, flag, ~, ~, resvec] = eval([params.solver '(@(x)compute_Ax(x), b, params.tol, params.maxiter, [], [], scribble_);']);
elapsed_time = toc;
fprintf('\n%f seconds consumed for optimization.\n', elapsed_time);
out = reshape(out, h,w);

% figure; plot(resvec);   title('Relative residual');
% xlabel('Iteration number');
% ylabel('Relative residual');


end

function [ Ax ] = compute_Ax( x )
global w_rs; global maskimg_;    global params_;    global w_sm;
[h,w,~] = size(w_rs);
fprintf('.');
eight_neighbours = eight_neighbour_extract(reshape(x,h,w));
Ax = params_.lambda2 .* x + params_.lambda.*maskimg_.*x - params_.lambda2 .* reshape(sum(w_rs.*eight_neighbours ,3), [],1) ...
    + params_.smoothness .* vectorize_any(sum(w_sm .* (repmat(reshape(x,h,w),[1,1,8]) - eight_neighbours), 3));

end