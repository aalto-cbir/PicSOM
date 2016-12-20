function [K,dist] = mkl3(ref,chosen,feedback,unchosen,eyemovements,mu,C,...
		    numfeat,numshown,choosemkl,feat,learntype,learn,...
		    ridge,choosetensor,kerneltype,forceread)

%
% function [K] = MKL(chosen,feedback,numfeat,choosemkl,learn) 
%  Multiple Kernel learning using relevant images
%
% input: 
%   chosen         -   indices of chosen images
%   feedback       -   feedback given as an indicator vector (1 in position
%                      means image was relevant, -1 otherwise)
%   numfeat        -   number of feature spaces (extraction methods)
%   numshown       -   number of images shown
%   n              -   number of presented images
%   choosemkl      -   1 for MKL, 0 for no MKL (Euclidean distance)
%   learn          -   0 = compute metric with old z and mu, 1 = compute
%                      new metric
% output:
%   K              -   kernel matrix
%   dist           -   Euclidean distance matrix
%
% Written by Zakria Hussain, UCL, Nov 2009
% Updated March 2010
% Updated March 2011
%

persistent zK % weights of the kernels
persistent alphaK % dual weight vector
persistent featlist % name of feature extraction methods 
persistent dataK;
persistent gm; % width parameter for Gaussian kernel (as a vector)

gm = [0.05,0.15,0.5,1,2,5];

if length(chosen)==numshown || forceread
    if kerneltype == 'linr'
        zK = ones(numfeat,1)/numfeat
    elseif kerneltype == 'gaus'
        zK = ones(length(gm)*numfeat,1)/(length(gm)*numfeat)
    end    

	if ~isempty(ref) % use data from PicSOM
		featlist = pinview_indexfullnamelist(ref)
	elseif isempty(ref) % use our own data
        if feat == 'raw'
            load picsomdata;
            for i=1:numfeat	
                dataK{i} = data{i};
            end
        elseif feat == 'som'
            load separateSOMMaps;
            for i=1:numfeat	
                dataK{i} = som_data{i};
            end
        end
        if choosetensor == 1
            load picsomdata_SOTON_eye;
            dataK = I.bicycles;
            %for i=1:numfeat
            %    dataK{i} = I.bicycles{i}; 
            %end
            eye_data = eyemovements;
            neye_data = normalise_data(eye_data);
        end
        
	end
end

if ~isempty(ref) % use data from PicSOM
    if length(chosen)==numshown || forceread
        for i=1:numfeat
            featlist(i,:)
            if feat == 'raw'
                dataK{i} = pinview_featuredata(ref,featlist(i,:)); % get original features from PicSOM
                dataK{i}(:,1) = [];
            elseif feat == 'som'
                dataK{i} = pinview_tssomcoord(ref,featlist(i,:)); % get som features from PicSOM
                dataK{i}(:,1) = [];
            end
            % extract eye movement features
            %eye_data = eyemovements;%chosen(:,5:37);
        end
        %dataK = cellfun(@normalise_data,dataK,'UniformOutput',0);
        %neye_data = normalise_data(eye_data);
    end
end 

allchosen = [chosen; unchosen];

t = length(chosen); % number of chosen images so far
m = size(allchosen,1); % number of images (rows)

kernels = repmat({zeros(t,m)},1,numfeat);
trainkernels = repmat({zeros(t,t)},1,numfeat);

dataX = cellfun(@(x) x(chosen,:),dataK,'UniformOutput',0);
dataY = cellfun(@(x) x(allchosen,:),dataK,'UniformOutput',0);
ndataX = cellfun(@normalise_data,dataX,'UniformOutput',0);
ndataY = cellfun(@normalise_data,dataY,'UniformOutput',0);

if kerneltype == 'linr'
    kernels = cellfun(@lin_kernel,ndataX,ndataY,'UniformOutput',0); % train-test kernels
elseif kerneltype == 'gaus'
    kernels = cellfun(@distance,ndataX,ndataY,'UniformOutput',0);
    XX = repmat(kernels,length(gm),1);
    XX = reshape(XX,1,length(gm)*numfeat);
    GG = repmat(gm,1,numfeat);
    GG = mat2cell(-1./(2*GG).^2,1,ones(1,length(GG)));
    XX = cellfun(@(x,y) x.*y, XX,GG,'UniformOutput',0); 
    kernels = cellfun(@exp,XX,'UniformOutput',0); % train-test kernels
end


