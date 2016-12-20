function data = normalise_data(x)

% normalise data matrix
%

data = x./(sqrt(sum(x'.^2))'*ones(1,size(x,2)));

data(isnan(data)) = 0;

if sum(sum(isnan(data))) > 0
  error('corrupt output features!');
end

