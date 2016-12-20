function X = randpMatlab(P,varargin) ;
% RANDP - pick random values with relative probability
%
%     R = RANDP(PROB,..) returns integers in the range from 1 to
%     NUMEL(PROB) with a relative probability, so that the value X is
%     present approximately (PROB(X)./sum(PROB)) times in the matrix R.
%
%     All values of PROB should be equal to or larger than 0. 
%
%     RANDP(PROB,N) is an N-by-N matrix, RANDP(PROB,M,N) and
%     RANDP(PROB,[M,N]) are M-by-N matrices. RANDP(PROB, M1,M2,M3,...) or
%     RANDP(PROB,[M1,M2,M3,...]) generate random arrays.
%     RANDP(PROB,SIZE(A)) is the same size as A.
%
%     Example:
%       R = RANDP([1 3 2],10) will return 100 values in a 10-by-10 matrix,
%       being about 17 ones, 50 twos, and 33 threes.
% 
%     Also see RAND, RANDPERM

try,
    X = rand(varargin{:}) ;
    sz = size(X) ;
catch
    error(lasterr)
end

if any(P<0),
    error('All probabilities should be 0 or larger.') ;
end

try
    P = reshape(P,1,[]) ; % row vector
    P = cumsum(P) ./ sum(P) ;
    X = repmat(X(:),1,numel(P)) < repmat(P,numel(X),1) ; 
    X = numel(P) - sum(X,2) + 1 ; 
    X = reshape(X,sz) ; 
catch
    X = [] ;
end







