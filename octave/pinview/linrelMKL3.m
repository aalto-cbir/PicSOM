function CI = linrelMKL3(ref,chosen,unchosen,nreturn,...
					  linrel,mkl,numfeat,feat,learntype,...
					  kerneltype,tensor,rndseed,forceread,...
					  offline,verbose,ridge,cweight,gweight,iInnerLoop,iOuterLoop)

%warning("off","all");

tic

persistent invksquare; % in order to use matrix inversion lemma
persistent weight p CFeature CI randomImagesOnFirstPage

aPosterior=1;
alpha=0;
gamma=5;
numUCB=3;
replacement=0;

mouseMovement=1; % online experiments with mouse movements
mouseMovementWeight=0.8;

numImagesInPage=nreturn;
chooselinrel = linrel % AUCB, 1UCB or AExp
choosemkl = mkl; % 1 for MKL, 0 for no MKL
choosetensor = tensor; % 1 for tensor, 0 for no tensor
kerneltype = 'gaus'; % 'linr' for linear, 'gaus' for Gaussian
%kerneltype = 'linr'; % 'linr' for linear, 'gaus' for Gaussian
choosetensor = tensor % 1 for tensor, 0 for no tensor
%kerneltype = 'gaus'; % 'linr' for linear, 'gaus' for Gaussian
%kerneltype = 'linr'; % 'linr' for linear, 'gaus' for Gaussian
%kerneltype is given as parameter in the script files

% load separateSOMMaps
C=0.1;

if length(ridge)==0
  ridge = 1.0;
end
if length(cweight)==0
  cweight = 1.0;
end

muK=0.5; %zK= zeros(11,1); zK(1)=1; 
CK=10;
%selectMethod=3;

%offline = ~sum(sum(chosen(:,5:37))); %in offline experiment: can simulate click and eye
%offline = 1;
simulate_click = 1;
simulate_eye_features = 1;
use_ground_truths = 0;

if size(chosen,1) > 0
%weight eye movements
chosen(:,3) = gweight*chosen(:,3);

  if offline && simulate_eye_features %offline experiment
    ground = chosen(:,2);
    [eye_relevance,eye_features] = simulate_eye(ground);
    chosen(:,3) = gweight*eye_relevance;
    chosen(:,4) = (eye_relevance>0.5)*2-1;
    chosen(:,5:end) = eye_features;
  end

  if offline && simulate_click %offline experiment
    disp('offline test: simulating one click')
    l = numImagesInPage;
    for I=1:size(chosen,1)/l,
      recent_ground = chosen(l*(I-1)+1:l*I,2);
      n_rel = sum(recent_ground==1);
      click = zeros(size(recent_ground));
      if n_rel == 0 %no relevant images, take one at random
        click(1) = 1;
        click = click(randperm(length(recent_ground)));
      else %several relevant images, take one of _them_ at random
        rel_indexes = (1:length(recent_ground))(recent_ground==1);
        click_index = rel_indexes(randperm(length(rel_indexes)))(1);
        click(click_index) = 1;
      end
      chosen(l*(I-1)+1:l*I,3) = chosen(l*(I-1)+1:l*I,3) + cweight*click;
    end
  elseif offline && use_ground_truths
    chosen(:,3) = (chosen(:,2)==1);
    chosen(1,4) = 1; %to make mkl work correctly
  end
  if ~offline % in online experiment: record click position
    ground = chosen(:,2);
    click = (ground == 1); %clicked images
    if mouseMovement==1
    	chosen(:,3) = mouseMovementWeight*(chosen(:,3)>0);
    end
    chosen(:,3) = max(chosen(:,3),cweight*click);
  end

end

'tkk code'
toc
tic

if ~isempty(chosen)

It=chosen(:,1)';
numChosenImages=size(It,2);
pageNum=numChosenImages/nreturn