if learn == 1 % learn MKL  
    if choosemkl==0 % compute uniform weighted Linear kernels
        
        if kerneltype == 'linr'
            lambda = mat2cell(ones(numfeat,1)/numfeat,ones(numfeat,1),1);
        elseif kerneltype == 'gaus'
            lambda = mat2cell(ones(length(gm)*numfeat,1)/(length(gm)*numfeat),ones(length(gm)*numfeat,1),1);
        end
        weightkernels = cellfun(@(x,y) x.*y, kernels,lambda','UniformOutput',0);
        K = sum(cat(3,weightkernels{:}),3);
    elseif choosemkl ==1 % run MKL for LinRel
        if sum((feedback.ground+1)/2) == 0 % if no relevant images seen yet
            
            if kerneltype == 'linr'
                lambda = mat2cell(ones(numfeat,1)/numfeat,ones(numfeat,1),1);
            elseif kerneltype == 'gaus'
                lambda = mat2cell(ones(length(gm)*numfeat,1)/(length(gm)*numfeat),ones(length(gm)*numfeat,1),1);
            end
            weightkernels = cellfun(@(x,y) x.*y, kernels,lambda','UniformOutput',0);
            K = sum(cat(3,weightkernels{:}),3);	            
        else
            if kerneltype == 'linr'
                trainkernels = cellfun(@lin_kernel,ndataX,ndataX,'UniformOutput',0);
            elseif kerneltype == 'gaus'
                trainkernels = cellfun(@distance,ndataX,ndataX,'UniformOutput',0);
                XX = repmat(trainkernels,length(gm),1);
                XX = reshape(XX,1,length(gm)*numfeat);
                GG = repmat(gm,1,numfeat);
                GG = mat2cell(-1./(2*GG).^2,1,ones(1,length(GG)));
                XX = cellfun(@(x,y) x.*y, XX,GG,'UniformOutput',0); 
                trainkernels = cellfun(@exp,XX,'UniformOutput',0);
            end
            if length(chosen)==numshown
                al = zeros(t,1);
                al(1)=1;
                alphaK = [];
                alphaK = [alphaK;al];
            else
                alphaK = [alphaK; zeros(numshown,1)];
	    end

            if strcmp(learntype,'rr') % ridge regression MKL
                y = feedback.eye;% outputs in [0,1]
                Ridge = ridge*ones(t,1);
                Ridge(find(y>0))=sum(~y)/t; % reweight positive ridge (larger)
                Ridge(find(y<=0))=sum(y)/t; % reweight negative ridge (smaller)
                [alp,z] = rrmkltrain(y,trainkernels,C,zK,mu,Ridge);
            elseif strcmp(learntype,'2c') % binary classification MKL
                y = feedback.deye; % outputs in {-1,+1}
                %CC = C*ones(t,1);
                %CC(find(y>0))=sum(y<0)/t;
                %CC(find(y<0))=sum(y>0)/t;
                [alp,z] = mkltrain(y,trainkernels,C,zK,mu,alphaK);
            elseif strcmp(learntype,'1c') % one-class SVM MKL
                y = feedback.deye; % outputs in {-1,+1}
                [alp,z] = mkltrain(y,trainkernels,C,zK,mu,alphaK);
            end
            alphaK = alp;
            zK = z;	    
	
            if kerneltype == 'linr'
                z = mat2cell(z,ones(numfeat,1),1); % convert z to cell array
            elseif kerneltype == 'gaus'
                z = mat2cell(z,ones(length(gm)*numfeat,1),1);
            end
    
            weightkernels = cellfun(@(x,y) x.*y, kernels,z','UniformOutput',0);
            K = sum(cat(3,weightkernels{:}),3);
        end
    end
elseif learn == 0 % no learning but compute distance for last chosen vector
    z = zK;
    if kerneltype == 'linr'
        z = mat2cell(z,ones(numfeat,1),1); % convert z to cell array
    elseif kerneltype == 'gaus'        
        z = mat2cell(z,ones(length(gm)*numfeat,1),1);      
    end
    weightkernels = cellfun(@(x,y) x.*y, kernels,z','UniformOutput',0);        
    K = sum(cat(3,weightkernels{:}),3);
end

if kerneltype == 'linr'
    z = zK;		    	
    z = mat2cell(z,ones(numfeat,1),1); % convert z to cell array 
    dist = cellfun(@distance,ndataX,ndataY,'UniformOutput',0);
    sqrtdist = cellfun(@sqrt,dist,'UniformOutput',0);	
    sqrtdist = cellfun(@real,sqrtdist,'UniformOutput',0);
    weightdist = cellfun(@(x,y) x.*y, sqrtdist,z','UniformOutput',0);
    dist = sum(cat(3,weightdist{:}),3);	
elseif kerneltype == 'gaus'
    z = zK;
    z = mat2cell(z,ones(length(gm)*numfeat,1),1);
    dist = cellfun(@distance,ndataX,ndataY,'UniformOutput',0);       
    sqrtdist = cellfun(@sqrt,dist,'UniformOutput',0);	
    sqrtdist = cellfun(@real,sqrtdist,'UniformOutput',0);
    XX = repmat(sqrtdist,length(gm),1);
    XX = reshape(XX,1,length(gm)*numfeat);
    weightdist = cellfun(@(x,y) x.*y, XX,z','UniformOutput',0);	
    dist = sum(cat(3,weightdist{:}),3);	
end        

% using tensor
if choosetensor == 1 && sum(sum(eyemovements)) != 0 % do tensor
    if strcmp(learntype,'2c') || strcmp(learntype,'1c')
        foptions(1) = 1;
        foptions(2) = C;
        y = feedback.deye;
    elseif learntype == 'rr'
        foptions(1) = 2;
        foptions(2) = ridge;
        y = feedback.eye;
    end
    if learn == 1
        %eye_data = eyemovements;
        %neye_data = normalise_data(eye_data);
        %[ndX,ndY] = tensorprojection(K(1:t,1:t),eyemovements,y,K',foptions);
        [ndX,ndY] = synthesis4_2(K(1:t,1:t),eyemovements,y,K',foptions);
        %ndX = K(1:t,1:t);
	%ndY = K';
	K = ndX*ndY';
    end
end
