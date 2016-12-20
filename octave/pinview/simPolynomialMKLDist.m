function s1=simPolynomialMKLDist(dist,a)
epsilon=10^-10;

s1 = (dist'+epsilon).^(-a);
