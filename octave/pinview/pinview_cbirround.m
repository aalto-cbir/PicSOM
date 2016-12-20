function ret = pinview_cbirround(ref, seen, unseen, nreturn, ...
                                 linrel, mkl, tensor, kerneltype, feat, ridge, ...
				 clickw, gazew, rndseed, forceread, offline, verbose)

%% $Id: pinview_cbirround.m,v 1.35 2011/03/17 16:51:05 aleung Exp $	

%%
%% A wrapper function for PicSOM/PinView octave linkage.
%%

warning('off','all');

if verbose>0
  printf('Now in pinview_cbirround(), ref=%d linrel=%s mkl=%s tensor=%d kerneltype=%s feat=%s ridge=%f clickw=%f gazew=%f rndseed=%d forceread=%d offline=%d verbose=%d\n', ...
	 ref, linrel, mkl, tensor, kerneltype, feat, ridge, clickw, gazew, ...
	 rndseed, forceread, offline, verbose);
end

%% printf('First feature name is %s\n', pinview_indexfullname(ref, 0));

featlist = pinview_indexfullnamelist(ref);

numfeat=size(featlist,1);

%feat = pinview_featuredata(ref, "dominantcolor");

%somcoord = pinview_tssomcoord(ref, "dominantcolor");

if verbose>0
  printf('there are %d images to choose from with %d features\n', size(unseen)(1), numfeat);
end

%idx = unseen(1:nreturn)

% a temporary helper
mklbool=~strcmp(mkl, "") && ~strcmp(mkl, "none")

if ~isempty(seen)
	chosen=seen;
	chosen(:,1)=chosen(:,1)+1;
else
	chosen=[];
end
unchosen=unseen;
unchosen(:,1)=unchosen(:,1)+1;

if verbose>0
  feat
end

learntype = mkl;

iOuterLoop=size(chosen,1)/nreturn+1;
if ~strcmp(linrel,'Comb') 
			% no loop is needed with a fixed dataset without subsampling
	idx = linrelMKL3(ref,chosen,unchosen,nreturn,linrel,mklbool,...
			 numfeat,feat,learntype,kerneltype,tensor,...
			 rndseed,forceread,offline,verbose,ridge,clickw,gazew,iOuterLoop,iOuterLoop)-1;
else
			% a loop to calculate weights in previous iterations with a changing dataset due to subsampling
	allImages=[chosen;[unchosen zeros(size(unchosen,1),size(chosen,2)-1)]];
	for i=1:iOuterLoop
		idx = linrelMKL3(ref,allImages(1:(i-1)*nreturn,:),allImages((i-1)*nreturn+1:end,:),nreturn,linrel,mklbool,...
			 numfeat,feat,learntype,kerneltype,tensor,...
			 rndseed,forceread,offline,verbose,ridge,clickw,gazew,i,iOuterLoop)-1;
	end
end

ret = [ idx , zeros(nreturn, 1) ];
