function k = lin_kernel(x,y)

sizX=size(x);
sizY=size(y);

k = (x*y');