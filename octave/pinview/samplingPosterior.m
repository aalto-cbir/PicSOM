function [CI,weight]=samplingPosterior(dist,a,alpha,y,UCB,weight,gamma,numUCB,numCluster,replacement,unchosen,iInnerLoop)

[maxY, yi]=max(y);

weightUpdated=weight((iInnerLoop-1)*numCluster+1:end);
weightUpdated=likeliFuncMKLDist( dist(:,(iInnerLoop-1)*numCluster+1:end) ,a,alpha,yi).*weightUpdated; % Posterior update with only user clicks
%weight=likeliFuncEyeTracking2(C,CI,p,a,alpha,y).*weight; % Posterior Update with eye tracking data
weightUpdated=weightUpdated/sum(weightUpdated);

combinedWeight=weightUpdated.*exp(gamma*UCB); % using LINREL to estimate the prior
combinedWeight=combinedWeight/sum(combinedWeight);

'samplingPosterior debug';
weightUpdated;
combinedWeight;

[sortUCB Isort]=sort(UCB,'descend'); % n images selected by max(UCB)
CI(1:numUCB)=Isort(1:numUCB);

if ~replacement % n random images without replacement
combinedWeight(CI(1:numUCB))=0;
	for i=numUCB+1:numCluster
	        CI(i)=randpMatlab(combinedWeight,1,1);
		combinedWeight(CI(i))=0;
	end
else
	CI(numUCB+1:numCluster)=randpMatlab(combinedWeight,1,numCluster-numUCB);
end

CI=unchosen(CI,1);
weight((iInnerLoop-1)*numCluster+1:end)=weightUpdated;
