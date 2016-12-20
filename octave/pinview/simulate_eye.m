function [eye_rel,eye_fea] = simulate_eye(ground)
%
% Loads previously recorded eye movements from a file and uses them to simulate during offline experiments
%
% Input:
%   ground - vector of groundtruths for relevance
% Output:
%   eye_rel - vector of predicted relevances at interval [0,1] that are from eyemovements
%   eye_fea - matrix of eye movement features, eye_fea(i,:) is one feature vector
%

NON_SEEN_REL = 0.15; %relevance for non-seen images

%ground = (ground == 1) %0/1 -data

sim_type = 'groups'; #grouped data according to how many relevant images were seen
%sim_type = 'normal';
zero_relevant_feedback = false;

persistent pos_data = 0;
persistent neg_data = 0;
persistent init_done = 0;


if init_done == 0
  disp('reading eye movement data from file...')
  %M = csvread("/home/jukujala/picsom/picsom-current/octave/pinview/eye_movement_examples.csv");
  %eye_file=[getenv('HOME'),"/picsom/octave/pinview/eye_movement_examples.csv"]
  if strcmp(sim_type,'normal') == 1
    eye_file=[getenv('HOME'),"/picsom/octave/pinview/eye_movement_examples_newregressor.csv"]
    M = csvread(eye_file);
    pos_data = M(M(:,1)==1,:);
    neg_data = M(M(:,1)==0,:);
    pos_data(pos_data(:,2)==-1,2) = NON_SEEN_REL;
    neg_data(neg_data(:,2)==-1,2) = NON_SEEN_REL;
  elseif strcmp(sim_type,'groups') == 1 
    disp('simulating eye movements in groups depending on the number of relevant images on the collage')
    eye_file=[getenv('HOME'),"/picsom/octave/pinview/eye_movement_examples_newregressor_groups.csv"]
    M = csvread(eye_file);
    pos_data = cell(6,1);
    neg_data = cell(6,1);
    for I=1:6    %separate to different groups...each 
      pos_data{I} = M(M(:,1)==1 & M(:,2) == I-1,:);
      neg_data{I}= M(M(:,1)==0 & M(:,2) == I-1,:);
      pos_data{I}(pos_data{I}(:,3)==-1,3) = NON_SEEN_REL;
      neg_data{I}(neg_data{I}(:,3)==-1,3) = NON_SEEN_REL;
    end
    if zero_relevant_feedback
      disp('explicit relevance feedback with zero relevant images');
      pos_data{1} = [];
      neg_data{1} = [0,0,0.0,zeros(1,33)];
    end
  else
    assert(false);
  end
  init_done = 1;
  disp('read eye movement data')
end

eye_rel = zeros(size(ground));
dim = size(pos_data,2)-2;
if strcmp(sim_type,'groups') == 1 
  dim = size(pos_data{2},2)-3;
end
eye_fea = zeros(length(ground),dim);

grp = getgrp(sum(ground==1))+1; 

for I = 1:length(ground),
  if ground(I) == 1 % pick random relevant example
    if strcmp(sim_type,'normal') == 1
      ind = randint(1,size(pos_data,1)); 
      eye_rel(I) = pos_data(ind,2);
      eye_fea(I,:) = pos_data(ind,3:end);
    elseif strcmp(sim_type,'groups') == 1
      ind = randint(1,size(pos_data{grp},1)); 
      eye_rel(I) = pos_data{grp}(ind,3);
      eye_fea(I,:) = pos_data{grp}(ind,4:end);
    end
  else % pick random non-relevant example
    if strcmp(sim_type,'normal') == 1
      ind = randint(1,size(neg_data,1));
      eye_rel(I) = neg_data(ind,2);
      eye_fea(I,:) = neg_data(ind,3:end);
    elseif strcmp(sim_type,'groups') == 1 
      ind = randint(1,size(neg_data{grp},1));     
      eye_rel(I) = neg_data{grp}(ind,3);
      eye_fea(I,:) = neg_data{grp}(ind,4:end);
    end
  end
end

function r = randint(M,N)
r = round((N - M) * rand(1,1) + M);

function grp = getgrp(x)
if x == 0
  grp = 0;
elseif x==1
  grp = 1;
elseif x==2 || x==3
  grp = 2;
elseif x > 3 && x < 7
  grp = 3;
elseif x > 6 && x < 11
  grp = 4;
else
  grp = 5;
end

