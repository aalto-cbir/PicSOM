function D2 = distance(x,y)

sizX=size(x,1);
sizY=size(y,1);

D2	= sum(x.^2,2)*ones(1,sizY) + ones(sizX,1)*sum(y.^2,2)' - 2*(x*y');