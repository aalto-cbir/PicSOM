function s1=simPolynomialVectorized(x,I,a)
epsilon=10^-10;

sizX=size(x);
sizI=size(I);

sizX1=sizX(1);
sizI1=sizI(1);

s1=zeros(sizI1,sizX1);
for i=1:sizX1
    s1(:,i) = sum((I - repmat(x(i,:),sizI1,1)).^2+epsilon,2).^(-a);
end
