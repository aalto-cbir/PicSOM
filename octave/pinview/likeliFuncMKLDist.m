function D=likeliFuncMKLDist(dist,a,alpha,chosenF)

%tempSim=simPolynomialVectorized(x,I,a);
tempSim=simPolynomialMKLDist(dist,a);
% tempSim=sim2(x,I,a);

sizSim=size(tempSim);
sumSim=sum(tempSim,2);

D=((1-alpha)* tempSim(:,chosenF)./sumSim +alpha/sizSim(2))';
