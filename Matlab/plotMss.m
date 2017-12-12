function plotMss(mssData, ssthreshData, ssPhaseData, caPhaseData, frPhaseData, congData, dTitle)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here

    maxY = max(mssData(:, 2) + 2);
    maxBgY = min([maxY]);
    ssPhaseL = length(ssPhaseData);
    caPhaseL = length(caPhaseData);
    frPhaseL = length(frPhaseData);
    congL = length(congData);
    pSsPhase = 0;
    pCaPhase = 0;
    pFrPhase = 0;
    pCong = 0;
    
    hold on;
    grid on;
    
    for i = 1:ssPhaseL
        pSsPhase = area(ssPhaseData(i, :), [maxBgY, maxBgY], 'FaceColor', 'g', 'FaceAlpha', 0.55, 'EdgeAlpha', 0);
    end
    
    for i = 1:caPhaseL
        pCaPhase = area(caPhaseData(i, :), [maxBgY, maxBgY], 'FaceColor', 'r', 'FaceAlpha', 0.4, 'EdgeAlpha', 0);
    end
    
    for i = 1:frPhaseL
        pFrPhase = area([frPhaseData(i), frPhaseData(i) + 1], [maxBgY, maxBgY], 'FaceColor', 'y', 'FaceAlpha', 0.55, 'EdgeAlpha', 0);
    end
    
    for i = 1:congL
        pCong = plot([congData(i), congData(i)], [0, maxY], 'Color', 'k', 'LineWidth', 1, 'LineStyle', '-.');
    end
    
    pMss = plot(mssData(:, 1), mssData(:, 2), 'Color', 'b');
    pMssPoints = scatter(mssData(:, 1), mssData(:, 2), 25, 'b', 'filled');
    pSsThresh = plot(ssthreshData(:, 1), ssthreshData(:, 2), 'Color', 'r');
    legend([pSsThresh, pCong, pSsPhase, pCaPhase, pFrPhase], 'Slow Start Threshold', 'Congestion Time', 'Slow Start', 'Congestion Avoidance', 'Fast Recovery');
    hold off;
    
    set(gca, 'Xlim', [0, max(mssData(:, 1))]);
    set(gca, 'Ylim', [0, maxY]);
    xlabel('Zeit t [RTT]');
    ylabel('Fenstergröße [MSS]');
    title(dTitle);
end
