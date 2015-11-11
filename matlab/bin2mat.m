function bin2mat(featurename)
% BIN2MAT - Converts a PicSOM binary to a mat file
%           binmat(featurename)
%           e.g. binmat('sift-bow') reads sift-bow.bin and writes sift-bow.mat

%% function bin2mat(featurename, Nobj)
%% % BIN2MAT - Converts a PicSOM binary to a mat file
%% %           binmat(featurename, Nobj)
%% %           e.g. binmat('sift-bow', 100)

binfile = [featurename, '.bin'];
matfile = [featurename, '.mat'];

fprintf('\nReading PSBD header from %s\n', binfile);
fid = fopen(binfile, 'rb');

% PSBD version 1.0 header 
magic   = fread(fid, 4, 'char');
version = fread(fid, 1, 'float');
hsize   = fread(fid, 1, 'uint64');
rlength = fread(fid, 1, 'uint64');
vdim    = fread(fid, 1, 'uint64');
format  = fread(fid, 1, 'uint64');
tmp     = fread(fid, 3, 'uint64');

fileinfo = dir(binfile);
Nbytes = fileinfo.bytes;
Nobj   = (Nbytes-hsize)/rlength;

fprintf('\n%s, %f, %u, %u, %u, %u, %u, %u, %u : bytes=%u => vectors=%u\n', ...
	magic, version, hsize, rlength, vdim, format, ...
	tmp(1), tmp(2), tmp(3), Nbytes, Nobj);

fprintf('\nAllocating a matrix of size %d x %d\n', Nobj, vdim);
X = zeros(Nobj, vdim);

for i = 1:size(X,1);
        if ~ rem(i,1000)
            fprintf('Read %d entries of data\n', i);
            drawnow;
        end
	fv = fread(fid, vdim, 'float');
	X(i, :) = fv;
 end

fclose(fid);

fprintf('\nSaving data to %s\n', matfile);
save(matfile, 'X', '-v7.3');

fprintf('\nAll done\n');

