function [train,test,lambda] = rrmkl(x,y,labels,mu,penalty,ridge)
% pass back distance using ridge regression mkl 
% 

N = size(x,2);

Ktrain = cellfun(@lin_kernel,x,x,'UniformOutput',0);
Ktraintest = cellfun(@lin_kernel,x,y,'UniformOutput',0);

lambda = ones(N,1)/N;

if sum(labels==1)>0
    %[alpha,lambda] = mkltrain(labels',Ktrain,penalty,lambda,mu);
    [alpha,lambda] = rrmkltrain(labels',Ktrain,penalty,lambda,mu,ridge);
    lambda = mat2cell(lambda,ones(N,1),1);
    tr = cellfun(@(x,y) x.*y, Ktrain,lambda','UniformOutput',0);
    te = cellfun(@(x,y) x.*y, Ktraintest,lambda','UniformOutput',0);
else
    lambda = mat2cell(lambda,ones(N,1),1);
    tr = cellfun(@(x,y) x.*y, Ktrain,lambda','UniformOutput',0);
    te = cellfun(@(x,y) x.*y, Ktraintest,lambda','UniformOutput',0);
end

train = sum(cat(3,tr{:}),3);
test = sum(cat(3,te{:}),3);

%%%%%%%%%%%%%%%%
function k = lin_kernel(x,y)

sizX=size(x);
sizY=size(y);

k = (x'*y)./sqrt(repmat(sum(x.^2)',1,sizY(2)).*repmat(sum(y.^2),sizX(2),1));