function [ weight ] = weight_compute( Y, window_size, weight_type )
%WEIGHT_COMPUTE Weight matrix computation function
% Weight matrix computation function based on 8 neighbours
% 
% 
% [ weight ] = WEIGHT_COMPUTE( Y, window_size, weight_type )
% 
% Inputs
%   Y: intensity map in the YUV space (h,w)
%   window_size: the size of a window (scalar, odd number)
%   weight_type: the type of a weight (string)
%       'squared': squared difference between two intensities
%                         (Y(r) - Y(s)).^2
%           w_rs = exp( - ----------------- )
%                            2*sigma_r^2
%       'normalized': normalized correlation between two intensities
%                       (Y(r) - mu_r) * (Y(s) - mu_r)
%           w_rs = 1 + -------------------------------
%                                 sigma_r^2
% Outputs
%   weight: weight matrix (h,w,8)
%               1 2 3
%               4   5
%               6 7 8

%% 1. Initialisation
[h,w] = size(Y);
num_neigh = 8;

%% 2. Compute local mean and variance
% pad_Y = padarray(Y, [floor(window_size/2),floor(window_size/2)], 'symmetric');
% col_Y = im2col(pad_Y, [window_size,window_size]);
% mu_r = reshape(mean(col_Y), h,w);   sigma_r_squared = reshape(var(col_Y), h,w);
mu_r = conv2(Y,ones(window_size)/(window_size.^2),'same');  sigma_r_squared = stdfilt(Y, ones(window_size)).^2;
epsilon = 1e-4;
% sigma_r_squared(sigma_r_squared<epsilon) = sigma_r_squared(sigma_r_squared<epsilon) + epsilon;       % prevent singularity error
sigma_r_squared(sigma_r_squared<epsilon) = epsilon;       % prevent singularity error
% clear pad_Y col_Y;


%% 3. Extract 8 neighbours
Y_s = eight_neighbour_extract(Y);
Y_r = repmat(Y, [1,1,num_neigh]);

mu_r_rep = repmat(mu_r, [1,1,num_neigh]);   sigma_r_squared_rep = repmat(sigma_r_squared, [1,1,num_neigh]);
clear up_left up up_right left right down_left down down_right zeropad_Y;


%% 4. Compute weight
switch weight_type
    case 'squared'
%                     (Y(r) - Y(s)).^2
%       w_rs = exp( - ----------------- )
%                        2*sigma_r^2
        weight = exp(- ((Y_r - Y_s).^2) ./ (2*sigma_r_squared_rep));        
    case 'normalized'
%                   (Y(r) - mu_r) * (Y(s) - mu_r)
%       w_rs = 1 + -------------------------------
%                             sigma_r^2
        pad_Y_s = padarray(Y_s, [floor(window_size/2),floor(window_size/2)], 'symmetric');
        col_Y_s = [];
        for i = 1:num_neigh
            col_Y_s = cat(3, col_Y_s, im2col(pad_Y_s(:,:,i), [window_size,window_size]));
        end
        mu_s = reshape(mean(col_Y_s), h,w,num_neigh);
        weight = 1 + ((Y_r - mu_r_rep).*(Y_s - mu_s)) ./ sigma_r_squared_rep;
    case {'tukey', 'lorentzian', 'exponential', 'huber', 'isotropic'}
        weight = edge_stopping_function((Y_r - Y_s), sqrt(sigma_r_squared_rep), weight_type);
%         weight = edge_stopping_function((Y_r - Y_s), 0.05, weight_type);
    otherwise
        error('Wrong weight type input.');
end

%% 5. Normalize weight
weight_sum = sum(weight, 3);
weight = weight ./ repmat(weight_sum, [1,1,num_neigh]);

end

