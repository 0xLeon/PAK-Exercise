function [distanceVector, rttVector, cov, cor] = rttPlot(rttData, distance)
%UNTITLED3 Summary of this function goes here
%   Detailed explanation goes here
    
    l = length(rttData);
    s = 0;
    distanceVector = [];
    rttVector = [];
    
    for i = 2:l
        m = length(rttData{i});
        s = s + m;
        
        distanceVector = [distanceVector, repmat(distance(i - 1), 1, m)];
        rttVector = [rttVector, cell2mat(rttData{i})];
    end
    
    cov = cova(distanceVector, rttVector);
    cor = core(distanceVector, rttVector);
    
    maxX = max(distanceVector) * 1.05;
    
    figure;
    set(gcf, 'Units', 'Normalized');
    set(gcf, 'OuterPosition', [0.225, 0.25, 0.55, 0.5]);
    scatter(distanceVector, rttVector, 8, 'filled');
    set(gca, 'Xlim', [min([0, min(distanceVector) * 1.05]), maxX]);
    set(gca, 'Ylim', [min([0, min(rttVector) * 1.05]), max(rttVector) * 1.05]);
    xlabel('Distance in km');
    ylabel('RTT in ms');
end

function [c] = cova(A, B)
    c = double(sum((A - mean(A)) .* (B - mean(B))) / (length(A)) - 1);
end

function [c] = covaE(A)
    c = double(sum((A - mean(A)) .^ 2)) / (length(A) - 1);
end

function [c] = core(A, B)
    if isequal(A, B)
        c = 1.0;
    else
        c = double(cova(A, B) / sqrt(covaE(A) * covaE(B)));
    end
end