% presenting images to user
feedback.ground=chosen(:,2); % ground truth in {-1,+1}
feedback.eye = chosen(:,3); % eye movement feedback in [0,1]
feedback.deye = chosen(:,4);  % discretized eye movement feedback in {-1,+1}
eyemovements = chosen(:,5:37);
Xt = feedback.eye
Xt;
It;
'house keeping'
toc
tic

learn=1;
if strcmp(chooselinrel,'Comb') % parameters needed to enable the inner loop for the posterior update with the changing dataset
    	learn=(iInnerLoop==iOuterLoop);
	p=iInnerLoop;
else
	p=pageNum;
end

'linrelMKL3 debug';
[iInnerLoop iOuterLoop learn];
chosen;
unchosen;
Xt;

if sum(Xt)>0 %compute a weighted sum of kernels using mkl
    	[k,dist]=mkl3(ref,It',feedback,unchosen(:,1),eyemovements,muK,CK,numfeat,numImagesInPage,choosemkl,feat,learntype,learn,ridge,choosetensor,kerneltype,forceread);
elseif sum(Xt) == 0
	[k,dist]=mkl3(ref,It',feedback,unchosen(:,1),eyemovements,muK,CK,numfeat,numImagesInPage,choosemkl,feat,learntype,0,ridge,choosetensor,kerneltype,forceread);
end

if strcmp(chooselinrel,'AExp')
	loopUntil=numChosenImages+numImagesInPage;
else
	loopUntil=numChosenImages+1;
end
'sizUnchosen'
size(unchosen)
'MKL'
toc
tic
for i=numChosenImages+1:loopUntil

	% LINREL with MKL
	a=k'/(k(:,1:i-1)+ridge*eye(i-1));
	Ex(i,:)=Xt'*a';
	UCB(i,:)=Ex(i,:)+C*sqrt(sum(a'.^2,1));
	if strcmp(chooselinrel,'AExp')
		[xi,Imax]=max(UCB(i,i:end));
		It=[It,unchosen(Imax,1)];
		unchosen(Imax,:)=[];

		Xt(i)=Ex(i,i-1+Imax);
		if learntype == 'rr' 
			feedback.eye=Xt(1:i);
		else
			feedback.deye=(Xt(1:i)>0)*2-1;
		end
	end
end
if strcmp(chooselinrel,'AUCB')
	[xi,Isorted]=sort(UCB(i,i:end),'descend');
        %Isorted = Isorted(randperm(length(Isorted))); %random baselinetest
	Imax=Isorted(1:numImagesInPage);
	It=[It,unchosen(Imax,1)'];
	unchosen(Imax,:)=[];
elseif strcmp(chooselinrel,'1UCB')
	[xi,Imax]=max(UCB(i,i:end));
	[xi,Isorted]=sort(Ex(i,i:end),'descend');
	Imax(2:numImagesInPage)=Isorted(1:numImagesInPage-1);
	if size(union(Imax,Imax(2:numImagesInPage)),2)==numImagesInPage-1
		Imax(1:numImagesInPage)=Isorted(1:numImagesInPage);
	end
	It=[It,unchosen(Imax,1)'];
	unchosen(Imax,:)=[];
end

'LINREL'
toc
tic

if ~strcmp(chooselinrel,'Comb')
	CI=It(numChosenImages+1:numChosenImages+numImagesInPage)';
else
	[CI,weight]=samplingPosterior(dist,aPosterior,alpha,Xt(end-numImagesInPage+1:end)',UCB(loopUntil,loopUntil:end),weight,gamma,numUCB,numImagesInPage,replacement,unchosen,iInnerLoop);
end

'mkl3 linrel debug';
weight;
CI;

else
	if iOuterLoop==1
		% rand('seed', rndseed);
		temprand=randperm(size(unchosen,1));
		randomImagesOnFirstPage=unchosen(temprand(1:nreturn));
		CI=randomImagesOnFirstPage; % n random images with repetitions (change this)
	else
		CI=randomImagesOnFirstPage;
	end

	sizUnchosen=size(unchosen,1);
	weight=1/sizUnchosen*ones(sizUnchosen,1)';
end

'posterior update'
toc
