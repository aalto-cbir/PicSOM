function [invAb] = invlemma(invA,b,c,m)

%
% input
%
%	invA	-	inverse of A (m-1 by m-1 matrix)
%	b		-	new vector added to invAb = [invA b]
%	m		-	dimensionality of invAb
%
% output
%
%	invAb	-	returned inverse
%
% written by Zakria Hussain, UCL, Nov 2009
%

if isempty(b)
	invAb = 1/invA;
else
	v = invA'*b;
	d = c - b'*v;
	invAb = [invA+(v*v'/d),-v/d; -v'/d,1/d]; % inverse computed using inverse lemma
end

