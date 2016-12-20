function D=probDnVectorized(x,xI,I,a,alpha)

tempSim=simPolynomialVectorized(x,I,a);
% tempSim=sim2(x,I,a);

sizX=size(x)
sizI=size(I)

sizSim=size(tempSim)
sumSim=sum(tempSim,2);

temp=permute(sum(~(permute(repmat(x,[1,1,sizI(1)]),[3 2 1])==repmat(I,[1,1,sizX(1)])),2)<1,[1 3 2]);
size(temp)

noTarget=repmat(sum(temp,2)<1,1,sizX(1));
size(noTarget)
D=(1-alpha)*( noTarget.*tempSim./repmat(sumSim,1,sizX(1)) + (~noTarget).*temp )+alpha/sizSim(2);
