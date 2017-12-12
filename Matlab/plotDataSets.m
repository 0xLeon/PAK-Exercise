function plotDataSets(step, pTitle, xLab, yLab, setNames, varargin)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
    lVarArgs = length(varargin);
    
    figure;
    set(gcf, 'Units', 'Normalized');
    set(gcf, 'OuterPosition', [0.225, 0.25, 0.55, 0.5]);
    
    miny = inf;
    maxy = -inf;
    
    for i = 1:lVarArgs
        cx = i;
        dataset = varargin{i};
        
        if isa(dataset, 'cell')
           dataset = cell2mat(dataset); 
        end
        
        len = length(dataset);
        
        mi = min(dataset);
        ma = max(dataset);
        me = mean(dataset);
        sd = std(dataset);
        sem = sd / sqrt(len);
        ci = me + tinv([0.025, 0.975], len) * sem;
        
        prc90upper = prctile(dataset, 90);
        prc10lower = prctile(dataset, 10);
        data80prc = dataset(dataset <= prc90upper & dataset >= prc10lower);
        min80prc = min(data80prc);
        max80prc = max(data80prc);
        
        if mi < miny
            miny = mi;
        end
        
        if ma > maxy
            maxy = ma;
        end
        
        scatter(repmat(cx, len, 1), dataset, 8, 'filled');
        hold on;
        cx = cx + step;
        
        cx = errorBarHelper(cx, me, ma, mi, 'R', step);
        cx = errorBarHelper(cx, me, max80prc, min80prc, '80', step);
        cx = errorBarHelper(cx, me, me + sd, me - sd, 'SD', step);
        cx = errorBarHelper(cx, me, me + sem, me - sem, 'SE', step);
        cx = errorBarHelper(cx, me, ci(2), ci(1), 'CI', step);
        
        ebCount = uint16((cx - i) / step);
        
        scatter(medianHelper(ebCount, i, step), repmat(me, 1, ebCount), 14, 'black', 'filled');
        text(i - 0.03, me, 'ME', 'HorizontalAlignment', 'right', 'VerticalAlignment', 'middle');
        hold on;
    end
    
    set(gca, 'Xlim', [0.65, i + 0.8]);
    set(gca, 'Ylim', [miny - 20, maxy + 20]);
    set(gca, 'XTick', 0:1:i);
    xticklabels(getTickLabels(setNames));
    xlabel(xLab);
    ylabel(yLab);
    title(pTitle);
end

function [nArray] = medianHelper(n, start, step)
    nArray = zeros(1, n);
    current = start;
    
    for i = 1:n
        nArray(i) = current;
        current = current + step;
    end
end

function [nextXPos] = errorBarHelper(x, me, yU, yL, label, step)
    errorbar(x, me, me - yL, yU - me);
    text(x, yU, label, 'HorizontalAlignment', 'center', 'VerticalAlignment', 'bottom');
    hold on;
    
    nextXPos = x + step;
end

function [tickLabels] = getTickLabels(labels)
    tickLabels = {''};
    l = length(labels) + 1;
    
    for i = 2:l
        tickLabels{i} = labels{i - 1};
    end
end
