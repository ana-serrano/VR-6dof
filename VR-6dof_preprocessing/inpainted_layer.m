in_path = ['_extrapolated_layer/' filename '/' filename];
out_path = ['_inpainted_layer/' filename] ;
mkdir(out_path)

rgb_tex = im2double(imread([in_path '_BG.png']));
d_encoded = im2double(imread([in_path '_BG_depth.png']));

alpha = im2double(imread([in_path '_BGA.png']));
alpha = alpha(:,:,1);

%For color
bg_r = rgb_tex(:,:,1);
bg_r(alpha<0.5) = NaN;
bg_g = rgb_tex(:,:,2);
bg_g(alpha<0.5) = NaN;
bg_b = rgb_tex(:,:,3);
bg_b(alpha<0.5) = NaN;

c_r = inpaint_nans(bg_r,4);
c_g = inpaint_nans(bg_g,4);
c_b = inpaint_nans(bg_b,4);
inpainted(:,:,1) = c_r;
inpainted(:,:,2) = c_g; 
inpainted(:,:,3) = c_b;

imwrite(inpainted,[out_path '/' filename '_BG_inp.png']);

%For depth
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bg_d = d_encoded(:,:,1);
bg_d(alpha<0.5) = NaN;

d_d = inpaint_nans(bg_d,4);
inpainted(:,:,1) = d_d;
inpainted(:,:,2) = d_d; 
inpainted(:,:,3) = d_d;

imwrite(inpainted,[out_path '/' filename '_BGD_inp.png']);



