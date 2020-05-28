function [ out ] = clip01( in )
%CLIP01 Clipping function
%   Clipping function that cuts out values except within [0,1]
% 
%   [ out ] = CLIP01( in )
% 
%   Input
%       in: input matrix (any dimension)
%   Output
%       out: clipped results (size(in))

out = in;

out(out>1) = 1;
out(out<0) = 0;

end

