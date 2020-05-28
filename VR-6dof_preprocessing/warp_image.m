function [ warped ] = warp_image( img, flow )
%WARP_IMAGE Warp an image with an optical flow
%   Warp an image with an optical flow
% 
%   [ warped ] = WARP_IMAGE( img, flow )
% 
%   Inputs
%       img: input image (h,w,d)
%       flow: optical flow field (h,w,2) -> horizontal/vertical flow fields
%   Output
%       warped: warped image (h,w,d)

ch = size(img, 3);
warped = zeros(size(img));

[X,Y] = meshgrid(1:size(img,2), 1:size(img,1));

for i = 1:ch
    warped(:,:,i) = interp2(img(:,:,i), X-flow(:,:,1), Y-flow(:,:,2));
end

% NaN filling
nanmap = isnan(warped);
warped(nanmap) = img(nanmap);

end

