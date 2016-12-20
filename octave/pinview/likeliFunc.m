function D=likeliFunc(x,xI,I,a,alpha,chosenF)

%tempSim=simPolynomialVectorized(x,I,a);
tempSim=simPolynomialLoop(x,I,a);
% tempSim=sim2(x,I,a);

sizX=size(x);
sizI=size(I);

sizSim=size(tempSim);
sumSim=sum(tempSim,2);

D=((1-alpha)* tempSim(:,chosenF)./sumSim +alpha/sizSim(2))';
