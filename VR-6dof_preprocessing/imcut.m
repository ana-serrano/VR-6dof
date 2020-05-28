function [ out ] = imcut( img, left_or_right )
%IMCUT Summary of this function goes here
%   Detailed explanation goes here
h = size(img,1);    
switch left_or_right
    case 'left'
        out = img(1:floor(h/2),:,:);
    case 'right'
        out = img(ceil((h+1)/2):end,:,:);
    case 'none'
        out = img;
end

end

