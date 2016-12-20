function [alpha,lambda,err] = mkltrain(y,K,C,lambda,mu,alpha)

%
% Input: 
%   y       -   labels
%   K       -   cell of kernels
%   C       -   misclassification parameter
%   lambda  -   weights of kernels
%   mu      -   parameter to regularise between 1-norm and 2-norm. mu in (0,1]
%
% Outputs:
%   alpha   -   final dual weight vector
%   lambda  -   weights of kernels
%
% Written by Zakria Hussain, UCL, March 2010
%

m = length(y);
N = size(K,2);
ld = ones(N,1);

mpos = sum(y>0);
mneg = sum(y<0);

aa = ones(m,1);

% Cpos = C*ones(mpos,1);
% Cneg = (mpos/mneg)*C*ones(mneg,1);
% 
% C = zeros(m,1);
% C(find(y>0)) = Cpos;
% C(find(y<0)) = Cneg;

C = C*ones(m,1);

count = 0;
oldalpha = inf*ones(m,1);

fprintf('Start MKL optimization...\n')

while sum(abs(ld-lambda)) > 0.001
    
    sum(abs(ld-lambda))
    [ld lambda]
    ld = lambda;
    
    
    count = count+1;
    
    if mod(count,25)==0
        fprintf('.\n');
    else
        fprintf('.');
    end
    
    newK = zeros(m,m);

    for i=1:N
        lambda(i) = 1/((mu/lambda(i))+1-mu); 
        newK = newK + lambda(i)*K{i}; 
    end

%     libsvm_options = sprintf('-t 4 -c %f',C(1));
%     model=svmtrain(y,[[1:m]' newK],libsvm_options);
%     alpha = zeros(m,1);
%     alpha(model.SVs) = abs(model.sv_coef);
  
    Q = repmat(y,1,m).*newK.*repmat(y',m,1);
    alpha = smo_train(Q,y,C,alpha);

%     newK = [zeros(1,m);newK];
%     newK = [zeros(m+1,1),newK];
%     alpha = smo_train_1class(newK,[-1;ones(m,1)],C);
%     alpha = alpha(2:end)/alpha(1);
    
    for i=1:N
        lambda(i) = lambda(i)*sqrt((y.*alpha)'*K{i}*(y.*alpha));
        if lambda(i) < 10e-5
            lambda(i) = 0;
        end
    end
    
    err(count) = sum(oldalpha-alpha);
    oldalpha = alpha;
    
    lambda = lambda/sum(lambda);
end

lambda = lambda/sum(lambda);

fprintf('. \n');