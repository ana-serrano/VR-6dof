function [ out ] = eight_neighbour_extract( in )
%EIGHT_NEIGHBOUR_EXTRACT Eight-neighbour extraction function
% Eight-neighbour extraction function
% 
%       1 2 3 ...             0 0 0 ... (up-left) .... ~ (down-right), total 8 matrices
%       4 x 5 ...     ->      0 1 2 ...
%       6 7 8 ...             0 4 x ...
%       .........
% 
% [ out ] = EIGHT_NEIGHBOUR_EXTRACT( in )
% 
% Inputs
%   in: a matrix (h,w)
% Outputs
%   out: eight-neighbour matrix (h,w,8)

zeropad_in = padarray(in, [1,1]);
[h,w] = size(zeropad_in);

up_left = zeropad_in(1:(h-2),1:(w-2)); up = zeropad_in(1:(h-2),2:(w-1));  up_right = zeropad_in(1:(h-2),3:w);
left = zeropad_in(2:(h-1),1:(w-2));    right = zeropad_in(2:(h-1),3:w);
down_left = zeropad_in(3:h,1:(w-2));   down = zeropad_in(3:h,2:(w-1));    down_right = zeropad_in(3:h,3:w);

out = cat(3, up_left, up, up_right, left, right, down_left, down, down_right);

end

