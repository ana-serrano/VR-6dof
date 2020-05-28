function [ out ] = edge_stopping_function( in_grad, sigma, functype )
%EDGE_STOPPING_FUNCTION Edge-stopping function for anisotropic diffusion
% Edge-stopping function for anisotropic diffusion
% 
% [ out ] = EDGE_STOPPING_FUNCTION( in, sigma, functype )
% 
% Inputs
%   in_grad: input gradient matrix (any dimension)
%   sigma: deviation value (scalar)
%   functype: edge-stopping function type (string)
%       'tukey': Tukey's biweight (default)
%       'huber': Huber's minimax estimator
%       'lorentzian': Lorentzian error (equivalanet to one Perona-Malik's edge stopping function)
%       'exponential': Gaussian error (equivalanet to the other Perona-Malik's edge stopping function)
%       'isotropic': no edge-stopping function (isotropic diffusion, heat equation)

if ~exist('functype', 'var')
    functype = 'tukey';
end

inlier_set = (in_grad<=sigma & in_grad>=-sigma);
outlier_set = (in_grad>sigma | in_grad<-sigma);
epsilon = 1e-20;

switch functype
    case 'tukey'
        %{
               |  1         x^2
               |  - ( 1 - ------- )^2     |x| <= sigma
        g(x) = |  2       sigma^2
               |
               |
               |  0                       otherwise
        %}
        out = ( 0.5 * (1 - (in_grad.^2)./(sigma.^2)).^2 ).*inlier_set;
        out(out<epsilon) = epsilon;
    case 'huber'
        %{
               |    1        
               |  -----                   |x| <= sigma
        g(x) = |  sigma
               |
               |   sign(x)
               |  --------                otherwise
               |      x
        %}
        out = 1 ./ sigma .* inlier_set + sign(in_grad)./max(in_grad,epsilon) .* outlier_set;
    case 'lorentzian'
        %{
                      1
        g(x) = ---------------
                       x^2
                1 + --------- 
                    2*sigma^2
        %}
        out = 1 ./ (1 + (in_grad.^2) ./ (sigma.^2));
    case 'exponential'
        %{
                          x^2
        g(x) =  exp( - --------- )
                        sigma^2
        %}
        out = exp(-(in_grad.^2) ./ (sigma.^2));
    case 'isotropic'
        out = ones(size(in_grad));
    otherwise
        error('Wrong function type entered.');
end

end

