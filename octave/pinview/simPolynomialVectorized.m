function s=simPolynomialVectorized(x,I,a)

epsilon=10^-10;
sizX=size(x);
sizI=size(I);
%s=permute(sqrt(sum((permute(repmat(x,[1,1,sizI(1)]),[3 2 1])-repmat(I,[1,1,sizX(1)])).^2,2)+epsilon).^(-a*2),[1 3 2]);
%s=permute(sqrt(sum((repmat(permute(x,[3 2 1]),[sizI(1),1,1])-repmat(I,[1,1,sizX(1)])).^2,2)+epsilon).^(-a*2),[1 3 2]);
%s=permute(sqrt(sum((permute(x(:,:,ones(sizI(1),1)),[3 2 1])-I(:,:,ones(sizX(1),1))).^2,2)+epsilon).^(-a*2),[1 3 2]);

tempX=permute(x,[3 2 1]);
s=sqrt(sum((tempX(ones(sizI(1),1),:,:)-I(:,:,ones(sizX(1),1))).^2,2)+epsilon).^(-a*2);
s=s(:,:);

%tempX=permute(x,[3 1 2]);
%tempI=permute(I,[1 3 2]);
%s=sqrt(sum((tempX(ones(sizI(1),1),:,:)-tempI(:,ones(sizX(1),1),:)).^2,3)+epsilon).^(-a*2);
