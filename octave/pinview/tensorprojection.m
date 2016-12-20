function [I_new, IUnseen_new]=tensorprojection(Kx,E,Y,Kxt,foptions)
% Project image feature space into new semantic space
% Input: I - Seen image features
%        E - Eye movements features
%        Y - Target vector
%        IUnseen - Unseen image features
%        foptions: foptions(1) - 1 for classification
%                              - 2 for regression
%                  foptions(2) - c parameter for SVM-classification
%                              - parameter for ridge regression
% Output: I_new - New seen image features
%         IUnseen_new - New unseen image features
% (c) Kitsuchart Pasupa, School of Electronics & Computer Science,
%     University of Southampton

norm_on = 1;

% Change the lable to +1 & -1
Y=double(Y);
if foptions(1)==1 || foptions(1)==3 && sum(ismember(Y,[0 1]))==length(Y)
    Y(Y==0)=-1;
end

% Compute Linear Kernel
%Kx =I*I';
%Kxt=IUnseen*I';

% Compute Tensor Decomposition Projection Metrix
gamma = tsvm(Kx,E*E',Y,foptions);

% Use only the largest component
% gamma=gamma(:,end);

I_new = Kx*gamma;
% Sample-wise normalisation?
if norm_on
    I_new=I_new./repmat(sqrt(sum(I_new.^2,2)),1,size(I_new,2));
end

% projecting the test kernel into the tensor decomposition
IUnseen_new = Kxt*gamma;
% Sample-wise normalisation?
if norm_on
    IUnseen_new=IUnseen_new./repmat(sqrt(sum(IUnseen_new.^2,2)),1,size(IUnseen_new,2));
end

% % Normalisation?
% if norm_on
%     for i=1:size(testx,1)
%         testx(i,:) = testx(i,:)/norm(testx(i,:));
%     end
% end

end

function gamma = tsvm(Kx,Ky,trainz,foptions)
% Calculate the projection matric
% Input: trainx - feature vector for source 1 -- image
%        trainy - feature vector for source 2 -- eye
%        trainz - target vector -- relevance feedback
%        foptions: foptions(1) - 1 for classification
%                              - 2 for regression
%                  foptions(2) - c parameter for SVM-classification
%                              - parameter for ridge regression
% Output: gamma - projection matric
% Required toolbox: LibSVM
% (c) Kitsuchart Pasupa, School of Electronics & Computer Science,
%     University of Southampton

% Compute kernel matrix for each source (Linear Kernel at this moment)
%Kx = trainx*trainx';
%Ky = trainy*trainy';

% Combine both sources together
K = Ky.*Kx;

if foptions(1)==1
    % Linear SVM on Tensor kernel
    % (default settings)
%     modelFile = svmtrain(trainz,[(1:size(K,1)); K]','-t 4');
    % (input c parameter)
%     modelFile = svmtrain(trainz,[(1:size(K,1)); K]',['-t 4'...
%         ' -c '   num2str(foptions(2))]);
    % (Unbalance dataset)
%     modelFile = svmtrain(trainz,[(1:size(K,1)); K]',['-t 4'...
%         ' -c '   num2str(foptions(2))  ...
%         ' -w-1 ' num2str(sum(trainz==1)/length(trainz))...
%         ' -w1 '  num2str(sum(trainz==-1)/length(trainz))]);
%     % Compute the support vectors
%     alpha = zeros(length(K),1);
% %     alpha(modelFile.SVs) = modelFile.sv_coef;
%     alpha(modelFile.SVs) = abs(modelFile.sv_coef);
% elseif foptions(1)==2
%     % Ridge Regression
% %     alpha=pinv(K)*trainz;
%     alpha=(K+eye(size(K))*foptions(2))\trainz;
    m = length(trainz);
    C = foptions(2)*ones(m,1);
    Q = repmat(trainz,1,m).*K.*repmat(trainz',m,1);
    alpha = smo_train(Q,trainz,C);
    
elseif foptions(1)==2
    % Ridge Regression
%     alpha=pinv(K)*trainz;
    alpha=(K+eye(size(K))*foptions(2))\trainz;
elseif foptions(1)==3
    % Fisher Discriminant Analysis
    target=zeros(size(trainz));
    target(trainz==1)=length(trainz)/sum(trainz==1);
    target(trainz==-1)=-length(trainz)/sum(trainz==-1);
    alpha=pinv(K)*trainz;
%     alpha=(K+eye(size(K))*foptions(2))\trainz;
end
    
% Decompose into the images and eyes (dual variables) beta-Ky & gamma-Kx
T=min(rank(Ky),rank(Kx));
T=ceil(T/2);
gamma=i;
while ~isreal(gamma)
    [beta,gamma,T] = tensor(Ky,Kx,alpha,T);
    T=T-1;
end    
end

function [beta,gamma,T] = tensor(Kx,Ky,alpha,T)
% Tensor Decomposition
% (c) David R. Hardoon, Kitsuchart Pasupa

disp('performing tensor projection...')

if sum(sum(isnan(Ky)+isinf(Ky))) > 0
  error('input to tensor(...) contains nan or inf')
end

%Ky(isnan(Ky)) = 0; %remove bad behavior
%Ky(isinf(Ky)) = 0;

[U,Lambda] = eig((Ky+Ky')/2);
Lambda = diag(Lambda);
[tv,idx] = sort(Lambda);
Lambda = diag(Lambda(idx));
U = U(:,idx);

%t = real(diag(Lambda));
%T = sum(t/norm(t) > 1e-6);

m = length(Ky);

if nargin < 4
    val = diag(Lambda);
    cumval	= flipdim(cumsum(val),1);
    keepval = Inf;
    T = 0;
    
    for i=1:m-1
        [bnd(i)] = pca_bound(m,i,cumval);
        if bnd(i) < keepval
            keepval =	bnd(i);
            T	= i+1;
        end
    end
end

U = U(:,end-T+1:end);
Lambda = Lambda(end-T+1:end,end-T+1:end);
eya = diag(alpha);

% HH = sqrt(Lambda)*U'*eya*Kx*eya*U*sqrt(Lambda);

HH = sqrt(Lambda)*U'*eya*(Kx+10^(-8)*eye(size(Kx,1)))*eya*U*sqrt(Lambda);

if sum(sum(isnan(HH)+isinf(HH)))>0
  error('HH in tensor(...) contains nan or inf')
end
%HH(isnan(HH)) = 0; %remove bad behavior
%HH(isinf(HH)) = 0;

[Z,up] = eig(HH);

% making sure the eigenvalues are in increasing order
up = diag(up);
[up, idx] = sort(up);
up = diag(up);
up = sqrt(up);
Z = Z(:,idx);

% for i=1:m
%     beta(i,:) = ((alpha(i)./diag(up)).*(sqrt(Lambda)*Z*U(i,:)'));
% end
% for i=1:m
%     gamma(i,:) = alpha(i)*Kx(i,:)*beta;
% end

beta = (repmat(alpha,1,length(up))./repmat(diag(up),1,m)') .* (sqrt(Lambda)*Z*U')';
gamma= repmat(alpha,1,m).*Kx*beta;

end

function bnd = pca_bound(m,k,cumval)
t = (1:k)';
minbracket = ((1/m)*cumval(2:k+1)) + ((2*(1+sqrt(t)))/sqrt(m));
bnd = min(minbracket);
end
