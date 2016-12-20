function [alpha,lambda] = rrmkltrain(y,K,C,lambda,mu,gm)

% 
% ridge regression for MKL
%
%

m = size(K{1},1);
N = size(K,2);
ld = ones(N,1);

count = 0;

while sum(abs(ld-lambda)) > 0.001
    
    count = count+1;
    
    ld = lambda;
    
    for i=1:N
        lambda(i) = 1/((mu/lambda(i))+1-mu); 
    end
   
    lambda = mat2cell(lambda,ones(N,1),1);

    Ke = cellfun(@(x,y) x.*y, K,lambda','UniformOutput',0);
    sumKe = sum(cat(3,Ke{:}),3);

    %alpha = (sumKe+gm*eye(m))\y; % ridge regression
    alpha = (sumKe+diag(gm))\y;	
    lambda = cell2mat(lambda);

    for i = 1:N
        lambda(i) = lambda(i)*sqrt(alpha'*K{i}*alpha);
        if lambda(i) < 10e-5
            lambda(i) = 0;
        end
    end

    lambda = lambda/sum(lambda);

end

if sum(isnan(lambda)) > 0
  y
  K
  C
  lambda
  mu
  gm
  error('Weights are NaN in MKL!');
end

